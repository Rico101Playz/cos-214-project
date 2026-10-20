#include "patterns/structural/WorkNode.h"
#include "patterns/execution/DepthFirstIterator.h"

WorkNode::WorkNode(const std::string& id, const std::string& name)
    : id_(id), name_(name), parent_(nullptr) {}

const std::string& WorkNode::id() const { return id_; }

const std::string& WorkNode::name() const { return name_; }

WorkNode* WorkNode::parent() const { return parent_; }

void WorkNode::setParent(WorkNode* parent) { parent_ = parent; }

bool WorkNode::isReady() const { return false; }

std::size_t WorkNode::childCount() const { return 0; }

WorkNode* WorkNode::child(std::size_t) const { return nullptr; }

WorkNode* WorkNode::find(const std::string& id) {
    if (id_ == id) {
        return this;
    }
    for (std::size_t i = 0; i < childCount(); ++i) {
        WorkNode* found = child(i)->find(id);
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

std::unique_ptr<WorkIterator> WorkNode::createIterator() {
    return std::unique_ptr<WorkIterator>(new DepthFirstIterator(this));
}
