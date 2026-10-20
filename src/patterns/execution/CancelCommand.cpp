#include "patterns/execution/CancelCommand.h"

CancelCommand::CancelCommand(SnapshotStore* store, std::string instanceId, const std::string& nodeId, const std::string& reason)
    : WorkCommand(store, std::move(instanceId)), 
    nodeId(nodeId), reason(reason) {}

void CancelCommand::perform(WorkflowInstance* instance) {}

std::string CancelCommand::describe() const {
    return "CancelCommand: node=" + nodeId + ", reason=" + reason;
}