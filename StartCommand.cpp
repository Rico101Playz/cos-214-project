#include "StartCommand.h"

StartCommand::StartCommand(WorkflowInstance& instance, Participant* actor, const std::string& itemId)
    : WorkCommand(instance, actor), itemId_(itemId) {}

std::string StartCommand::describe() const {}

void StartCommand::perform() {}