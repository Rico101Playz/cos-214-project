#include "state/CreatedState.h"
#include "state/AvailableState.h"
#include "composite/LeafTask.h"

#include <memory>

std::string CreatedState::name() const { return "Created"; }

void CreatedState::makeAvailable(LeafTask& item) {
    if (item.isBlocked()) {
        return;
    }
    item.transitionTo(std::unique_ptr<TaskState>(new AvailableState()));
}