#include "composite/TaskNode.h"

TaskNode::TaskNode(const std::string& id, const std::string& name) : id_(id), name_(name), parent_(nullptr) {}

const std::string& TaskNode::id() const { return id_; }

const std::string& TaskNode::name() const { return name_; }

Task* TaskNode::parent() const { return parent_; }

void TaskNode::setParent(Task* parent) { parent_ = parent; }

bool TaskNode::isReady() const { return false; }

std::size_t TaskNode::childCount() const { return 0; }

Task* TaskNode::child(std::size_t) const { return nullptr; }
