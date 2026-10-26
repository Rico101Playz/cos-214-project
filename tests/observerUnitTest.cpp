#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_utils.h"

#include "core/WorkflowDefinition.h"
#include "patterns/execution/ActivityFeed.h"
#include "patterns/execution/AutomationRule.h"
#include "patterns/execution/EventBus.h"
#include "patterns/execution/MetricsCollector.h"
#include "patterns/execution/Notifier.h"
#include "patterns/structural/Tividy.h"

namespace {

// Remembers the type of every event it is given.
class RecordingObserver : public WorkflowObserver {
public:
    std::vector<std::string> types;
    void onEvent(const WorkflowEvent& event) override { types.push_back(event.type); }
};

// Detaches itself from the bus the first time it hears anything.
class OneShotObserver : public WorkflowObserver {
private:
    EventBus& bus_;
public:
    int heard;
    explicit OneShotObserver(EventBus& bus) : bus_(bus), heard(0) {}
    void onEvent(const WorkflowEvent&) override {
        ++heard;
        bus_.detach(this);
    }
};

template <typename Action>
bool throwsLogicError(Action action) {
    try {
        action();
    } catch (const std::logic_error&) {
        return true;
    }
    return false;
}

} // namespace

// ============================================================================
// TEST: EventBus
// ============================================================================
void test_EventBus() {
    std::cout << "\n--- Testing EventBus ---\n";

    EventBus bus;
    RecordingObserver first;
    RecordingObserver second;
    WorkflowEvent started(WorkflowEvent::INSTANCE_STARTED, "wf-1");

    bus.notify(started);
    check(bus.observerCount() == 0, "EventBus: starts with no observers");

    bus.attach(&first);
    bus.attach(&second);
    bus.attach(&first);
    bus.attach(nullptr);
    check(bus.observerCount() == 2, "EventBus: ignores duplicate and null observers");

    bus.notify(started);
    check(first.types.size() == 1 && second.types.size() == 1,
          "EventBus: notify() reaches every attached observer");
    check(first.types[0] == "InstanceStarted", "EventBus: observers receive the event");

    bus.detach(&first);
    bus.notify(started);
    check(first.types.size() == 1, "EventBus: a detached observer hears nothing more");
    check(second.types.size() == 2, "EventBus: the others keep hearing events");
    bus.detach(&first);
    check(bus.observerCount() == 1, "EventBus: detaching a stranger changes nothing");

    OneShotObserver shy(bus);
    bus.attach(&shy);
    bus.notify(started);
    bus.notify(started);
    check(shy.heard == 1, "EventBus: an observer can detach itself while being notified");
    check(second.types.size() == 4, "EventBus: others are still notified when one detaches");
}

// ============================================================================
// TEST: the concrete observers
// ============================================================================
void test_Observers() {
    std::cout << "\n--- Testing the concrete observers ---\n";

    WorkflowEvent assigned(WorkflowEvent::ITEM_ASSIGNED, "wf-1", "F01", "Alice");
    check(assigned.describe() == "ItemAssigned wf-1/F01 - Alice",
          "WorkflowEvent: describe() summarises the event");
    check(WorkflowEvent(WorkflowEvent::INSTANCE_STARTED, "wf-1").describe() ==
              "InstanceStarted wf-1",
          "WorkflowEvent: describe() leaves out empty parts");

    ActivityFeed feed;
    feed.onEvent(assigned);
    feed.onEvent(WorkflowEvent(WorkflowEvent::ITEM_STARTED, "wf-1", "F01", "Alice"));
    check(feed.size() == 2, "ActivityFeed: logs every event");
    check(feed.entries()[0] == "ItemAssigned wf-1/F01 - Alice",
          "ActivityFeed: keeps the events in order");

    std::ostringstream printed;
    Notifier notifier(&printed);
    notifier.onEvent(assigned);
    notifier.onEvent(WorkflowEvent(WorkflowEvent::ITEM_STARTED, "wf-1", "F01", "Alice"));
    notifier.onEvent(WorkflowEvent(WorkflowEvent::ITEM_REJECTED, "wf-1", "F01", "Missing tests"));
    notifier.onEvent(WorkflowEvent(WorkflowEvent::INSTANCE_CANCELLED, "wf-1", "", "Scope cut"));
    notifier.onEvent(WorkflowEvent(WorkflowEvent::INSTANCE_COMPLETED, "wf-1"));
    check(notifier.messages().size() == 4, "Notifier: ignores events nobody needs telling about");
    check(notifier.messages()[0] == "Alice was assigned F01", "Notifier: announces assignments");
    check(notifier.messages()[1] == "F01 was rejected: Missing tests", "Notifier: announces rejections");
    check(notifier.messages()[2] == "wf-1 was cancelled: Scope cut", "Notifier: announces cancellation");
    check(notifier.messages()[3] == "wf-1 is complete", "Notifier: announces completion");
    check(printed.str().find("[notify] Alice was assigned F01") != std::string::npos,
          "Notifier: also writes to the stream it was given");

    MetricsCollector metrics;
    check(metrics.summary() == "0 events", "MetricsCollector: starts empty");
    metrics.onEvent(assigned);
    metrics.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F01"));
    metrics.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F02"));
    check(metrics.total() == 3, "MetricsCollector: counts every event");
    check(metrics.count(WorkflowEvent::ITEM_APPROVED) == 2, "MetricsCollector: counts per type");
    check(metrics.count(WorkflowEvent::ITEM_REJECTED) == 0, "MetricsCollector: unseen types count 0");
    check(metrics.summary() == "3 events (ItemApproved=2, ItemAssigned=1)",
          "MetricsCollector: summary() lists the counts");

    std::vector<std::string> seen;
    AutomationRule rule("log approvals", WorkflowEvent::ITEM_APPROVED,
                        [&seen](const WorkflowEvent& event) { seen.push_back(event.itemId); });
    rule.onEvent(assigned);
    check(rule.timesFired() == 0 && seen.empty(), "AutomationRule: ignores other event types");
    rule.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F01"));
    check(rule.timesFired() == 1 && seen.size() == 1 && seen[0] == "F01",
          "AutomationRule: runs its action when the event type matches");
    rule.trigger(assigned);
    check(rule.timesFired() == 2, "AutomationRule: trigger() runs the action regardless of type");
    check(rule.name() == "log approvals", "AutomationRule: keeps its name");

    AutomationRule inert("no action", WorkflowEvent::ITEM_APPROVED, AutomationRule::Action());
    inert.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F01"));
    check(inert.timesFired() == 1, "AutomationRule: a rule without an action does not crash");
}

