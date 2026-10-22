#ifndef AUTOMATIONRULE_H
#define AUTOMATIONRULE_H

#include <functional>
#include <string>
#include "patterns/execution/WorkflowObserver.h"

// OBSERVER pattern - a CONCRETE OBSERVER.
//
// "When an event of this type happens, do this." The action is supplied by the
// caller, e.g. a lambda that opens a pull request through an ExternalConnector
// once an item is approved.
class AutomationRule : public WorkflowObserver {
public:
    typedef std::function<void(const WorkflowEvent&)> Action;

private:
    std::string name_;
    std::string eventType_;
    Action action_;
    int fired_;

public:
    AutomationRule(const std::string& name, const std::string& eventType, Action action);

    const std::string& name() const;
    int timesFired() const;

    // Fires the rule when the event has the type this rule is waiting for.
    void onEvent(const WorkflowEvent& event) override;

    // Runs the action now, whatever the event type.
    void trigger(const WorkflowEvent& event);
};

#endif // AUTOMATIONRULE_H