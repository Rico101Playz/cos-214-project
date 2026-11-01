#include "patterns/execution/SignCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"
#include <stdexcept>
#include <utility>

SignCommand::SignCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId,
                         const std::string& slot, const std::string& person)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), slot(slot), person(person) {}

void SignCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr) throw std::invalid_argument("SignCommand: instance is null");
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(instance->item(itemId));
    if (leaf == nullptr) throw std::invalid_argument("SignCommand: unknown item '" + itemId + "'");
    if (leaf->status() != WorkItem::AWAITING_APPROVAL) {
        throw std::logic_error("SignCommand: item '" + itemId + "' is not waiting for approval");
    }
    if (!leaf->takePendingSlot(slot)) {
        throw std::logic_error("SignCommand: item '" + itemId + "' is not waiting for a " + slot);
    }
    leaf->addSignature(slot, person);
}

std::string SignCommand::describe() const {
    return "SignCommand: item=" + itemId + ", slot=" + slot + ", person=" + person;
}
