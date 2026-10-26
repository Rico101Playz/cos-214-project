#include "patterns/structural/Tividy.h"
#include <sstream>
#include <stdexcept>
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "patterns/execution/ApproveCommand.h"
#include "patterns/execution/AssignCommand.h"
#include "patterns/execution/CancelCommand.h"
#include "patterns/execution/CompleteCommand.h"
#include "patterns/execution/RejectCommand.h"
#include "patterns/execution/StartCommand.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/execution/ReadyOnlyIterator.h"

Tividy::Tividy(const std::string& organisationName)
    : organisation_(organisationName), nextInstance_(1) {}

Organisation& Tividy::organisation() { return organisation_; }

EventBus& Tividy::bus() { return bus_; }

WorkflowInstance& Tividy::findInstance(const std::string& instanceId) const {
    std::map<std::string, std::unique_ptr<WorkflowInstance> >::const_iterator it =
        instances_.find(instanceId);
    if (it == instances_.end()) {
        throw std::invalid_argument("Unknown instance: " + instanceId);
    }
    return *it->second;
}

WorkItem& Tividy::findItem(const std::string& instanceId, const std::string& itemId) const {
    WorkItem* item = findInstance(instanceId).item(itemId);
    if (item == nullptr) {
        throw std::invalid_argument("Unknown item " + itemId + " in instance " + instanceId);
    }
    return *item;
}

Participant& Tividy::findParticipant(const std::string& name) const {
    Participant* participant = organisation_.find(name);
    if (participant == nullptr) {
        throw std::invalid_argument("Unknown participant: " + name);
    }
    return *participant;
}

void Tividy::record(const std::string& instanceId, WorkCommand* command) {
    invoker_.invoke(std::unique_ptr<WorkCommand>(command), &findInstance(instanceId));
    commandInstances_.push_back(instanceId);
}

void Tividy::publish(const std::string& type, const std::string& instanceId,
                     const std::string& itemId, const std::string& detail) const {
    bus_.notify(WorkflowEvent(type, instanceId, itemId, detail));
}

void Tividy::loadDefinition(const std::string& name) {
    ConcreteDefinitionBuilder builder;
    FeatureDeliveryProcess process;
    process.construct(&builder);
    loadDefinition(name, builder.build());
}

void Tividy::loadDefinition(const std::string& name, const WorkflowDefinition& definition) {
    definitions_[name] = definition;
}


bool Tividy::hasDefinition(const std::string& name) const {
    return definitions_.find(name) != definitions_.end();
}

std::string Tividy::startInstance(const std::string& definition) {
    std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.find(definition);
    if (it == definitions_.end()) {
        throw std::invalid_argument("Unknown definition: " + definition);
    }

    std::ostringstream id;
    id << "instance-" << nextInstance_++;

    std::unique_ptr<WorkflowInstance> instance(new WorkflowInstance(id.str(), it->second));
    instance->start();
    bool finished = instance->state() == WorkflowInstance::COMPLETED;
    instances_[id.str()] = std::move(instance);

    publish(WorkflowEvent::INSTANCE_STARTED, id.str(), "", it->second.name);
    if (finished) {
        publish(WorkflowEvent::INSTANCE_COMPLETED, id.str());
    }
    return id.str();
}

void Tividy::assign(const std::string& instanceId, const std::string& itemId,
                    const std::string& participant) {
    // 1. Validate up front (so we throw invalid_argument, not logic_error).
    WorkItem& item = findItem(instanceId, itemId);
    Participant& assignee = findParticipant(participant);

    Participant* previous = nullptr;
    if (item.status() == WorkItem::ASSIGNED) {
        previous = organisation_.find(item.assignee());
    }

    // 2. Let the command perform the mutation (single source of truth).
    record(instanceId, new AssignCommand(&snapshots_, instanceId, itemId, participant));

    // 3. Side effects AFTER the successful mutation.
    if (previous != nullptr) {
        previous->addLoad(-1);
    }
    assignee.addLoad(1);
    publish(WorkflowEvent::ITEM_ASSIGNED, instanceId, itemId, participant);
}

void Tividy::start(const std::string& instanceId, const std::string& itemId) {
    // The command drives the transition; if it throws (illegal state),
    // the event never gets published, and undo history stays consistent.
    record(instanceId, new StartCommand(&snapshots_, instanceId, itemId));

    WorkItem& item = findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_STARTED, instanceId, itemId, item.assignee());
}

