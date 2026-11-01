#ifndef COMPOSITETASK_H
#define COMPOSITETASK_H

#include <memory>
#include <string>
#include <vector>
#include "composite/TaskNode.h"
#include "strategy/StageMode.h"

class LeafTask;

// COMPOSITE pattern - the COMPOSITE.
//
// A node that owns an ordered list of child nodes. It is either a stage (or
// sub-process), whose completion and progress are worked out from its
// children, so a whole tree can be asked "are you done?" through the root
// alone; or a task with sub-tasks, which keeps the task itself as its own
// work: the task's status, approval and Requests stay the task's, and the
// sub-tasks are its children.
class CompositeTask : public TaskNode {
private:
    std::vector<std::unique_ptr<Task> > children_;
    std::unique_ptr<Task> work_;    // the task's own (decorated) work; null for a stage

public:
    CompositeTask(const std::string& id, const std::string& name);
    // A task with sub-tasks: takes over the task's place and keeps it as its own work.
    explicit CompositeTask(std::unique_ptr<Task> work);

    // True for a stage, false for a task with sub-tasks.
    bool isStage() const;
    // Adds a sub-task under a task. The first one turns the task's node into a
    // CompositeTask that keeps the task as its own work. Returns the sub-task.
    static Task& addSubtask(LeafTask& parent, std::unique_ptr<Task> subtask);
    std::unique_ptr<StageMode> mode_;
    StageMode& mode();

    // Takes ownership and returns the added node.
    // Throws std::invalid_argument for a null child.
    Task& add(std::unique_ptr<Task> child);

    // Removes a direct child by id; returns false when there is no such child.
    bool remove(const std::string& id);
    // Puts a direct child task inside a new CompositeTask that keeps it as its
    // own work, in the child's place, and returns that CompositeTask.
    CompositeTask& groupChild(Task& child);

    void setParent(Task* parent) override;
    bool isReady() const override;
    LeafTask* leaf() override;
    ApprovalWork* approvalWork() override;
    BlockedWork* blockedWork() override;
    ValidatedWork* validatedWork() override;
    // A task with sub-tasks passes the Request to its own work; a stage refuses it.
    void handle(const Request& request) override;

    std::size_t childCount() const override;
    Task* child(std::size_t index) const override;

    // A stage is complete when every child is (an empty stage is complete);
    // a task with sub-tasks when its own work is.
    bool isComplete() const override;
    // The average progress of the children (1.0 for an empty stage); a task
    // with sub-tasks reports its own work's progress.
    double progress() const override;
    // Cancels every child; a task with sub-tasks cancels its own work, which
    // cancels the sub-tasks.
    void cancel() override;

    void setMode(std::unique_ptr<StageMode> mode);

    void activate();
    
    void childCompleted(Task& child);
};

// The stage a node is, or nullptr when the node is a task (with or without sub-tasks).
CompositeTask* stageOf(Task* node);

#endif // COMPOSITETASK_H
