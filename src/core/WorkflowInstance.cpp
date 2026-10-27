#include "core/WorkflowInstance.h"
#include "patterns/creation/StateFactory.h"
#include "patterns/execution/CreatedInstance.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/structural/LeafWorkItem.h"
#include "patterns/creation/WorkflowFactory.h"

#include <stdexcept>
#include <utility>

const char* const WorkflowInstance::CREATED   = "Created";
const char* const WorkflowInstance::RUNNING   = "Running";
const char* const WorkflowInstance::COMPLETED = "Completed";
const char* const WorkflowInstance::CANCELLED = "Cancelled";
const char* const WorkflowInstance::AWAITING_SIGNOFF = "AwaitingSignOff";

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
      currentStage_(0),
      name_(definition.name),
      requiresSignOff_(false)
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

WorkflowInstance::WorkflowInstance(const std::string& id,
                                   const WorkflowDefinition& definition,
                                   const WorkflowFactory& factory,
                                   const std::string& title)
    : id_(id),
      definition_(definition),
      root_(factory.createStage(id, title.empty() ? definition.name : title)),
      instanceState_(new CreatedInstance()),
      state_(instanceState_->name()),
      currentStage_(0),
      name_(title.empty() ? definition.name : title),
      requiresSignOff_(false)
{
    definition_.stages.clear();
    for (std::size_t i = 0; i < definition.layout.size(); ++i) {
        const StageTemplate& spec = definition.layout[i];
        CompositeStage* parent = root_.get();
        if (!spec.parent.empty()) {
            parent = dynamic_cast<CompositeStage*>(root_->find(spec.parent));
            if (parent == nullptr) {
                throw std::invalid_argument("Unknown parent stage: " + spec.parent);
            }
        } else {
            definition_.stages.push_back(spec.id);
        }
        parent->add(std::unique_ptr<WorkNode>(factory.createStage(spec.id, spec.name).release()));
    }

    for (std::size_t i = 0; i < definition.templates.size(); ++i) {
        const ItemTemplate& spec = definition.templates[i];
        std::unique_ptr<LeafWorkItem> leaf = factory.createWorkItem(spec.id, spec.name, spec.role);
        leaf->setApproverRoles(spec.approverRoles);
        leaf->setPriority(spec.priority.empty() ? std::string("Medium") : spec.priority);
        if (!spec.unit.empty()) leaf->setUnit(spec.unit);
        for (std::size_t b = 0; b < spec.blockedBy.size(); ++b) {
            leaf->addBlocker(spec.blockedBy[b]);
        }
        CompositeStage* stage = spec.stage.empty()
            ? root_.get()
            : dynamic_cast<CompositeStage*>(root_->find(spec.stage));
        if (stage == nullptr) {
            throw std::invalid_argument("Unknown stage for item " + spec.id + ": " + spec.stage);
        }
        stage->add(std::unique_ptr<WorkNode>(leaf.release()));
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
const std::string& WorkflowInstance::name() const              { return name_; }
std::size_t WorkflowInstance::currentStageIndex() const        { return currentStage_; }
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

std::size_t WorkflowInstance::topLevelIndex(const WorkNode* node) const {
    const WorkNode* cursor = node;
    while (cursor != nullptr && cursor->parent() != root_.get()) {
        cursor = cursor->parent();
    }
    for (std::size_t i = 0; i < root_->childCount(); ++i) {
        if (root_->child(i) == cursor) return i;
    }
    return root_->childCount();
}

bool WorkflowInstance::isActive(const WorkNode* node) const {
    if (state_ != RUNNING || node == nullptr) return false;
    if (definition_.stages.empty()) return true;
    return topLevelIndex(node) <= currentStage_;
}

bool WorkflowInstance::adoptItem(const std::string& parentId, std::unique_ptr<LeafWorkItem> leaf) {
    if (!leaf) return false;
    WorkNode* parent = root_->find(parentId);
    if (parent == nullptr) return false;
    if (root_->find(leaf->id()) != nullptr) return false;
    LeafWorkItem* added = leaf.get();
    CompositeStage* stage = dynamic_cast<CompositeStage*>(parent);
    LeafWorkItem* parentItem = dynamic_cast<LeafWorkItem*>(parent);
    if (stage != nullptr) {
        stage->add(std::unique_ptr<WorkNode>(leaf.release()));
    } else if (parentItem != nullptr) {
        parentItem->addSubtask(std::move(leaf));
    } else {
        return false;
    }
    bool waitsForSiblings = false;
    if (stage != nullptr && stage->mode().name() == "Sequential") {
        for (std::size_t i = 0; i + 1 < stage->childCount(); ++i) {
            if (!stage->child(i)->isComplete()) waitsForSiblings = true;
        }
    }
    if (isActive(parent) && !waitsForSiblings &&
        (parentItem == nullptr || parentItem->status() != WorkItem::CREATED)) {
        added->makeAvailable();
    }
    return true;
}

bool WorkflowInstance::requiresSignOff() const { return requiresSignOff_; }

void WorkflowInstance::setRequiresSignOff(bool required) { requiresSignOff_ = required; }

void WorkflowInstance::signOff() { instanceState_->signOff(*this); }

CompositeStage& WorkflowInstance::reopenForFixes(const WorkflowFactory& factory) {
    instanceState_->reopen(*this);
    const std::string fixesId = id_ + "-fixes";
    CompositeStage* fixes = dynamic_cast<CompositeStage*>(root_->find(fixesId));
    if (fixes == nullptr) {
        root_->add(std::unique_ptr<WorkNode>(factory.createStage(fixesId, "Sign-off fixes").release()));
        definition_.stages.push_back(fixesId);
        fixes = dynamic_cast<CompositeStage*>(root_->find(fixesId));
    }
    currentStage_ = topLevelIndex(fixes);
    return *fixes;
}

void WorkflowInstance::rebuild(const WorkflowDefinition& definition, const std::string& name,
                               std::unique_ptr<CompositeStage> root, const std::string& state,
                               std::size_t stageIndex, const std::string& reason, bool signOffRequired) {
    definition_ = definition;
    name_ = name;
    root_ = std::move(root);
    setState(createInstanceState(state));
    currentStage_ = stageIndex;
    reason_ = reason;
    requiresSignOff_ = signOffRequired;
}

// ---------------------------------------------------------
// Memento
// ---------------------------------------------------------
std::shared_ptr<InstanceMemento>
WorkflowInstance::createMemento(const std::string& label) const {
    std::shared_ptr<InstanceMemento> m(
        new InstanceMemento(label, instanceState_->name()));
    m->setStageIndex(currentStage_);

    DepthFirstIterator it(root_.get());
    for (it.first(); !it.isDone(); it.next()) {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(it.currentItem());
        if (leaf != nullptr) {
            m->addItem(WorkItemSnapshot(leaf->status(),
                                        leaf->assignee(),
                                        leaf->reason(),
                                        leaf->id()));
        }
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(InstanceMemento* memento) {
    if (!memento) return;

    // 1. Rebuild the instance state object (not just the cached string).
    setState(createInstanceState(memento->getStatusName()));
    if (memento->getStageIndex() < root_->childCount() || definition_.stages.empty()) {
        currentStage_ = memento->getStageIndex();
    }

    // 2. Rebuild every leaf's polymorphic state object.
    const std::vector<WorkItemSnapshot>& snaps = memento->getItems();
    std::size_t i = 0;
    DepthFirstIterator it(root_.get());
    for (it.first(); !it.isDone(); it.next()) {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(it.currentItem());
        if (leaf == nullptr) continue;

        const WorkItemSnapshot* snap = nullptr;
        for (std::size_t s = 0; s < snaps.size() && snap == nullptr; ++s) {
            if (!snaps[s].getItemId().empty() && snaps[s].getItemId() == leaf->id()) {
                snap = &snaps[s];
            }
        }
        if (snap == nullptr && i < snaps.size() && snaps[i].getItemId().empty()) {
            snap = &snaps[i];
        }
        ++i;
        if (snap == nullptr) continue;

        leaf->restore(snap->getStateName(),
                      snap->getAssignee(),
                      snap->getReason());
    }
}