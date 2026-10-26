#include "patterns/execution/WorkflowEvent.h"

const char* const WorkflowEvent::INSTANCE_STARTED = "InstanceStarted";
const char* const WorkflowEvent::INSTANCE_COMPLETED = "InstanceCompleted";
const char* const WorkflowEvent::INSTANCE_CANCELLED = "InstanceCancelled";
const char* const WorkflowEvent::ITEM_ASSIGNED = "ItemAssigned";
const char* const WorkflowEvent::ITEM_STARTED = "ItemStarted";
const char* const WorkflowEvent::ITEM_COMPLETED = "ItemCompleted";
const char* const WorkflowEvent::ITEM_APPROVED = "ItemApproved";
const char* const WorkflowEvent::ITEM_REJECTED = "ItemRejected";
const char* const WorkflowEvent::ITEM_ADDED = "ItemAdded";
const char* const WorkflowEvent::ITEM_ESCALATED = "ItemEscalated";
const char* const WorkflowEvent::APPROVAL_ROUTED = "ApprovalRouted";
const char* const WorkflowEvent::ORG_CHANGED = "OrgChanged";
const char* const WorkflowEvent::UNDONE = "Undone";
const char* const WorkflowEvent::REDONE = "Redone";
const char* const WorkflowEvent::SUBTASK_ADDED = "SubtaskAdded";
const char* const WorkflowEvent::ITEM_WITHDRAWN = "ItemWithdrawn";
const char* const WorkflowEvent::ITEM_SIGNED = "ItemSigned";
const char* const WorkflowEvent::PRIORITY_CHANGED = "PriorityChanged";
const char* const WorkflowEvent::SIGNOFF_REQUESTED = "SignOffRequested";
const char* const WorkflowEvent::ISSUE_FLAGGED = "IssueFlagged";
const char* const WorkflowEvent::STEP_COMMITTED = "StepCommitted";

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