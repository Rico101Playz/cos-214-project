#include <iostream>
#include <memory>
#include <string>

#include "test_utils.h"
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
static void test_StateNames() {
    std::cout << "\n--- Testing concrete state names ---\n";
    check(CreatedState().name() == "Created", "CreatedState: name()");
    check(AvailableState().name() == "Available", "AvailableState: name()");
    check(AssignedState().name() == "Assigned", "AssignedState: name()");
    check(InProgressState().name() == "InProgress", "InProgressState: name()");
    check(AwaitingApprovalState().name() == "AwaitingApproval", "AwaitingApprovalState: name()");
    check(CompletedState().name() == "Completed", "CompletedState: name()");
    check(RejectedState().name() == "Rejected", "RejectedState: name()");
    check(CancelledState().name() == "Cancelled", "CancelledState: name()");
    check(EscalatedState().name() == "Escalated", "EscalatedState: name()");
}

static void test_InitialState() {
    std::cout << "\n--- Testing a new LeafWorkItem ---\n";
    LeafWorkItem item("t1", "Write code", "Developer", 8);
    check(item.status() == "Created", "LeafWorkItem: starts in Created");
    check(item.state == "Created", "LeafWorkItem: legacy 'state' mirror starts as Created");
    check(item.id() == "t1" && item.name() == "Write code", "LeafWorkItem: keeps id and name");
    check(item.role() == "Developer", "LeafWorkItem: keeps role");
    check(item.dueHours() == 8, "LeafWorkItem: keeps dueHours");
    check(item.assignee.empty() && item.reason().empty(), "LeafWorkItem: no assignee or reason yet");
    check(!item.isComplete() && nearly(item.progress(), 0.0), "LeafWorkItem: not complete, progress 0");
    check(!item.isReady(), "LeafWorkItem: a Created item is not ready");
}

static void test_HappyPath() {
    std::cout << "\n--- Testing the full lifecycle ---\n";
    LeafWorkItem item("t1", "Task", "Developer");

    item.makeAvailable();
    check(item.status() == "Available", "Happy path: Created -> Available");
    check(item.isReady(), "Happy path: an Available item is ready");

    item.assign("alice");
    check(item.status() == "Assigned" && item.assignee == "alice", "Happy path: Available -> Assigned (assignee recorded)");
    check(!item.isReady(), "Happy path: an Assigned item is no longer ready");

    item.start();
    check(item.status() == "InProgress", "Happy path: Assigned -> InProgress");

    item.complete();
    check(item.status() == "AwaitingApproval", "Happy path: InProgress -> AwaitingApproval (complete needs approval)");
    check(!item.isComplete(), "Happy path: not complete until approved");

    item.approve();
    check(item.status() == "Completed", "Happy path: AwaitingApproval -> Completed");
    check(item.isComplete() && nearly(item.progress(), 1.0), "Happy path: Completed means complete, progress 1.0");
    check(item.state == "Completed", "Happy path: legacy 'state' mirror follows along");
}

static void test_TransitionMatrix() {
    std::cout << "\n--- Testing every state x operation (legal accepted, illegal refused) ---\n";
    const int rows = sizeof(kRows) / sizeof(kRows[0]);
    for (int r = 0; r < rows; ++r) {
        for (int o = 0; o < kOpCount; ++o) {
            LeafWorkItem item("m", "Matrix", "Developer");
            driveTo(item, kRows[r].state);
            std::string before = item.status();
            check(before == kRows[r].state, (std::string("setup reaches ") + kRows[r].state).c_str());

            bool legal = std::string(kRows[r].legalOps).find(std::string(" ") + kOps[o] + " ") != std::string::npos;
            bool accepted = tryOp(item, kOps[o]);

            std::string label = std::string(kRows[r].state) + " + " + kOps[o] + (legal ? " is accepted" : " is refused");
            check(accepted == legal, label.c_str());
            if (!legal) {
                check(item.status() == before, (label + " and leaves the state unchanged").c_str());
            }
        }
    }
}

static void test_RejectAndRework() {
    std::cout << "\n--- Testing rejection and rework ---\n";
    LeafWorkItem item("r1", "Rework me", "Developer");
    driveTo(item, "InProgress");

    item.reject("needs more tests");
    check(item.status() == "Rejected", "Reject: InProgress -> Rejected");
    check(item.reason() == "needs more tests", "Reject: the reason is stored");

    item.start();
    check(item.status() == "InProgress", "Rework: Rejected -> InProgress (same assignee carries on)");
    check(item.reason() == "needs more tests", "Rework: the reason is kept until reassignment");

    item.complete();
    item.reject("wrong approach");
    check(item.status() == "Rejected" && item.reason() == "wrong approach",
          "Reject: AwaitingApproval -> Rejected, reason replaced");

    item.assign("carol");
    check(item.status() == "Assigned" && item.assignee == "carol", "Reassign: Rejected -> Assigned to someone else");
    check(item.reason().empty(), "Reassign: a fresh assignment clears the rejection reason");
}

