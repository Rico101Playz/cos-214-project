#include "patterns/execution/AvailableState.h"
#include "patterns/execution/AssignedState.h"
#include "core/LeafWorkItem.h"
#include "core/Participant.h"

#include <memory>

std::string AvailableState::name() const { return "Available"; }

void AvailableState::assign(LeafWorkItem& item, Participant& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<WorkItemState>(new AssignedState()));
}