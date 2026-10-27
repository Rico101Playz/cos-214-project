#include "patterns/structural/Tividy.h"
#include <set>
#include <sstream>
#include <stdexcept>
#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include "patterns/creation/FeatureDeliveryProcess.h"
#include "patterns/creation/PolicyCatalog.h"
#include "patterns/creation/WorkflowAssembler.h"
#include "patterns/execution/ApproveCommand.h"
#include "patterns/execution/AssignCommand.h"
#include "patterns/execution/CancelCommand.h"
#include "patterns/execution/CompleteCommand.h"
#include "patterns/execution/EscalateCommand.h"
#include "patterns/execution/RejectCommand.h"
#include "patterns/execution/StartCommand.h"
#include "patterns/execution/WithdrawCommand.h"
#include "patterns/execution/AddWorkCommand.h"
#include "patterns/execution/SetPriorityCommand.h"
#include "patterns/execution/SignOffCommand.h"
#include "patterns/execution/SignCommand.h"
#include "patterns/execution/RouteApprovalCommand.h"
#include "patterns/execution/SetDataCommand.h"
#include "patterns/execution/OwnerHandler.h"
#include "patterns/execution/ManagerFallbackHandler.h"
#include "patterns/execution/PolicyChainHandler.h"
#include "patterns/execution/DepthFirstIterator.h"
#include "patterns/execution/ReadyOnlyIterator.h"
#include "patterns/execution/AssignedToIterator.h"
#include "patterns/execution/AwaitingApprovalIterator.h"
#include "patterns/execution/ApprovalRequest.h"
#include "patterns/execution/AssignmentPipeline.h"
#include "patterns/structural/LeafWorkItem.h"

Tividy::ActorScope::ActorScope(Tividy& engine, const std::string& actor)
    : engine_(engine), previous_(engine.actor_) {
    engine_.actor_ = actor;
}

Tividy::ActorScope::~ActorScope() { engine_.actor_ = previous_; }

Tividy::Step::Step(Tividy& engine, const std::string& label)
    : engine_(engine), outer_(engine.depth_ == 0), committed_(false), label_(label) {
    if (outer_) before_ = engine_.snapshot();
    ++engine_.depth_;
}

Tividy::Step::~Step() {
    --engine_.depth_;
    if (!outer_) return;
    std::vector<WorkflowEvent> events;
    events.swap(engine_.pendingEvents_);
    if (committed_) {
        try {
            WorkflowEvent event(WorkflowEvent::STEP_COMMITTED, "", "", label_);
            event.actor = engine_.actor_;
            event.before = std::shared_ptr<const EngineMemento>(new EngineMemento(label_, before_));
            engine_.historyBus_.notify(event);
        } catch (...) {
        }
        for (std::size_t i = 0; i < events.size(); ++i) {
            try {
                engine_.bus_.notify(events[i]);
            } catch (...) {
            }
        }
        return;
    }
    try {
        if (engine_.snapshot() != before_) engine_.restore(before_);
    } catch (...) {
    }
}

void Tividy::Step::commit() { committed_ = true; }

Tividy::Tividy(const std::string& organisationName)
    : organisation_(organisationName), ownHistory_(new UndoHistory()), history_(nullptr),
      nextInstance_(1), nextTask_(1), depth_(0) {
    useHistory(ownHistory_.get());
    invoker_.setLimit(200);
}

std::shared_ptr<const EngineMemento> Tividy::createMemento(const std::string& label) const {
    return std::shared_ptr<const EngineMemento>(new EngineMemento(label, snapshot()));
}

void Tividy::restoreMemento(const EngineMemento& memento) {
    restore(memento.state_);
}

UndoHistory* Tividy::history() const { return history_; }

void Tividy::useHistory(UndoHistory* history) {
    if (history_ != nullptr) historyBus_.detach(history_);
    history_ = history;
    if (history_ != nullptr) historyBus_.attach(history_);
}

Company& Tividy::organisation() { return organisation_; }

const Company& Tividy::organisation() const { return organisation_; }

EventBus& Tividy::bus() { return bus_; }

EventBus& Tividy::historyBus() { return historyBus_; }

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

LeafWorkItem& Tividy::findLeaf(const std::string& instanceId, const std::string& itemId) const {
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(&findItem(instanceId, itemId));
    if (leaf == nullptr) {
        throw std::invalid_argument("Item " + itemId + " is not a work item leaf");
    }
    return *leaf;
}

Tividy::Runtime& Tividy::runtime(const std::string& instanceId) const {
    std::map<std::string, std::unique_ptr<Runtime> >::const_iterator it = runtimes_.find(instanceId);
    if (it == runtimes_.end()) {
        throw std::invalid_argument("Unknown instance: " + instanceId);
    }
    return *it->second;
}

