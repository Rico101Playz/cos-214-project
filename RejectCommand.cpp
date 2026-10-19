#include "RejectCommand.h"

RejectCommand::RejectCommand(WorkflowInstance& instance, Participant* actor, 
                             const std::string& itemId, const std::string& reason)
    : WorkCommand(instance, actor), itemId_(itemId), reason_(reason) {}

std::string RejectCommand::describe() const {}

void RejectCommand::perform() {}