#include "core/WorkflowInstance.h"
#include <iostream>

WorkflowInstance::WorkflowInstance(std::string id, InstanceStatus initialStatus)
    : id(std::move(id)), status(initialStatus) {}

std::string WorkflowInstance::getId() const { return id; }
InstanceStatus WorkflowInstance::getStatus() const { return status; }

void WorkflowInstance::addItem(std::shared_ptr<WorkItem> item) {
    if (!item) return;
    items[item->itemId] = item;
}

std::shared_ptr<WorkItem> WorkflowInstance::getItem(const std::string& itemId) const {
    auto it = items.find(itemId);
    return it != items.end() ? it->second : nullptr;
}

std::shared_ptr<InstanceMemento> WorkflowInstance::createMemento(const std::string& label) const {
    auto m = std::make_shared<InstanceMemento>(label, status);
    for (const auto& kv : items) {
        const auto& item = kv.second;
        m->addItem(WorkItemSnapshot(item->state, item->assignee, ""));
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(InstanceMemento* memento) {
    if (!memento) return;
    status = memento->getStatus();

    const auto& snaps = memento->getItems();
    size_t i = 0;
    for (auto& kv : items) {
        if (i >= snaps.size()) break;
        kv.second->state    = snaps[i].getState();
        kv.second->assignee = snaps[i].getAssignee();
        ++i;
    }
}

void WorkflowInstance::approveItem(const std::string& itemId, bool approved, const std::string& comment) {
    auto item = getItem(itemId);
    if (item) {
        item->state = approved ? "Approved" : "Rejected";
    }
}

void WorkflowInstance::startItem(const std::string& itemId) {}
void WorkflowInstance::completeItem(const std::string& itemId) {}
void WorkflowInstance::assignItem(const std::string& itemId, const std::string& participantName) {}
void WorkflowInstance::addWorkItem(const std::string& stageId, const std::string& itemId, std::shared_ptr<WorkItem> itemTemplate) {}
void WorkflowInstance::rejectItem(const std::string& itemId, const std::string& reason) {}