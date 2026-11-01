#ifndef TASK_H
#define TASK_H

#include <cstddef>
#include <memory>
#include <string>

class ApprovalWork;
class BlockedWork;
class ValidatedWork;
class LeafTask;
class Request;
class WorkIterator;

// COMPOSITE pattern - the COMPONENT. DECORATOR pattern - the COMPONENT.
//
// Every node in a project's work tree is a Task: a CompositeTask that groups
// other nodes, a LeafTask that is actual work, or a TaskDecorator that wraps
// one of those and adds behaviour. Task holds no data, so a decorator carries
// no empty copies; TaskNode holds the id, name and parent of real nodes.
// Callers track completion and progress through this interface without caring
// which kind of node they hold.
class Task {
public:
    Task() = default;
    virtual ~Task() = default;

    // Nodes sit inside a tree, so copying one would break its links.
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    virtual const std::string& id() const = 0;
    virtual const std::string& name() const = 0;
    virtual Task* parent() const = 0;
    virtual void setParent(Task* parent) = 0;

    virtual bool isComplete() const = 0;
    virtual double progress() const = 0;    // 0.0 .. 1.0
    virtual void cancel() = 0;
    // True when the node can be picked up right now.
    virtual bool isReady() const = 0;

    // Child access used by the iterators. Leaves have no children.
    virtual std::size_t childCount() const = 0;
    virtual Task* child(std::size_t index) const = 0;

    // The work behind this node: the LeafTask itself, the task a decorator
    // wraps, or nullptr for a stage.
    virtual LeafTask* leaf();
    const LeafTask* leaf() const;
    // The ApprovalWork among this node's decorators, or nullptr.
    virtual ApprovalWork* approvalWork();
    // The BlockedWork among this node's decorators, or nullptr.
    virtual BlockedWork* blockedWork();
    // The ValidatedWork among this node's decorators, or nullptr.
    virtual ValidatedWork* validatedWork();

    // Performs a Request aimed at this task. A decorator may refuse it or add
    // to it before passing it on; a LeafTask performs it. Stages throw
    // std::invalid_argument.
    virtual void handle(const Request& request);

    // Searches this node and everything below it; nullptr when not found.
    Task* find(const std::string& id);

    // ITERATOR pattern - the factory method on the aggregate. Returns a
    // depth-first iterator over this node and everything below it.
    std::unique_ptr<WorkIterator> createIterator();
};

// The LeafTask behind a node that may be null or decorated.
LeafTask* leafOf(Task* task);
const LeafTask* leafOf(const Task* task);

#endif // TASK_H