void Tividy::complete(const std::string& instanceId, const std::string& itemId) {
    record(instanceId, new CompleteCommand(&snapshots_, instanceId, itemId));
    WorkItem& item = findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_COMPLETED, instanceId, itemId, item.assignee());
}

void Tividy::approve(const std::string& instanceId, const std::string& itemId) {
    WorkflowInstance& instance = findInstance(instanceId);

    record(instanceId, new ApproveCommand(&snapshots_, instanceId, itemId, true, ""));

    // Now the item really is approved; adjust loads and possibly complete the instance.
    WorkItem& item = findItem(instanceId, itemId);
    Participant* assignee = organisation_.find(item.assignee());
    if (assignee != nullptr) {
        assignee->addLoad(-1);
    }

    // Note: instance.advance() used to be here. That has side effects (releases
    // next stage). If approve() is the last item in the current stage, you want
    // advance() — but it mutates the instance, so it should be inside the command
    // OR treated as a separate command. For now, keep it here (post-command, pre-event).
    instance.advance();
    publish(WorkflowEvent::ITEM_APPROVED, instanceId, itemId, item.assignee());
    if (instance.state() == WorkflowInstance::COMPLETED) {
        publish(WorkflowEvent::INSTANCE_COMPLETED, instanceId);
    }
}

void Tividy::reject(const std::string& instanceId, const std::string& itemId,
                    const std::string& reason) {
    record(instanceId, new RejectCommand(&snapshots_, instanceId, itemId, reason));

    WorkItem& item = findItem(instanceId, itemId);
    Participant* assignee = organisation_.find(item.assignee());
    if (assignee != nullptr) {
        assignee->addLoad(-1);
    }
    publish(WorkflowEvent::ITEM_REJECTED, instanceId, itemId, reason);
}

void Tividy::cancel(const std::string& instanceId, const std::string& reason) {
    WorkflowInstance& instance = findInstance(instanceId);

    // 1. Snapshot who is busy — must happen BEFORE the command cancels,
    //    because cancellation wipes item statuses.
    std::vector<Participant*> busy;
    DepthFirstIterator it(&instance.root());
    for (it.first(); !it.isDone(); it.next()) {
        WorkItem* item = dynamic_cast<WorkItem*>(it.currentItem());
        if (item != nullptr &&
            (item->status() == WorkItem::ASSIGNED ||
             item->status() == WorkItem::IN_PROGRESS ||
             item->status() == WorkItem::AWAITING_APPROVAL)) {
            Participant* assignee = organisation_.find(item->assignee());
            if (assignee != nullptr) {
                busy.push_back(assignee);
            }
        }
    }

    // 2. The command performs the actual cancel (single source of mutation).
    record(instanceId, new CancelCommand(&snapshots_, instanceId, instanceId, reason));

    // 3. Release loads AFTER the successful cancellation.
    for (std::size_t i = 0; i < busy.size(); ++i) {
        busy[i]->addLoad(-1);
    }
    publish(WorkflowEvent::INSTANCE_CANCELLED, instanceId, "", reason);
}

void Tividy::undo() {
    if (commandInstances_.empty()) {
        return;
    }
    std::string instanceId = commandInstances_.back();
    commandInstances_.pop_back();
    invoker_.undoLast(&findInstance(instanceId));
}

const WorkflowInstance& Tividy::instance(const std::string& instanceId) const {
    return findInstance(instanceId);
}

std::vector<std::string> Tividy::readyItems(const std::string& instanceId) const {
    std::vector<std::string> ids;
    ReadyOnlyIterator it(&findInstance(instanceId).root());
    for (it.first(); !it.isDone(); it.next()) {
        ids.push_back(it.currentItem()->id());
    }
    return ids;
}

double Tividy::progress(const std::string& instanceId) const {
    // Counted per work item, so an empty stage does not inflate the figure.
    int total = 0;
    int done = 0;
    DepthFirstIterator it(&findInstance(instanceId).root());
    for (it.first(); !it.isDone(); it.next()) {
        WorkItem* item = dynamic_cast<WorkItem*>(it.currentItem());
        if (item != nullptr) {
            ++total;
            if (item->isComplete()) {
                ++done;
            }
        }
    }
    return total == 0 ? 1.0 : static_cast<double>(done) / total;
}
