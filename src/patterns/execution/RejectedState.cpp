#include "patterns/execution/RejectedState.h"
#include "patterns/execution/AssignedState.h"
#include "patterns/execution/InProgressState.h"
#include "patterns/structural/LeafWorkItem.h"

#include <memory>

std::string RejectedState::name() const { return "Rejected"; }

void RejectedState::assign(LeafWorkItem& item, const std::string& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<WorkItemState>(new AssignedState()));
}

void RejectedState::start(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new InProgressState()));
}