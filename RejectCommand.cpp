#include "RejectCommand.h"

RejectCommand::RejectCommand(WorkflowInstance& instance, Participant* actor, 
                             const std::string& itemId, const std::string& reason)
    : WorkCommand(instance, actor), itemId_(itemId), reason_(reason) {}

std::string RejectCommand::describe() const {
    return "Reject item " + itemId_ + ": " + reason_;
}

void RejectCommand::perform() {}