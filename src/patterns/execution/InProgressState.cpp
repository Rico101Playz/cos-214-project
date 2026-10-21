#include "patterns/execution/InProgressState.h"
#include "patterns/execution/AwaitingApprovalState.h"
#include "patterns/execution/RejectedState.h"
#include "patterns/execution/EscalatedState.h"
#include "patterns/structural/LeafWorkItem.h"

#include <memory>

std::string InProgressState::name() const { return "InProgress"; }

void InProgressState::complete(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new AwaitingApprovalState()));
}

void InProgressState::reject(LeafWorkItem& item, const std::string& reason) {
    item.setReason(reason);
    item.transitionTo(std::unique_ptr<WorkItemState>(new RejectedState()));
}

void InProgressState::escalate(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new EscalatedState()));
}