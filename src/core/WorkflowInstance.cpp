#include "core/WorkflowInstance.h"
#include <iostream>

WorkflowInstance::WorkflowInstance(std::string id, InstanceStatus initialStatus)
    : id(std::move(id)), status(initialStatus) {}

std::string WorkflowInstance::getId() const { return id; }
InstanceStatus WorkflowInstance::getStatus() const { return status; }

void WorkflowInstance::addItem(std::shared_ptr<WorkItem> item) {}

std::shared_ptr<WorkItem> WorkflowInstance::getItem(const std::string& itemId) const {}

std::shared_ptr<InstanceMemento> WorkflowInstance::createMemento(const std::string& label) const {}

void WorkflowInstance::restoreFromMemento(InstanceMemento* memento) {}

void WorkflowInstance::approveItem(const std::string& itemId, bool approved) {}