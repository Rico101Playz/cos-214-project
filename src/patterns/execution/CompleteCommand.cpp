#include "patterns/execution/CompleteCommand.h"

CompleteCommand::CompleteCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId) 
    : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void CompleteCommand::perform(WorkflowInstance* instance) {}

std::string CompleteCommand::describe() const {
    return "CompleteCommand: item=" + itemId;
}