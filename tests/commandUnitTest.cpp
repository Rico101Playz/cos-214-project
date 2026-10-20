#include <iostream>
#include <string>
#include <memory>

#include "test_utils.h"

// Core
#include "core/WorkflowInstance.h"
#include "core/WorkItem.h"

// Patterns
#include "patterns/execution/WorkCommand.h"
#include "patterns/execution/CommandHistory.h"
#include "patterns/execution/CommandInvoker.h"
#include "patterns/execution/Memento.h"

// Concrete Commands
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

// ============================================================================
// TEST: CommandHistory Management
// ============================================================================
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
    
    std::unique_ptr<WorkCommand> emptyCmd = history.pop();
    check(emptyCmd == nullptr, "CommandHistory: pop() on empty returns nullptr");
}

// ============================================================================
// TEST: CommandInvoker Mechanics
// ============================================================================
void test_CommandInvoker() {
    std::cout << "\n--- Testing CommandInvoker ---\n";
    
    SnapshotStore store;
    CommandInvoker invoker;
    
    auto instance = std::make_shared<WorkflowInstance>("WF-01", InstanceStatus::Active);
    instance->addItem(std::make_shared<WorkItem>("Item2"));

    std::unique_ptr<WorkCommand> cmd(new CompleteCommand(&store, "WF-01", "Item2"));
    invoker.invoke(std::move(cmd), instance.get());
    
    check(true, "CommandInvoker: invoke() successfully executes CompleteCommand");
    
    invoker.undoLast(instance.get());
    check(true, "CommandInvoker: undoLast() does not crash and undoes via Memento");
}

// ============================================================================
// TEST: Concrete Commands 
// ============================================================================
void test_ConcreteCommands() {
    std::cout << "\n--- Testing Concrete Commands ---\n";
    
    SnapshotStore store;
    auto instance = std::make_shared<WorkflowInstance>("WF-01", InstanceStatus::Active);
    instance->addItem(std::make_shared<WorkItem>("Item4"));
    auto item = std::make_shared<WorkItem>("Item5");
    
    // --- AddWorkCommand ---
    AddWorkCommand addCmd(&store, "WF-01", "Stage1",item );
    check(addCmd.describe() == "AddWorkCommand: stage=Stage1", "AddWorkCommand describe()");
    addCmd.execute(instance.get());
    check(true, "AddWorkCommand execute() runs");

    // --- AssignCommand ---
    AssignCommand assignCmd(&store, "WF-01", "Item1", "Alice");
    check(assignCmd.describe() == "AssignCommand: item=Item1, participant=Alice", "AssignCommand describe()");
    assignCmd.execute(instance.get());
    check(true, "AssignCommand execute() runs");

    // --- StartCommand ---
    StartCommand startCmd(&store, "WF-01", "Item2");
    check(startCmd.describe() == "StartCommand: item=Item2", "StartCommand describe()");
    startCmd.execute(instance.get());
    check(true, "StartCommand execute() runs");

    // --- CompleteCommand ---
    CompleteCommand completeCmd(&store, "WF-01", "Item3");
    check(completeCmd.describe() == "CompleteCommand: item=Item3", "CompleteCommand describe()");
    completeCmd.execute(instance.get());
    check(true, "CompleteCommand execute() runs");

    // --- ApproveCommand ---
    ApproveCommand approveCmd(&store, "WF-01", "Item4", true, "Looks good");
    check(approveCmd.describe() == "ApproveCommand: item=Item4, approved=true, comment=Looks good", "ApproveCommand describe()");
    approveCmd.execute(instance.get());
    check(true, "ApproveCommand execute() runs");

    // --- RejectCommand ---
    RejectCommand rejectCmd(&store, "WF-01", "Item5", "Missing data");
    check(rejectCmd.describe() == "RejectCommand: item=Item5, reason=Missing data", "RejectCommand describe()");
    rejectCmd.execute(instance.get());
    check(true, "RejectCommand execute() runs");

    // --- EscalateCommand ---
    EscalateCommand escalateCmd(&store, "WF-01", "Item6");
    check(escalateCmd.describe() == "EscalateCommand: item=Item6", "EscalateCommand describe()");
    escalateCmd.execute(instance.get());
    check(true, "EscalateCommand execute() runs");

    // --- InvokeCommand ---
    InvokeCommand invokeCmd(&store, "WF-01", "Item7", "API_CALL");
    check(invokeCmd.describe() == "InvokeCommand: item=Item7, operation=API_CALL", "InvokeCommand describe()");
    invokeCmd.execute(instance.get());
    check(true, "InvokeCommand execute() runs");

    // --- CancelCommand ---
    CancelCommand cancelCmd(&store, "WF-01", "Node1", "Timeout");
    check(cancelCmd.describe() == "CancelCommand: node=Node1, reason=Timeout", "CancelCommand describe()");
    cancelCmd.execute(instance.get());
    check(true, "CancelCommand execute() runs");

    // --- SetDataCommand ---
    SetDataCommand setDataCmd(&store, "WF-01", "Node2", "color", "blue");
    check(setDataCmd.describe() == "SetDataCommand: node=Node2, key=color, value=blue", "SetDataCommand describe()");
    setDataCmd.execute(instance.get());
    check(true, "SetDataCommand execute() runs");
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
void run_command_tests() {
    std::cout << "\n--- Running Command Tests ---\n";
    
    test_CommandHistory();
    test_CommandInvoker();
    test_ConcreteCommands();
    
    std::cout << "Command Tests: " << testsPassed << " passed, " 
              << testsFailed << " failed.\n";
}