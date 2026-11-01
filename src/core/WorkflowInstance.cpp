#include "core/WorkflowInstance.h"
#include "decorator/ApprovalWork.h"
#include "decorator/BlockedWork.h"
#include "decorator/ValidatedWork.h"
#include "state/StateFactory.h"
#include "iterator/DepthFirstIterator.h"
#include "composite/LeafTask.h"
#include "abstract_factory/WorkflowFactory.h"

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
      root_(new CompositeTask(id, definition.name)),
      started_(false),
      cancelled_(false),
      signedOff_(false),
      currentStage_(0),
      name_(definition.name),
      requiresSignOff_(false)
{
    // One CompositeTask per declared stage.
    for (std::size_t i = 0; i < definition_.stages.size(); ++i) {
        const std::string& stage = definition_.stages[i];
        root_->add(std::unique_ptr<Task>(new CompositeTask(stage, stage)));
    }

    // Items go into the first stage, or straight under the root when the
    // definition has no stages.
    CompositeTask* target = root_.get();
    if (root_->childCount() > 0) {
        target = static_cast<CompositeTask*>(root_->child(0));
    }
    for (std::size_t i = 0; i < definition_.items.size(); ++i) {
        const std::string& it = definition_.items[i];
        target->add(decorateProjectTask(std::unique_ptr<Task>(new LeafTask(it, it, ""))));
    }
}

WorkflowInstance::WorkflowInstance(const std::string& id,
                                   const WorkflowDefinition& definition,
                                   const WorkflowFactory& factory,
                                   const std::string& title)
    : id_(id),
      definition_(definition),
      root_(factory.createStage(id, title.empty() ? definition.name : title)),
      started_(false),
      cancelled_(false),
      signedOff_(false),
      currentStage_(0),
      name_(title.empty() ? definition.name : title),
      requiresSignOff_(false)
{
    definition_.stages.clear();
    for (std::size_t i = 0; i < definition.layout.size(); ++i) {
        const StageTemplate& spec = definition.layout[i];
        CompositeTask* parent = root_.get();
        if (!spec.parent.empty()) {
            parent = stageOf(root_->find(spec.parent));
            if (parent == nullptr) {
                throw std::invalid_argument("Unknown parent stage: " + spec.parent);
            }
        } else {
            definition_.stages.push_back(spec.id);
        }
        parent->add(std::unique_ptr<Task>(factory.createStage(spec.id, spec.name).release()));
    }

    for (std::size_t i = 0; i < definition.templates.size(); ++i) {
        const TaskTemplate& spec = definition.templates[i];
        std::unique_ptr<Task> task = factory.createWorkItem(spec.id, spec.name, spec.role);
        LeafTask* leaf = task->leaf();
        task->approvalWork()->setApproverRoles(spec.approverRoles);
        leaf->setPriority(spec.priority.empty() ? std::string("Medium") : spec.priority);
        if (!spec.unit.empty()) leaf->setUnit(spec.unit);
        for (std::size_t b = 0; b < spec.blockedBy.size(); ++b) {
            task->blockedWork()->addBlocker(spec.blockedBy[b]);
        }
        CompositeTask* stage = spec.stage.empty()
            ? root_.get()
            : stageOf(root_->find(spec.stage));
        if (stage == nullptr) {
            throw std::invalid_argument("Unknown stage for item " + spec.id + ": " + spec.stage);
        }
        stage->add(std::move(task));
    }
}

// ---------------------------------------------------------
// Accessors
// ---------------------------------------------------------
const std::string& WorkflowInstance::id() const                { return id_; }
const std::string& WorkflowInstance::title() const             { return definition_.name; }
std::string WorkflowInstance::state() const {
    if (cancelled_) return CANCELLED;
    if (!started_) return CREATED;
    if (!root_->isComplete()) return RUNNING;
    return requiresSignOff_ && !signedOff_ ? AWAITING_SIGNOFF : COMPLETED;
}
const std::string& WorkflowInstance::reason() const            { return reason_; }
const WorkflowDefinition& WorkflowInstance::definition() const { return definition_; }
const std::string& WorkflowInstance::name() const              { return name_; }
std::size_t WorkflowInstance::currentStageIndex() const        { return currentStage_; }
CompositeTask& WorkflowInstance::root() const                 { return *root_; }

