#include <gtest/gtest.h>
#include "patterns/execution/AssignmentPipeline.h"
#include "patterns/execution/RoleBasedAssignment.h"
#include <cstdio>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <iostream>
#include <stdexcept>
#include <string>

#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/SecuredTividy.h"
#include "patterns/structural/Tividy.h"
#include "server/ActivityLog.h"
#include "server/DemoSeed.h"
#include "server/ScenarioStore.h"
#include "patterns/execution/UndoHistory.h"
#include "patterns/execution/ApprovalRequest.h"
#include "patterns/execution/ReviewerHandler.h"
#include "patterns/execution/LeastLoadedAssignment.h"
#include "patterns/execution/CommandInvoker.h"
#include "patterns/execution/SetDataCommand.h"

namespace {

template <typename F>
bool throwsLogic(F f) {
    try { f(); } catch (const std::invalid_argument&) { return false; } catch (const std::logic_error&) { return true; } catch (...) { return false; }
    return false;
}

template <typename F>
bool throwsInvalid(F f) {
    try { f(); } catch (const std::invalid_argument&) { return true; } catch (...) { return false; }
    return false;
}

template <typename F>
bool denied(F f) {
    try { f(); } catch (const AccessDenied&) { return true; } catch (...) { return false; }
    return false;
}

LeafWorkItem& leafOf(Tividy& engine, const std::string& p, const std::string& id) {
    return *dynamic_cast<LeafWorkItem*>(engine.instance(p).item(id));
}

void seed(Tividy& engine) {
    DemoSeed::company(engine);
    DemoSeed::templates(engine);
    engine.clearHistory();
}

class ApprovalFlowTest : public ::testing::Test {
protected:
    Tividy engine;

    ApprovalFlowTest() : engine("Acme") {}

    void SetUp() override {
        seed(engine);
    }
};

void work(Tividy& engine, const std::string& p, const std::string& id) {
    LeafWorkItem& leaf = leafOf(engine, p, id);
    if (leaf.assignee().empty() || leaf.status() == WorkItem::AVAILABLE) engine.autoAssign(p, id);
    engine.start(p, id);
    engine.complete(p, id);
}

void signAll(Tividy& engine, const std::string& p, const std::string& id) {
    int guard = 0;
    while (leafOf(engine, p, id).status() == WorkItem::AWAITING_APPROVAL && guard++ < 5) {
        std::string role = leafOf(engine, p, id).pendingApprover();
        std::vector<Participant*> people = engine.organisation().eligible(role, "");
        std::string signer;
        for (std::size_t i = 0; i < people.size() && signer.empty(); ++i) {
            if (people[i]->name() != leafOf(engine, p, id).assignee() && !leafOf(engine, p, id).hasSigned(people[i]->name())) {
                signer = people[i]->name();
            }
        }
        if (signer.empty()) signer = "Olivia";
        engine.approveAs(p, id, signer);
    }
}

TEST_F(ApprovalFlowTest, Priority) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    EXPECT_EQ(leafOf(engine, p, "migration").priority(), "Urgent") << "Priority: template priorities reach the items";
    EXPECT_EQ(leafOf(engine, p, "ui").priority(), "Low") << "Priority: every level is supported";
    engine.setPriority(p, "ui", "High");
    EXPECT_EQ(leafOf(engine, p, "ui").priority(), "High") << "Priority: setPriority() changes it";
    EXPECT_THROW(([&] { engine.setPriority(p, "ui", "Critical"); })(), std::invalid_argument) << "Priority: only Low, Medium, High and Urgent are allowed";
    TaskSpec spec;
    spec.name = "Hotfix";
    spec.priority = "Urgent";
    std::string t = engine.addTask(p, spec);
    EXPECT_EQ(leafOf(engine, p, t).priority(), "Urgent") << "Priority: new tasks take the chosen priority";
    TaskSpec plain;
    plain.name = "Chore";
    EXPECT_EQ(leafOf(engine, p, engine.addTask(p, plain)).priority(), "Medium") << "Priority: Medium is the default";
    EXPECT_TRUE(LeafWorkItem::priorityRank("Urgent") > LeafWorkItem::priorityRank("Low")) << "Priority: levels are ordered";
}

