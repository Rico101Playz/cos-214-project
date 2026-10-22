#include "patterns/execution/Notifier.h"
#include <ostream>

Notifier::Notifier(std::ostream* out) : out_(out) {}

void Notifier::onEvent(const WorkflowEvent& event) {
    std::string message;
    if (event.type == WorkflowEvent::ITEM_ASSIGNED) {
        message = event.detail + " was assigned " + event.itemId;
    } else if (event.type == WorkflowEvent::ITEM_REJECTED) {
        message = event.itemId + " was rejected: " + event.detail;
    } else if (event.type == WorkflowEvent::INSTANCE_CANCELLED) {
        message = event.instanceId + " was cancelled: " + event.detail;
    } else if (event.type == WorkflowEvent::INSTANCE_COMPLETED) {
        message = event.instanceId + " is complete";
    } else {
        return;
    }

    messages_.push_back(message);
    if (out_ != nullptr) {
        *out_ << "[notify] " << message << "\n";
    }
}

const std::vector<std::string>& Notifier::messages() const { return messages_; }