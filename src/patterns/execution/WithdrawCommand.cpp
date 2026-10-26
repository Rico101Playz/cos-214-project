#include "patterns/execution/WithdrawCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"
#include <stdexcept>
#include <utility>

WithdrawCommand::WithdrawCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId) {}

void WithdrawCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr) throw std::invalid_argument("WithdrawCommand: instance is null");
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(instance->item(itemId));
    if (leaf == nullptr) throw std::invalid_argument("WithdrawCommand: unknown item '" + itemId + "'");
    try {
        leaf->withdraw();
    } catch (const std::logic_error&) {
        throw std::logic_error("Only work waiting for approval can be taken back");
    }
}

std::string WithdrawCommand::describe() const {
    return "WithdrawCommand: item=" + itemId;
}
