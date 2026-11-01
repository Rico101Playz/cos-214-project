#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "core/WorkflowInstance.h"
#include "mediator/TividyMediator.h"

namespace {

std::unique_ptr<TividyMediator> makeMediator() {
    std::unique_ptr<TividyMediator> engine(new TividyMediator("Acme Software"));
    engine->organisation().addParticipant("Alice", "Developer");
    engine->organisation().addParticipant("Bob", "Developer");
    engine->loadDefinition("outline");
    return engine;
}

std::string startOutline(TividyMediator& engine) {
    return engine.send(Request(Action::StartProject).with("definition", "outline").with("policy", "BusyTeam")).data("project");
}

} // namespace

// Every change goes in as a Request and comes back as a Response.
TEST(Mediator, Requests) {
    std::unique_ptr<TividyMediator> engine(makeMediator());
    Response started = engine->send(Request(Action::StartProject).with("definition", "outline").with("policy", "BusyTeam"));
    std::string project = started.data("project");
    ASSERT_TRUE(started.ok() && !project.empty()) << "Mediator: StartProject answers with the project id";
    EXPECT_EQ(engine->instance(project).state(), WorkflowInstance::RUNNING) << "Mediator: StartProject starts the project";

    TaskRef ref(project, engine->readyItems(project).at(0));
    EXPECT_TRUE(engine->send(Request(Action::Assign, ref).with("participant", "Alice")).ok()) << "Mediator: Assign succeeds";
    EXPECT_EQ(engine->organisation().find("Alice")->load(), 1) << "Mediator: ProjectFacade reports the Assign, so Alice's load goes up";
    EXPECT_TRUE(engine->send(Request(Action::Start, ref)).ok()) << "Mediator: Start succeeds";
    EXPECT_TRUE(engine->send(Request(Action::Complete, ref)).ok()) << "Mediator: Complete succeeds";
    EXPECT_TRUE(engine->send(Request(Action::Approve, ref)).ok()) << "Mediator: Approve succeeds";
    EXPECT_EQ(engine->organisation().find("Alice")->load(), 0) << "Mediator: the Approve report releases the load";

    Response added = engine->send(Request(Action::AddTask, TaskRef(project)).with("name", "Write the docs").with("role", "Developer"));
    ASSERT_TRUE(added.ok()) << "Mediator: AddTask succeeds";
    EXPECT_EQ(engine->instance(project).item(added.data("item"))->name(), "Write the docs") << "Mediator: ProjectFacade builds the task the Request describes";
}

// Organisation changes go to OrgFacade, undo and redo to HistoryFacade.
TEST(Mediator, OrganisationAndHistory) {
    std::unique_ptr<TividyMediator> engine(makeMediator());
    EXPECT_TRUE(engine->send(Request(Action::AddUnit).with("id", "qa").with("name", "QA")).ok()) << "Mediator: AddUnit succeeds";
    EXPECT_TRUE(engine->send(Request(Action::AddPerson).with("name", "Quinn").with("role", "Tester").with("unit", "qa")).ok())
        << "Mediator: AddPerson succeeds";
    ASSERT_NE(engine->organisation().find("Quinn"), nullptr) << "Mediator: OrgFacade added the person";
    EXPECT_EQ(engine->organisation().findUnit("qa")->name(), "QA") << "Mediator: OrgFacade added the unit";

    EXPECT_TRUE(engine->send(Request(Action::Undo)).ok()) << "Mediator: Undo succeeds";
    EXPECT_EQ(engine->organisation().find("Quinn"), nullptr) << "Mediator: HistoryFacade undid the last step";
    EXPECT_TRUE(engine->send(Request(Action::Redo)).ok()) << "Mediator: Redo succeeds";
    EXPECT_NE(engine->organisation().find("Quinn"), nullptr) << "Mediator: HistoryFacade redid it";
}

// A refused request is a Response code, not an exception.
TEST(Mediator, Refusals) {
    std::unique_ptr<TividyMediator> engine(makeMediator());
    Response unknown = engine->send(Request(Action::StartProject).with("definition", "missing"));
    EXPECT_TRUE(!unknown.ok() && unknown.code() == "invalid") << "Mediator: an unknown definition is refused as invalid";
    EXPECT_FALSE(unknown.message().empty()) << "Mediator: a refusal says why";

    std::string project = startOutline(*engine);
    TaskRef ref(project, engine->readyItems(project).at(0));
    Response early = engine->send(Request(Action::Approve, ref));
    EXPECT_TRUE(!early.ok() && early.code() == "not_allowed_now") << "Mediator: approving unstarted work is refused as not allowed now";

    Response nobody = engine->send(Request(Action::Assign, ref).with("participant", "Zed"));
    EXPECT_TRUE(!nobody.ok() && nobody.code() == "invalid") << "Mediator: assigning an unknown person is refused as invalid";

    Response internal = engine->send(Request(Action::SetData, ref).with("key", "k").with("value", "v"));
    EXPECT_TRUE(!internal.ok() && internal.code() == "invalid") << "Mediator: the engine's own steps cannot be sent from outside";

    std::unique_ptr<TividyMediator> fresh(makeMediator());
    EXPECT_TRUE(fresh->send(Request(Action::Undo)).ok()) << "Mediator: Undo with an empty history is not an error";
}
