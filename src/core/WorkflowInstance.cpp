#include "core/WorkflowInstance.h"
#include "patterns/creation/StateFactory.h"
#include "patterns/execution/CreatedInstance.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/structural/LeafWorkItem.h"

#include <stdexcept>
#include <utility>

const char* const WorkflowInstance::CREATED   = "Created";
const char* const WorkflowInstance::RUNNING   = "Running";
const char* const WorkflowInstance::COMPLETED = "Completed";
const char* const WorkflowInstance::CANCELLED = "Cancelled";

// ---------------------------------------------------------
// Construction
// ---------------------------------------------------------
WorkflowInstance::WorkflowInstance(const std::string& id,
                                   const WorkflowDefinition& definition)
    : id_(id),
      definition_(definition),
      root_(new CompositeStage(id, definition.name)),
      instanceState_(new CreatedInstance()),
      state_(instanceState_->name()),
      currentStage_(0)
{
    // One CompositeStage per declared stage.
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
        const std::string& it = definition_.items[i];
        target->add(std::unique_ptr<WorkNode>(new LeafWorkItem(it, it, "")));
    }
}

// ---------------------------------------------------------
// Accessors
// ---------------------------------------------------------
const std::string& WorkflowInstance::id() const                { return id_; }
const std::string& WorkflowInstance::title() const             { return definition_.name; }
const std::string& WorkflowInstance::state() const             { return state_; }
const std::string& WorkflowInstance::reason() const            { return reason_; }
const WorkflowDefinition& WorkflowInstance::definition() const { return definition_; }
CompositeStage& WorkflowInstance::root() const                 { return *root_; }

WorkNode* WorkflowInstance::currentStage() const {
    if (state_ != RUNNING) return nullptr;
    if (definition_.stages.empty()) return root_.get();
    return root_->child(currentStage_);
}

WorkNode* WorkflowInstance::find(const std::string& nodeId) const {
    return root_->find(nodeId);
}

WorkItem* WorkflowInstance::item(const std::string& itemId) const {
    return dynamic_cast<WorkItem*>(root_->find(itemId));
}

// ---------------------------------------------------------
// Stage release / activation
// ---------------------------------------------------------
void WorkflowInstance::release(WorkNode* stage) {
    DepthFirstIterator it(stage);
    for (it.first(); !it.isDone(); it.next()) {
        WorkItem* work = dynamic_cast<WorkItem*>(it.currentItem());
        if (work != nullptr && work->status() == WorkItem::CREATED) {
            work->makeAvailable();
        }
    }
}

void WorkflowInstance::activateStage(WorkNode* node) {
    if (node == nullptr) return;
    CompositeStage* stage = dynamic_cast<CompositeStage*>(node);
    if (stage != nullptr) {
        stage->activate();      // stage's mode decides parallel/sequential
    } else {
        release(node);
    }
}

// ---------------------------------------------------------
// State plumbing (used by InstanceState subclasses)
// ---------------------------------------------------------
void WorkflowInstance::setState(std::unique_ptr<InstanceState> next) {
    instanceState_ = std::move(next);
    state_ = instanceState_->name();
}

void WorkflowInstance::setReason(const std::string& reason) { reason_ = reason; }

void WorkflowInstance::openFirstStage() {
    currentStage_ = 0;
    activateStage(currentStage());
}

bool WorkflowInstance::stagesFinished() {
    if (definition_.stages.empty()) return root_->isComplete();
    while (root_->child(currentStage_)->isComplete()) {
        if (currentStage_ + 1 == root_->childCount()) return true;
        ++currentStage_;
        activateStage(root_->child(currentStage_));
    }
    return false;
}

// ---------------------------------------------------------
// Lifecycle — polymorphic dispatch
// ---------------------------------------------------------
void WorkflowInstance::start()   { instanceState_->start(*this); }
void WorkflowInstance::advance() { instanceState_->advance(*this); }
void WorkflowInstance::cancel(const std::string& reason) {
    instanceState_->cancel(*this, reason);
}

// ---------------------------------------------------------
// Per-item operations — the Command classes' receiver side.
//
// Each one finds the target item and delegates the operation to the
// polymorphic WorkItemState (via LeafWorkItem). If the current state
// refuses the transition, the state throws std::logic_error; we catch
// it and return false so commands can decide how to report failure.
// ---------------------------------------------------------
bool WorkflowInstance::assignItem(const std::string& itemId,
                                  const std::string& participantName) {
    WorkItem* it = item(itemId);
    if (it == nullptr) return false;
    try {
        it->assign(participantName);
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::startItem(const std::string& itemId) {
    WorkItem* it = item(itemId);
    if (it == nullptr) return false;
    try {
        it->start();
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::completeItem(const std::string& itemId) {
    WorkItem* it = item(itemId);
    if (it == nullptr) return false;
    try {
        it->complete();
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::approveItem(const std::string& itemId,
                                   bool approved,
                                   const std::string& comment) {
    WorkItem* it = item(itemId);
    if (it == nullptr) return false;
    try {
        if (approved) {
            it->approve();
        } else {
            it->reject(comment);
        }
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::rejectItem(const std::string& itemId,
                                  const std::string& reason) {
    WorkItem* it = item(itemId);
    if (it == nullptr) return false;
    try {
        it->reject(reason);
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::addWorkItem(const std::string& stageId,
                                   const std::string& itemId,
                                   std::shared_ptr<WorkItem> itemTemplate) {
    if (!itemTemplate) return false;

    // Resolve the target stage. It must exist and be a CompositeStage —
    // adding a leaf under another leaf is meaningless.
    CompositeStage* stage = dynamic_cast<CompositeStage*>(root_->find(stageId));
    if (stage == nullptr) return false;

    // Reject duplicate ids so find() stays unambiguous.
    if (root_->find(itemId) != nullptr) return false;

    // Build a fresh LeafWorkItem from the template's metadata so it takes
    // part in the polymorphic state machine.
    stage->add(std::unique_ptr<WorkNode>(
        new LeafWorkItem(itemId, itemTemplate->name(), itemTemplate->role())));
    return true;
}

// ---------------------------------------------------------
// Memento
// ---------------------------------------------------------
std::shared_ptr<InstanceMemento>
WorkflowInstance::createMemento(const std::string& label) const {
    std::shared_ptr<InstanceMemento> m(
        new InstanceMemento(label, instanceState_->name()));

    DepthFirstIterator it(root_.get());
    for (it.first(); !it.isDone(); it.next()) {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(it.currentItem());
        if (leaf != nullptr) {
            m->addItem(WorkItemSnapshot(leaf->status(),
                                        leaf->assignee(),
                                        leaf->reason()));
        }
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(InstanceMemento* memento) {
    if (!memento) return;

    // 1. Rebuild the instance state object (not just the cached string).
    setState(createInstanceState(memento->getStatusName()));

    // 2. Rebuild every leaf's polymorphic state object.
    const std::vector<WorkItemSnapshot>& snaps = memento->getItems();
    std::size_t i = 0;
    DepthFirstIterator it(root_.get());
    for (it.first(); !it.isDone(); it.next()) {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(it.currentItem());
        if (leaf == nullptr) continue;
        if (i >= snaps.size()) break;

        leaf->restore(snaps[i].getStateName(),
                      snaps[i].getAssignee(),
                      snaps[i].getReason());
        ++i;
    }
}