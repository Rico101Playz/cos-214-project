#include <iostream>
#include <string>
#include <memory>

#include "test_utils.h"

#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "patterns/execution/Memento.h"

#include "patterns/execution/WorkCommand.h"
#include "patterns/execution/CommandHistory.h"
#include "patterns/execution/CommandInvoker.h"

#include "patterns/execution/AddWorkCommand.h"
#include "patterns/execution/AssignCommand.h"
#include "patterns/execution/StartCommand.h"
#include "patterns/execution/CompleteCommand.h"
#include "patterns/execution/ApproveCommand.h"
#include "patterns/execution/RejectCommand.h"
#include "patterns/execution/EscalateCommand.h"
#include "patterns/execution/InvokeCommand.h"
#include "patterns/execution/CancelCommand.h"
#include "patterns/execution/SetDataCommand.h"

#include "patterns/structural/LeafWorkItem.h"

// One stage, one item — a live instance the commands can operate on.
static WorkflowDefinition singleItemDefinition() {
    WorkflowDefinition def;
    def.name = "Command Test";
    def.stages.push_back("Stage1");
    def.items.push_back("Item1");
    return def;
}

// -------------------------------------------------------------------------
// CommandHistory (unchanged behaviour, kept minimal)
// -------------------------------------------------------------------------
void test_CommandHistory() {
    std::cout << "\n--- Testing CommandHistory ---\n";

    CommandHistory history;
    SnapshotStore store;

    check(history.canUndo() == false, "CommandHistory: empty initially");

    history.push(std::unique_ptr<WorkCommand>(new StartCommand(&store, "WF-01", "Item1")));
    check(history.canUndo() == true, "CommandHistory: canUndo() after push");

    std::unique_ptr<WorkCommand> cmd = history.pop();
    check(cmd != nullptr, "CommandHistory: pop() returns non-null");
    check(history.canUndo() == false, "CommandHistory: empty after pop");
    check(history.pop() == nullptr, "CommandHistory: pop() on empty returns nullptr");
}

// -------------------------------------------------------------------------
// Invoker + undo round-trip
// -------------------------------------------------------------------------
void test_CommandInvoker() {
    std::cout << "\n--- Testing CommandInvoker ---\n";

    SnapshotStore store;
    CommandInvoker invoker;
    WorkflowInstance instance("WF-01", singleItemDefinition());
    instance.start();

    LeafWorkItem* item = static_cast<LeafWorkItem*>(instance.item("Item1"));
    check(item != nullptr, "Invoker: item exists after start()");
    check(item->status() == "Available", "Invoker: item is Available before the command");

    // Complete is illegal from Available — the invoker must propagate the throw
    // without corrupting state.
    std::unique_ptr<WorkCommand> badCmd(new CompleteCommand(&store, "WF-01", "Item1"));
    bool threw = false;
    try {
        invoker.invoke(std::move(badCmd), &instance);
    } catch (const std::logic_error&) {
        threw = true;
    }
    check(threw, "Invoker: an illegal command throws");
    check(item->status() == "Available",
          "Invoker: a rejected command leaves the item untouched");

    // A legal command round-trips through the memento.
    item->assign("Alice");
    item->start();   // -> InProgress
    check(item->status() == "InProgress", "Invoker: item is InProgress before complete()");

    std::unique_ptr<WorkCommand> okCmd(new CompleteCommand(&store, "WF-01", "Item1"));
    invoker.invoke(std::move(okCmd), &instance);
    check(item->status() == "AwaitingApproval",
          "Invoker: CompleteCommand moved item to AwaitingApproval");

    invoker.undoLast(&instance);
    check(item->status() == "InProgress",
          "Invoker: undoLast() restored the pre-command state");
}

