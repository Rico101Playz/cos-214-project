#include <iostream>
#include <memory>
#include <string>

#include "test_utils.h"

#include "patterns/creation/WorkflowFactory.h"
#include "patterns/creation/SmallTeamFactory.h"
#include "patterns/creation/BusyTeamFactory.h"
#include "patterns/execution/ApprovalRequest.h"

// ============================================================================
// Helpers
// ============================================================================

// Runs a request for `role` through a freshly made chain; true when it ends
// up resolved (approved by somebody in the chain).
static bool chainResolves(const WorkflowFactory& factory, const std::string& role) {
    std::unique_ptr<ApprovalHandler> chain = factory.createApprovalChain();
    ApprovalRequest request(role);
    chain->handle(request);
    return request.isResolved();
}

// ============================================================================
// TEST: SmallTeamFactory
// ============================================================================
static void test_SmallTeamFactory() {
    std::cout << "\n--- Testing SmallTeamFactory ---\n";

    SmallTeamFactory concrete;
    const WorkflowFactory& factory = concrete;   // used through the interface

    check(factory.name() == "SmallTeam", "SmallTeam: knows its name");

    std::unique_ptr<CompositeStage> stage = factory.createStage("s1", "Build");
    check(stage.get() != nullptr, "SmallTeam: createStage() returns a stage");
    check(stage->id() == "s1" && stage->name() == "Build",
          "SmallTeam: the stage has the id and name it was given");
    check(stage->mode().name() == "Sequential", "SmallTeam: stages run Sequential");

    std::unique_ptr<LeafWorkItem> item = factory.createWorkItem("i1", "Implement login", "Developer");
    check(item.get() != nullptr, "SmallTeam: createWorkItem() returns a work item");
    check(item->id() == "i1" && item->name() == "Implement login",
          "SmallTeam: the item has the id and name it was given");
    check(item->role() == "Developer", "SmallTeam: the item has the role it was given");
    check(item->dueHours() == 48, "SmallTeam: work is due in 48 hours");
    check(item->status() == WorkItem::CREATED, "SmallTeam: a new item starts Created");

    std::unique_ptr<AssignmentStrategy> strategy = factory.createAssignmentStrategy();
    check(strategy.get() != nullptr, "SmallTeam: createAssignmentStrategy() returns a strategy");
    check(strategy->name() == "RoleBased", "SmallTeam: assigns by role only");

    check(chainResolves(factory, "Auto"), "SmallTeam: the chain approves 'Auto' automatically");
    check(chainResolves(factory, "Reviewer"), "SmallTeam: a Reviewer can approve");
    check(!chainResolves(factory, "TechLead"), "SmallTeam: the chain has no Tech Lead step");
    check(!chainResolves(factory, "ReleaseManager"), "SmallTeam: the chain has no Release Manager step");

    // Every call makes a new, independent object.
    std::unique_ptr<CompositeStage> other = factory.createStage("s2", "Test");
    check(other.get() != stage.get(), "SmallTeam: every createStage() call makes a new stage");
}

// ============================================================================
// TEST: BusyTeamFactory
// ============================================================================
static void test_BusyTeamFactory() {
    std::cout << "\n--- Testing BusyTeamFactory ---\n";

    BusyTeamFactory concrete;
    const WorkflowFactory& factory = concrete;   // used through the interface

    check(factory.name() == "BusyTeam", "BusyTeam: knows its name");

    std::unique_ptr<CompositeStage> stage = factory.createStage("s1", "Build");
    check(stage->id() == "s1" && stage->name() == "Build",
          "BusyTeam: the stage has the id and name it was given");
    check(stage->mode().name() == "Parallel", "BusyTeam: stages run Parallel");

    std::unique_ptr<LeafWorkItem> item = factory.createWorkItem("i1", "Fix bug", "Developer");
    check(item->id() == "i1" && item->name() == "Fix bug",
          "BusyTeam: the item has the id and name it was given");
    check(item->role() == "Developer", "BusyTeam: the item has the role it was given");
    check(item->dueHours() == 24, "BusyTeam: work is due in 24 hours");
    check(item->status() == WorkItem::CREATED, "BusyTeam: a new item starts Created");

    std::unique_ptr<AssignmentStrategy> strategy = factory.createAssignmentStrategy();
    check(strategy->name() == "LeastLoaded", "BusyTeam: assigns to the least loaded person");

    check(chainResolves(factory, "Reviewer"), "BusyTeam: a Reviewer can approve");
    check(chainResolves(factory, "TechLead"), "BusyTeam: the Tech Lead can approve");
    check(!chainResolves(factory, "Auto"), "BusyTeam: nothing is approved automatically");
    check(!chainResolves(factory, "ReleaseManager"), "BusyTeam: the chain has no Release Manager step");

    // The policies really differ from each other.
    SmallTeamFactory small;
    check(small.createStage("x", "x")->mode().name() != stage->mode().name(),
          "BusyTeam vs SmallTeam: the stages run differently");
    check(small.createAssignmentStrategy()->name() != strategy->name(),
          "BusyTeam vs SmallTeam: the assignment strategies differ");
}

// ============================================================================
// MAIN
// ============================================================================
void run_factory_tests() {
    std::cout << "\n--- Running Abstract Factory (workflow policy) Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_SmallTeamFactory();
    test_BusyTeamFactory();

    std::cout << "Factory Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}