TEST_F(ApprovalFlowTest, SubtasksAndWithdraw) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    work(engine, p, "spec");
    LeafWorkItem& spec = leafOf(engine, p, "spec");
    EXPECT_EQ(spec.status(), WorkItem::AWAITING_APPROVAL) << "Withdraw: spec is waiting for a manager";

    engine.withdraw(p, "spec");
    EXPECT_TRUE(spec.status() == WorkItem::IN_PROGRESS && spec.pendingApprovers().empty()) << "Withdraw: taking it back returns it to In progress and clears the approvals";
    EXPECT_TRUE(throwsLogic([&] { engine.withdraw(p, "spec"); })) << "Withdraw: only work in review can be taken back";
    engine.complete(p, "spec");

    TaskSpec missing;
    missing.name = "Add the error cases";
    std::string sub = engine.addSubtask(p, "spec", missing);
    LeafWorkItem& subtask = leafOf(engine, p, sub);
    EXPECT_EQ(spec.status(), WorkItem::REJECTED) << "Sub-task: adding one to work in review sends it back";
    EXPECT_NE(spec.reason().find("Add the error cases"), std::string::npos) << "Sub-task: the reason names the sub-task";
    EXPECT_TRUE(subtask.parentItem() == &spec && spec.childCount() == 1) << "Sub-task: it sits under its parent item (Composite)";
    EXPECT_TRUE(subtask.unit() == "engineering" && subtask.priority() == spec.priority()) << "Sub-task: it inherits team and priority";
    EXPECT_TRUE(subtask.role() == "TechLead" && subtask.assignee() == "Tom") << "Sub-task: it inherits the parent's role, so the pipeline finds the TechLead in Engineering";
    EXPECT_EQ(engine.workFor("Tom").size(), 2u) << "Sub-task: the iterator lists it under My work";

    engine.start(p, "spec");
    EXPECT_TRUE(throwsLogic([&] { engine.complete(p, "spec"); })) << "Sub-task: the parent cannot go back to review while a sub-task is open";
    engine.start(p, sub);
    engine.complete(p, sub);
    EXPECT_TRUE(throwsLogic([&] { engine.complete(p, "spec"); })) << "Sub-task: a sub-task in review still blocks the parent";
    signAll(engine, p, sub);
    EXPECT_EQ(subtask.status(), WorkItem::COMPLETED) << "Sub-task: it is approved on its own";
    engine.complete(p, "spec");
    EXPECT_EQ(spec.status(), WorkItem::AWAITING_APPROVAL) << "Sub-task: once it is done the parent can go back to review";

    TaskSpec nested;
    nested.name = "Level";
    std::string a = engine.addSubtask(p, "spec", nested);
    std::string b = engine.addSubtask(p, a, nested);
    std::string c = engine.addSubtask(p, b, nested);
    EXPECT_TRUE(throwsLogic([&] { engine.addSubtask(p, c, nested); })) << "Sub-task: nesting stops at three levels";
    EXPECT_TRUE(throwsLogic([&] { engine.addSubtask(p, sub, nested); })) << "Sub-task: finished work cannot get new sub-tasks";

    engine.cancel(p, "stop");
    EXPECT_EQ(leafOf(engine, p, c).status(), WorkItem::CANCELLED) << "Sub-task: cancelling the project reaches every level";
}

TEST_F(ApprovalFlowTest, Pipeline) {
    std::string p = engine.startProject("Blank board", "Board", "BusyTeam");

    TaskSpec backend;
    backend.name = "Fix the API";
    backend.unit = "backend";
    std::string first = engine.addTask(p, backend);
    EXPECT_TRUE(leafOf(engine, p, first).assignee() == "Bob" || leafOf(engine, p, first).assignee() == "Ben") << "Pipeline: work goes to someone in the chosen team";
    EXPECT_EQ(leafOf(engine, p, first).getData("assignedVia"), "Back-end") << "Pipeline: it records which step found them";

    engine.organisation().find("Bob")->setAvailable(false);
    engine.organisation().find("Ben")->setAvailable(false);
    std::string widened = engine.addTask(p, backend);
    std::string who = leafOf(engine, p, widened).assignee();
    EXPECT_EQ(who, "Fay") << "Pipeline: with Back-end away it widens to Engineering";
    EXPECT_EQ(leafOf(engine, p, widened).getData("assignedVia").find("widened"), 0) << "Pipeline: the widening is recorded";

    TaskSpec picked = backend;
    picked.assignee = "Quinn";
    EXPECT_EQ(leafOf(engine, p, engine.addTask(p, picked)).assignee(), "Quinn") << "Pipeline: a hand-picked person comes first";

    TaskSpec nobody;
    nobody.name = "Ship it";
    nobody.role = "ReleaseManager";
    nobody.unit = "platform";
    engine.organisation().find("Ray")->setAvailable(false);
    std::string waiting = engine.addTask(p, nobody);
    EXPECT_TRUE(leafOf(engine, p, waiting).assignee().empty()) << "Pipeline: when nobody fits the task waits unassigned";
    EXPECT_TRUE(throwsLogic([&] { engine.autoAssign(p, waiting); })) << "Pipeline: an explicit auto-assign says nobody can take it";
}

