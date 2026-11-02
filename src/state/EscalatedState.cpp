#include "state/EscalatedState.h"
#include "state/AssignedState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string EscalatedState::name() const { return "Escalated"; }

void EscalatedState::assign(LeafTask& item, const std::string& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<TaskState>(new AssignedState()));
}