#include "patterns/execution/SetDataCommand.h"
#include "core/WorkflowInstance.h"
#include "core/WorkItem.h"

SetDataCommand::SetDataCommand(SnapshotStore* store, std::string instanceId, const std::string& nodeId, const std::string& key, const std::string& value)
    : WorkCommand(store, std::move(instanceId)), nodeId(nodeId), key(key), value(value) {}

void SetDataCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("SetDataCommand: instance is null");

    WorkItem* item = instance->item(nodeId);
    if (item == nullptr)
        throw std::invalid_argument(
            "SetDataCommand: unknown node '" + nodeId + "'");

    item->setData(key, value);
}

std::string SetDataCommand::describe() const {
    return "SetDataCommand: node=" + nodeId + ", key=" + key + ", value=" + value;
}