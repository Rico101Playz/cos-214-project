#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <cstddef>
#include <vector>
#include "patterns/execution/WorkflowEvent.h"
#include "patterns/execution/WorkflowObserver.h"

// OBSERVER pattern - the SUBJECT.
//
// Observers attach to the bus and are told about every event published to it.
// The bus does not own its observers: they must stay alive while attached, or
// be detached first.
class EventBus {
private:
    std::vector<WorkflowObserver*> observers_;

    bool isAttached(const WorkflowObserver* observer) const;

public:
    // Ignores a null observer and one that is already attached.
    void attach(WorkflowObserver* observer);
    // Ignores an observer that is not attached.
    void detach(WorkflowObserver* observer);

    // Delivers the event to the observers in the order they attached.
    // An observer may detach itself, or others, while being notified.
    void notify(const WorkflowEvent& event) const;

    std::size_t observerCount() const;
};

#endif // EVENTBUS_H