#include "state/InProgressState.h"
#include "state/AwaitingApprovalState.h"
#include "state/RejectedState.h"
#include "state/EscalatedState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string InProgressState::name() const { return "InProgress"; }

void InProgressState::complete(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new AwaitingApprovalState()));
}

void InProgressState::reject(LeafTask& item, const std::string& reason) {
    item.setReason(reason);
    item.transitionTo(std::unique_ptr<TaskState>(new RejectedState()));
}

void InProgressState::escalate(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new EscalatedState()));
}