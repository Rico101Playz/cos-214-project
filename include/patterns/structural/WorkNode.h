#ifndef WORKNODE_H
#define WORKNODE_H

#include <cstddef>
#include <memory>
#include <string>

class WorkIterator;

// COMPOSITE pattern - the COMPONENT.
//
// Every node in an instance's work tree is a WorkNode: a CompositeStage that
// groups other nodes, or a WorkItem leaf that is actual work. Callers track
// completion and progress through this interface without caring which is which.
//
// NOTE (integration point): this is the minimal shape the Iterator pattern
// needs. The Visitor hooks (accept) from the class diagram are left for
// whoever lands that pattern.
class WorkNode {
private:
    std::string id_;
    std::string name_;
    WorkNode* parent_;      // non-owning; nullptr for the root

public:
    WorkNode(const std::string& id, const std::string& name);
    virtual ~WorkNode() = default;

    // Nodes sit inside a tree, so copying one would break its links.
    WorkNode(const WorkNode&) = delete;
    WorkNode& operator=(const WorkNode&) = delete;

    const std::string& id() const;
    const std::string& name() const;

    WorkNode* parent() const;
    void setParent(WorkNode* parent);

    virtual bool isComplete() const = 0;
    virtual double progress() const = 0;    // 0.0 .. 1.0
    virtual void cancel() = 0;

    // True when the node can be picked up right now. Only leaves are ever
    // ready, so stages keep this default.
    virtual bool isReady() const;

    // Child access used by the iterators. Leaves have no children.
    virtual std::size_t childCount() const;
    virtual WorkNode* child(std::size_t index) const;

    // Searches this node and everything below it; nullptr when not found.
    WorkNode* find(const std::string& id);

    // ITERATOR pattern - the factory method on the aggregate. Returns a
    // depth-first iterator over this node and everything below it.
    std::unique_ptr<WorkIterator> createIterator();
};

#endif // WORKNODE_H
