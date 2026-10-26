#include "patterns/execution/RejectCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

RejectCommand::RejectCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& reason)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), reason(reason) {}

void RejectCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("RejectCommand: instance is null");

    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: unknown item '" + itemId + "'");

    if (dynamic_cast<LeafWorkItem*>(item) == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: '" + itemId + "' is not a leaf work item");

    if (!instance->rejectItem(itemId, reason)) {
        throw std::logic_error(
            "RejectCommand: item '" + itemId +
            "' cannot be rejected in its current state");
    }
    static_cast<LeafWorkItem*>(item)->clearApprovals();
}

std::string RejectCommand::describe() const {
    return "RejectCommand: item=" + itemId + ", reason=" + reason;
}