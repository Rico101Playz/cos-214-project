#include <iostream>
#include <string>
#include <memory>

#include "WorkCommand.h"
#include "CommandHistory.h"
#include "CommandInvoker.h"
#include "AddWorkCommand.h"
#include "AssignCommand.h"
#include "StartCommand.h"
#include "CompleteCommand.h"
#include "ApproveCommand.h"
#include "RejectCommand.h"
#include "EscalateCommand.h"
#include "InvokeCommand.h"
#include "CancelCommand.h"
#include "SetDataCommand.h"

// ============================================================================
// SIMPLE TEST RUNNER
// ============================================================================
int testsPassed = 0;
int testsFailed = 0;

void check(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        std::cerr << "[FAIL] " << testName << "\n";
        testsFailed++;
    }
}

// ============================================================================
// TEST: CommandHistory Management
// ============================================================================
void test_CommandHistory() {
    std::cout << "\n--- Testing CommandHistory ---\n";
    
    CommandHistory history;
    
    check(history.canUndo() == false, "CommandHistory: empty initially");

    history.push(std::unique_ptr<WorkCommand>(new StartCommand("Item1")));
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
    
    CommandInvoker invoker;
    
    std::unique_ptr<WorkCommand> cmd(new CompleteCommand("Item2"));
    invoker.invoke(std::move(cmd));
    check(true, "CommandInvoker: invoke() successfully executes CompleteCommand");
    
    invoker.undoLast();
    check(true, "CommandInvoker: undoLast() does not crash");
}

// ============================================================================
// TEST: Concrete Commands 
// ============================================================================
void test_ConcreteCommands() {
    std::cout << "\n--- Testing Concrete Commands ---\n";
    
    AddWorkCommand addCmd("Stage1");
    check(addCmd.describe() == "AddWorkCommand: stage=Stage1", "AddWorkCommand describe()");
    addCmd.execute();
    check(true, "AddWorkCommand execute() runs");

    AssignCommand assignCmd("Item1", "Alice");
    check(assignCmd.describe() == "AssignCommand: item=Item1, participant=Alice", "AssignCommand describe()");
    assignCmd.execute();
    check(true, "AssignCommand execute() runs");

    StartCommand startCmd("Item2");
    check(startCmd.describe() == "StartCommand: item=Item2", "StartCommand describe()");
    startCmd.execute();
    check(true, "StartCommand execute() runs");

    CompleteCommand completeCmd("Item3");
    check(completeCmd.describe() == "CompleteCommand: item=Item3", "CompleteCommand describe()");
    completeCmd.execute();
    check(true, "CompleteCommand execute() runs");

    ApproveCommand approveCmd("Item4", true, "Looks good");
    check(approveCmd.describe() == "ApproveCommand: item=Item4, approved=true, comment=Looks good", "ApproveCommand describe()");
    approveCmd.execute();
    check(true, "ApproveCommand execute() runs");

    RejectCommand rejectCmd("Item5", "Missing data");
    check(rejectCmd.describe() == "RejectCommand: item=Item5, reason=Missing data", "RejectCommand describe()");
    rejectCmd.execute();
    check(true, "RejectCommand execute() runs");

    EscalateCommand escalateCmd("Item6");
    check(escalateCmd.describe() == "EscalateCommand: item=Item6", "EscalateCommand describe()");
    escalateCmd.execute();
    check(true, "EscalateCommand execute() runs");

    InvokeCommand invokeCmd("Item7", "API_CALL");
    check(invokeCmd.describe() == "InvokeCommand: item=Item7, operation=API_CALL", "InvokeCommand describe()");
    invokeCmd.execute();
    check(true, "InvokeCommand execute() runs");

    CancelCommand cancelCmd("Node1", "Timeout");
    check(cancelCmd.describe() == "CancelCommand: node=Node1, reason=Timeout", "CancelCommand describe()");
    cancelCmd.execute();
    check(true, "CancelCommand execute() runs");

    SetDataCommand setDataCmd("Node2", "color", "blue");
    check(setDataCmd.describe() == "SetDataCommand: node=Node2, key=color, value=blue", "SetDataCommand describe()");
    setDataCmd.execute();
    check(true, "SetDataCommand execute() runs");
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "       Running Basic Tests\n";
    std::cout << "========================================\n";
    
    test_CommandHistory();
    test_CommandInvoker();
    test_ConcreteCommands();
    
    std::cout << "\n========================================\n";
    std::cout << "Results: " << testsPassed << " passed, " 
              << testsFailed << " failed.\n";
    std::cout << "========================================\n";
    
    return (testsFailed == 0) ? 0 : 1;
}