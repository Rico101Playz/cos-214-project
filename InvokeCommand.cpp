#include "InvokeCommand.h"

InvokeCommand::InvokeCommand(WorkflowInstance& instance, Participant* actor, 
                             const std::string& itemId, ExternalConnector& connector, 
                             const std::string& operation, const Payload& arguments)
    : WorkCommand(instance, actor), itemId_(itemId), connector_(connector), 
      operation_(operation), arguments_(arguments) {}

std::string InvokeCommand::describe() const {}

const Payload& InvokeCommand::result() const { 
    return result_; 
}

void InvokeCommand::perform() {}