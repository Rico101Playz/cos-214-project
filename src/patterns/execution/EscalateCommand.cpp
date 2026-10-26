#include "patterns/execution/EscalateCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

EscalateCommand::EscalateCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId) : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void EscalateCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("EscalateCommand: instance is null");

    WorkItem* item = instance->item(itemId);
    if (item == nullptr)
        throw std::invalid_argument(
            "EscalateCommand: unknown item '" + itemId + "'");

    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(item);
    if (leaf == nullptr)
        throw std::invalid_argument(
            "EscalateCommand: item '" + itemId + "' has no escalation state");

    try {
        leaf->escalate();
    } catch (const std::logic_error& e) {
        throw std::logic_error(
            std::string("EscalateCommand: ") + e.what());
    }
}

std::string EscalateCommand::describe() const {
    return "EscalateCommand: item=" + itemId;
}