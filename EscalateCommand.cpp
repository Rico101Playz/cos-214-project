#include "EscalateCommand.h"

EscalateCommand::EscalateCommand(WorkflowInstance& instance, Participant* actor, const std::string& itemId)
    : WorkCommand(instance, actor), itemId_(itemId) {}

std::string EscalateCommand::describe() const {
    return "Escalate item " + itemId_;
}

std::string EscalateCommand::target() const {
    return target_; 
}

void EscalateCommand::perform() {}