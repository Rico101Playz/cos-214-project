#include "state/AwaitingApprovalState.h"
#include "state/CompletedState.h"
#include "state/RejectedState.h"
#include "state/InProgressState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string AwaitingApprovalState::name() const { return "AwaitingApproval"; }

void AwaitingApprovalState::approve(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new CompletedState()));
}

void AwaitingApprovalState::withdraw(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new InProgressState()));
}

void AwaitingApprovalState::reject(LeafTask& item, const std::string& reason) {
    item.setReason(reason);
    item.transitionTo(std::unique_ptr<TaskState>(new RejectedState()));
}