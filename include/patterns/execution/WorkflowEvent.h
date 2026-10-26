#ifndef WORKFLOWEVENT_H
#define WORKFLOWEVENT_H

#include <string>

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

    std::string type;
    std::string instanceId;
    std::string itemId; // empty for events about a whole instance
    std::string detail; // participant, reason, workflow name, ...

    WorkflowEvent(const std::string& type, const std::string& instanceId,
                  const std::string& itemId = "", const std::string& detail = "");

    // One line for logs, e.g. "ItemAssigned instance-1/F01 - Alice".
    std::string describe() const;
};

#endif // WORKFLOWEVENT_H