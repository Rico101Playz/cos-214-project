#ifndef TASK_NODE_H
#define TASK_NODE_H

#include <string>
#include "composite/Task.h"

// COMPOSITE pattern - the data every real node of the work tree shares: its
// id, its name and its parent. CompositeTask and LeafTask extend it;
// decorators do not, so they never hold a second copy.
class TaskNode : public Task {
private:
    std::string id_;
    std::string name_;
    Task* parent_;      // non-owning; nullptr for the root

public:
    TaskNode(const std::string& id, const std::string& name);

    const std::string& id() const override;
    const std::string& name() const override;
    Task* parent() const override;
    void setParent(Task* parent) override;

    // Stages are never ready and leaves have no children; LeafTask and
    // CompositeTask override what applies to them.
    bool isReady() const override;
    std::size_t childCount() const override;
    Task* child(std::size_t index) const override;
};

#endif // TASK_NODE_H
