#include "patterns/execution/InvokeCommand.h"

InvokeCommand::InvokeCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& operation)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), operation(operation) {}

void InvokeCommand::perform(WorkflowInstance* instance) {}

std::string InvokeCommand::describe() const {
    return "InvokeCommand: item=" + itemId + ", operation=" + operation;
}