// ============================================================================
// TEST: events published by the Tividy facade
// ============================================================================
void test_FacadeEvents() {
    std::cout << "\n--- Testing events published by Tividy ---\n";

    Tividy engine("Acme Software");
    engine.organisation().addParticipant("Alice", "Developer");

    WorkflowDefinition definition;
    definition.name = "Feature Delivery";
    definition.stages.push_back("Build");
    definition.items.push_back("Implement");
    definition.items.push_back("Test");
    engine.loadDefinition("delivery", definition);

    ActivityFeed feed;
    MetricsCollector metrics;
    Notifier notifier;
    engine.bus().attach(&feed);
    engine.bus().attach(&metrics);
    engine.bus().attach(&notifier);

    std::string id = engine.startInstance("delivery");
    check(feed.size() == 1, "Tividy: startInstance() publishes an event");
    check(feed.entries()[0] == "InstanceStarted " + id + " - Feature Delivery",
          "Tividy: the started event names the workflow");

    engine.assign(id, "Implement", "Alice");
    engine.start(id, "Implement");
    engine.complete(id, "Implement");
    engine.approve(id, "Implement");
    check(metrics.total() == 5, "Tividy: each successful operation publishes one event");
    check(metrics.count(WorkflowEvent::ITEM_ASSIGNED) == 1, "Tividy: publishes ItemAssigned");
    check(metrics.count(WorkflowEvent::ITEM_STARTED) == 1, "Tividy: publishes ItemStarted");
    check(metrics.count(WorkflowEvent::ITEM_COMPLETED) == 1, "Tividy: publishes ItemCompleted");
    check(metrics.count(WorkflowEvent::ITEM_APPROVED) == 1, "Tividy: publishes ItemApproved");
    check(metrics.count(WorkflowEvent::INSTANCE_COMPLETED) == 0,
          "Tividy: no completion event while work remains");

    engine.assign(id, "Test", "Alice");
    engine.start(id, "Test");
    engine.complete(id, "Test");
    engine.reject(id, "Test", "Missing tests");
    check(notifier.messages().back() == "Test was rejected: Missing tests",
          "Tividy: a rejection reaches the Notifier with its reason");

    int before = metrics.total();
    check(throwsLogicError([&] { engine.approve(id, "Test"); }),
          "Tividy: approving rejected work is refused");
    check(metrics.total() == before, "Tividy: a call that throws publishes nothing");

    engine.cancel(id, "Scope cut");
    check(metrics.count(WorkflowEvent::INSTANCE_CANCELLED) == 1, "Tividy: cancel() publishes an event");
    check(notifier.messages().back() == id + " was cancelled: Scope cut",
          "Tividy: the cancellation reason reaches the Notifier");

    // A definition with nothing to do finishes as soon as it starts.
    WorkflowDefinition empty;
    empty.name = "Empty";
    engine.loadDefinition("empty", empty);
    std::string emptyId = engine.startInstance("empty");
    check(metrics.count(WorkflowEvent::INSTANCE_COMPLETED) == 1,
          "Tividy: an instance that completes on start publishes InstanceCompleted");
    (void)emptyId;
}

// ============================================================================
// MAIN
// ============================================================================
void run_observer_tests() {
    std::cout << "\n--- Running Observer Tests ---\n";
    int passedBefore = testsPassed;
    int failedBefore = testsFailed;

    test_EventBus();
    test_Observers();
    test_FacadeEvents();

    std::cout << "Observer Tests: " << (testsPassed - passedBefore) << " passed, "
              << (testsFailed - failedBefore) << " failed.\n";
}