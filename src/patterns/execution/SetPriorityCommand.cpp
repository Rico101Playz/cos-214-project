#include "patterns/execution/SetPriorityCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"
#include <stdexcept>
#include <utility>

SetPriorityCommand::SetPriorityCommand(SnapshotStore* store, std::string instanceId,
                                       const std::string& itemId, const std::string& priority)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), priority(priority) {}

void SetPriorityCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr) throw std::invalid_argument("SetPriorityCommand: instance is null");
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(instance->item(itemId));
    if (leaf == nullptr) throw std::invalid_argument("SetPriorityCommand: unknown item '" + itemId + "'");
    leaf->setPriority(priority);
}

std::string SetPriorityCommand::describe() const {
    return "SetPriorityCommand: item=" + itemId + ", priority=" + priority;
}