// -------------------------------------------------------------------------
// Each concrete command: assert describe() AND the post-execute effect
// -------------------------------------------------------------------------
void test_ConcreteCommands() {
    std::cout << "\n--- Testing Concrete Commands ---\n";

    SnapshotStore store;
    CommandInvoker invoker;

    // --- AddWorkCommand: adds an item into a stage ---
    {
        WorkflowInstance inst("WF-ADD", singleItemDefinition());
        WorkflowDefinition emptyDef;
        emptyDef.name = "Empty";
        emptyDef.stages.push_back("Stage1");
        WorkflowInstance inst2("WF-ADD2", emptyDef);
        inst2.start();

        std::shared_ptr<LeafWorkItem> newItem(
            new LeafWorkItem("NewItem", "NewItem", "Developer"));
        AddWorkCommand cmd(&store, "WF-ADD2", "Stage1", newItem);
        check(cmd.describe() == "AddWorkCommand: stage=Stage1",
              "AddWorkCommand: describe()");
        // AddWorkCommand target is instance 2
        invoker.invoke(std::unique_ptr<WorkCommand>(new AddWorkCommand(
            &store, "WF-ADD2", "Stage1", newItem)), &inst2);
        check(inst2.item("NewItem") != nullptr,
              "AddWorkCommand: the item is now in the instance");
    }

    // --- AssignCommand on a live Available item ---
    {
        WorkflowInstance inst("WF-ASG", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));

        AssignCommand cmd(&store, "WF-ASG", "Item1", "Alice");
        check(cmd.describe() == "AssignCommand: item=Item1, participant=Alice",
              "AssignCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new AssignCommand(&store, "WF-ASG", "Item1", "Alice")), &inst);

        check(item->status() == "Assigned" && item->assignee() == "Alice",
              "AssignCommand: item is Assigned to Alice");
    }

    // --- StartCommand ---
    {
        WorkflowInstance inst("WF-STR", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));
        item->assign("Bob");

        StartCommand cmd(&store, "WF-STR", "Item1");
        check(cmd.describe() == "StartCommand: item=Item1",
              "StartCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new StartCommand(&store, "WF-STR", "Item1")), &inst);
        check(item->status() == "InProgress",
              "StartCommand: item is InProgress");
    }

    // --- CompleteCommand ---
    {
        WorkflowInstance inst("WF-CMP", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));
        item->assign("Bob");
        item->start();

        CompleteCommand cmd(&store, "WF-CMP", "Item1");
        check(cmd.describe() == "CompleteCommand: item=Item1",
              "CompleteCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new CompleteCommand(&store, "WF-CMP", "Item1")), &inst);
        check(item->status() == "AwaitingApproval",
              "CompleteCommand: item is AwaitingApproval");
    }

    // --- ApproveCommand ---
    {
        WorkflowInstance inst("WF-APR", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));
        item->assign("Bob");
        item->start();
        item->complete();

        ApproveCommand cmd(&store, "WF-APR", "Item1", true, "Looks good");
        check(cmd.describe() == "ApproveCommand: item=Item1, approved=true, comment=Looks good",
              "ApproveCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new ApproveCommand(&store, "WF-APR", "Item1", true, "Looks good")), &inst);
        check(item->status() == "Completed",
              "ApproveCommand: item is Completed");
    }

    // --- RejectCommand ---
    {
        WorkflowInstance inst("WF-REJ", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));
        item->assign("Bob");
        item->start();
        item->complete();

        RejectCommand cmd(&store, "WF-REJ", "Item1", "Missing data");
        check(cmd.describe() == "RejectCommand: item=Item1, reason=Missing data",
              "RejectCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new RejectCommand(&store, "WF-REJ", "Item1", "Missing data")), &inst);
        check(item->status() == "Rejected" && item->reason() == "Missing data",
              "RejectCommand: item is Rejected with the given reason");
    }

    // --- EscalateCommand ---
    {
        WorkflowInstance inst("WF-ESC", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));
        item->assign("Bob");

        EscalateCommand cmd(&store, "WF-ESC", "Item1");
        check(cmd.describe() == "EscalateCommand: item=Item1",
              "EscalateCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new EscalateCommand(&store, "WF-ESC", "Item1")), &inst);
        check(item->status() == "Escalated",
              "EscalateCommand: item is Escalated");
    }

    // --- SetDataCommand ---
    {
        WorkflowInstance inst("WF-DAT", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));

        SetDataCommand cmd(&store, "WF-DAT", "Item1", "color", "blue");
        check(cmd.describe() == "SetDataCommand: node=Item1, key=color, value=blue",
              "SetDataCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new SetDataCommand(&store, "WF-DAT", "Item1", "color", "blue")), &inst);
        check(item->getData("color") == "blue",
              "SetDataCommand: item data updated");
    }

    // --- CancelCommand (item-level) ---
    {
        WorkflowInstance inst("WF-CAN", singleItemDefinition());
        inst.start();
        LeafWorkItem* item = static_cast<LeafWorkItem*>(inst.item("Item1"));
        check(item->status() == "Available",
            "CancelCommand: item starts Available");

        CancelCommand cmd(&store, "WF-CAN", "Item1", "Timeout");
        check(cmd.describe() == "CancelCommand: node=Item1, reason=Timeout",
            "CancelCommand: describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new CancelCommand(&store, "WF-CAN", "Item1", "Timeout")), &inst);

        check(item->status() == "Cancelled",
            "CancelCommand: item is Cancelled");
        check(inst.state() == WorkflowInstance::RUNNING,
            "CancelCommand: cancelling one item does not cancel the instance");
    }

    // --- CancelCommand (instance-level) ---
    {
        WorkflowInstance inst("WF-CANI", singleItemDefinition());
        inst.start();

        CancelCommand cmd(&store, "WF-CANI", inst.id(), "Timeout");
        check(cmd.describe() == "CancelCommand: node=WF-CANI, reason=Timeout",
            "CancelCommand (instance): describe()");

        invoker.invoke(std::unique_ptr<WorkCommand>(
            new CancelCommand(&store, "WF-CANI", inst.id(), "Timeout")), &inst);

        check(inst.state() == WorkflowInstance::CANCELLED,
            "CancelCommand (instance): instance is Cancelled");
        check(inst.reason() == "Timeout",
            "CancelCommand (instance): the reason is recorded");
    }
}

void run_command_tests() {
    std::cout << "\n--- Running Command Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_CommandHistory();
    test_CommandInvoker();
    test_ConcreteCommands();

    std::cout << "Command Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}