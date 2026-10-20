#include "patterns/execution/SetDataCommand.h"

SetDataCommand::SetDataCommand(const std::string& nodeId, const std::string& key, const std::string& value)
    : nodeId(nodeId), key(key), value(value) {}

void SetDataCommand::perform() {}

std::string SetDataCommand::describe() const {
    return "SetDataCommand: node=" + nodeId + ", key=" + key + ", value=" + value;
}