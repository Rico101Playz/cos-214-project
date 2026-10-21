#include "patterns/execution/AssignedState.h"
#include "patterns/execution/InProgressState.h"
#include "patterns/execution/EscalatedState.h"
#include "core/LeafWorkItem.h"

#include <memory>

std::string AssignedState::name() const { return "Assigned"; }

// Reassignment: the old WorkItem allowed assign() while already Assigned.
void AssignedState::assign(LeafWorkItem& item, const std::string& participant) {
    item.setAssignee(participant);   // stays Assigned
}

void AssignedState::start(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new InProgressState()));
}

void AssignedState::escalate(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new EscalatedState()));
}