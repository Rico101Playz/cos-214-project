#include "patterns/execution/CreatedState.h"
#include "patterns/execution/AvailableState.h"
#include "core/LeafWorkItem.h"

#include <memory>

std::string CreatedState::name() const { return "Created"; }

void CreatedState::makeAvailable(LeafWorkItem& item) {
    item.transitionTo(std::unique_ptr<WorkItemState>(new AvailableState()));
}