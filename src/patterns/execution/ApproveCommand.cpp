#include "patterns/execution/ApproveCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

ApproveCommand::ApproveCommand(SnapshotStore* store, std::string instanceId, 
                               const std::string& itemId, bool approved, const std::string& comment)
    : WorkCommand(store, std::move(instanceId)), 
      itemId(itemId), 
      approved(approved), 
      comment(comment) {}

void ApproveCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("ApproveCommand: instance is null");

    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: unknown item '" + itemId + "'");

    if (dynamic_cast<LeafWorkItem*>(item) == nullptr)
        throw std::invalid_argument(
            "CompleteCommand: '" + itemId + "' is not a leaf work item");

    if (!instance->approveItem(itemId, approved, comment)) {
        throw std::logic_error(
            "ApproveCommand: item '" + itemId +
            "' is not in a state that accepts " +
            (approved ? "approval" : "rejection"));
    }
}


std::string ApproveCommand::describe() const {
    return "ApproveCommand: item=" + itemId + ", approved=" + (approved ? "true" : "false") + ", comment=" + comment;
}