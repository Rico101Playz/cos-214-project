#include "state/AvailableState.h"
#include "state/AssignedState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string AvailableState::name() const { return "Available"; }

void AvailableState::assign(LeafTask& item, const std::string& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<TaskState>(new AssignedState()));
}