TEST_F(ApprovalFlowTest, MultiApproval) {
    ConcreteDefinitionBuilder b;
    b.reset("Review");
    b.setPolicy("BusyTeam");
    b.addStructuredStage(StageTemplate("only", "Only"));
    b.addStructuredItem("only", ItemTemplate("change", "Risky change", "Developer", "backend", "TechLead").alsoApprovedBy("Manager"));
    engine.loadDefinition("Review", b.build());
    std::string p = engine.startProject("Review", "", "");
    work(engine, p, "change");
    LeafWorkItem& change = leafOf(engine, p, "change");
    EXPECT_EQ(change.pendingApprovers().size(), 2u) << "Approvals: the item waits for two sign-offs";
    EXPECT_TRUE(engine.approvalsFor("Tom").size() == 1 && engine.approvalsFor("Maya").size() == 1) << "Approvals: both approvers see it in their list";
    EXPECT_TRUE(engine.approvalsFor("Rita").empty()) << "Approvals: other roles do not";

    engine.approveAs(p, "change", "Tom");
    EXPECT_TRUE(change.status() == WorkItem::AWAITING_APPROVAL && change.signatures().size() == 1) << "Approvals: one signature is not enough";
    EXPECT_TRUE(throwsLogic([&] { engine.approveAs(p, "change", "Tom"); })) << "Approvals: nobody signs twice";
    EXPECT_TRUE(throwsLogic([&] { engine.approveAs(p, "change", "Rita"); })) << "Approvals: a Reviewer cannot fill a Manager slot";
    engine.approveAs(p, "change", "Maya");
    EXPECT_EQ(change.status(), WorkItem::COMPLETED) << "Approvals: the last signature completes it";

    engine.undo();
    LeafWorkItem& restored = leafOf(engine, p, "change");
    EXPECT_TRUE(restored.status() == WorkItem::AWAITING_APPROVAL && restored.signatures().size() == 1) << "Approvals: undo takes back just the last signature";
    engine.approveAs(p, "change", "Olivia");
    EXPECT_EQ(leafOf(engine, p, "change").status(), WorkItem::COMPLETED) << "Approvals: the owner can fill any slot";

    std::string again = engine.startProject("Review", "", "");
    work(engine, again, "change");
    engine.reject(again, "change", "Not yet");
    EXPECT_TRUE(leafOf(engine, again, "change").pendingApprovers().empty()) << "Approvals: requesting changes clears the slots";
}

std::string runToSignOff(Tividy& engine) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    const char* order[] = { "spec", "api", "migration", "mockups", "ui", "tests", "code-review", "deploy" };
    for (int i = 0; i < 8; ++i) {
        work(engine, p, order[i]);
        signAll(engine, p, order[i]);
    }
    return p;
}

TEST_F(ApprovalFlowTest, SignOff) {
    std::string p = runToSignOff(engine);
    EXPECT_EQ(engine.instance(p).state(), "AwaitingSignOff") << "Sign-off: a finished project waits for the owner";
    EXPECT_EQ(engine.signOffsFor("Olivia").size(), 1u) << "Sign-off: the owner sees it";
    EXPECT_TRUE(engine.signOffsFor("Maya").empty()) << "Sign-off: managers do not, there is an owner";
    EXPECT_FALSE(engine.canSignOff("Maya", p)) << "Sign-off: a manager cannot sign it off";

    IssueSpec issue;
    issue.name = "Dark mode breaks the charts";
    issue.units.push_back("backend");
    issue.units.push_back("platform");
    issue.priority = "Urgent";
    std::vector<std::string> ids;
    {
        Tividy::ActorScope as(engine, "Olivia");
        ids = engine.flagIssue(p, issue);
    }
    EXPECT_EQ(ids.size(), 2u) << "Sign-off: one issue task per team";
    EXPECT_EQ(engine.instance(p).state(), WorkflowInstance::RUNNING) << "Sign-off: flagging reopens the project";
    EXPECT_EQ(leafOf(engine, p, ids[0]).assignee(), "Maya") << "Sign-off: the Back-end issue goes up to Engineering's manager";
    EXPECT_EQ(leafOf(engine, p, ids[1]).assignee(), "Nina") << "Sign-off: the Platform issue goes to Platform's manager";
    EXPECT_TRUE(leafOf(engine, p, ids[0]).priority() == "Urgent" && leafOf(engine, p, ids[0]).getData("kind") == "issue") << "Sign-off: issues keep their priority and kind";
    EXPECT_EQ(engine.undoLabels(1)[0].find("Olivia flagged"), 0) << "Sign-off: the undo history names who did it";

    work(engine, p, ids[0]);
    EXPECT_EQ(leafOf(engine, p, ids[0]).pendingApprover(), "Owner") << "Sign-off: issue fixes go back to the owner";
    EXPECT_FALSE(engine.canApprove("Nina", p, ids[0])) << "Sign-off: a manager cannot approve an owner's issue";
    engine.approveAs(p, ids[0], "Olivia");
    work(engine, p, ids[1]);
    engine.approveAs(p, ids[1], "Olivia");
    EXPECT_EQ(engine.instance(p).state(), "AwaitingSignOff") << "Sign-off: fixing the issues asks for sign-off again";
    engine.signOff(p);
    EXPECT_EQ(engine.instance(p).state(), WorkflowInstance::COMPLETED) << "Sign-off: the owner's sign-off completes it";
    EXPECT_TRUE(throwsLogic([&] { engine.signOff(p); })) << "Sign-off: it cannot be signed off twice";
}

