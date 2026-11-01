#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


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
TEST(Observer, EventBus) {

    EventBus bus;
    RecordingObserver first;
    RecordingObserver second;
    WorkflowEvent started(WorkflowEvent::INSTANCE_STARTED, "wf-1");

    bus.notify(started);
    EXPECT_EQ(bus.observerCount(), 0u) << "EventBus: starts with no observers";

    bus.attach(&first);
    bus.attach(&second);
    bus.attach(&first);
    bus.attach(nullptr);
    EXPECT_EQ(bus.observerCount(), 2u) << "EventBus: ignores duplicate and null observers";

    bus.notify(started);
    EXPECT_TRUE(first.types.size() == 1 && second.types.size() == 1) << "EventBus: notify() reaches every attached observer";
    EXPECT_EQ(first.types[0], "InstanceStarted") << "EventBus: observers receive the event";

    bus.detach(&first);
    bus.notify(started);
    EXPECT_EQ(first.types.size(), 1u) << "EventBus: a detached observer hears nothing more";
    EXPECT_EQ(second.types.size(), 2u) << "EventBus: the others keep hearing events";
    bus.detach(&first);
    EXPECT_EQ(bus.observerCount(), 1u) << "EventBus: detaching a stranger changes nothing";

    OneShotObserver shy(bus);
    bus.attach(&shy);
    bus.notify(started);
    bus.notify(started);
    EXPECT_EQ(shy.heard, 1) << "EventBus: an observer can detach itself while being notified";
    EXPECT_EQ(second.types.size(), 4u) << "EventBus: others are still notified when one detaches";
}

// ============================================================================
// TEST: the concrete observers
// ============================================================================
TEST(Observer, Observers) {

    WorkflowEvent assigned(WorkflowEvent::ITEM_ASSIGNED, "wf-1", "F01", "Alice");
    EXPECT_EQ(assigned.describe(), "ItemAssigned wf-1/F01 - Alice") << "WorkflowEvent: describe() summarises the event";
    EXPECT_EQ(WorkflowEvent(WorkflowEvent::INSTANCE_STARTED, "wf-1").describe(), "InstanceStarted wf-1") << "WorkflowEvent: describe() leaves out empty parts";

    ActivityFeed feed;
    feed.onEvent(assigned);
    feed.onEvent(WorkflowEvent(WorkflowEvent::ITEM_STARTED, "wf-1", "F01", "Alice"));
    EXPECT_EQ(feed.size(), 2u) << "ActivityFeed: logs every event";
    EXPECT_EQ(feed.entries()[0], "ItemAssigned wf-1/F01 - Alice") << "ActivityFeed: keeps the events in order";

    std::ostringstream printed;
    Notifier notifier(&printed);
    notifier.onEvent(assigned);
    notifier.onEvent(WorkflowEvent(WorkflowEvent::ITEM_STARTED, "wf-1", "F01", "Alice"));
    notifier.onEvent(WorkflowEvent(WorkflowEvent::ITEM_REJECTED, "wf-1", "F01", "Missing tests"));
    notifier.onEvent(WorkflowEvent(WorkflowEvent::INSTANCE_CANCELLED, "wf-1", "", "Scope cut"));
    notifier.onEvent(WorkflowEvent(WorkflowEvent::INSTANCE_COMPLETED, "wf-1"));
    EXPECT_EQ(notifier.messages().size(), 4u) << "Notifier: ignores events nobody needs telling about";
    EXPECT_EQ(notifier.messages()[0], "Alice was assigned F01") << "Notifier: announces assignments";
    EXPECT_EQ(notifier.messages()[1], "F01 was rejected: Missing tests") << "Notifier: announces rejections";
    EXPECT_EQ(notifier.messages()[2], "wf-1 was cancelled: Scope cut") << "Notifier: announces cancellation";
    EXPECT_EQ(notifier.messages()[3], "wf-1 is complete") << "Notifier: announces completion";
    EXPECT_NE(printed.str().find("[notify] Alice was assigned F01"), std::string::npos) << "Notifier: also writes to the stream it was given";

    MetricsCollector metrics;
    EXPECT_EQ(metrics.summary(), "0 events") << "MetricsCollector: starts empty";
    metrics.onEvent(assigned);
    metrics.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F01"));
    metrics.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F02"));
    EXPECT_EQ(metrics.total(), 3) << "MetricsCollector: counts every event";
    EXPECT_EQ(metrics.count(WorkflowEvent::ITEM_APPROVED), 2) << "MetricsCollector: counts per type";
    EXPECT_EQ(metrics.count(WorkflowEvent::ITEM_REJECTED), 0) << "MetricsCollector: unseen types count 0";
    EXPECT_EQ(metrics.summary(), "3 events (ItemApproved=2, ItemAssigned=1)") << "MetricsCollector: summary() lists the counts";

    std::vector<std::string> seen;
    AutomationRule rule("log approvals", WorkflowEvent::ITEM_APPROVED,
                        [&seen](const WorkflowEvent& event) { seen.push_back(event.itemId); });
    rule.onEvent(assigned);
    EXPECT_TRUE(rule.timesFired() == 0 && seen.empty()) << "AutomationRule: ignores other event types";
    rule.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F01"));
    EXPECT_TRUE(rule.timesFired() == 1 && seen.size() == 1 && seen[0] == "F01") << "AutomationRule: runs its action when the event type matches";
    rule.trigger(assigned);
    EXPECT_EQ(rule.timesFired(), 2) << "AutomationRule: trigger() runs the action regardless of type";
    EXPECT_EQ(rule.name(), "log approvals") << "AutomationRule: keeps its name";

    AutomationRule inert("no action", WorkflowEvent::ITEM_APPROVED, AutomationRule::Action());
    inert.onEvent(WorkflowEvent(WorkflowEvent::ITEM_APPROVED, "wf-1", "F01"));
    EXPECT_EQ(inert.timesFired(), 1) << "AutomationRule: a rule without an action does not crash";
}

