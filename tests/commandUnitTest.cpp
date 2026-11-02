#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <memory>


#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "memento/Memento.h"

#include "command/Command.h"
#include "command/CommandHistory.h"
#include "command/CommandInvoker.h"
#include "command/RequestCommand.h"

#include "composite/LeafTask.h"
#include "project_test_helpers.h"

// One stage, one item — a live instance the commands can operate on.
static WorkflowDefinition singleItemDefinition() {
    WorkflowDefinition def;
    def.name = "Command Test";
    def.stages.push_back("Stage1");
    def.items.push_back("Item1");
    return def;
}

// A RequestCommand for the action on one item of one project, run by the ProjectFacade.
static RequestCommand* command(ProjectFacade& projects, SnapshotStore& store, Action action, const std::string& projectId,
                               const std::string& taskId = "") {
    return new RequestCommand(&projects, Request(action, TaskRef(projectId, taskId)), &store);
}

// -------------------------------------------------------------------------
// CommandHistory (unchanged behaviour, kept minimal)
// -------------------------------------------------------------------------
TEST(Command, History) {

    CommandHistory history;
    SnapshotStore store;
    ProjectFacade projects;

    EXPECT_FALSE(history.canUndo()) << "CommandHistory: empty initially";

    history.push(std::unique_ptr<Command>(command(projects, store, Action::Start, "WF-01", "Item1")));
    EXPECT_TRUE(history.canUndo()) << "CommandHistory: canUndo() after push";

    std::unique_ptr<Command> cmd = history.pop();
    EXPECT_NE(cmd, nullptr) << "CommandHistory: pop() returns non-null";
    EXPECT_FALSE(history.canUndo()) << "CommandHistory: empty after pop";
    EXPECT_EQ(history.pop(), nullptr) << "CommandHistory: pop() on empty returns nullptr";
}

// -------------------------------------------------------------------------
// Invoker + undo round-trip
// -------------------------------------------------------------------------
TEST(Command, Invoker) {

    SnapshotStore store;
    ProjectFacade projects;
    CommandInvoker invoker;
    WorkflowInstance& instance = adopt(projects, "WF-01", singleItemDefinition());
    instance.start();

    LeafTask* item = static_cast<LeafTask*>(instance.item("Item1"));
    EXPECT_NE(item, nullptr) << "Invoker: item exists after start()";
    EXPECT_TRUE(item->status() == "Available") << "Invoker: item is Available before the command";

    // Complete is illegal from Available — the invoker must propagate the throw
    // without corrupting state.
    std::unique_ptr<Command> badCmd(command(projects, store, Action::Complete, "WF-01", "Item1"));
    bool threw = false;
    try {
        invoker.invoke(std::move(badCmd));
    } catch (const std::logic_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw) << "Invoker: an illegal command throws";
    EXPECT_TRUE(item->status() == "Available") << "Invoker: a rejected command leaves the item untouched";

    // A legal command round-trips through the memento.
    item->assign("Alice");
    item->start();   // -> InProgress
    EXPECT_TRUE(item->status() == "InProgress") << "Invoker: item is InProgress before complete()";

    std::unique_ptr<Command> okCmd(command(projects, store, Action::Complete, "WF-01", "Item1"));
    invoker.invoke(std::move(okCmd));
    EXPECT_TRUE(item->status() == "AwaitingApproval") << "Invoker: Complete moved item to AwaitingApproval";

    invoker.undoLast();
    EXPECT_TRUE(item->status() == "InProgress") << "Invoker: undoLast() restored the pre-command state";
}

