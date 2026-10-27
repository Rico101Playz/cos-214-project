#include "patterns/execution/CreatedState.h"
#include "patterns/execution/AvailableState.h"
#include "patterns/structural/LeafWorkItem.h"

#include <memory>

std::string CreatedState::name() const { return "Created"; }

void CreatedState::makeAvailable(LeafWorkItem& item) {
    if (item.isBlocked()) {
        return;
    }
    item.transitionTo(std::unique_ptr<WorkItemState>(new AvailableState()));
}