static void test_ReassignAndEscalate() {
    std::cout << "\n--- Testing reassignment and escalation ---\n";
    LeafWorkItem item("e1", "Escalate me", "Developer");
    driveTo(item, "Assigned");

    item.assign("bob");
    check(item.status() == "Assigned" && item.assignee == "bob", "Reassign: Assigned stays Assigned, assignee changes");

    item.escalate();
    check(item.status() == "Escalated", "Escalate: Assigned -> Escalated");
    check(throwsLogicError([&]{ item.start(); }), "Escalated: cannot be started before reassignment");
    check(throwsLogicError([&]{ item.complete(); }), "Escalated: cannot be completed before reassignment");

    item.assign("dave");
    check(item.status() == "Assigned" && item.assignee == "dave", "Escalated: assign -> Assigned again");

    item.start();
    item.escalate();
    check(item.status() == "Escalated", "Escalate: InProgress -> Escalated");
}

static void test_Cancel() {
    std::cout << "\n--- Testing cancel from every state ---\n";
    for (int s = 0; s < kStateCount; ++s) {
        LeafWorkItem item("c", "Cancel me", "Developer");
        driveTo(item, kStates[s]);
        item.cancel();
        std::string expected = (std::string(kStates[s]) == "Completed") ? "Completed" : "Cancelled";
        std::string label = std::string("Cancel from ") + kStates[s] + " gives " + expected;
        check(item.status() == expected, label.c_str());
    }

    LeafWorkItem item("c2", "Cancelled", "Developer");
    item.cancel();
    check(!tryOp(item, "assign") && !tryOp(item, "start"), "Cancel: a cancelled item accepts no further work");
}

static void test_ErrorMessages() {
    std::cout << "\n--- Testing illegal-transition messages ---\n";
    LeafWorkItem item("x", "Message", "Developer");
    check(throwsLogicErrorContaining([&]{ item.start(); }, "Illegal transition"), "Message: says it is an illegal transition");
    check(throwsLogicErrorContaining([&]{ item.start(); }, "start"), "Message: names the operation");
    check(throwsLogicErrorContaining([&]{ item.start(); }, "Created"), "Message: names the current state");

    CompletedState done;
    check(throwsLogicErrorContaining([&]{ done.escalate(item); }, "Completed"),
          "Message: the base-class helper reports the state that refused");
}

static void test_Restore() {
    std::cout << "\n--- Testing restore() (used by Memento) ---\n";
    LeafWorkItem item("s1", "Snapshot me", "Developer");

    item.restore("InProgress", "erin", "");
    check(item.status() == "InProgress" && item.assignee == "erin", "Restore: sets state and assignee");
    check(item.state == "InProgress", "Restore: legacy 'state' mirror follows");
    item.complete();
    check(item.status() == "AwaitingApproval", "Restore: the restored state object behaves correctly");

    item.restore("Rejected", "frank", "bad naming");
    check(item.status() == "Rejected" && item.reason() == "bad naming", "Restore: sets the reason too");

    check(throwsInvalidArgument([&]{ item.restore("Nonsense", "", ""); }), "Restore: an unknown state name throws");
    check(item.status() == "Rejected", "Restore: a failed restore leaves the item untouched");
}

static void test_NullTransition() {
    std::cout << "\n--- Testing transitionTo(nullptr) ---\n";
    LeafWorkItem item("n1", "Null", "Developer");
    check(throwsInvalidArgument([&]{ item.transitionTo(std::unique_ptr<WorkItemState>()); }),
          "transitionTo: a null state is rejected");
    check(item.status() == "Created", "transitionTo: the item is unchanged after the rejection");
}

static void test_PolymorphicUse() {
    std::cout << "\n--- Testing use through the WorkItem / WorkNode interfaces ---\n";
    LeafWorkItem leaf("p1", "Poly", "Developer");
    WorkItem& asItem = leaf;
    asItem.makeAvailable();
    asItem.assign("gina");
    check(leaf.status() == "Assigned", "Polymorphism: WorkItem calls reach the State machine");

    WorkNode& asNode = leaf;
    check(!asNode.isComplete() && nearly(asNode.progress(), 0.0), "Polymorphism: WorkNode queries work");
    asNode.cancel();
    check(leaf.status() == "Cancelled", "Polymorphism: WorkNode::cancel() reaches LeafWorkItem::cancel()");
}

// ============================================================================
// MAIN
// ============================================================================
void run_workitem_state_tests() {
    std::cout << "\n--- Running WorkItem State Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_StateNames();
    test_InitialState();
    test_HappyPath();
    test_TransitionMatrix();
    test_RejectAndRework();
    test_ReassignAndEscalate();
    test_Cancel();
    test_ErrorMessages();
    test_Restore();
    test_NullTransition();
    test_PolymorphicUse();

    std::cout << "WorkItem State Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}