#include "memento/Memento.h"
#include <utility>

// ---------------------------------------------------------
// WorkItemSnapshot
// ---------------------------------------------------------
WorkItemSnapshot::WorkItemSnapshot(std::string stateName, std::string assignee, std::string reason,
                                   std::string itemId)
    : stateName(std::move(stateName)), assignee(std::move(assignee)), reason(std::move(reason)),
      itemId(std::move(itemId)) {}

const std::string& WorkItemSnapshot::getItemId() const {
    return itemId;
}

const std::string& WorkItemSnapshot::getStateName() const { 
    return stateName; 
}

const std::string& WorkItemSnapshot::getAssignee() const { 
    return assignee; 
}

const std::string& WorkItemSnapshot::getReason() const { 
    return reason; 
}

// ---------------------------------------------------------
// ApprovalSnapshot
// ---------------------------------------------------------
ApprovalSnapshot::ApprovalSnapshot(std::string itemId, std::string pendingRole, bool approved)
    : itemId(std::move(itemId)), pendingRole(std::move(pendingRole)), approved(approved) {}

const std::string& ApprovalSnapshot::getItemId() const { 
    return itemId; 
}

const std::string& ApprovalSnapshot::getPendingRole() const { 
    return pendingRole; 
}

bool ApprovalSnapshot::getApproved() const { 
    return approved; 
}

// ---------------------------------------------------------
// Memento
// ---------------------------------------------------------
Memento::Memento(std::string lbl, std::string statName)
    : label(std::move(lbl)), statusName(std::move(statName)), stageIndex(0) {}

std::size_t Memento::getStageIndex() const {
    return stageIndex;
}

void Memento::setStageIndex(std::size_t index) {
    stageIndex = index;
}

const std::string& Memento::getLabel() const {
    return label; 
}

const std::string& Memento::getStatusName() const { 
    return statusName; 
}

const std::vector<WorkItemSnapshot>& Memento::getItems() const { 
    return items; 
}

const std::vector<ApprovalSnapshot>& Memento::getApprovals() const { 
    return approvals; 
}

void Memento::addItem(WorkItemSnapshot item) {
    items.push_back(std::move(item));
}

void Memento::addApproval(ApprovalSnapshot approval) {
    approvals.push_back(std::move(approval));
}

// ---------------------------------------------------------
// SnapshotStore
// ---------------------------------------------------------
void SnapshotStore::save(const std::string& instanceId, std::shared_ptr<Memento> memento) {
    if (!memento) return;
    std::string label = memento->getLabel();
    snapshots[instanceId][label] = std::move(memento);
}

Memento* SnapshotStore::get(const std::string& instanceId, const std::string& label) const {
    auto itInstance = snapshots.find(instanceId);
    if (itInstance != snapshots.end()) {
        auto itLabel = itInstance->second.find(label);
        if (itLabel != itInstance->second.end()) {
            return itLabel->second.get();
        }
    }
    return nullptr;
}

bool SnapshotStore::has(const std::string& instanceId, const std::string& label) const {
    auto itInstance = snapshots.find(instanceId);
    if (itInstance != snapshots.end()) {
        return itInstance->second.find(label) != itInstance->second.end();
    }
    return false;
}