#include "patterns/execution/EscalatedState.h"
#include "patterns/execution/AssignedState.h"
#include "patterns/structural/LeafWorkItem.h"

#include <memory>

std::string EscalatedState::name() const { return "Escalated"; }

void EscalatedState::assign(LeafWorkItem& item, const std::string& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<WorkItemState>(new AssignedState()));
}