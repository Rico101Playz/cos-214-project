#include <gtest/gtest.h>
#include <iostream>
#include <stdexcept>
#include <string>

#include "core/WorkflowInstance.h"
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryDirector.h"
#include "patterns/creation/SmallTeamFactory.h"
#include "patterns/creation/BusyTeamFactory.h"
#include "patterns/creation/HighRiskFactory.h"
#include "patterns/execution/AssignedToIterator.h"
#include "patterns/execution/AwaitingApprovalIterator.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/Tividy.h"
#include "server/DemoSeed.h"

namespace {

template <typename F>
bool throwsInvalid(F f) {
    try { f(); } catch (const std::invalid_argument&) { return true; } catch (...) { return false; }
    return false;
}

LeafWorkItem* leaf(const WorkflowInstance& instance, const std::string& id) {
    return dynamic_cast<LeafWorkItem*>(instance.item(id));
}

WorkflowDefinition feature(const std::string& policy) {
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryDirector director(&builder);
    director.construct(policy);
    return builder.build();
}

void finish(LeafWorkItem* item, const std::string& who) {
    item->assign(who);
    item->start();
    item->complete();
    item->approve();
}

TEST(StructuredWorkflow, Builder) {
    WorkflowDefinition def = feature("BusyTeam");
    EXPECT_TRUE(def.structured()) << "Builder: the Feature Delivery director makes a structured definition";
    EXPECT_EQ(def.policy, "BusyTeam") << "Builder: the policy is stored on the definition";
    EXPECT_TRUE(def.layout.size() == 6 && def.templates.size() == 8) << "Builder: six stages and eight items";
    EXPECT_EQ(def.layout[2].parent, "build") << "Builder: sub-stages keep their parent";

    ConcreteDefinitionBuilder b;
    b.reset("Test");
    b.addStructuredStage(StageTemplate("a", "A"));
    EXPECT_THROW(([&] { b.addStructuredStage(StageTemplate("a", "Again")); })(), std::invalid_argument) << "Builder: duplicate stage ids are rejected";
    EXPECT_THROW(([&] { b.addStructuredStage(StageTemplate("x", "X", "missing")); })(), std::invalid_argument) << "Builder: unknown parent stages are rejected";
    EXPECT_THROW(([&] { b.addStructuredItem("missing", ItemTemplate("i", "I", "")); })(), std::invalid_argument) << "Builder: items need a known stage";
    b.addStructuredItem("a", ItemTemplate("one", "One", ""));
    b.addStructuredItem("a", ItemTemplate("two", "Two", ""));
    EXPECT_THROW(([&] { b.addStructuredItem("a", ItemTemplate("one", "Dup", "")); })(), std::invalid_argument) << "Builder: duplicate item ids are rejected";
    EXPECT_THROW(([&] { b.addBlocker("one", "two"); })(), std::invalid_argument) << "Builder: an item cannot be blocked by a later item (no cycles)";
    EXPECT_THROW(([&] { b.addBlocker("two", "two"); })(), std::invalid_argument) << "Builder: an item cannot block itself";
    b.addBlocker("two", "one");
    EXPECT_EQ(b.build().templates[1].blockedBy.size(), 1u) << "Builder: blockers are recorded on the item";
}

TEST(StructuredWorkflow, FactoryTree) {
    WorkflowDefinition def = feature("BusyTeam");
    BusyTeamFactory busy;
    WorkflowInstance instance("p1", def, busy, "Dark mode");
    EXPECT_EQ(instance.name(), "Dark mode") << "Instance: the project title is used as its name";
    EXPECT_EQ(instance.root().childCount(), 4u) << "Instance: four top-level stages";
    CompositeStage* build = dynamic_cast<CompositeStage*>(instance.find("build"));
    EXPECT_TRUE(build != nullptr && build->childCount() == 2) << "Instance: the Build stage nests two sub-stages (Composite)";
    EXPECT_TRUE(build->mode().name() == "Parallel") << "Instance: stages come from the policy's factory";
    EXPECT_TRUE(leaf(instance, "api")->unit() == "backend") << "Instance: items keep their team";
    EXPECT_TRUE(leaf(instance, "api")->approverRole() == "Reviewer") << "Instance: items keep their approver role";
    EXPECT_TRUE(leaf(instance, "api")->dueHours() == BusyTeamFactory::DUE_HOURS) << "Instance: items come from the policy's factory";

    HighRiskFactory risky;
    WorkflowInstance strict("p2", def, risky, "");
    EXPECT_TRUE(leaf(strict, "api")->getData("risk") == "high") << "Instance: a HighRisk factory tags every item";
    EXPECT_EQ(strict.name(), "Feature Delivery") << "Instance: without a title the definition name is used";
}

TEST(StructuredWorkflow, Blocking) {
    WorkflowDefinition def = feature("BusyTeam");
    BusyTeamFactory busy;
    WorkflowInstance instance("p1", def, busy, "");
    instance.start();

    EXPECT_TRUE(leaf(instance, "spec")->status() == WorkItem::AVAILABLE) << "Blocking: the first stage opens";
    EXPECT_TRUE(leaf(instance, "api")->status() == WorkItem::CREATED) << "Blocking: later stages stay closed";
    finish(leaf(instance, "spec"), "Tom");
    instance.advance();

    LeafWorkItem* ui = leaf(instance, "ui");
    EXPECT_TRUE(leaf(instance, "api")->status() == WorkItem::AVAILABLE) << "Blocking: api opens with the Build stage";
    EXPECT_TRUE(leaf(instance, "mockups")->status() == WorkItem::AVAILABLE) << "Blocking: parallel sub-stages open together";
    EXPECT_TRUE(ui->isWaiting() && ui->status() == WorkItem::CREATED) << "Blocking: ui waits for its blockers";
    EXPECT_TRUE(ui->openBlockers().size() == 2) << "Blocking: ui is blocked by two unfinished items";

    finish(leaf(instance, "mockups"), "Dee");
    EXPECT_TRUE(ui->isWaiting() && ui->openBlockers().size() == 1) << "Blocking: one blocker done, ui still waits";

    LeafWorkItem* api = leaf(instance, "api");
    api->assign("Bob");
    api->start();
    api->complete();
    EXPECT_TRUE(ui->isWaiting()) << "Blocking: a blocker awaiting approval still blocks";
    api->approve();
    EXPECT_TRUE(ui->status() == WorkItem::AVAILABLE && !ui->isWaiting()) << "Blocking: ui opens once every blocker is approved";

    EXPECT_THROW(([&] { ui->addBlocker("ui"); })(), std::invalid_argument) << "Blocking: an item cannot block itself";
}

TEST(StructuredWorkflow, Sequential) {
    WorkflowDefinition def = feature("SmallTeam");
    SmallTeamFactory small;
    WorkflowInstance instance("p1", def, small, "");
    instance.start();
    finish(leaf(instance, "spec"), "Tom");
    instance.advance();
    EXPECT_TRUE(leaf(instance, "api")->status() == WorkItem::AVAILABLE) << "Sequential: the first item of the first sub-stage opens";
    EXPECT_TRUE(leaf(instance, "migration")->status() == WorkItem::CREATED) << "Sequential: the next item waits its turn";
    EXPECT_TRUE(leaf(instance, "mockups")->status() == WorkItem::CREATED) << "Sequential: the next sub-stage waits";
}

TEST(StructuredWorkflow, Iterators) {
    WorkflowDefinition def = feature("BusyTeam");
    BusyTeamFactory busy;
    WorkflowInstance instance("p1", def, busy, "");
    instance.start();
    finish(leaf(instance, "spec"), "Tom");
    instance.advance();

    leaf(instance, "api")->assign("Bob");
    leaf(instance, "migration")->assign("Bob");
    leaf(instance, "mockups")->assign("Dee");
    leaf(instance, "migration")->start();
    leaf(instance, "migration")->complete();
    leaf(instance, "migration")->setPendingApprover("Reviewer");

    int bobs = 0;
    AssignedToIterator mine(&instance.root(), "Bob");
    for (mine.first(); !mine.isDone(); mine.next()) ++bobs;
    EXPECT_EQ(bobs, 2) << "AssignedTo: finds every open item assigned to Bob";

    int toms = 0;
    AssignedToIterator done(&instance.root(), "Tom");
    for (done.first(); !done.isDone(); done.next()) ++toms;
    EXPECT_EQ(toms, 0) << "AssignedTo: skips finished work";

    AwaitingApprovalIterator reviewer(&instance.root(), "Reviewer");
    EXPECT_TRUE(!reviewer.isDone() && reviewer.currentItem()->id() == "migration") << "AwaitingApproval: finds work routed to a role";
    reviewer.next();
    EXPECT_TRUE(reviewer.isDone()) << "AwaitingApproval: stops after the last match";

    AwaitingApprovalIterator other(&instance.root(), "TechLead");
    EXPECT_TRUE(other.isDone()) << "AwaitingApproval: other roles see nothing";

    AwaitingApprovalIterator self(&instance.root(), "Reviewer", "Bob");
    EXPECT_TRUE(self.isDone()) << "AwaitingApproval: nobody is asked to approve their own work";

    AwaitingApprovalIterator any = AwaitingApprovalIterator::anyApprover(&instance.root());
    EXPECT_FALSE(any.isDone()) << "AwaitingApproval: anyApprover() lists everything awaiting approval";
}

TEST(StructuredWorkflow, FacadeTasks) {
    Tividy engine("Acme");
    DemoSeed::company(engine);
    DemoSeed::templates(engine);

    std::string p = engine.startProject("Feature Delivery", "Dark mode", "BusyTeam");
    EXPECT_EQ(engine.policyOf(p), "BusyTeam") << "Facade: the chosen policy overrides the template";
    EXPECT_THROW(([&] { engine.startProject("Feature Delivery", "x", "Nope"); })(), std::invalid_argument) << "Facade: unknown policies are rejected";

    engine.autoAssign(p, "spec");
    EXPECT_TRUE(engine.instance(p).item("spec")->assignee() == "Tom") << "Facade: autoAssign() finds the TechLead in Engineering";
    engine.start(p, "spec");
    engine.complete(p, "spec");
    LeafWorkItem* spec = dynamic_cast<LeafWorkItem*>(engine.instance(p).item("spec"));
    EXPECT_TRUE(spec->pendingApprover() == "Manager") << "Chain: BusyTeam has no Manager handler, so it falls to a Manager";
    EXPECT_EQ(engine.approvalsFor("Maya").size(), 1u) << "Facade: the manager sees it under approvals";
    EXPECT_TRUE(engine.approvalsFor("Tom").empty()) << "Facade: the person who did the work does not";
    EXPECT_FALSE(engine.canApprove("Tom", p, "spec")) << "Facade: nobody approves their own work";
    engine.approve(p, "spec");

    TaskSpec task;
    task.name = "Fix flaky login";
    task.unit = "backend";
    task.blockedBy.push_back("api");
    std::string t = engine.addTask(p, task);
    LeafWorkItem* added = dynamic_cast<LeafWorkItem*>(engine.instance(p).item(t));
    EXPECT_TRUE(added != nullptr && added->unit() == "backend") << "Facade: addTask() makes a task for the chosen team";
    EXPECT_TRUE(added->isWaiting() && added->assignee().empty()) << "Facade: a blocked task waits and is not assigned yet";

    engine.autoAssign(p, "api");
    std::string apiDev = engine.instance(p).item("api")->assignee();
    EXPECT_TRUE(apiDev == "Bob" || apiDev == "Ben") << "Facade: api goes to a back-end developer";
    engine.start(p, "api");
    engine.complete(p, "api");
    EXPECT_TRUE(dynamic_cast<LeafWorkItem*>(engine.instance(p).item("api"))->pendingApprover() == "Reviewer") << "Chain: api is routed to a Reviewer";
    EXPECT_TRUE(engine.canApprove("Rita", p, "api")) << "Facade: the Reviewer can approve it";
    engine.approve(p, "api");
    std::string taskDev = engine.instance(p).item(t)->assignee();
    EXPECT_TRUE(taskDev == "Bob" || taskDev == "Ben") << "Facade: once unblocked, the task is auto-assigned inside Back-end";

    EXPECT_EQ(engine.workFor(taskDev).size(), 1u) << "Facade: workFor() lists the task for its assignee";
    EXPECT_TRUE(engine.organisation().find(taskDev)->load() == 1) << "Facade: loads follow the work trees";
    engine.undo();
    EXPECT_TRUE(engine.instance(p).item(t)->assignee().empty()) << "Facade: one undo rolls back the approval and the auto-assign it caused";
    EXPECT_TRUE(engine.instance(p).item("api")->status() == WorkItem::AWAITING_APPROVAL) << "Facade: undo puts api back in review";
    EXPECT_TRUE(engine.organisation().find(apiDev)->load() == 1) << "Facade: undo puts loads back too";
    engine.redo();
    EXPECT_TRUE(engine.instance(p).item(t)->assignee() == taskDev) << "Facade: redo reapplies it";

    TaskSpec later;
    later.name = "Too early";
    later.stageId = "build";
    later.blockedBy.push_back("deploy");
    EXPECT_THROW(([&] { engine.addTask(p, later); })(), std::invalid_argument) << "Facade: a task cannot wait on something in a later stage";
    TaskSpec badUnit;
    badUnit.name = "Nowhere";
    badUnit.unit = "missing";
    EXPECT_THROW(([&] { engine.addTask(p, badUnit); })(), std::invalid_argument) << "Facade: unknown teams are rejected";

    std::string small = engine.startProject("Feature Delivery", "Small", "SmallTeam");
    LeafWorkItem* deploy = dynamic_cast<LeafWorkItem*>(engine.instance(small).item("deploy"));
    EXPECT_TRUE(deploy->approverRole() == "Auto") << "Facade: deploy asks for automatic approval";
    std::string high = engine.startProject("Feature Delivery", "High", "HighRisk");
    EXPECT_EQ(engine.policyOf(high), "HighRisk") << "Facade: several policies run side by side";
}

}

