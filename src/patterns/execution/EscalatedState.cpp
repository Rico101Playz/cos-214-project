#include "patterns/execution/EscalatedState.h"
#include "patterns/execution/AssignedState.h"
#include "core/LeafWorkItem.h"
#include "core/Participant.h"

#include <memory>

std::string EscalatedState::name() const { return "Escalated"; }

void EscalatedState::assign(LeafWorkItem& item, Participant& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<WorkItemState>(new AssignedState()));
}