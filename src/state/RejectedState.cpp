#include "state/RejectedState.h"
#include "state/AssignedState.h"
#include "state/InProgressState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string RejectedState::name() const { return "Rejected"; }

void RejectedState::assign(LeafTask& item, const std::string& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<TaskState>(new AssignedState()));
}

void RejectedState::start(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new InProgressState()));
}