TEST_F(ApprovalFlowTest, UndoAndSnapshots) {
    EXPECT_FALSE(engine.canUndo()) << "Undo: a fresh engine has nothing to undo";
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    const WorkflowInstance& instance = engine.instance(p);

    TaskSpec spec;
    spec.name = "Extra";
    std::string t = engine.addTask(p, spec);
    engine.undo();
    EXPECT_EQ(instance.item(t), nullptr) << "Undo: adding a task can be undone";
    engine.redo();
    EXPECT_NE(instance.item(t), nullptr) << "Redo: and redone";

    engine.addPerson("Eve", "Developer", "backend");
    engine.addUnit("engineering", "mobile", "Mobile");
    engine.moveMember("mobile", "Eve");
    engine.undo();
    EXPECT_TRUE(engine.organisation().unitOf("Eve")->id() == "backend") << "Undo: moving a person can be undone";
    engine.undo();
    EXPECT_EQ(engine.organisation().findUnit("mobile"), nullptr) << "Undo: adding a team can be undone";
    engine.undo();
    EXPECT_EQ(engine.organisation().find("Eve"), nullptr) << "Undo: adding a person can be undone";
    EXPECT_EQ(engine.redoLabels(5).size(), 3u) << "Redo: three steps are waiting to be redone";
    engine.addPerson("Zed", "QA", "qa");
    EXPECT_FALSE(engine.canRedo()) << "Redo: a new action clears the redo list";

    std::string before = engine.snapshot();
    EXPECT_TRUE(throwsLogic([&] { engine.start(p, "api"); })) << "Rollback: an illegal action throws";
    EXPECT_EQ(engine.snapshot(), before) << "Rollback: and leaves nothing changed";

    work(engine, p, "spec");
    std::string text = engine.snapshot();
    Tividy copy("Other");
    copy.restore(text);
    EXPECT_EQ(copy.snapshot(), text) << "Snapshot: restoring into a new engine gives the same text";
    EXPECT_TRUE(copy.instance(p).item("spec")->status() == WorkItem::AWAITING_APPROVAL) << "Snapshot: item states survive";
    EXPECT_EQ(copy.approvalsFor("Maya").size(), 1u) << "Snapshot: approval routing survives";
    EXPECT_TRUE(copy.organisation().find("Tom")->load() == engine.organisation().find("Tom")->load() &&
          copy.organisation().find("Tom")->load() > 0) << "Snapshot: loads are worked out again";
    EXPECT_THROW(([&] { copy.restore("hello"); })(), std::invalid_argument) << "Snapshot: text that is not a scenario is rejected";
    EXPECT_NE(copy.instance(p).item("spec"), nullptr) << "Snapshot: a rejected restore leaves the engine alone";
    std::string broken = text;
    std::size_t at = broken.find("\tBusyTeam\t");
    broken.replace(at, 10, "\tNoSuchPolicy\t");
    EXPECT_THROW(([&] { copy.restore(broken); })(), std::invalid_argument) << "Snapshot: a project with an unknown policy is rejected";
    EXPECT_TRUE(copy.snapshot() == text && copy.instance(p).item("spec") != nullptr) << "Snapshot: and the engine is left exactly as it was";
    std::string orphan = text + "person\tGhost\tDeveloper\tnowhere\t1\n";
    EXPECT_THROW(([&] { copy.restore(orphan); })(), std::invalid_argument) << "Snapshot: a person in an unknown team is rejected";
}

