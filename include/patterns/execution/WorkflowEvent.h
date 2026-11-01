#ifndef WORKFLOWEVENT_H
#define WORKFLOWEVENT_H

#include <memory>
#include <string>

class EngineMemento;

// OBSERVER pattern - the EVENT.
//
// Describes one thing that happened in the engine. The EventBus hands it to
// every attached observer.
class WorkflowEvent {
public:
    // Event types.
    static const char* const INSTANCE_STARTED;
    static const char* const INSTANCE_COMPLETED;
    static const char* const INSTANCE_CANCELLED;
    static const char* const ITEM_ASSIGNED;
    static const char* const ITEM_STARTED;
    static const char* const ITEM_COMPLETED; // work finished, awaiting approval
    static const char* const ITEM_APPROVED;
    static const char* const ITEM_REJECTED;
    static const char* const ITEM_ADDED;
    static const char* const ITEM_ESCALATED;
    static const char* const APPROVAL_ROUTED;
    static const char* const ORG_CHANGED;
    static const char* const UNDONE;
    static const char* const REDONE;
    static const char* const SUBTASK_ADDED;
    static const char* const ITEM_WITHDRAWN;
    static const char* const ITEM_SIGNED;
    static const char* const PRIORITY_CHANGED;
    static const char* const SIGNOFF_REQUESTED;
    static const char* const ISSUE_FLAGGED;
    static const char* const STEP_COMMITTED;

    std::string type;
    std::string instanceId;
    std::string itemId; // empty for events about a whole instance
    std::string detail; // participant, reason, workflow name, ...
    std::string actor;
    std::shared_ptr<const EngineMemento> before;

    WorkflowEvent(const std::string& type, const std::string& instanceId,
                  const std::string& itemId = "", const std::string& detail = "");

    // One line for logs, e.g. "ItemAssigned instance-1/F01 - Alice".
    std::string describe() const;
};

#endif // WORKFLOWEVENT_H