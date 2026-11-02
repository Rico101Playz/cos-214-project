#ifndef TASK_DECORATOR_H
#define TASK_DECORATOR_H

#include <memory>
#include "composite/Task.h"

// DECORATOR pattern - the DECORATOR.
//
// Wraps a Task and passes everything on to it, so it can sit in the work tree
// wherever the task it wraps would. Concrete decorators override handle() to
// refuse or add to a Request before (or after) the wrapped task performs it.
// The wrapped leaf learns its outermost wrapper, so a parent stage always sees
// the decorated node.
class TaskDecorator : public Task {
protected:
    std::unique_ptr<Task> inner_;

public:
    explicit TaskDecorator(std::unique_ptr<Task> inner);

    Task& inner() const;

    const std::string& id() const override;
    const std::string& name() const override;
    Task* parent() const override;
    void setParent(Task* parent) override;
    bool isComplete() const override;
    double progress() const override;
    void cancel() override;
    bool isReady() const override;
    std::size_t childCount() const override;
    Task* child(std::size_t index) const override;
    LeafTask* leaf() override;
    ApprovalWork* approvalWork() override;
    BlockedWork* blockedWork() override;
    ValidatedWork* validatedWork() override;
    void handle(const Request& request) override;
};

#endif // TASK_DECORATOR_H
