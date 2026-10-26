#include "patterns/execution/EventBus.h"
#include <algorithm>

bool EventBus::isAttached(const WorkflowObserver* observer) const {
    return std::find(observers_.begin(), observers_.end(), observer) != observers_.end();
}

void EventBus::attach(WorkflowObserver* observer) {
    if (observer == nullptr || isAttached(observer)) {
        return;
    }
    observers_.push_back(observer);
}

void EventBus::detach(WorkflowObserver* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
}

void EventBus::notify(const WorkflowEvent& event) const {
    // Walk a copy so observers can attach or detach during delivery, and skip
    // any that were detached by an earlier observer.
    std::vector<WorkflowObserver*> snapshot = observers_;
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        if (isAttached(snapshot[i])) {
            snapshot[i]->onEvent(event);
        }
    }
}

std::size_t EventBus::observerCount() const { return observers_.size(); }