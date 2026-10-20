#include "patterns/execution/RejectCommand.h"

RejectCommand::RejectCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& reason)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), reason(reason) {}

void RejectCommand::perform(WorkflowInstance* instance) {}

std::string RejectCommand::describe() const {
    return "RejectCommand: item=" + itemId + ", reason=" + reason;
}