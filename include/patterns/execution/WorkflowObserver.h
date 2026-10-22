#ifndef WORKFLOWOBSERVER_H
#define WORKFLOWOBSERVER_H

#include "patterns/execution/WorkflowEvent.h"

// OBSERVER pattern - the OBSERVER interface.
//
// Anything that wants to react to workflow events implements this and is
// attached to the EventBus.
class WorkflowObserver {
public:
    virtual ~WorkflowObserver() = default;

    virtual void onEvent(const WorkflowEvent& event) = 0;
};

#endif // WORKFLOWOBSERVER_H