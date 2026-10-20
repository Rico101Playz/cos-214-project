#include <iostream>
#include <string>
#include <memory>

#include "test_utils.h"

#include "core/WorkflowInstance.h"
#include "patterns/execution/ApproveCommand.h"
#include "patterns/execution/CommandInvoker.h"
#include "patterns/execution/Memento.h"

// ============================================================================
// SIMPLE TEST RUNNER
// ============================================================================
// ============================================================================
// TEST: ApproveCommand with Memento & CommandInvoker
// ============================================================================
void test_ApproveCommand_And_Undo() {
    std::cout << "\n--- Testing ApproveCommand + Memento Undo ---\n";

    auto instance = std::make_shared<WorkflowInstance>("WF-001", InstanceStatus::Active);
    auto task = std::make_shared<WorkItem>("Task-1");
    instance->addItem(task);

    SnapshotStore store;
    CommandInvoker invoker;

    check(instance->getItem("Task-1")->state == "Pending", 
          "Initial state should be 'Pending'");

    auto cmd = std::unique_ptr<ApproveCommand>(new ApproveCommand(
        &store,
        instance->getId(),
        "Task-1",
        true,
        "Looks good"
    ));

    invoker.invoke(std::move(cmd), instance.get());

    check(instance->getItem("Task-1")->state == "Approved", 
          "State should be 'Approved' after command execution");

    std::string expectedLabel = "ApproveCommand: item=Task-1, approved=true, comment=Looks good";
    check(store.get("WF-001", expectedLabel) != nullptr, 
          "Snapshot should exist in the SnapshotStore");

    invoker.undoLast(instance.get());

    check(instance->getItem("Task-1")->state == "Pending", 
          "State should be reverted to 'Pending' after Undo");
}

// ============================================================================
// Test method
// ============================================================================
void run_memento_test() {
    std::cout << "\n--- Running Memento Tests ---\n";
    
    test_ApproveCommand_And_Undo();
    
    std::cout << "Memento Tests: " << testsPassed << " passed, " 
              << testsFailed << " failed.\n";
    
}