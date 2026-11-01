#include "state/AssignedState.h"
#include "state/InProgressState.h"
#include "state/EscalatedState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string AssignedState::name() const { return "Assigned"; }

// Reassignment: the old LeafTask allowed assign() while already Assigned.
void AssignedState::assign(LeafTask& item, const std::string& participant) {
    item.setAssignee(participant);   // stays Assigned
}

void AssignedState::start(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new InProgressState()));
}

void AssignedState::escalate(LeafTask& item) {
    item.transitionTo(std::unique_ptr<TaskState>(new EscalatedState()));
}