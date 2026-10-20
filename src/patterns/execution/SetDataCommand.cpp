#include "patterns/execution/SetDataCommand.h"

SetDataCommand::SetDataCommand(SnapshotStore* store, std::string instanceId, const std::string& nodeId, const std::string& key, const std::string& value)
    : WorkCommand(store, std::move(instanceId)), nodeId(nodeId), key(key), value(value) {}

void SetDataCommand::perform(WorkflowInstance* instance) {}

std::string SetDataCommand::describe() const {
    return "SetDataCommand: node=" + nodeId + ", key=" + key + ", value=" + value;
}