#include "patterns/execution/AwaitingApprovalState.h"
#include "patterns/execution/CompletedState.h"
#include "patterns/execution/RejectedState.h"
#include "core/LeafWorkItem.h"

#include <memory>

std::string AwaitingApprovalState::name() const { return "AwaitingApproval"; }

void AwaitingApprovalState::complete(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new CompletedState()));
}

void AwaitingApprovalState::reject(LeafWorkItem& item, const std::string& reason) {
    item.setReason(reason);
    item.transitionTo(std::unique_ptr<WorkItemState>(new RejectedState()));
}