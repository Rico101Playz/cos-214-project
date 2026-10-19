#include "CancelCommand.h"

CancelCommand::CancelCommand(WorkflowInstance& instance, Participant* actor, 
                             const std::string& nodeId, const std::string& reason)
    : WorkCommand(instance, actor), nodeId_(nodeId), reason_(reason) {}

std::string CancelCommand::describe() const {}

void CancelCommand::perform() {}