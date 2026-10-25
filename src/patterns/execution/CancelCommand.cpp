#include "patterns/execution/CancelCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"


CancelCommand::CancelCommand(SnapshotStore* store, std::string instanceId, const std::string& nodeId, const std::string& reason)
    : WorkCommand(store, std::move(instanceId)), 
    nodeId(nodeId), reason(reason) {}

void CancelCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("CancelCommand: instance is null");

    // If the nodeId is the instance id, cancel the whole instance.
    if (nodeId == instance->id()) {
        try {
            instance->cancel(reason);
            return;
        } catch (const std::logic_error& e) {
            throw std::logic_error(
                std::string("CancelCommand: cannot cancel instance: ") + e.what());
        }
    }

    // Otherwise, cancel a single item.
    WorkItem* item = instance->item(nodeId);
    if (item == nullptr) {
        throw std::invalid_argument(
            "CancelCommand: unknown node '" + nodeId + "'");
    }

    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(item);
    if (leaf == nullptr) {
        throw std::invalid_argument(
            "CancelCommand: node '" + nodeId + "' is not cancellable");
    }

    // LeafWorkItem::cancel() is a no-op on terminal states, so check first.
    if (leaf->status() == WorkItem::COMPLETED ||
        leaf->status() == WorkItem::CANCELLED) {
        throw std::logic_error(
            "CancelCommand: item '" + nodeId +
            "' is already " + leaf->status());
    }
    leaf->cancel();
}

std::string CancelCommand::describe() const {
    return "CancelCommand: node=" + nodeId + ", reason=" + reason;
}