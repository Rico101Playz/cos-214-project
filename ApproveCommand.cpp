#include "ApproveCommand.h"

ApproveCommand::ApproveCommand(WorkflowInstance& instance, Participant* approver, 
                               const std::string& itemId, bool approved, const std::string& comment)
    : WorkCommand(instance, approver), itemId_(itemId), approved_(approved), comment_(comment) {}

std::string ApproveCommand::describe() const {}

void ApproveCommand::perform() {}