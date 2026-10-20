#include "patterns/execution/AssignCommand.h"

AssignCommand::AssignCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& participantName) 
    : WorkCommand(store, std::move(instanceId)), 
      itemId(itemId), 
      participantName(participantName){}

void AssignCommand::perform(WorkflowInstance* instance) {}

std::string AssignCommand::describe() const {
    return "AssignCommand: item=" + itemId + ", participant=" + participantName;
}