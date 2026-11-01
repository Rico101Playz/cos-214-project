#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

#include "state_test_helpers.h"

#include "patterns/execution/CreatedState.h"
#include "patterns/execution/AvailableState.h"
#include "patterns/execution/AssignedState.h"
#include "patterns/execution/InProgressState.h"
#include "patterns/execution/AwaitingApprovalState.h"
#include "patterns/execution/CompletedState.h"
#include "patterns/execution/RejectedState.h"
#include "patterns/execution/CancelledState.h"
#include "patterns/execution/EscalatedState.h"

// ============================================================================
// HELPERS
// ============================================================================
static const char* kStates[] = {
    "Created", "Available", "Assigned", "InProgress", "AwaitingApproval",
    "Completed", "Rejected", "Escalated", "Cancelled"
};
static const int kStateCount = sizeof(kStates) / sizeof(kStates[0]);

static const char* kOps[] = {
    "makeAvailable", "assign", "start", "complete", "approve", "reject", "escalate"
};
static const int kOpCount = sizeof(kOps) / sizeof(kOps[0]);

// For every state: the operations it accepts (space-delimited). Everything else must throw.
struct StateRow { const char* state; const char* legalOps; };
static const StateRow kRows[] = {
    { "Created",          " makeAvailable " },
    { "Available",        " assign " },
    { "Assigned",         " assign start escalate " },
    { "InProgress",       " complete reject escalate " },
    { "AwaitingApproval", " approve reject " },
    { "Completed",        " " },
    { "Rejected",         " assign start " },
    { "Escalated",        " assign " },
    { "Cancelled",        " " }
};

// Takes a brand-new item to the named state through legal transitions.
static void driveTo(LeafWorkItem& item, const std::string& target) {
    if (target == "Created") return;
    if (target == "Cancelled") { item.cancel(); return; }
    item.makeAvailable();
    if (target == "Available") return;
    item.assign("alice");
    if (target == "Assigned") return;
    if (target == "Escalated") { item.escalate(); return; }
    item.start();
    if (target == "InProgress") return;
    item.complete();
    if (target == "AwaitingApproval") return;
    if (target == "Rejected") { item.reject("not good enough"); return; }
    item.approve();   // Completed
}

