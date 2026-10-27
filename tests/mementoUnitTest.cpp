#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <memory>


#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "patterns/execution/ApproveCommand.h"
#include "patterns/execution/CommandInvoker.h"
#include "patterns/execution/Memento.h"
#include "patterns/structural/LeafWorkItem.h"

// A minimal definition: one stage holding one item.
static WorkflowDefinition oneItemDefinition() {
    WorkflowDefinition def;
    def.name = "Memento Test";
    def.stages.push_back("Work");
    def.items.push_back("Task-1");
    return def;
}

TEST(Memento, ApproveCommandAndUndo) {

    // Build a live instance through the real constructor.
    std::shared_ptr<WorkflowInstance> instance(
        new WorkflowInstance("WF-001", oneItemDefinition()));

    // Drive the item into a state where approve() is legal:
    // Created -> Available -> Assigned -> InProgress -> AwaitingApproval.
    instance->start();
    LeafWorkItem* task = static_cast<LeafWorkItem*>(instance->item("Task-1"));
    EXPECT_NE(task, nullptr) << "Setup: the item exists";
    EXPECT_TRUE(instance->state() == WorkflowInstance::RUNNING) << "Setup: instance is Running after start()";
    EXPECT_TRUE(task->status() == "Available") << "Setup: item is Available after start()";

    task->assign("Alice");
    task->start();
    task->complete();
    EXPECT_TRUE(task->status() == "AwaitingApproval") << "Setup: item is AwaitingApproval before approve()";

    SnapshotStore store;
    CommandInvoker invoker;
    std::string label = "ApproveCommand: item=Task-1, approved=true, comment=Looks good";
    EXPECT_EQ(store.get("WF-001", label), nullptr) << "Pre: no snapshot exists before the command runs";

    // Invoke the command (which snapshots, then executes).
    std::unique_ptr<ApproveCommand> cmd(
        new ApproveCommand(&store, "WF-001", "Task-1", true, "Looks good"));
    invoker.invoke(std::move(cmd), instance.get());

    EXPECT_TRUE(task->status() == "Completed") << "Execute: item is Completed after approve()";
    EXPECT_NE(store.get("WF-001", label), nullptr) << "Execute: the snapshot was stored under the command's label";

    // Undo: the memento restores the polymorphic state object.
    invoker.undoLast(instance.get());

    EXPECT_TRUE(task->status() == "AwaitingApproval") << "Undo: item is back to AwaitingApproval";
    EXPECT_TRUE(task->isComplete() == false) << "Undo: the restored item is no longer complete";

    // Prove that the *polymorphic* state was restored, not just the string:
    // approve() must work again on the restored object.
    task->approve();
    EXPECT_TRUE(task->status() == "Completed") << "Undo: the restored state object behaves correctly (approve() works again)";
}
