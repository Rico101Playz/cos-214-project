#include "patterns/execution/CompleteCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

CompleteCommand::CompleteCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId) 
    : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void CompleteCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("CompleteCommand: instance is null");
    
    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: unknown item '" + itemId + "'");

    if (dynamic_cast<LeafWorkItem*>(item) == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: '" + itemId + "' is not a leaf work item");

    if (!instance->completeItem(itemId)) {
        throw std::logic_error(
            "CompleteCommand: item '" + itemId +
            "' cannot be completed in its current state");
    }
}

std::string CompleteCommand::describe() const {
    return "CompleteCommand: item=" + itemId;
}