TEST(ApprovalFlow, ScenarioFiles) {
    std::string dir = "tividy-test-scenarios";
    ScenarioStore::ensureDirectory(dir);
    std::vector<ScenarioInfo> listed = ScenarioStore::list(dir);
    for (std::size_t i = 0; i < listed.size(); ++i) std::remove(listed[i].path.c_str());

    std::string blank = ScenarioStore::createBlank(dir, "My Startup!", "Startup Ltd", "Zara");
    EXPECT_NE(blank.find("my-startup.txt"), std::string::npos) << "Scenario: file names come from the scenario name";
    EXPECT_NE(ScenarioStore::createBlank(dir, "My Startup!", "Other", "Zed").find("my-startup-2.txt"), std::string::npos) << "Scenario: an existing name is not overwritten";

    Tividy engine("x");
    ActivityLog log(engine);
    engine.bus().attach(&log);
    ScenarioStore::load(blank, engine, &log);
    EXPECT_TRUE(engine.organisation().name() == "Startup Ltd" && engine.organisation().isOwner("Zara")) << "Scenario: a blank company has just its owner";
    EXPECT_TRUE(engine.hasDefinition("Feature Delivery") && engine.hasDefinition("Blank board")) << "Scenario: the templates come with it";
    EXPECT_FALSE(engine.canUndo()) << "Scenario: loading starts a fresh undo history";

    engine.addUnit("", "ops", "Ops");
    engine.addPerson("Sam", "Manager", "ops");
    std::string p = engine.startProject("Blank board", "First", "");
    ScenarioStore::save(blank, engine, &log);

    Tividy again("y");
    ActivityLog log2(again);
    ScenarioStore::load(blank, again, &log2);
    EXPECT_TRUE(again.organisation().find("Sam") != nullptr && again.instanceIds().size() == 1) << "Scenario: saved changes load back";
    EXPECT_EQ(again.instance(p).name(), "First") << "Scenario: projects keep their names";
    EXPECT_TRUE(log2.entries().size() == log.entries().size() && !log2.entries().empty()) << "Scenario: the activity log is saved too";

    std::vector<ScenarioInfo> found = ScenarioStore::list(dir);
    const ScenarioInfo* mine = nullptr;
    for (std::size_t i = 0; i < found.size(); ++i) if (found[i].file == "my-startup.txt") mine = &found[i];
    EXPECT_TRUE(found.size() == 2 && mine != nullptr && mine->readable && mine->projects == 1 && mine->people == 2) << "Scenario: list() reads the company, people and projects";
    for (std::size_t i = 0; i < found.size(); ++i) std::remove(found[i].path.c_str());
#ifdef _WIN32
    _rmdir(dir.c_str());
#else
    rmdir(dir.c_str());
#endif
    EXPECT_FALSE(ScenarioStore::exists(dir)) << "Scenario: the test folder is cleaned up";
    EXPECT_EQ(ScenarioStore::slug("  Hello, World  "), "hello-world") << "Scenario: slug() makes safe file names";
}

TEST_F(ApprovalFlowTest, ProxyRules) {
    SecuredTividy maya(engine, "Maya");
    SecuredTividy tom(engine, "Tom");
    SecuredTividy bob(engine, "Bob");
    SecuredTividy olivia(engine, "Olivia");
    std::string p = maya.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    tom.assign(p, "spec", "Tom");
    tom.start(p, "spec");
    tom.complete(p, "spec");

    EXPECT_THROW(([&] { bob.withdraw(p, "spec"); })(), AccessDenied) << "Proxy: only the assignee or a manager takes work back";
    EXPECT_THROW(([&] { bob.setPriority(p, "spec", "Low"); })(), AccessDenied) << "Proxy: workers cannot change priority";
    TaskSpec sub;
    sub.name = "More";
    EXPECT_THROW(([&] { bob.addSubtask(p, "spec", sub); })(), AccessDenied) << "Proxy: workers cannot add sub-tasks to others' work";
    EXPECT_EQ(engine.undoLabels(1)[0].find("Tom sent"), 0) << "Proxy: actions are labelled with the person";
    std::string own = tom.addSubtask(p, "spec", sub);
    EXPECT_FALSE(own.empty()) << "Proxy: the assignee can split their own work";
    TaskSpec handOff = sub;
    handOff.assignee = "Bob";
    EXPECT_THROW(([&] { tom.addSubtask(p, "spec", handOff); })(), AccessDenied) << "Proxy: only managers hand sub-tasks to someone else";
    EXPECT_THROW(([&] { maya.addPerson("Boss", "Owner", "engineering"); })(), AccessDenied) << "Proxy: only an owner can add an owner";
    EXPECT_THROW(([&] { bob.redo(); })(), AccessDenied) << "Proxy: workers cannot redo";
    EXPECT_THROW(([&] { maya.signOff(p); })(), AccessDenied) << "Proxy: managers cannot sign off when there is an owner";
    EXPECT_TRUE(olivia.isManager() && olivia.isOwner()) << "Proxy: the owner has manager rights too";
}

