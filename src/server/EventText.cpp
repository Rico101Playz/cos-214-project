#include "server/EventText.h"
#include "patterns/structural/Tividy.h"

std::string EventText::describe(const Tividy& engine, const WorkflowEvent& event) {
    std::string itemName = event.itemId;
    std::string projectName = event.instanceId;
    try {
        if (!event.instanceId.empty()) {
            const WorkflowInstance& instance = engine.instance(event.instanceId);
            projectName = instance.name();
            const WorkItem* item = instance.item(event.itemId);
            if (item != nullptr) itemName = item->name();
        }
    } catch (const std::exception&) {
    }

    const std::string& t = event.type;
    if (t == WorkflowEvent::INSTANCE_STARTED) return "Project " + projectName + " started";
    if (t == WorkflowEvent::INSTANCE_COMPLETED) return "Project " + projectName + " is complete";
    if (t == WorkflowEvent::INSTANCE_CANCELLED) return "Project " + projectName + " was cancelled";
    if (t == WorkflowEvent::ITEM_ASSIGNED) return itemName + " was assigned to " + event.detail;
    if (t == WorkflowEvent::ITEM_STARTED) return event.detail + " started " + itemName;
    if (t == WorkflowEvent::ITEM_COMPLETED) return event.detail + " sent " + itemName + " for approval";
    if (t == WorkflowEvent::ITEM_APPROVED) return itemName + " was approved";
    if (t == WorkflowEvent::ITEM_REJECTED) return "Changes requested on " + itemName + ": " + event.detail;
    if (t == WorkflowEvent::ITEM_ADDED) return "New task " + itemName + " in " + projectName;
    if (t == WorkflowEvent::ITEM_ESCALATED) return event.detail + " escalated " + itemName;
    if (t == WorkflowEvent::ITEM_WITHDRAWN) return event.detail + " took " + itemName + " back out of approval";
    if (t == WorkflowEvent::ITEM_SIGNED) return event.detail + " signed off " + itemName;
    if (t == WorkflowEvent::PRIORITY_CHANGED) return itemName + " is now " + event.detail + " priority";
    if (t == WorkflowEvent::SUBTASK_ADDED) {
        std::string parentName = event.detail;
        try {
            const WorkItem* parent = engine.instance(event.instanceId).item(event.detail);
            if (parent != nullptr) parentName = parent->name();
        } catch (const std::exception&) {
        }
        return "Sub-task " + itemName + " added to " + parentName;
    }
    if (t == WorkflowEvent::APPROVAL_ROUTED) {
        return event.detail == "Auto" ? itemName + " was approved automatically"
                                      : itemName + " needs approval from a " + event.detail;
    }
    if (t == WorkflowEvent::SIGNOFF_REQUESTED) return "Project " + projectName + " is ready for the owner's sign-off";
    if (t == WorkflowEvent::ISSUE_FLAGGED) return "Sign-off issue flagged on " + projectName + ": " + event.detail;
    if (t == WorkflowEvent::ORG_CHANGED) return "Company layout changed";
    if (t == WorkflowEvent::UNDONE) return "Undone: " + event.detail;
    if (t == WorkflowEvent::REDONE) return "Redone: " + event.detail;
    return event.describe();
}