// ============================================================================
// TEST: events published by the Tividy facade
// ============================================================================
TEST(Observer, FacadeEvents) {

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
    EXPECT_EQ(feed.size(), 1u) << "Tividy: startInstance() publishes an event";
    EXPECT_EQ(feed.entries()[0], "InstanceStarted " + id + " - Feature Delivery") << "Tividy: the started event names the workflow";

    engine.assign(id, "Implement", "Alice");
    engine.start(id, "Implement");
    engine.complete(id, "Implement");
    engine.approve(id, "Implement");
    EXPECT_EQ(metrics.total(), 5) << "Tividy: each successful operation publishes one event";
    EXPECT_EQ(metrics.count(WorkflowEvent::ITEM_ASSIGNED), 1) << "Tividy: publishes ItemAssigned";
    EXPECT_EQ(metrics.count(WorkflowEvent::ITEM_STARTED), 1) << "Tividy: publishes ItemStarted";
    EXPECT_EQ(metrics.count(WorkflowEvent::ITEM_COMPLETED), 1) << "Tividy: publishes ItemCompleted";
    EXPECT_EQ(metrics.count(WorkflowEvent::ITEM_APPROVED), 1) << "Tividy: publishes ItemApproved";
    EXPECT_EQ(metrics.count(WorkflowEvent::INSTANCE_COMPLETED), 0) << "Tividy: no completion event while work remains";

    engine.assign(id, "Test", "Alice");
    engine.start(id, "Test");
    engine.complete(id, "Test");
    engine.reject(id, "Test", "Missing tests");
    EXPECT_EQ(notifier.messages().back(), "Test was rejected: Missing tests") << "Tividy: a rejection reaches the Notifier with its reason";

    int before = metrics.total();
    EXPECT_THROW(([&] { engine.approve(id, "Test"); })(), std::logic_error) << "Tividy: approving rejected work is refused";
    EXPECT_EQ(metrics.total(), before) << "Tividy: a call that throws publishes nothing";

    engine.cancel(id, "Scope cut");
    EXPECT_EQ(metrics.count(WorkflowEvent::INSTANCE_CANCELLED), 1) << "Tividy: cancel() publishes an event";
    EXPECT_EQ(notifier.messages().back(), id + " was cancelled: Scope cut") << "Tividy: the cancellation reason reaches the Notifier";

    // A definition with nothing to do finishes as soon as it starts.
    WorkflowDefinition empty;
    empty.name = "Empty";
    engine.loadDefinition("empty", empty);
    std::string emptyId = engine.startInstance("empty");
    EXPECT_EQ(metrics.count(WorkflowEvent::INSTANCE_COMPLETED), 1) << "Tividy: an instance that completes on start publishes InstanceCompleted";
    (void)emptyId;
}

// ============================================================================
// MAIN
// ============================================================================
