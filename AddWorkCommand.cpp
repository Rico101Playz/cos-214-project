#include "AddWorkCommand.h"

AddWorkCommand::AddWorkCommand(WorkflowInstance& instance, Participant* actor, 
                               const std::string& stageId, const ItemTemplate& spec)
    : WorkCommand(instance, actor), stageId_(stageId), spec_(spec) {}

std::string AddWorkCommand::describe() const {
    return "Add work item to stage " + stageId_;
}

void AddWorkCommand::_perform() {}