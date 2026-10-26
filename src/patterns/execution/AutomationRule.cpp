#include "patterns/execution/AutomationRule.h"

AutomationRule::AutomationRule(const std::string& name, const std::string& eventType,
                               Action action)
    : name_(name), eventType_(eventType), action_(action), fired_(0) {}

const std::string& AutomationRule::name() const { return name_; }

int AutomationRule::timesFired() const { return fired_; }

void AutomationRule::onEvent(const WorkflowEvent& event) {
    if (event.type == eventType_) {
        trigger(event);
    }
}

void AutomationRule::trigger(const WorkflowEvent& event) {
    ++fired_;
    if (action_) {
        action_(event);
    }
}