// Performs one named operation: true if accepted, false if the state refused it.
static bool tryOp(LeafWorkItem& item, const std::string& op) {
    try {
        if (op == "makeAvailable")      item.makeAvailable();
        else if (op == "assign")        item.assign("bob");
        else if (op == "start")         item.start();
        else if (op == "complete")      item.complete();
        else if (op == "approve")       item.approve();
        else if (op == "reject")        item.reject("because");
        else if (op == "escalate")      item.escalate();
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

// ============================================================================
// TESTS
// ============================================================================
TEST(WorkItemState, StateNames) {
    EXPECT_EQ(CreatedState().name(), "Created") << "CreatedState: name()";
    EXPECT_EQ(AvailableState().name(), "Available") << "AvailableState: name()";
    EXPECT_EQ(AssignedState().name(), "Assigned") << "AssignedState: name()";
    EXPECT_EQ(InProgressState().name(), "InProgress") << "InProgressState: name()";
    EXPECT_EQ(AwaitingApprovalState().name(), "AwaitingApproval") << "AwaitingApprovalState: name()";
    EXPECT_EQ(CompletedState().name(), "Completed") << "CompletedState: name()";
    EXPECT_EQ(RejectedState().name(), "Rejected") << "RejectedState: name()";
    EXPECT_EQ(CancelledState().name(), "Cancelled") << "CancelledState: name()";
    EXPECT_EQ(EscalatedState().name(), "Escalated") << "EscalatedState: name()";
}

TEST(WorkItemState, InitialState) {
    LeafWorkItem item("t1", "Write code", "Developer", 8);
    EXPECT_EQ(item.status(), "Created") << "LeafWorkItem: starts in Created";
    EXPECT_EQ(item.state, "Created") << "LeafWorkItem: legacy 'state' mirror starts as Created";
    EXPECT_TRUE(item.id() == "t1" && item.name() == "Write code") << "LeafWorkItem: keeps id and name";
    EXPECT_EQ(item.role(), "Developer") << "LeafWorkItem: keeps role";
    EXPECT_EQ(item.dueHours(), 8) << "LeafWorkItem: keeps dueHours";
    EXPECT_TRUE(item.assignee().empty() && item.reason().empty()) << "LeafWorkItem: no assignee or reason yet";
    EXPECT_TRUE(!item.isComplete() && nearly(item.progress(), 0.0)) << "LeafWorkItem: not complete, progress 0";
    EXPECT_FALSE(item.isReady()) << "LeafWorkItem: a Created item is not ready";
}

TEST(WorkItemState, HappyPath) {
    LeafWorkItem item("t1", "Task", "Developer");

    item.makeAvailable();
    EXPECT_EQ(item.status(), "Available") << "Happy path: Created -> Available";
    EXPECT_TRUE(item.isReady()) << "Happy path: an Available item is ready";

    item.assign("alice");
    EXPECT_TRUE(item.status() == "Assigned" && item.assignee() == "alice") << "Happy path: Available -> Assigned (assignee recorded)";
    EXPECT_FALSE(item.isReady()) << "Happy path: an Assigned item is no longer ready";

    item.start();
    EXPECT_EQ(item.status(), "InProgress") << "Happy path: Assigned -> InProgress";

    item.complete();
    EXPECT_EQ(item.status(), "AwaitingApproval") << "Happy path: InProgress -> AwaitingApproval (complete needs approval)";
    EXPECT_FALSE(item.isComplete()) << "Happy path: not complete until approved";

    item.approve();
    EXPECT_EQ(item.status(), "Completed") << "Happy path: AwaitingApproval -> Completed";
    EXPECT_TRUE(item.isComplete() && nearly(item.progress(), 1.0)) << "Happy path: Completed means complete, progress 1.0";
    EXPECT_EQ(item.state, "Completed") << "Happy path: legacy 'state' mirror follows along";
}

TEST(WorkItemState, TransitionMatrix) {
    const int rows = sizeof(kRows) / sizeof(kRows[0]);
    for (int r = 0; r < rows; ++r) {
        for (int o = 0; o < kOpCount; ++o) {
            LeafWorkItem item("m", "Matrix", "Developer");
            driveTo(item, kRows[r].state);
            std::string before = item.status();
            EXPECT_EQ(before, kRows[r].state) << (std::string("setup reaches ") + kRows[r].state).c_str();

            bool legal = std::string(kRows[r].legalOps).find(std::string(" ") + kOps[o] + " ") != std::string::npos;
            bool accepted = tryOp(item, kOps[o]);

            std::string label = std::string(kRows[r].state) + " + " + kOps[o] + (legal ? " is accepted" : " is refused");
            EXPECT_EQ(accepted, legal) << label.c_str();
            if (!legal) {
                EXPECT_EQ(item.status(), before) << (label + " and leaves the state unchanged").c_str();
            }
        }
    }
}

TEST(WorkItemState, RejectAndRework) {
    LeafWorkItem item("r1", "Rework me", "Developer");
    driveTo(item, "InProgress");

    item.reject("needs more tests");
    EXPECT_EQ(item.status(), "Rejected") << "Reject: InProgress -> Rejected";
    EXPECT_EQ(item.reason(), "needs more tests") << "Reject: the reason is stored";

    item.start();
    EXPECT_EQ(item.status(), "InProgress") << "Rework: Rejected -> InProgress (same assignee carries on)";
    EXPECT_EQ(item.reason(), "needs more tests") << "Rework: the reason is kept until reassignment";

    item.complete();
    item.reject("wrong approach");
    EXPECT_TRUE(item.status() == "Rejected" && item.reason() == "wrong approach") << "Reject: AwaitingApproval -> Rejected, reason replaced";

    item.assign("carol");
    EXPECT_TRUE(item.status() == "Assigned" && item.assignee() == "carol") << "Reassign: Rejected -> Assigned to someone else";
    EXPECT_TRUE(item.reason().empty()) << "Reassign: a fresh assignment clears the rejection reason";
}

TEST(WorkItemState, ReassignAndEscalate) {
    LeafWorkItem item("e1", "Escalate me", "Developer");
    driveTo(item, "Assigned");

    item.assign("bob");
    EXPECT_TRUE(item.status() == "Assigned" && item.assignee() == "bob") << "Reassign: Assigned stays Assigned, assignee changes";

    item.escalate();
    EXPECT_EQ(item.status(), "Escalated") << "Escalate: Assigned -> Escalated";
    EXPECT_THROW(([&]{ item.start(); })(), std::logic_error) << "Escalated: cannot be started before reassignment";
    EXPECT_THROW(([&]{ item.complete(); })(), std::logic_error) << "Escalated: cannot be completed before reassignment";

    item.assign("dave");
    EXPECT_TRUE(item.status() == "Assigned" && item.assignee() == "dave") << "Escalated: assign -> Assigned again";

    item.start();
    item.escalate();
    EXPECT_EQ(item.status(), "Escalated") << "Escalate: InProgress -> Escalated";
}

TEST(WorkItemState, Cancel) {
    for (int s = 0; s < kStateCount; ++s) {
        LeafWorkItem item("c", "Cancel me", "Developer");
        driveTo(item, kStates[s]);
        item.cancel();
        std::string expected = (std::string(kStates[s]) == "Completed") ? "Completed" : "Cancelled";
        std::string label = std::string("Cancel from ") + kStates[s] + " gives " + expected;
        EXPECT_EQ(item.status(), expected) << label.c_str();
    }

    LeafWorkItem item("c2", "Cancelled", "Developer");
    item.cancel();
    EXPECT_TRUE(!tryOp(item, "assign") && !tryOp(item, "start")) << "Cancel: a cancelled item accepts no further work";
}

TEST(WorkItemState, ErrorMessages) {
    LeafWorkItem item("x", "Message", "Developer");
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ item.start(); }, "Illegal transition")) << "Message: says it is an illegal transition";
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ item.start(); }, "start")) << "Message: names the operation";
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ item.start(); }, "Created")) << "Message: names the current state";

    CompletedState done;
    EXPECT_TRUE(throwsLogicErrorContaining([&]{ done.escalate(item); }, "Completed")) << "Message: the base-class helper reports the state that refused";
}