TEST_F(ApprovalFlowTest, ObserverUndo) {
    EXPECT_TRUE(engine.history() != nullptr && engine.historyBus().observerCount() == 1) << "Undo: the built-in history observes the history bus";
    EXPECT_EQ(engine.bus().observerCount(), 0u) << "Undo: domain observers are not disturbed by it";

    UndoHistory mine;
    engine.useHistory(&mine);
    std::string p = engine.startProject("Feature Delivery", "One", "BusyTeam");
    EXPECT_TRUE(mine.canUndo()) << "Undo: a history attached later records the next steps";
    engine.useHistory(nullptr);
    engine.startProject("Feature Delivery", "Two", "BusyTeam");
    EXPECT_FALSE(engine.canUndo()) << "Undo: with no history observer there is nothing to undo";
    EXPECT_EQ(mine.undoEntries(5).size(), 1u) << "Undo: a detached history stops recording";
    engine.useHistory(&mine);
    engine.undo();
    EXPECT_TRUE(engine.instanceIds().empty()) << "Undo: restoring a recorded step also drops later work nobody recorded, so keep a history attached";

    Tividy shared("Acme");
    seed(shared);
    SecuredTividy maya(shared, "Maya");
    SecuredTividy tom(shared, "Tom");
    SecuredTividy bob(shared, "Bob");
    std::string q = maya.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    tom.assign(q, "spec", "Tom");
    EXPECT_TRUE(tom.mayUndo() && !bob.mayUndo()) << "Undo: a worker can undo their own last action, others cannot";
    tom.undo();
    EXPECT_TRUE(shared.instance(q).item("spec")->assignee().empty()) << "Undo: Tom took back his claim";
    EXPECT_TRUE(tom.mayRedo()) << "Redo: and can redo it";
    tom.redo();
    TaskSpec loose;
    loose.name = "Tidy the logs";
    loose.unit = "backend";
    loose.autoAssign = false;
    std::string looseId = maya.addTask(q, loose);
    tom.start(q, "spec");
    bob.assign(q, looseId, "Bob");
    EXPECT_FALSE(tom.mayUndo()) << "Undo: once Bob acted, Tom's action is no longer the latest";
    EXPECT_THROW(([&] { tom.undo(); })(), AccessDenied) << "Undo: so the proxy refuses Tom";
    EXPECT_TRUE(maya.mayUndo()) << "Undo: a manager can undo whoever acted last";
    maya.undo();
    EXPECT_TRUE(shared.instance(q).item(looseId)->assignee().empty()) << "Undo: Maya undid Bob's claim";
    EXPECT_TRUE(tom.mayUndo()) << "Undo: now Tom's start is the latest again";
}

TEST_F(ApprovalFlowTest, SignOffRules) {
    EXPECT_TRUE(engine.definition("Feature Delivery").signOff) << "Sign-off: Feature Delivery asks for it";
    EXPECT_FALSE(engine.definition("Blank board").signOff) << "Sign-off: the blank board does not";

    std::string skipped = engine.startProject("Feature Delivery", "Quick", "BusyTeam", SignOffRule::NotRequired);
    EXPECT_FALSE(engine.instance(skipped).requiresSignOff()) << "Sign-off: a project can skip it";
    std::string forced = engine.startProject("Blank board", "Strict", "", SignOffRule::Required);
    EXPECT_TRUE(engine.instance(forced).requiresSignOff()) << "Sign-off: or ask for it when the template does not";
    std::string board = engine.startProject("Blank board", "Board", "");
    EXPECT_FALSE(engine.instance(board).requiresSignOff()) << "Sign-off: the template decides by default";

    work(engine, board, "kickoff");
    signAll(engine, board, "kickoff");
    EXPECT_EQ(engine.instance(board).state(), WorkflowInstance::COMPLETED) << "Sign-off: without it the project completes on the last approval";
    work(engine, forced, "kickoff");
    signAll(engine, forced, "kickoff");
    EXPECT_EQ(engine.instance(forced).state(), "AwaitingSignOff") << "Sign-off: with it the project waits for the owner";

    engine.loadDefinition("outline");
    std::string legacy = engine.startInstance("outline");
    EXPECT_FALSE(engine.instance(legacy).requiresSignOff()) << "Sign-off: legacy outlines never need it";
    std::string legacyStrict = engine.startProject("outline", "", "", SignOffRule::Required);
    EXPECT_TRUE(engine.instance(legacyStrict).requiresSignOff()) << "Sign-off: but a legacy project can ask for it";

    Tividy copy("x");
    copy.restore(engine.snapshot());
    EXPECT_TRUE(copy.instance(skipped).requiresSignOff() == false && copy.instance(forced).requiresSignOff() &&
          copy.definition("Feature Delivery").signOff) << "Sign-off: the rule survives the scenario file";
}

