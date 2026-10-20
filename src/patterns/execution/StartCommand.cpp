#include "patterns/execution/StartCommand.h"

StartCommand::StartCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId) : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void StartCommand::perform(WorkflowInstance* instance) {}

std::string StartCommand::describe() const {
    return "StartCommand: item=" + itemId;
}