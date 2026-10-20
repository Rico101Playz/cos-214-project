#include "core/WorkflowInstance.h"
#include <stdexcept>
#include "patterns/execution/DepthFirstIterator.h"
#include <iostream>

WorkflowInstance::WorkflowInstance(std::string id, InstanceStatus initialStatus)
    : id(std::move(id)), status(initialStatus) {}

std::string WorkflowInstance::getId() const { return id; }
InstanceStatus WorkflowInstance::getStatus() const { return status; }

void WorkflowInstance::addItem(std::shared_ptr<WorkItem> item) {
    if (!item) return;
    items[item->itemId] = item;
}

std::shared_ptr<WorkItem> WorkflowInstance::getItem(const std::string& itemId) const {
    auto it = items.find(itemId);
    return it != items.end() ? it->second : nullptr;
}

std::shared_ptr<InstanceMemento> WorkflowInstance::createMemento(const std::string& label) const {
    auto m = std::make_shared<InstanceMemento>(label, status);
    for (const auto& kv : items) {
        const auto& item = kv.second;
        m->addItem(WorkItemSnapshot(item->state, item->assignee, ""));
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(InstanceMemento* memento) {
    if (!memento) return;
    status = memento->getStatus();

    const auto& snaps = memento->getItems();
    size_t i = 0;
    for (auto& kv : items) {
        if (i >= snaps.size()) break;
        kv.second->state    = snaps[i].getState();
        kv.second->assignee = snaps[i].getAssignee();
        ++i;
    }
}

void WorkflowInstance::approveItem(const std::string& itemId, bool approved, const std::string& comment) {
    auto item = getItem(itemId);
    if (item) {
        item->state = approved ? "Approved" : "Rejected";
    }
}

void WorkflowInstance::startItem(const std::string& itemId) {}
void WorkflowInstance::completeItem(const std::string& itemId) {}
void WorkflowInstance::assignItem(const std::string& itemId, const std::string& participantName) {}
void WorkflowInstance::addWorkItem(const std::string& stageId, const std::string& itemId, std::shared_ptr<WorkItem> itemTemplate) {}
void WorkflowInstance::rejectItem(const std::string& itemId, const std::string& reason) {}



const char* const WorkflowInstance::CREATED = "Created";
const char* const WorkflowInstance::RUNNING = "Running";
const char* const WorkflowInstance::COMPLETED = "Completed";
const char* const WorkflowInstance::CANCELLED = "Cancelled";

WorkflowInstance::WorkflowInstance(const std::string& id, const WorkflowDefinition& definition)
    : id_(id),
      definition_(definition),
      root_(new CompositeStage(id, definition.name)),
      state_(CREATED),
      currentStage_(0) {
    for (std::size_t i = 0; i < definition_.stages.size(); ++i) {
        const std::string& stage = definition_.stages[i];
        root_->add(std::unique_ptr<WorkNode>(new CompositeStage(stage, stage)));
    }

    // Items go into the first stage, or straight under the root when the
    // definition has no stages.
    CompositeStage* target = root_.get();
    if (root_->childCount() > 0) {
        target = static_cast<CompositeStage*>(root_->child(0));
    }
    for (std::size_t i = 0; i < definition_.items.size(); ++i) {
        const std::string& item = definition_.items[i];
        target->add(std::unique_ptr<WorkNode>(new WorkItem(item, item, "")));
    }
}

const std::string& WorkflowInstance::id() const { return id_; }

const std::string& WorkflowInstance::title() const { return definition_.name; }

const std::string& WorkflowInstance::state() const { return state_; }

const std::string& WorkflowInstance::reason() const { return reason_; }

const WorkflowDefinition& WorkflowInstance::definition() const { return definition_; }

CompositeStage& WorkflowInstance::root() const { return *root_; }

WorkNode* WorkflowInstance::currentStage() const {
    if (state_ != RUNNING) {
        return nullptr;
    }
    if (definition_.stages.empty()) {
        return root_.get();
    }
    return root_->child(currentStage_);
}

void WorkflowInstance::release(WorkNode* stage) {
    DepthFirstIterator it(stage);
    for (it.first(); !it.isDone(); it.next()) {
        WorkItem* work = dynamic_cast<WorkItem*>(it.currentItem());
        if (work != nullptr && work->status() == WorkItem::CREATED) {
            work->makeAvailable();
        }
    }
}

void WorkflowInstance::start() {
    if (state_ != CREATED) {
        throw std::logic_error("Cannot start instance " + id_ + " while it is " + state_);
    }
    state_ = RUNNING;
    currentStage_ = 0;
    release(currentStage());
    advance();
}

void WorkflowInstance::advance() {
    if (state_ != RUNNING) {
        return;
    }
    if (definition_.stages.empty()) {
        if (root_->isComplete()) {
            state_ = COMPLETED;
        }
        return;
    }
    while (root_->child(currentStage_)->isComplete()) {
        if (currentStage_ + 1 == root_->childCount()) {
            state_ = COMPLETED;
            return;
        }
        ++currentStage_;
        release(root_->child(currentStage_));
    }
}

void WorkflowInstance::cancel(const std::string& reason) {
    if (state_ == COMPLETED || state_ == CANCELLED) {
        throw std::logic_error("Cannot cancel instance " + id_ + " while it is " + state_);
    }
    root_->cancel();
    reason_ = reason;
    state_ = CANCELLED;
}

WorkNode* WorkflowInstance::find(const std::string& nodeId) const {
    return root_->find(nodeId);
}

WorkItem* WorkflowInstance::item(const std::string& itemId) const {
    return dynamic_cast<WorkItem*>(root_->find(itemId));
}
