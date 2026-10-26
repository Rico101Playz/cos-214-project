#include "patterns/execution/RouteApprovalCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"
#include <stdexcept>
#include <utility>

RouteApprovalCommand::RouteApprovalCommand(SnapshotStore* store, std::string instanceId,
                                           const std::string& itemId,
                                           const std::vector<std::string>& pending,
                                           int automaticSignatures)
    : WorkCommand(store, std::move(instanceId)), itemId(itemId), pending(pending),
      automaticSignatures(automaticSignatures) {}

void RouteApprovalCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr) throw std::invalid_argument("RouteApprovalCommand: instance is null");
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(instance->item(itemId));
    if (leaf == nullptr) throw std::invalid_argument("RouteApprovalCommand: unknown item '" + itemId + "'");
    leaf->clearApprovals();
    for (int i = 0; i < automaticSignatures; ++i) leaf->addSignature("Auto", "System");
    leaf->setPendingApprovers(pending);
}

std::string RouteApprovalCommand::describe() const {
    std::string roles;
    for (std::size_t i = 0; i < pending.size(); ++i) roles += (i ? "," : "") + pending[i];
    return "RouteApprovalCommand: item=" + itemId + ", pending=" + roles;
}
