#include "patterns/execution/InvokeCommand.h"
#include "core/WorkflowInstance.h"

InvokeCommand::InvokeCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& operation)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), operation(operation) {}

void InvokeCommand::perform(WorkflowInstance* instance) {
     if (instance == nullptr)
        throw std::invalid_argument("InvokeCommand: instance is null");

    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "InvokeCommand: unknown item '" + itemId + "'");

    item->setData("last_operation", operation);
}

std::string InvokeCommand::describe() const {
    return "InvokeCommand: item=" + itemId + ", operation=" + operation;
}