TEST(WorkItemState, Restore) {
    LeafWorkItem item("s1", "Snapshot me", "Developer");

    item.restore("InProgress", "erin", "");
    EXPECT_TRUE(item.status() == "InProgress" && item.assignee() == "erin") << "Restore: sets state and assignee";
    EXPECT_EQ(item.state, "InProgress") << "Restore: legacy 'state' mirror follows";
    item.complete();
    EXPECT_EQ(item.status(), "AwaitingApproval") << "Restore: the restored state object behaves correctly";

    item.restore("Rejected", "frank", "bad naming");
    EXPECT_TRUE(item.status() == "Rejected" && item.reason() == "bad naming") << "Restore: sets the reason too";

    EXPECT_THROW(([&]{ item.restore("Nonsense", "", ""); })(), std::invalid_argument) << "Restore: an unknown state name throws";
    EXPECT_EQ(item.status(), "Rejected") << "Restore: a failed restore leaves the item untouched";
}

TEST(WorkItemState, NullTransition) {
    LeafWorkItem item("n1", "Null", "Developer");
    EXPECT_THROW(([&]{ item.transitionTo(std::unique_ptr<WorkItemState>()); })(), std::invalid_argument) << "transitionTo: a null state is rejected";
    EXPECT_EQ(item.status(), "Created") << "transitionTo: the item is unchanged after the rejection";
}

TEST(WorkItemState, PolymorphicUse) {
    LeafWorkItem leaf("p1", "Poly", "Developer");
    WorkItem& asItem = leaf;
    asItem.makeAvailable();
    asItem.assign("gina");
    EXPECT_EQ(leaf.status(), "Assigned") << "Polymorphism: WorkItem calls reach the State machine";

    WorkNode& asNode = leaf;
    EXPECT_TRUE(!asNode.isComplete() && nearly(asNode.progress(), 0.0)) << "Polymorphism: WorkNode queries work";
    asNode.cancel();
    EXPECT_EQ(leaf.status(), "Cancelled") << "Polymorphism: WorkNode::cancel() reaches LeafWorkItem::cancel()";
}

// ============================================================================
// MAIN
// ============================================================================
