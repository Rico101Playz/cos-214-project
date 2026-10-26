#include "patterns/execution/WorkflowEvent.h"

const char* const WorkflowEvent::INSTANCE_STARTED = "InstanceStarted";
const char* const WorkflowEvent::INSTANCE_COMPLETED = "InstanceCompleted";
const char* const WorkflowEvent::INSTANCE_CANCELLED = "InstanceCancelled";
const char* const WorkflowEvent::ITEM_ASSIGNED = "ItemAssigned";
const char* const WorkflowEvent::ITEM_STARTED = "ItemStarted";
const char* const WorkflowEvent::ITEM_COMPLETED = "ItemCompleted";
const char* const WorkflowEvent::ITEM_APPROVED = "ItemApproved";
const char* const WorkflowEvent::ITEM_REJECTED = "ItemRejected";

WorkflowEvent::WorkflowEvent(const std::string& type, const std::string& instanceId,
                             const std::string& itemId, const std::string& detail)
    : type(type), instanceId(instanceId), itemId(itemId), detail(detail) {}

std::string WorkflowEvent::describe() const {
    std::string line = type + " " + instanceId;
    if (!itemId.empty()) {
        line += "/" + itemId;
    }
    if (!detail.empty()) {
        line += " - " + detail;
    }
    return line;
}