// -------------------------------------------------------------------------
// Each action a RequestCommand performs: assert describe() AND the effect
// -------------------------------------------------------------------------
TEST(Command, RequestCommands) {

    SnapshotStore store;
    ProjectFacade projects;
    CommandInvoker invoker;

    // --- AddTask: adds a prepared item into a stage ---
    {
        WorkflowDefinition emptyDef;
        emptyDef.name = "Empty";
        emptyDef.stages.push_back("Stage1");
        WorkflowInstance& inst2 = adopt(projects, "WF-ADD2", emptyDef);
        inst2.start();

        Request add = Request(Action::AddTask, TaskRef("WF-ADD2")).with("parent", "Stage1").with("name", "NewItem")
                          .with("role", "Developer");
        RequestCommand cmd(&projects, add, &store);
        EXPECT_EQ(cmd.describe(), "AddTask: name=NewItem, parent=Stage1, role=Developer") << "AddTask: describe()";
        Response added = invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, add, &store)));
        ASSERT_NE(inst2.item(added.data("item")), nullptr) << "AddTask: the new task is in the instance under the id the answer gives";
        EXPECT_EQ(inst2.item(added.data("item"))->name(), "NewItem") << "AddTask: the facade built the task from the Request";
    }

    // --- Assign on a live Available item ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-ASG", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));

        Request assign = Request(Action::Assign, TaskRef("WF-ASG", "Item1")).with("participant", "Alice");
        RequestCommand cmd(&projects, assign, &store);
        EXPECT_EQ(cmd.describe(), "Assign: task=Item1, participant=Alice") << "Assign: describe()";

        invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, assign, &store)));

        EXPECT_TRUE(item->status() == "Assigned" && item->assignee() == "Alice") << "Assign: item is Assigned to Alice";
    }

    // --- Start ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-STR", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));
        item->assign("Bob");

        std::unique_ptr<RequestCommand> cmd(command(projects, store, Action::Start, "WF-STR", "Item1"));
        EXPECT_EQ(cmd->describe(), "Start: task=Item1") << "Start: describe()";

        invoker.invoke(std::unique_ptr<Command>(cmd.release()));
        EXPECT_TRUE(item->status() == "InProgress") << "Start: item is InProgress";
    }

    // --- Complete ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-CMP", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));
        item->assign("Bob");
        item->start();

        std::unique_ptr<RequestCommand> cmd(command(projects, store, Action::Complete, "WF-CMP", "Item1"));
        EXPECT_EQ(cmd->describe(), "Complete: task=Item1") << "Complete: describe()";

        invoker.invoke(std::unique_ptr<Command>(cmd.release()));
        EXPECT_TRUE(item->status() == "AwaitingApproval") << "Complete: item is AwaitingApproval";
    }

    // --- Approve ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-APR", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));
        item->assign("Bob");
        item->start();
        item->complete();

        Request approve = Request(Action::Approve, TaskRef("WF-APR", "Item1")).with("comment", "Looks good");
        RequestCommand cmd(&projects, approve, &store);
        EXPECT_EQ(cmd.describe(), "Approve: task=Item1, comment=Looks good") << "Approve: describe()";

        invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, approve, &store)));
        EXPECT_TRUE(item->status() == "Completed") << "Approve: item is Completed";
    }

    // --- Reject ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-REJ", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));
        item->assign("Bob");
        item->start();
        item->complete();

        Request reject = Request(Action::Reject, TaskRef("WF-REJ", "Item1")).with("reason", "Missing data");
        RequestCommand cmd(&projects, reject, &store);
        EXPECT_EQ(cmd.describe(), "Reject: task=Item1, reason=Missing data") << "Reject: describe()";

        invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, reject, &store)));
        EXPECT_TRUE(item->status() == "Rejected" && item->reason() == "Missing data") << "Reject: item is Rejected with the given reason";
    }

    // --- Escalate ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-ESC", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));
        item->assign("Bob");

        std::unique_ptr<RequestCommand> cmd(command(projects, store, Action::Escalate, "WF-ESC", "Item1"));
        EXPECT_EQ(cmd->describe(), "Escalate: task=Item1") << "Escalate: describe()";

        invoker.invoke(std::unique_ptr<Command>(cmd.release()));
        EXPECT_TRUE(item->status() == "Escalated") << "Escalate: item is Escalated";
    }

    // --- SetData ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-DAT", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));

        Request set = Request(Action::SetData, TaskRef("WF-DAT", "Item1")).with("key", "color").with("value", "blue");
        RequestCommand cmd(&projects, set, &store);
        EXPECT_EQ(cmd.describe(), "SetData: task=Item1, key=color, value=blue") << "SetData: describe()";

        invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, set, &store)));
        EXPECT_TRUE(item->getData("color") == "blue") << "SetData: item data updated";
    }

    // --- CancelProject naming one task ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-CAN", singleItemDefinition());
        inst.start();
        LeafTask* item = static_cast<LeafTask*>(inst.item("Item1"));
        EXPECT_TRUE(item->status() == "Available") << "Cancel task: item starts Available";

        Request cancel = Request(Action::CancelProject, TaskRef("WF-CAN", "Item1")).with("reason", "Timeout");
        RequestCommand cmd(&projects, cancel, &store);
        EXPECT_EQ(cmd.describe(), "CancelProject: task=Item1, reason=Timeout") << "Cancel task: describe()";

        invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, cancel, &store)));

        EXPECT_TRUE(item->status() == "Cancelled") << "Cancel task: item is Cancelled";
        EXPECT_EQ(inst.state(), WorkflowInstance::RUNNING) << "Cancel task: cancelling one item does not cancel the instance";
    }

    // --- CancelProject for the whole project ---
    {
        WorkflowInstance& inst = adopt(projects, "WF-CANI", singleItemDefinition());
        inst.start();

        Request cancel = Request(Action::CancelProject, TaskRef("WF-CANI")).with("reason", "Timeout");
        RequestCommand cmd(&projects, cancel, &store);
        EXPECT_EQ(cmd.describe(), "CancelProject: reason=Timeout") << "Cancel project: describe()";

        invoker.invoke(std::unique_ptr<Command>(new RequestCommand(&projects, cancel, &store)));

        EXPECT_EQ(inst.state(), WorkflowInstance::CANCELLED) << "Cancel project: instance is Cancelled";
        EXPECT_EQ(inst.reason(), "Timeout") << "Cancel project: the reason is recorded";
    }
}
