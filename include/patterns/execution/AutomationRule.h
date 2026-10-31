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
/**
 * @class AutomationRule
 * @brief Concrete observer that executes a custom action when a specific workflow event occurs.
 * 
 * Implements the Observer pattern to trigger a caller-provided callback 
 * (e.g., an automated webhook, pull request, or external notification) whenever 
 * a matching event type is published across the workflow.
 */
class AutomationRule : public WorkflowObserver {
public:
    /**
     * @brief Callback function type invoked when the rule fires.
     */
    typedef std::function<void(const WorkflowEvent&)> Action;

private:
    std::string name_;      /*!< Descriptive name of the automation rule. */
    std::string eventType_; /*!< The specific event type this rule listens for. */
    Action action_;         /*!< The callable action executed upon event match. */
    int fired_;             /*!< Counter tracking how many times this rule has fired. */

public:
    /**
     * @brief Constructs an AutomationRule.
     * 
     * @param[in] name The descriptive name or label for this rule.
     * @param[in] eventType The target event type that triggers the rule.
     * @param[in] action The callable function or lambda to execute on trigger.
     */
    AutomationRule(const std::string& name, const std::string& eventType, Action action);

    /**
     * @brief Gets the name of the automation rule.
     * @return const std::string& The rule's name.
     */
    const std::string& name() const;

    /**
     * @brief Retrieves the number of times this rule has been executed.
     * @return int Total execution count.
     */
    int timesFired() const;

    // Fires the rule when the event has the type this rule is waiting for.
    /**
     * @brief Receives incoming workflow events and triggers the action if the event type matches.
     * @param[in] event The event dispatched by the workflow engine.
     */
    void onEvent(const WorkflowEvent& event) override;

    // Runs the action now, whatever the event type.
    /**
     * @brief Unconditionally executes the associated action and increments the fire counter.
     * @param[in] event The event context passed into the callback action.
     */
    void trigger(const WorkflowEvent& event);
};

#endif // AUTOMATIONRULE_H