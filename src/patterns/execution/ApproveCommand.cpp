#include "patterns/execution/ApproveCommand.h"
#include "core/WorkflowInstance.h"

ApproveCommand::ApproveCommand(SnapshotStore* store, std::string instanceId, 
                               const std::string& itemId, bool approved, const std::string& comment)
    : WorkCommand(store, std::move(instanceId)), 
      itemId(itemId), 
      approved(approved), 
      comment(comment) {}

void ApproveCommand::perform(WorkflowInstance* instance) {
    if (instance) {
        instance->approveItem(itemId, approved);
    }
}


std::string ApproveCommand::describe() const {
    return "ApproveCommand: item=" + itemId + ", approved=" + (approved ? "true" : "false") + ", comment=" + comment;
}