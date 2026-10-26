#include "patterns/execution/AssignCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

AssignCommand::AssignCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& participantName) 
    : WorkCommand(store, std::move(instanceId)), 
      itemId(itemId), 
      participantName(participantName){}

void AssignCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("AssignCommand: instance is null");
    
    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: unknown item '" + itemId + "'");

    if (dynamic_cast<LeafWorkItem*>(item) == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: '" + itemId + "' is not a leaf work item");

    if (!instance->assignItem(itemId, participantName)) {
        throw std::logic_error(
            "AssignCommand: item '" + itemId +
            "' cannot be assigned to '" + participantName + "'");
    }
}

std::string AssignCommand::describe() const {
    return "AssignCommand: item=" + itemId + ", participant=" + participantName;
}