#include "patterns/execution/EscalateCommand.h"

EscalateCommand::EscalateCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId) : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void EscalateCommand::perform(WorkflowInstance* instance) {}

std::string EscalateCommand::describe() const {
    return "EscalateCommand: item=" + itemId;
}