std::unique_ptr<Tividy::Runtime> Tividy::makeRuntime(const std::string& policy, bool structured,
                                                     const Organisation& people) {
    std::unique_ptr<Runtime> rt(new Runtime());
    rt->factory = PolicyCatalog::create(policy);
    rt->assignment = WorkflowAssembler::buildAssignmentService(*rt->factory, people);
    rt->approvals.reset(new OwnerHandler(organisation_));
    PolicyChainHandler* policyLink = new PolicyChainHandler(rt->factory->createApprovalChain());
    rt->approvals->setNext(policyLink);
    policyLink->setNext(new ManagerFallbackHandler());
    rt->structured = structured;
    return rt;
}

void Tividy::recomputeLoads() {
    std::map<std::string, int> counts;
    for (std::map<std::string, std::unique_ptr<WorkflowInstance> >::const_iterator inst = instances_.begin();
         inst != instances_.end(); ++inst) {
        DepthFirstIterator it(&inst->second->root());
        for (it.first(); !it.isDone(); it.next()) {
            WorkItem* item = dynamic_cast<WorkItem*>(it.currentItem());
            if (item == nullptr || item->assignee().empty()) continue;
            if (item->holdsLoad()) {
                ++counts[item->assignee()];
            }
        }
    }
    std::vector<Participant*> people = organisation_.participants();
    for (std::size_t i = 0; i < people.size(); ++i) {
        people[i]->addLoad(-people[i]->load());
        people[i]->addLoad(counts[people[i]->name()]);
    }
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
}

void Tividy::publish(const std::string& type, const std::string& instanceId,
                     const std::string& itemId, const std::string& detail) const {
    WorkflowEvent event(type, instanceId, itemId, detail);
    event.actor = actor_;
    if (depth_ > 0) {
        pendingEvents_.push_back(event);
    } else {
        bus_.notify(event);
    }
}

