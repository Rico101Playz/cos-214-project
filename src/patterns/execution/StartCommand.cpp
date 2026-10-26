#include "patterns/execution/StartCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

StartCommand::StartCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId) : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void StartCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("StartCommand: instance is null");

    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "StartCommand: unknown item '" + itemId + "'");

    if (dynamic_cast<LeafWorkItem*>(item) == nullptr)
        throw std::invalid_argument(
            "StartCommand: '" + itemId + "' is not a leaf work item");

    if (!instance->startItem(itemId))
        throw std::logic_error(
            "StartCommand: item '" + itemId +
            "' cannot be started in its current state");
}

std::string StartCommand::describe() const {
    return "StartCommand: item=" + itemId;
}