TEST(ApprovalFlow, ReviewFixes) {
    ApprovalRequest routed("Reviewer");
    ReviewerHandler reviewer;
    reviewer.handle(routed);
    EXPECT_TRUE(routed.isResolved() && routed.getPendingRole() == "Reviewer") << "Chain: a role handler answers with its role, and the engine waits for a person in that role";

    Tividy engine("Acme");
    seed(engine);
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    work(engine, p, "spec");
    EXPECT_EQ(leafOf(engine, p, "spec").pendingApprover(), "Manager") << "Chain: the Manager fallback is the last link";

    ConcreteDefinitionBuilder b;
    b.reset("Two");
    b.setPolicy("BusyTeam");
    b.addStructuredStage(StageTemplate("only", "Only"));
    b.addStructuredItem("only", ItemTemplate("x", "Change", "Developer", "backend", "TechLead").alsoApprovedBy("Owner"));
    engine.loadDefinition("Two", b.build());
    std::string q = engine.startProject("Two", "", "");
    work(engine, q, "x");
    EXPECT_TRUE(leafOf(engine, q, "x").pendingApprovers().size() == 2 &&
          leafOf(engine, q, "x").pendingApprovers()[1] == "Owner") << "Chain: the Owner handler routes owner approvals";
    {
        Tividy::ActorScope as(engine, "Tom");
        engine.approve(q, "x");
    }
    EXPECT_TRUE(leafOf(engine, q, "x").status() == WorkItem::AWAITING_APPROVAL && leafOf(engine, q, "x").signatures().size() == 1) << "Proxy: approve() through the facade with a person signs one slot, like the proxy does";

    EXPECT_FALSE(leafOf(engine, p, "spec").hasData("assignScope")) << "Strategy: the pipeline no longer writes into the item";
    LeastLoadedAssignment least;
    RoleBasedAssignment first;
    WorkItem probe("p", "Probe", "Developer");
    probe.setData("unit", "frontend");
    TeamFirstPipeline teamFirst(engine.organisation(), least);
    CompanyWidePipeline wide(engine.organisation(), first);
    EXPECT_EQ(teamFirst.choose(probe, "Developer").person->name(), "Fay") << "Bridge: the team-first pipeline keeps the strategy inside the item's team";
    EXPECT_EQ(wide.choose(probe, "Developer").person, engine.organisation().eligible("Developer").front()) << "Bridge: the company-wide pipeline lets the strategy see everyone";
    CompanyWidePipeline wideLeast(engine.organisation(), least);
    EXPECT_EQ(wideLeast.choose(probe, "Developer").person, least.assign(probe, "Developer", engine.organisation().people())) << "Bridge: company-wide, the pipeline gives the same answer as the unchanged strategy on its own";

    struct Throwing : public WorkflowObserver {
        void onEvent(const WorkflowEvent&) override { throw std::runtime_error("socket gone"); }
    } throwing;
    struct Counting : public WorkflowObserver {
        int seen = 0;
        void onEvent(const WorkflowEvent&) override { ++seen; }
    } counting;
    engine.bus().attach(&throwing);
    engine.bus().attach(&counting);
    engine.clearHistory();
    engine.setPriority(p, "api", "Low");
    EXPECT_EQ(counting.seen, 1) << "Observer: one failing observer does not stop the others";
    EXPECT_TRUE(engine.canUndo()) << "Observer: and the undo step is still recorded";
    engine.bus().detach(&throwing);
    engine.bus().detach(&counting);

    CommandInvoker invoker;
    invoker.setLimit(2);
    WorkflowDefinition flat;
    flat.items.push_back("a");
    WorkflowInstance inst("i", flat);
    inst.start();
    for (int i = 0; i < 5; ++i) {
        invoker.invoke(std::unique_ptr<WorkCommand>(new SetDataCommand(nullptr, "i", "a", "k", std::to_string(i))), &inst);
    }
    EXPECT_EQ(invoker.size(), 2u) << "Command: the invoker keeps a bounded history";
}

}