std::string Tividy::label(const std::string& verb, const std::string& instanceId,
                          const std::string& itemId) const {
    std::string who = actor_.empty() ? std::string("System") : actor_;
    std::string what;
    std::string where = instanceId;
    try {
        if (!instanceId.empty()) {
            const WorkflowInstance& instance = findInstance(instanceId);
            where = instance.name();
            if (!itemId.empty()) {
                WorkItem* item = instance.item(itemId);
                what = item != nullptr ? item->name() : itemId;
            }
        }
    } catch (const std::exception&) {
    }
    std::string text = who + " " + verb;
    if (!what.empty()) text += " " + what;
    if (!where.empty()) text += " (" + where + ")";
    return text;
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

std::vector<std::string> Tividy::definitionNames() const {
    std::vector<std::string> names;
    for (std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.begin();
         it != definitions_.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

const WorkflowDefinition& Tividy::definition(const std::string& name) const {
    std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.find(name);
    if (it == definitions_.end()) {
        throw std::invalid_argument("Unknown definition: " + name);
    }
    return it->second;
}

std::string Tividy::launch(const WorkflowDefinition& definition, const std::string& title,
                           const std::string& policy, SignOffRule signOff) {
    std::ostringstream id;
    id << "instance-" << nextInstance_;

    std::unique_ptr<Runtime> rt = makeRuntime(policy.empty() ? definition.policy : policy,
                                              definition.structured(),
                                              organisation_.people());

    std::unique_ptr<WorkflowInstance> instance(
        rt->structured ? new WorkflowInstance(id.str(), definition, *rt->factory, title)
                       : new WorkflowInstance(id.str(), definition));
    instance->setRequiresSignOff(signOff == SignOffRule::Required ||
                                 (signOff == SignOffRule::FromTemplate && definition.signOff));
    ++nextInstance_;
    instance->start();
    bool finished = instance->state() == WorkflowInstance::COMPLETED;
    instances_[id.str()] = std::move(instance);
    runtimes_[id.str()] = std::move(rt);
    instanceOrder_.push_back(id.str());

    publish(WorkflowEvent::INSTANCE_STARTED, id.str(), "", definition.name);
    if (finished) {
        publish(WorkflowEvent::INSTANCE_COMPLETED, id.str());
    }
    return id.str();
}

std::string Tividy::startInstance(const std::string& definition) {
    std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.find(definition);
    if (it == definitions_.end()) {
        throw std::invalid_argument("Unknown definition: " + definition);
    }
    Step step(*this, label("started " + definition, ""));
    std::string id = launch(it->second, "", "", SignOffRule::FromTemplate);
    step.commit();
    return id;
}

std::string Tividy::startProject(const std::string& definition, const std::string& title,
                                 const std::string& policy, SignOffRule signOff) {
    std::map<std::string, WorkflowDefinition>::const_iterator it = definitions_.find(definition);
    if (it == definitions_.end()) {
        throw std::invalid_argument("Unknown definition: " + definition);
    }
    if (!policy.empty() && !PolicyCatalog::exists(policy)) {
        throw std::invalid_argument("Unknown policy: " + policy);
    }
    if (title.size() > 120) {
        throw std::invalid_argument("Project title is too long");
    }
    Step step(*this, label("started project " + (title.empty() ? definition : title), ""));
    std::string id = launch(it->second, title, policy, signOff);
    step.commit();
    return id;
}

bool Tividy::dependsOn(const LeafWorkItem& from, const LeafWorkItem& target) const {
    std::set<const LeafWorkItem*> waitingOn;
    for (const LeafWorkItem* up = &target; up != nullptr; up = up->parentItem()) waitingOn.insert(up);
    const WorkNode* top = &from;
    while (top->parent() != nullptr) top = top->parent();
    std::vector<const LeafWorkItem*> todo(1, &from);
    std::set<const LeafWorkItem*> seen;
    while (!todo.empty()) {
        const LeafWorkItem* item = todo.back();
        todo.pop_back();
        if (!seen.insert(item).second) continue;
        if (waitingOn.count(item) != 0) return true;
        for (std::size_t i = 0; i < item->childCount(); ++i) {
            const LeafWorkItem* sub = dynamic_cast<const LeafWorkItem*>(item->child(i));
            if (sub != nullptr) todo.push_back(sub);
        }
        std::vector<std::string> blockers = item->openBlockers();
        for (std::size_t i = 0; i < blockers.size(); ++i) {
            const LeafWorkItem* blocker =
                dynamic_cast<const LeafWorkItem*>(const_cast<WorkNode*>(top)->find(blockers[i]));
            if (blocker != nullptr) todo.push_back(blocker);
        }
    }
    return false;
}

std::string Tividy::nextTaskId(const WorkflowInstance& instance) {
    std::string id;
    do {
        std::ostringstream out;
        out << "T-" << nextTask_++;
        id = out.str();
    } while (instance.find(id) != nullptr);
    return id;
}

void Tividy::assignThroughPipeline(const std::string& instanceId, const std::string& itemId,
                                   const std::string& requested, bool mustSucceed) {
    LeafWorkItem& leaf = findLeaf(instanceId, itemId);
    std::string fallback;
    if (leaf.parentItem() != nullptr) fallback = leaf.parentItem()->assignee();
    Runtime& rt = runtime(instanceId);
    std::unique_ptr<AssignmentPipeline> pipeline;
    if (rt.structured) {
        pipeline.reset(new TeamFirstPipeline(organisation_, rt.assignment->strategy()));
    } else {
        pipeline.reset(new CompanyWidePipeline(organisation_, rt.assignment->strategy()));
    }
    AssignmentPipeline::Result chosen = pipeline->choose(leaf, leaf.role(), requested, fallback);
    if (chosen.person == nullptr) {
        if (mustSucceed) {
            std::string scope = leaf.unit();
            throw std::logic_error("Nobody " + (scope.empty() ? std::string("") : "in " + scope + " ") +
                                   "can take " + itemId + " right now");
        }
        record(instanceId, new SetDataCommand(nullptr, instanceId, itemId, "autoAssign", "pending"));
        return;
    }
    record(instanceId, new SetDataCommand(nullptr, instanceId, itemId, "autoAssign", "done"));
    record(instanceId, new SetDataCommand(nullptr, instanceId, itemId, "assignedVia", chosen.via));
    assign(instanceId, itemId, chosen.person->name());
}

std::string Tividy::addTask(const std::string& instanceId, const TaskSpec& spec) {
    WorkflowInstance& instance = findInstance(instanceId);
    Runtime& rt = runtime(instanceId);
    if (instance.state() != WorkflowInstance::RUNNING) {
        throw std::logic_error("Tasks can only be added to a running project");
    }
    if (spec.name.empty() || spec.name.size() > 120) {
        throw std::invalid_argument("A task needs a name of at most 120 characters");
    }
    if (!spec.unit.empty() && organisation_.findUnit(spec.unit) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + spec.unit);
    }
    if (!spec.priority.empty() && !LeafWorkItem::validPriority(spec.priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    if (!spec.assignee.empty()) findParticipant(spec.assignee);

    CompositeStage* stage = nullptr;
    if (spec.stageId.empty()) {
        stage = dynamic_cast<CompositeStage*>(instance.currentStage());
    } else {
        stage = dynamic_cast<CompositeStage*>(instance.find(spec.stageId));
    }
    if (stage == nullptr) {
        throw std::invalid_argument("Unknown stage: " + spec.stageId);
    }
    if (stage->isComplete() && stage->childCount() > 0 &&
        instance.topLevelIndex(stage) < instance.currentStageIndex()) {
        throw std::logic_error("Stage " + stage->name() + " is already finished");
    }

    std::size_t stageOrder = instance.topLevelIndex(stage);
    for (std::size_t i = 0; i < spec.blockedBy.size(); ++i) {
        LeafWorkItem& blocker = findLeaf(instanceId, spec.blockedBy[i]);
        if (instance.topLevelIndex(&blocker) > stageOrder) {
            throw std::invalid_argument("Blocker " + blocker.id() + " is in a later stage");
        }
    }

    Step step(*this, label("created task " + spec.name, instanceId));
    std::string id = nextTaskId(instance);
    std::unique_ptr<LeafWorkItem> leaf = rt.factory->createWorkItem(id, spec.name, spec.role);
    if (!spec.unit.empty()) leaf->setUnit(spec.unit);
    leaf->setApproverRoles(spec.approvers());
    if (!spec.priority.empty()) leaf->setPriority(spec.priority);
    for (std::size_t i = 0; i < spec.blockedBy.size(); ++i) {
        leaf->addBlocker(spec.blockedBy[i]);
    }
    rt.structured = true;
    record(instanceId, new AddWorkCommand(nullptr, instanceId, stage->id(), std::move(leaf)));
    publish(WorkflowEvent::ITEM_ADDED, instanceId, id, spec.name);

    if (spec.autoAssign || !spec.assignee.empty()) {
        if (findItem(instanceId, id).status() == WorkItem::AVAILABLE) {
            assignThroughPipeline(instanceId, id, spec.assignee, false);
        } else {
            record(instanceId, new SetDataCommand(nullptr, instanceId, id, "autoAssign", "pending"));
            if (!spec.assignee.empty()) {
                record(instanceId, new SetDataCommand(nullptr, instanceId, id, "requested", spec.assignee));
            }
        }
    }
    step.commit();
    return id;
}

std::string Tividy::addSubtask(const std::string& instanceId, const std::string& parentItemId,
                               const TaskSpec& spec) {
    WorkflowInstance& instance = findInstance(instanceId);
    Runtime& rt = runtime(instanceId);
    LeafWorkItem& parent = findLeaf(instanceId, parentItemId);
    if (instance.state() != WorkflowInstance::RUNNING) {
        throw std::logic_error("Sub-tasks can only be added to a running project");
    }
    if (parent.status() == WorkItem::COMPLETED || parent.status() == WorkItem::CANCELLED) {
        throw std::logic_error(parentItemId + " is already finished");
    }
    int depth = 0;
    for (LeafWorkItem* up = parent.parentItem(); up != nullptr; up = up->parentItem()) ++depth;
    if (depth >= 3) {
        throw std::logic_error("Sub-tasks can only be nested three levels deep");
    }
    if (spec.name.empty() || spec.name.size() > 120) {
        throw std::invalid_argument("A sub-task needs a name of at most 120 characters");
    }
    if (!spec.unit.empty() && organisation_.findUnit(spec.unit) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + spec.unit);
    }
    if (!spec.priority.empty() && !LeafWorkItem::validPriority(spec.priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    if (!spec.assignee.empty()) findParticipant(spec.assignee);
    for (std::size_t i = 0; i < spec.blockedBy.size(); ++i) {
        LeafWorkItem& blocker = findLeaf(instanceId, spec.blockedBy[i]);
        if (instance.topLevelIndex(&blocker) > instance.topLevelIndex(&parent)) {
            throw std::invalid_argument("Blocker " + blocker.id() + " is in a later stage");
        }
        if (dependsOn(blocker, parent)) {
            throw std::invalid_argument("The sub-task cannot wait for " + blocker.id() + ", which is waiting on " +
                                        parentItemId);
        }
    }

    Step step(*this, label("added sub-task " + spec.name + " to", instanceId, parentItemId));
    std::string id = nextTaskId(instance);
    std::string role = spec.role.empty() ? parent.role() : spec.role;
    std::unique_ptr<LeafWorkItem> leaf = rt.factory->createWorkItem(id, spec.name, role);
    std::string unit = spec.unit.empty() ? parent.unit() : spec.unit;
    if (!unit.empty()) leaf->setUnit(unit);
    leaf->setApproverRoles(spec.approvers());
    leaf->setPriority(spec.priority.empty() ? parent.priority() : spec.priority);
    leaf->setData("kind", "subtask");
    for (std::size_t i = 0; i < spec.blockedBy.size(); ++i) {
        leaf->addBlocker(spec.blockedBy[i]);
    }
    rt.structured = true;
    record(instanceId, new AddWorkCommand(nullptr, instanceId, parentItemId, std::move(leaf)));
    LeafWorkItem& sub = findLeaf(instanceId, id);
    publish(WorkflowEvent::SUBTASK_ADDED, instanceId, id, parentItemId);

    if (parent.status() == WorkItem::AWAITING_APPROVAL) {
        reject(instanceId, parentItemId, "Sub-task added: " + spec.name);
    }
    if (sub.status() == WorkItem::AVAILABLE) {
        assignThroughPipeline(instanceId, id, spec.assignee, false);
    } else {
        record(instanceId, new SetDataCommand(nullptr, instanceId, id, "autoAssign", "pending"));
        if (!spec.assignee.empty()) {
            record(instanceId, new SetDataCommand(nullptr, instanceId, id, "requested", spec.assignee));
        }
    }
    step.commit();
    return id;
}

void Tividy::assign(const std::string& instanceId, const std::string& itemId,
                    const std::string& participant) {
    // 1. Validate up front (so we throw invalid_argument, not logic_error).
    findItem(instanceId, itemId);
    findParticipant(participant);
    Step step(*this, label("assigned " + participant + " to", instanceId, itemId));

    // 2. Let the command perform the mutation (single source of truth).
    record(instanceId, new AssignCommand(nullptr, instanceId, itemId, participant));

    // 3. Side effects AFTER the successful mutation.
    recomputeLoads();
    publish(WorkflowEvent::ITEM_ASSIGNED, instanceId, itemId, participant);
    step.commit();
}

void Tividy::assignPending(const std::string& instanceId) {
    WorkflowInstance& instance = findInstance(instanceId);
    std::vector<std::string> ready;
    ReadyOnlyIterator it(&instance.root());
    for (it.first(); !it.isDone(); it.next()) {
        WorkItem* item = dynamic_cast<WorkItem*>(it.currentItem());
        if (item != nullptr && item->getData("autoAssign") == "pending") {
            ready.push_back(item->id());
        }
    }
    for (std::size_t i = 0; i < ready.size(); ++i) {
        std::string requested = findItem(instanceId, ready[i]).getData("requested");
        assignThroughPipeline(instanceId, ready[i], requested, false);
    }
}

void Tividy::autoAssign(const std::string& instanceId, const std::string& itemId) {
    findItem(instanceId, itemId);
    Step step(*this, label("auto-assigned", instanceId, itemId));
    assignThroughPipeline(instanceId, itemId, "", true);
    step.commit();
}

void Tividy::start(const std::string& instanceId, const std::string& itemId) {
    findItem(instanceId, itemId);
    Step step(*this, label("started", instanceId, itemId));
    // The command drives the transition; if it throws (illegal state),
    // the event never gets published, and undo history stays consistent.
    record(instanceId, new StartCommand(nullptr, instanceId, itemId));

    WorkItem& item = findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_STARTED, instanceId, itemId, item.assignee());
    step.commit();
}

void Tividy::complete(const std::string& instanceId, const std::string& itemId) {
    WorkItem& target = findItem(instanceId, itemId);
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(&target);
    if (leaf != nullptr && !leaf->openSubtasks().empty()) {
        throw std::logic_error("Finish the sub-tasks of " + target.name() + " before sending it for approval");
    }
    Step step(*this, label("sent for approval", instanceId, itemId));
    record(instanceId, new CompleteCommand(nullptr, instanceId, itemId));
    WorkItem& item = findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_COMPLETED, instanceId, itemId, item.assignee());
    if (runtime(instanceId).structured) {
        routeApproval(instanceId, itemId);
    }
    step.commit();
}

void Tividy::routeApproval(const std::string& instanceId, const std::string& itemId) {
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(&findItem(instanceId, itemId));
    if (leaf == nullptr) return;

    std::vector<std::string> wanted = leaf->approverRoles();
    if (wanted.empty()) wanted.push_back("");

    std::vector<std::string> pending;
    int automatic = 0;
    for (std::size_t i = 0; i < wanted.size(); ++i) {
        ApprovalRequest request(wanted[i]);
        runtime(instanceId).approvals->handle(request);
        const std::string role = request.getPendingRole();
        if (request.isApproved() && (role.empty() || role == "Auto")) {
            ++automatic;
        } else {
            pending.push_back(role);
        }
    }
    record(instanceId, new RouteApprovalCommand(nullptr, instanceId, itemId, pending, automatic));

    if (pending.empty()) {
        publish(WorkflowEvent::APPROVAL_ROUTED, instanceId, itemId, "Auto");
        finishApproval(instanceId, itemId);
        return;
    }
    if (automatic > 0) publish(WorkflowEvent::ITEM_SIGNED, instanceId, itemId, "System");
    for (std::size_t i = 0; i < pending.size(); ++i) {
        publish(WorkflowEvent::APPROVAL_ROUTED, instanceId, itemId, pending[i]);
    }
}

void Tividy::withdraw(const std::string& instanceId, const std::string& itemId) {
    findItem(instanceId, itemId);
    Step step(*this, label("took back", instanceId, itemId));
    record(instanceId, new WithdrawCommand(nullptr, instanceId, itemId));
    publish(WorkflowEvent::ITEM_WITHDRAWN, instanceId, itemId, findItem(instanceId, itemId).assignee());
    step.commit();
}

void Tividy::escalate(const std::string& instanceId, const std::string& itemId) {
    findItem(instanceId, itemId);
    Step step(*this, label("escalated", instanceId, itemId));
    record(instanceId, new EscalateCommand(nullptr, instanceId, itemId));
    recomputeLoads();
    publish(WorkflowEvent::ITEM_ESCALATED, instanceId, itemId, findItem(instanceId, itemId).assignee());
    step.commit();
}

void Tividy::setPriority(const std::string& instanceId, const std::string& itemId,
                         const std::string& priority) {
    findLeaf(instanceId, itemId);
    if (!LeafWorkItem::validPriority(priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    Step step(*this, label("set priority " + priority + " on", instanceId, itemId));
    record(instanceId, new SetPriorityCommand(nullptr, instanceId, itemId, priority));
    publish(WorkflowEvent::PRIORITY_CHANGED, instanceId, itemId, priority);
    step.commit();
}

void Tividy::approve(const std::string& instanceId, const std::string& itemId) {
    findItem(instanceId, itemId);
    if (!actor_.empty() && organisation_.find(actor_) != nullptr) {
        approveAs(instanceId, itemId, actor_);
        return;
    }
    forceApprove(instanceId, itemId);
}

void Tividy::forceApprove(const std::string& instanceId, const std::string& itemId) {
    Step step(*this, label("approved", instanceId, itemId));
    finishApproval(instanceId, itemId);
    step.commit();
}

void Tividy::finishApproval(const std::string& instanceId, const std::string& itemId) {
    WorkflowInstance& instance = findInstance(instanceId);

    record(instanceId, new ApproveCommand(nullptr, instanceId, itemId, true, ""));

    // Now the item really is approved; adjust loads and possibly complete the instance.
    WorkItem& item = findItem(instanceId, itemId);
    recomputeLoads();
    publish(WorkflowEvent::ITEM_APPROVED, instanceId, itemId, item.assignee());
    if (instance.state() == WorkflowInstance::COMPLETED) {
        publish(WorkflowEvent::INSTANCE_COMPLETED, instanceId);
    } else if (instance.state() == WorkflowInstance::AWAITING_SIGNOFF) {
        publish(WorkflowEvent::SIGNOFF_REQUESTED, instanceId);
    } else if (runtime(instanceId).structured) {
        assignPending(instanceId);
    }
}

void Tividy::approveAs(const std::string& instanceId, const std::string& itemId,
                       const std::string& person) {
    LeafWorkItem& leaf = findLeaf(instanceId, itemId);
    Participant& who = findParticipant(person);
    if (leaf.status() != WorkItem::AWAITING_APPROVAL) {
        throw std::logic_error(itemId + " is not waiting for approval");
    }
    if (leaf.assignee() == person) {
        throw std::logic_error("Nobody can approve their own work");
    }
    if (leaf.hasSigned(person)) {
        throw std::logic_error(person + " has already signed " + itemId);
    }

    if (leaf.pendingApprovers().empty()) {
        if (!organisation_.isManager(person)) {
            throw std::logic_error(itemId + " needs a manager to approve it");
        }
        forceApprove(instanceId, itemId);
        return;
    }

    std::string slot = who.role();
    bool found = false;
    for (std::size_t i = 0; i < leaf.pendingApprovers().size(); ++i) {
        if (leaf.pendingApprovers()[i] == slot) found = true;
    }
    if (!found && organisation_.isOwner(person)) {
        slot = leaf.pendingApprovers().front();
        found = true;
    }
    if (!found) {
        throw std::logic_error(itemId + " is not waiting for a " + who.role());
    }

    Step step(*this, label("signed off", instanceId, itemId));
    record(instanceId, new SignCommand(nullptr, instanceId, itemId, slot, person));
    publish(WorkflowEvent::ITEM_SIGNED, instanceId, itemId, person);
    if (leaf.pendingApprovers().empty()) {
        finishApproval(instanceId, itemId);
    }
    step.commit();
}

void Tividy::reject(const std::string& instanceId, const std::string& itemId,
                    const std::string& reason) {
    findItem(instanceId, itemId);
    Step step(*this, label("requested changes on", instanceId, itemId));
    record(instanceId, new RejectCommand(nullptr, instanceId, itemId, reason));

    recomputeLoads();
    publish(WorkflowEvent::ITEM_REJECTED, instanceId, itemId, reason);
    step.commit();
}

void Tividy::cancel(const std::string& instanceId, const std::string& reason) {
    findInstance(instanceId);
    Step step(*this, label("cancelled project", instanceId));

    // 2. The command performs the actual cancel (single source of mutation).
    record(instanceId, new CancelCommand(nullptr, instanceId, instanceId, reason));

    // 3. Release loads AFTER the successful cancellation.
    recomputeLoads();
    publish(WorkflowEvent::INSTANCE_CANCELLED, instanceId, "", reason);
    step.commit();
}

void Tividy::signOff(const std::string& instanceId) {
    findInstance(instanceId);
    Step step(*this, label("signed off project", instanceId));
    record(instanceId, new SignOffCommand(nullptr, instanceId, SignOffCommand::SIGN_OFF));
    publish(WorkflowEvent::INSTANCE_COMPLETED, instanceId);
    step.commit();
}

std::vector<std::string> Tividy::flagIssue(const std::string& instanceId, const IssueSpec& spec) {
    WorkflowInstance& instance = findInstance(instanceId);
    if (spec.name.empty() || spec.name.size() > 120) {
        throw std::invalid_argument("An issue needs a title of at most 120 characters");
    }
    if (spec.units.empty() || spec.units.size() > 10) {
        throw std::invalid_argument("Pick between one and ten teams for the issue");
    }
    for (std::size_t i = 0; i < spec.units.size(); ++i) {
        if (organisation_.findUnit(spec.units[i]) == nullptr) {
            throw std::invalid_argument("Unknown unit: " + spec.units[i]);
        }
    }
    if (!spec.priority.empty() && !LeafWorkItem::validPriority(spec.priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }

    Step step(*this, label("flagged " + spec.name, instanceId));
    Runtime& rt = runtime(instanceId);
    record(instanceId, new SignOffCommand(nullptr, instanceId, SignOffCommand::REOPEN_FOR_FIXES, rt.factory.get()));
    CompositeStage& fixes = dynamic_cast<CompositeStage&>(*instance.currentStage());
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < spec.units.size(); ++i) {
        std::string id = nextTaskId(instance);
        std::string name = spec.name;
        if (spec.units.size() > 1) name += " (" + organisation_.findUnit(spec.units[i])->name() + ")";
        std::unique_ptr<LeafWorkItem> leaf =
            rt.factory->createWorkItem(id, name, Company::MANAGER_ROLE);
        leaf->setUnit(spec.units[i]);
        leaf->setApproverRole(Company::OWNER_ROLE);
        leaf->setPriority(spec.priority.empty() ? std::string("High") : spec.priority);
        leaf->setData("kind", "issue");
        record(instanceId, new AddWorkCommand(nullptr, instanceId, fixes.id(), std::move(leaf)));
        ids.push_back(id);
        publish(WorkflowEvent::ISSUE_FLAGGED, instanceId, id, name);
        assignThroughPipeline(instanceId, id, "", false);
    }
    step.commit();
    return ids;
}

void Tividy::undo() { undoFor(actor_, true); }

void Tividy::redo() { redoFor(actor_, true); }

void Tividy::undoFor(const std::string& actor, bool anyone) {
    if (history_ == nullptr || !history_->canUndo()) {
        return;
    }
    UndoHistory::Entry entry = history_->undo(*this, actor, anyone);
    recomputeLoads();
    WorkflowEvent event(WorkflowEvent::UNDONE, "", "", entry.label);
    event.actor = actor;
    bus_.notify(event);
}

void Tividy::redoFor(const std::string& actor, bool anyone) {
    if (history_ == nullptr || !history_->canRedo()) {
        return;
    }
    UndoHistory::Entry entry = history_->redo(*this, actor, anyone);
    recomputeLoads();
    WorkflowEvent event(WorkflowEvent::REDONE, "", "", entry.label);
    event.actor = actor;
    bus_.notify(event);
}

bool Tividy::canUndo(const std::string& actor, bool anyone) const {
    return history_ != nullptr && history_->canUndo(actor, anyone);
}

bool Tividy::canRedo(const std::string& actor, bool anyone) const {
    return history_ != nullptr && history_->canRedo(actor, anyone);
}

std::vector<std::string> Tividy::undoLabels(std::size_t max) const {
    std::vector<std::string> labels;
    if (history_ == nullptr) return labels;
    std::vector<UndoHistory::Entry> entries = history_->undoEntries(max);
    for (std::size_t i = 0; i < entries.size(); ++i) labels.push_back(entries[i].label);
    return labels;
}

std::vector<std::string> Tividy::redoLabels(std::size_t max) const {
    std::vector<std::string> labels;
    if (history_ == nullptr) return labels;
    std::vector<UndoHistory::Entry> entries = history_->redoEntries(max);
    for (std::size_t i = 0; i < entries.size(); ++i) labels.push_back(entries[i].label);
    return labels;
}

void Tividy::clearHistory() {
    if (history_ != nullptr) history_->clear();
}

void Tividy::addUnit(const std::string& parentId, const std::string& id, const std::string& name) {
    if (id.size() > 40 || name.size() > 60) {
        throw std::invalid_argument("Unit id or name is too long");
    }
    Step step(*this, label("added team " + name, ""));
    organisation_.addUnit(parentId, id, name);
    publish(WorkflowEvent::ORG_CHANGED, "", "", "unit " + id);
    step.commit();
}

void Tividy::addPerson(const std::string& name, const std::string& role, const std::string& unitId) {
    if (name.empty() || name.size() > 40 || role.empty() || role.size() > 40) {
        throw std::invalid_argument("A person needs a name and a role of at most 40 characters");
    }
    if (organisation_.findUnit(unitId) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + unitId);
    }
    if (organisation_.structure().find(name) != nullptr || organisation_.find(name) != nullptr) {
        throw std::invalid_argument("Id already in use: " + name);
    }
    Step step(*this, label("added " + name + " as " + role, ""));
    organisation_.addParticipant(name, role);
    organisation_.placeMember(unitId, name);
    publish(WorkflowEvent::ORG_CHANGED, "", "", "person " + name);
    step.commit();
}

void Tividy::moveMember(const std::string& unitId, const std::string& name) {
    findParticipant(name);
    if (organisation_.findUnit(unitId) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + unitId);
    }
    Step step(*this, label("moved " + name + " to " + organisation_.findUnit(unitId)->name(), ""));
    organisation_.placeMember(unitId, name);
    publish(WorkflowEvent::ORG_CHANGED, "", "", "move " + name);
    step.commit();
}

const WorkflowInstance& Tividy::instance(const std::string& instanceId) const {
    return findInstance(instanceId);
}

std::vector<std::string> Tividy::instanceIds() const {
    return instanceOrder_;
}

std::string Tividy::policyOf(const std::string& instanceId) const {
    return runtime(instanceId).factory->name();
}

std::vector<ItemRef> Tividy::workFor(const std::string& participant) const {
    std::vector<ItemRef> refs;
    for (std::size_t i = 0; i < instanceOrder_.size(); ++i) {
        AssignedToIterator it(&findInstance(instanceOrder_[i]).root(), participant);
        for (it.first(); !it.isDone(); it.next()) {
            refs.push_back(ItemRef(instanceOrder_[i], it.currentItem()->id()));
        }
    }
    return refs;
}

std::vector<ItemRef> Tividy::approvalsFor(const std::string& participant) const {
    std::vector<ItemRef> refs;
    if (organisation_.find(participant) == nullptr) return refs;
    for (std::size_t i = 0; i < instanceOrder_.size(); ++i) {
        AwaitingApprovalIterator it = AwaitingApprovalIterator::anyApprover(
            &findInstance(instanceOrder_[i]).root(), participant);
        for (it.first(); !it.isDone(); it.next()) {
            if (canApprove(participant, instanceOrder_[i], it.currentItem()->id())) {
                refs.push_back(ItemRef(instanceOrder_[i], it.currentItem()->id()));
            }
        }
    }
    return refs;
}

std::vector<std::string> Tividy::signOffsFor(const std::string& participant) const {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < instanceOrder_.size(); ++i) {
        if (canSignOff(participant, instanceOrder_[i])) ids.push_back(instanceOrder_[i]);
    }
    return ids;
}

bool Tividy::canApprove(const std::string& participant, const std::string& instanceId,
                        const std::string& itemId) const {
    Participant* person = organisation_.find(participant);
    if (person == nullptr) return false;
    LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(&findItem(instanceId, itemId));
    if (leaf == nullptr || leaf->status() != WorkItem::AWAITING_APPROVAL) return false;
    if (leaf->assignee() == participant || leaf->hasSigned(participant)) return false;
    const std::vector<std::string>& pending = leaf->pendingApprovers();
    if (pending.empty()) return organisation_.isManager(participant);
    if (organisation_.isOwner(participant)) return true;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        if (pending[i] == person->role()) return true;
    }
    return false;
}

bool Tividy::canSignOff(const std::string& participant, const std::string& instanceId) const {
    const WorkflowInstance& instance = findInstance(instanceId);
    if (instance.state() != WorkflowInstance::AWAITING_SIGNOFF) return false;
    if (organisation_.hasOwner()) return organisation_.isOwner(participant);
    return organisation_.isManager(participant);
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
