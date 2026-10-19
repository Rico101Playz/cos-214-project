#include "AddWorkCommand.h"

AddWorkCommand::AddWorkCommand(WorkflowInstance& instance, Participant* actor, 
                               const std::string& stageId, const ItemTemplate& spec)
    : WorkCommand(instance, actor), stageId_(stageId), spec_(spec) {}

std::string AddWorkCommand::describe() const {}

void AddWorkCommand::_perform() {}