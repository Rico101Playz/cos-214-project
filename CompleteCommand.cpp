#include "CompleteCommand.h"

CompleteCommand::CompleteCommand(WorkflowInstance& instance, Participant* actor, const std::string& itemId)
    : WorkCommand(instance, actor), itemId_(itemId) {}

std::string CompleteCommand::describe() const {}

void CompleteCommand::perform() {}