TEST_F(ApprovalFlowTest, SubtaskBlockersCannotDeadlock) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    work(engine, p, "spec");
    engine.withdraw(p, "spec");

    TaskSpec loop;
    loop.name = "Waits on its own parent";
    loop.blockedBy.push_back("spec");
    EXPECT_THROW(engine.addSubtask(p, "spec", loop), std::invalid_argument) << "Sub-task: it cannot wait for the parent that waits for it";

    TaskSpec after;
    after.name = "After the spec";
    after.stageId = "intake";
    after.blockedBy.push_back("spec");
    std::string later = engine.addTask(p, after);
    TaskSpec indirect;
    indirect.name = "Waits on something that waits on the parent";
    indirect.blockedBy.push_back(later);
    EXPECT_THROW(engine.addSubtask(p, "spec", indirect), std::invalid_argument) << "Sub-task: a blocker that waits on the parent is refused too";

    TaskSpec verify;
    verify.name = "Late check";
    verify.stageId = "verify";
    std::string inVerify = engine.addTask(p, verify);
    TaskSpec early;
    early.name = "Needs the late check";
    early.blockedBy.push_back(inVerify);
    EXPECT_THROW(engine.addSubtask(p, "spec", early), std::invalid_argument) << "Sub-task: a blocker in a later stage is refused";

    TaskSpec fine;
    fine.name = "Normal sub-task";
    EXPECT_NO_THROW(engine.addSubtask(p, "spec", fine)) << "Sub-task: an ordinary sub-task is still fine";
}

TEST_F(ApprovalFlowTest, SequentialStagesHoldNewTasks) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "SmallTeam");
    EXPECT_EQ(leafOf(engine, p, "spec").status(), WorkItem::AVAILABLE) << "Sequential: the spec is the first item";
    TaskSpec extra;
    extra.name = "Extra intake work";
    extra.stageId = "intake";
    std::string id = engine.addTask(p, extra);
    EXPECT_EQ(leafOf(engine, p, id).status(), WorkItem::CREATED) << "Sequential: a task added behind the spec waits its turn";
    work(engine, p, "spec");
    signAll(engine, p, "spec");
    EXPECT_NE(leafOf(engine, p, id).status(), WorkItem::CREATED) << "Sequential: it opens (and goes down the pipeline) once the spec is done";

    std::string q = engine.startProject("Feature Delivery", "Search", "BusyTeam");
    std::string parallel = engine.addTask(q, extra);
    EXPECT_NE(leafOf(engine, q, parallel).status(), WorkItem::CREATED) << "Parallel: a new task opens straight away";
}

TEST_F(ApprovalFlowTest, ScenarioWithABadStageIsRefused) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    std::string text = engine.snapshot();
    std::string marker = "instance\t" + p + "\t";
    std::size_t at = text.find(marker);
    ASSERT_NE(at, std::string::npos);
    std::size_t end = text.find('\n', at);
    std::vector<std::string> fields;
    std::string line = text.substr(at, end - at);
    std::size_t start = 0;
    for (std::size_t tab = line.find('\t'); tab != std::string::npos; tab = line.find('\t', start)) {
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    fields.push_back(line.substr(start));
    ASSERT_GE(fields.size(), 9u);
    fields[5] = "9";
    std::string bad = fields[0];
    for (std::size_t i = 1; i < fields.size(); ++i) bad += "\t" + fields[i];
    std::string edited = text.substr(0, at) + bad + text.substr(end);
    EXPECT_THROW(engine.restore(edited), std::invalid_argument) << "Scenario: a stage index past the last stage is refused";
    EXPECT_EQ(engine.snapshot(), text) << "Scenario: and the engine is left as it was";
}

TEST_F(ApprovalFlowTest, ProxyMatchesThePermissionsTable) {
    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    TaskSpec backendLead;
    backendLead.name = "Review the back-end plan";
    backendLead.stageId = "intake";
    backendLead.unit = "backend";
    backendLead.role = "TechLead";
    engine.organisation().find("Tom")->setAvailable(false);
    std::string id = engine.addTask(p, backendLead);
    engine.organisation().find("Tom")->setAvailable(true);
    ASSERT_EQ(leafOf(engine, p, id).status(), WorkItem::AVAILABLE);
    SecuredTividy tom(engine, "Tom");
    EXPECT_NO_THROW(tom.assign(p, id, "Tom")) << "Proxy: Tom in Engineering may take work aimed at Back-end, a team under his";

    SecuredTividy bob(engine, "Bob");
    TaskSpec spec;
    spec.name = "Approve it myself";
    spec.approverRoles.push_back("Auto");
    EXPECT_THROW(tom.addSubtask(p, id, spec), AccessDenied) << "Proxy: a worker cannot choose who approves a sub-task";
    spec.approverRoles.clear();
    EXPECT_NO_THROW(tom.addSubtask(p, id, spec)) << "Proxy: but may add one that is approved like its parent";

    work(engine, p, "spec");
    SecuredTividy olivia(engine, "Olivia");
    EXPECT_NO_THROW(olivia.withdraw(p, "spec")) << "Proxy: the owner may take back anyone's work, like a manager";
    EXPECT_THROW(bob.withdraw(p, "spec"), AccessDenied) << "Proxy: a worker may not";
}