Task* WorkflowInstance::currentStage() const {
    if (state() != RUNNING) return nullptr;
    if (definition_.stages.empty()) return root_.get();
    return root_->child(currentStage_);
}

Task* WorkflowInstance::find(const std::string& nodeId) const {
    return root_->find(nodeId);
}

LeafTask* WorkflowInstance::item(const std::string& itemId) const {
    return leafOf(root_->find(itemId));
}

// ---------------------------------------------------------
// Stage release / activation
// ---------------------------------------------------------
void WorkflowInstance::release(Task* stage) {
    DepthFirstIterator it(stage);
    for (it.first(); !it.isDone(); it.next()) {
        LeafTask* work = leafOf(it.currentItem());
        if (work != nullptr && work->status() == LeafTask::CREATED) {
            work->makeAvailable();
        }
    }
}

void WorkflowInstance::activateStage(Task* node) {
    if (node == nullptr) return;
    CompositeTask* stage = stageOf(node);
    if (stage != nullptr) {
        stage->activate();      // stage's mode decides parallel/sequential
    } else {
        release(node);
    }
}

// ---------------------------------------------------------
// Stage progress
// ---------------------------------------------------------
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
// Lifecycle
// ---------------------------------------------------------
static void illegal(const char* operation, const std::string& state) {
    throw std::logic_error(std::string("Illegal instance operation: ") + operation +
                           "() is not allowed in state '" + state + "'");
}

void WorkflowInstance::start() {
    if (state() != CREATED) illegal("start", state());
    started_ = true;
    openFirstStage();
    advance();
}

void WorkflowInstance::advance() {
    if (started_ && !cancelled_) stagesFinished();
}

void WorkflowInstance::cancel(const std::string& reason) {
    std::string now = state();
    if (now == COMPLETED || now == CANCELLED) illegal("cancel", now);
    root_->cancel();
    reason_ = reason;
    cancelled_ = true;
}

void WorkflowInstance::setStatus(const std::string& state) {
    started_ = state != CREATED;
    cancelled_ = state == CANCELLED;
    signedOff_ = state == COMPLETED;
}

