#include "SetDataCommand.h"

SetDataCommand::SetDataCommand(WorkflowInstance& instance, Participant* actor, 
                               const std::string& nodeId, const std::string& key, const std::string& value)
    : WorkCommand(instance, actor), nodeId_(nodeId), key_(key), value_(value) {}

std::string SetDataCommand::describe() const {
    return "Set data for node " + nodeId_ + ": " + key_ + " = " + value_;
}

void SetDataCommand::perform() {}