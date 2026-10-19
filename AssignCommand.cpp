#include "AssignCommand.h"

AssignCommand::AssignCommand(WorkflowInstance& instance, Participant* actor, 
                             const std::string& itemId, const std::string& participantName)
    : WorkCommand(instance, actor), itemId_(itemId), participantName_(participantName) {}

std::string AssignCommand::describe() const {}

std::string AssignCommand::chosen() const { 
    return chosen_; 
}

void AssignCommand::perform() {}