// ---------------------------------------------------------
// Per-item operations — the Command classes' receiver side.
//
// Each one finds the target item and delegates the operation to the
// polymorphic TaskState (via LeafTask). If the current state
// refuses the transition, the state throws std::logic_error; we catch
// it and return false so commands can decide how to report failure.
// ---------------------------------------------------------
bool WorkflowInstance::assignItem(const std::string& itemId,
                                  const std::string& participantName) {
    LeafTask* it = item(itemId);
    if (it == nullptr) return false;
    try {
        it->assign(participantName);
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::startItem(const std::string& itemId) {
    LeafTask* it = item(itemId);
    if (it == nullptr) return false;
    try {
        it->start();
        return true;
    } catch (const std::logic_error&) {
        return false;
    }
}

bool WorkflowInstance::completeItem(const std::string& itemId) {
    LeafTask* it = item(itemId);
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
    LeafTask* it = item(itemId);
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
    LeafTask* it = item(itemId);
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
                                   std::shared_ptr<LeafTask> itemTemplate) {
    if (!itemTemplate) return false;

    // Resolve the target stage. It must exist and be a CompositeTask —
    // adding a leaf under another leaf is meaningless.
    CompositeTask* stage = stageOf(root_->find(stageId));
    if (stage == nullptr) return false;

    // Reject duplicate ids so find() stays unambiguous.
    if (root_->find(itemId) != nullptr) return false;

    // Build a fresh LeafTask from the template's metadata so it takes
    // part in the polymorphic state machine.
    stage->add(decorateProjectTask(std::unique_ptr<Task>(new LeafTask(itemId, itemTemplate->name(), itemTemplate->role()))));
    return true;
}

std::size_t WorkflowInstance::topLevelIndex(const Task* node) const {
    const Task* cursor = node;
    while (cursor != nullptr && cursor->parent() != root_.get()) {
        cursor = cursor->parent();
    }
    for (std::size_t i = 0; i < root_->childCount(); ++i) {
        if (root_->child(i) == cursor) return i;
    }
    return root_->childCount();
}

bool WorkflowInstance::isActive(const Task* node) const {
    if (state() != RUNNING || node == nullptr) return false;
    if (definition_.stages.empty()) return true;
    return topLevelIndex(node) <= currentStage_;
}

bool WorkflowInstance::adoptItem(const std::string& parentId, std::unique_ptr<Task> leaf) {
    if (!leaf || leaf->leaf() == nullptr) return false;
    Task* parent = root_->find(parentId);
    if (parent == nullptr) return false;
    if (root_->find(leaf->id()) != nullptr) return false;
    LeafTask* added = leaf->leaf();
    CompositeTask* stage = stageOf(parent);
    LeafTask* parentItem = leafOf(parent);
    if (stage != nullptr) {
        stage->add(std::move(leaf));
    } else if (parentItem != nullptr) {
        CompositeTask::addSubtask(*parentItem, std::move(leaf));
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
        (parentItem == nullptr || parentItem->status() != LeafTask::CREATED)) {
        added->makeAvailable();
    }
    return true;
}

bool WorkflowInstance::requiresSignOff() const { return requiresSignOff_; }

void WorkflowInstance::setRequiresSignOff(bool required) { requiresSignOff_ = required; }

void WorkflowInstance::signOff() {
    if (state() != AWAITING_SIGNOFF) illegal("signOff", state());
    signedOff_ = true;
}

CompositeTask& WorkflowInstance::reopenForFixes(const WorkflowFactory& factory) {
    if (state() != AWAITING_SIGNOFF) illegal("reopen", state());
    const std::string fixesId = id_ + "-fixes";
    CompositeTask* fixes = stageOf(root_->find(fixesId));
    if (fixes == nullptr) {
        root_->add(std::unique_ptr<Task>(factory.createStage(fixesId, "Sign-off fixes").release()));
        definition_.stages.push_back(fixesId);
        fixes = stageOf(root_->find(fixesId));
    }
    currentStage_ = topLevelIndex(fixes);
    return *fixes;
}

void WorkflowInstance::rebuild(const WorkflowDefinition& definition, const std::string& name,
                               std::unique_ptr<CompositeTask> root, const std::string& state,
                               std::size_t stageIndex, const std::string& reason, bool signOffRequired) {
    definition_ = definition;
    name_ = name;
    root_ = std::move(root);
    setStatus(state);
    currentStage_ = stageIndex;
    reason_ = reason;
    requiresSignOff_ = signOffRequired;
}

// ---------------------------------------------------------
// Memento
// ---------------------------------------------------------
std::shared_ptr<Memento>
WorkflowInstance::createMemento(const std::string& label) const {
    std::shared_ptr<Memento> m(
        new Memento(label, state()));
    m->setStageIndex(currentStage_);

    DepthFirstIterator it(root_.get());
    for (it.first(); !it.isDone(); it.next()) {
        LeafTask* leaf = leafOf(it.currentItem());
        if (leaf != nullptr) {
            m->addItem(WorkItemSnapshot(leaf->status(),
                                        leaf->assignee(),
                                        leaf->reason(),
                                        leaf->id()));
        }
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(Memento* memento) {
    if (!memento) return;

    // 1. The project's own flags; the tree decides the rest of its status.
    setStatus(memento->getStatusName());
    if (memento->getStageIndex() < root_->childCount() || definition_.stages.empty()) {
        currentStage_ = memento->getStageIndex();
    }

    // 2. Rebuild every leaf's polymorphic state object.
    const std::vector<WorkItemSnapshot>& snaps = memento->getItems();
    std::size_t i = 0;
    DepthFirstIterator it(root_.get());
    for (it.first(); !it.isDone(); it.next()) {
        LeafTask* leaf = leafOf(it.currentItem());
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