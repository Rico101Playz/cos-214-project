#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>

#include "composite/CompositeTask.h"
#include "composite/LeafTask.h"
#include "decorator/ApprovalWork.h"
#include "decorator/BlockedWork.h"
#include "decorator/ValidatedWork.h"
#include "mediator/Request.h"

namespace {

Request ask(Action action) { return Request(action, TaskRef("p", "")); }

// A stage holding two project tasks, "first" and "second", both released.
struct Board {
    CompositeTask stage;
    Task* first;
    Task* second;
    Board() : stage("board", "Board") {
        first = &stage.add(decorateProjectTask(std::unique_ptr<Task>(new LeafTask("first", "First", "Developer"))));
        second = &stage.add(decorateProjectTask(std::unique_ptr<Task>(new LeafTask("second", "Second", "Developer"))));
    }
};

void finish(Task* task) {
    task->handle(ask(Action::Assign).with("participant", "Bob"));
    task->handle(ask(Action::Start));
    task->handle(ask(Action::Complete));
    task->handle(ask(Action::Approve));
}

} // namespace

TEST(DecoratorTest, ProjectTasksWearEveryDecorator) {
    Board board;
    EXPECT_NE(board.first->blockedWork(), nullptr) << "Decorator: a project task can be blocked";
    EXPECT_NE(board.first->validatedWork(), nullptr) << "Decorator: a project task can require data";
    EXPECT_NE(board.first->approvalWork(), nullptr) << "Decorator: a project task can need approval";
    LeafTask* work = board.first->leaf();
    ASSERT_NE(work, nullptr);
    EXPECT_EQ(&work->outermost(), board.first) << "Decorator: the task knows its outermost wrapper";
    EXPECT_TRUE(board.first->validatedWork()->requiredKeys().empty()) << "Decorator: nothing is required by default";
}

TEST(DecoratorTest, BlockedWorkRefusesUntilItsBlockerIsDone) {
    Board board;
    board.second->blockedWork()->addBlocker("first");
    EXPECT_THROW(board.second->blockedWork()->addBlocker("second"), std::invalid_argument) << "Blocked: a task cannot block itself";
    board.first->leaf()->makeAvailable();
    board.second->leaf()->makeAvailable();
    EXPECT_EQ(board.second->leaf()->status(), LeafTask::CREATED) << "Blocked: a blocked task is not released";
    EXPECT_TRUE(board.second->leaf()->isBlocked());
    EXPECT_THROW(board.second->handle(ask(Action::Assign).with("participant", "Bob")), std::logic_error)
        << "Blocked: Assign is refused while a blocker is open";

    finish(board.first);
    EXPECT_FALSE(board.second->leaf()->isBlocked()) << "Blocked: a done blocker no longer blocks";
    EXPECT_EQ(board.second->leaf()->status(), LeafTask::AVAILABLE) << "Blocked: the waiting task opens once unblocked";
    board.second->handle(ask(Action::Assign).with("participant", "Bob"));
    EXPECT_EQ(board.second->leaf()->assignee(), "Bob");
}

TEST(DecoratorTest, ValidatedWorkRefusesCompleteWhileDataIsMissing) {
    Board board;
    board.first->validatedWork()->require("link");
    board.first->leaf()->makeAvailable();
    board.first->handle(ask(Action::Assign).with("participant", "Bob"));
    board.first->handle(ask(Action::Start));
    EXPECT_THROW(board.first->handle(ask(Action::Complete)), std::logic_error) << "Validated: Complete needs every required field";
    EXPECT_EQ(board.first->leaf()->status(), LeafTask::IN_PROGRESS) << "Validated: a refused Complete leaves the task as it was";

    board.first->handle(ask(Action::SetData).with("key", "link").with("value", "https://example.test/pr/1"));
    EXPECT_TRUE(board.first->validatedWork()->missingKeys().empty());
    board.first->handle(ask(Action::Complete));
    EXPECT_EQ(board.first->leaf()->status(), LeafTask::AWAITING_APPROVAL) << "Validated: Complete goes through once the data is there";
}

TEST(DecoratorTest, FirstSubtaskTurnsTheTaskIntoAComposite) {
    Board board;
    LeafTask* parent = board.first->leaf();
    parent->makeAvailable();
    parent->assign("Bob");
    CompositeTask::addSubtask(*parent, decorateProjectTask(std::unique_ptr<Task>(new LeafTask("sub", "Sub", "Developer"))));

    Task* node = board.stage.find("first");
    CompositeTask* group = dynamic_cast<CompositeTask*>(node);
    ASSERT_NE(group, nullptr) << "Composite: the task's node is now a CompositeTask";
    EXPECT_FALSE(group->isStage()) << "Composite: it is a task with sub-tasks, not a stage";
    EXPECT_EQ(stageOf(node), nullptr);
    EXPECT_EQ(node->leaf(), parent) << "Composite: it keeps the task as its own work";
    EXPECT_EQ(&parent->outermost(), node) << "Composite: the task's outermost node is the CompositeTask";
    EXPECT_NE(node->approvalWork(), nullptr) << "Composite: the task keeps its decorators";
    EXPECT_EQ(board.stage.childCount(), 2u) << "Composite: it takes the task's place in the stage";
    EXPECT_EQ(node->parent(), &board.stage);
    EXPECT_EQ(parent->parent(), &board.stage);

    LeafTask* sub = leafOf(board.stage.find("sub"));
    ASSERT_NE(sub, nullptr);
    EXPECT_EQ(sub->parentItem(), parent) << "Composite: the sub-task's parent item is the task";
    EXPECT_EQ(parent->openSubtasks().size(), 1u);

    CompositeTask::addSubtask(*parent, decorateProjectTask(std::unique_ptr<Task>(new LeafTask("sub2", "Sub 2", "Developer"))));
    EXPECT_EQ(board.stage.find("first"), node) << "Composite: later sub-tasks join the same node";
    EXPECT_EQ(node->childCount(), 2u);

    node->handle(ask(Action::Start));
    EXPECT_EQ(parent->status(), LeafTask::IN_PROGRESS) << "Composite: Requests reach the task's own work";
    EXPECT_THROW(node->handle(ask(Action::Complete)), std::logic_error) << "Composite: open sub-tasks hold the task back";

    node->cancel();
    EXPECT_EQ(sub->status(), LeafTask::CANCELLED) << "Composite: cancelling the task cancels its sub-tasks";
    EXPECT_EQ(parent->status(), LeafTask::CANCELLED);
}
