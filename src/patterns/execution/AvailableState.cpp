#include "patterns/execution/AvailableState.h"
#include "patterns/execution/AssignedState.h"
#include "patterns/structural/LeafWorkItem.h"

#include <memory>

std::string AvailableState::name() const { return "Available"; }

void AvailableState::assign(LeafWorkItem& item, const std::string& participant) {
    item.setAssignee(participant);
    item.transitionTo(std::unique_ptr<WorkItemState>(new AssignedState()));
}