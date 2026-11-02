#include "mediator/TividyMediator.h"
#include "command/RequestCommand.h"
#include "decorator/ApprovalWork.h"
#include "decorator/BlockedWork.h"
#include <set>
#include <sstream>
#include <stdexcept>
#include "builder/ConcreteDefinitionBuilder.h"
#include "abstract_factory/PolicyCatalog.h"
#include "builder/WorkflowAssembler.h"
#include "chain_of_responsibility/OwnerHandler.h"
#include "chain_of_responsibility/ManagerFallbackHandler.h"
#include "chain_of_responsibility/PolicyChainHandler.h"
#include "iterator/DepthFirstIterator.h"
#include "iterator/FilterIterator.h"
#include "chain_of_responsibility/ApprovalRequest.h"
#include "strategy/AssignmentPipeline.h"
#include "composite/LeafTask.h"

TividyMediator::ActorScope::ActorScope(TividyMediator& engine, const std::string& actor)
    : engine_(engine), previous_(engine.actor_) {
    engine_.actor_ = actor;
}

TividyMediator::ActorScope::~ActorScope() { engine_.actor_ = previous_; }

TividyMediator::Step::Step(TividyMediator& engine, const std::string& label)
    : engine_(engine), outer_(engine.depth_ == 0), committed_(false), label_(label) {
    if (outer_) before_ = engine_.snapshot();
    ++engine_.depth_;
}

TividyMediator::Step::~Step() {
    --engine_.depth_;
    if (!outer_) return;
    std::vector<WorkflowEvent> events;
    events.swap(engine_.pendingEvents_);
    if (committed_) {
        try {
            WorkflowEvent event(WorkflowEvent::STEP_COMMITTED, "", "", label_);
            event.actor = engine_.actor_;
            event.before = std::shared_ptr<const EngineMemento>(new EngineMemento(label_, before_));
            engine_.historyFacade_.historyBus().notify(event);
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

void TividyMediator::Step::commit() { committed_ = true; }

TividyMediator::TividyMediator(const std::string& organisationName)
    : orgFacade_(organisationName), invoker_(historyFacade_.commands()), depth_(0) {
    projectFacade_.setMediator(this);
    orgFacade_.setMediator(this);
    historyFacade_.setMediator(this);
    useHistory(historyFacade_.ownHistory());
    invoker_.setLimit(200);
}

std::shared_ptr<const EngineMemento> TividyMediator::createMemento(const std::string& label) const {
    return std::shared_ptr<const EngineMemento>(new EngineMemento(label, snapshot()));
}

void TividyMediator::restoreMemento(const EngineMemento& memento) {
    restore(memento.state_);
}

UndoHistory* TividyMediator::history() const { return historyFacade_.history(); }

void TividyMediator::useHistory(UndoHistory* history) { return historyFacade_.useHistory(history); }

Company& TividyMediator::organisation() { return orgFacade_.company(); }

const Company& TividyMediator::organisation() const { return orgFacade_.company(); }

EventBus& TividyMediator::bus() { return bus_; }

EventBus& TividyMediator::historyBus() { return historyFacade_.historyBus(); }









std::unique_ptr<TividyMediator::Runtime> TividyMediator::makeRuntime(const std::string& policy, bool structured,
                                                     const std::string& ownerRole) {
    std::unique_ptr<Runtime> rt(new Runtime());
    rt->factory = PolicyCatalog::create(policy);
    rt->policy = rt->factory->name();
    rt->approvals.reset(new OwnerHandler(ownerRole));
    PolicyChainHandler* policyLink = new PolicyChainHandler(rt->factory->createApprovalChain());
    rt->approvals->setNext(policyLink);
    policyLink->setNext(new ManagerFallbackHandler());
    rt->structured = structured;
    return rt;
}

void TividyMediator::recomputeLoads() {
    std::map<std::string, int> counts;
    std::vector<std::string> ids = projectFacade_.instanceIds();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        DepthFirstIterator it(&projectFacade_.findInstance(ids[i]).root());
        for (it.first(); !it.isDone(); it.next()) {
            LeafTask* item = leafOf(it.currentItem());
            if (item == nullptr || item->assignee().empty()) continue;
            if (item->holdsLoad()) {
                ++counts[item->assignee()];
            }
        }
    }
    std::vector<Participant*> people = orgFacade_.company().participants();
    for (std::size_t i = 0; i < people.size(); ++i) {
        people[i]->addLoad(-people[i]->load());
        people[i]->addLoad(counts[people[i]->name()]);
    }
}



Response TividyMediator::record(const Request& request) {
    return invoker_.invoke(std::unique_ptr<Command>(new RequestCommand(&projectFacade_, request)));
}

void TividyMediator::notify(Colleague* sender, const Response& change) {
    if (sender == &projectFacade_ && !change.data("loads").empty()) recomputeLoads();
}

void TividyMediator::publish(const std::string& type, const std::string& instanceId,
                     const std::string& itemId, const std::string& detail) const {
    WorkflowEvent event(type, instanceId, itemId, detail);
    event.actor = actor_;
    if (depth_ > 0) {
        pendingEvents_.push_back(event);
    } else {
        bus_.notify(event);
    }
}

std::string TividyMediator::label(const std::string& verb, const std::string& instanceId,
                          const std::string& itemId) const {
    std::string who = actor_.empty() ? std::string("System") : actor_;
    std::string what;
    std::string where = instanceId;
    try {
        if (!instanceId.empty()) {
            const WorkflowInstance& instance = projectFacade_.findInstance(instanceId);
            where = instance.name();
            if (!itemId.empty()) {
                LeafTask* item = instance.item(itemId);
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

void TividyMediator::loadDefinition(const std::string& name) { return projectFacade_.loadDefinition(name); }

void TividyMediator::loadDefinition(const std::string& name, const WorkflowDefinition& definition) { return projectFacade_.loadDefinition(name, definition); }


bool TividyMediator::hasDefinition(const std::string& name) const { return projectFacade_.hasDefinition(name); }

std::vector<std::string> TividyMediator::definitionNames() const { return projectFacade_.definitionNames(); }

const WorkflowDefinition& TividyMediator::definition(const std::string& name) const { return projectFacade_.definition(name); }

std::string TividyMediator::launch(const WorkflowDefinition& definition, const std::string& title,
                           const std::string& policy, SignOffRule signOff) {
    std::ostringstream id;
    id << projectFacade_.nextInstanceId();

    std::unique_ptr<Runtime> rt = makeRuntime(policy.empty() ? definition.policy : policy,
                                              definition.structured(),
                                              orgFacade_.ownerRole());

    std::unique_ptr<WorkflowInstance> instance(
        rt->structured ? new WorkflowInstance(id.str(), definition, *rt->factory, title)
                       : new WorkflowInstance(id.str(), definition));
    instance->setRequiresSignOff(signOff == SignOffRule::Required ||
                                 (signOff == SignOffRule::FromTemplate && definition.signOff));
    instance->start();
    bool finished = instance->state() == WorkflowInstance::COMPLETED;
    projectFacade_.addInstance(std::move(instance), std::move(rt));

    publish(WorkflowEvent::INSTANCE_STARTED, id.str(), "", definition.name);
    if (finished) {
        publish(WorkflowEvent::INSTANCE_COMPLETED, id.str());
    }
    return id.str();
}

std::string TividyMediator::startInstance(const std::string& definition) {
    const WorkflowDefinition& chosen = projectFacade_.definition(definition);
    Step step(*this, label("started " + definition, ""));
    std::string id = launch(chosen, "", "", SignOffRule::FromTemplate);
    step.commit();
    return id;
}

std::string TividyMediator::startProject(const std::string& definition, const std::string& title,
                                 const std::string& policy, SignOffRule signOff) {
    const WorkflowDefinition& chosen = projectFacade_.definition(definition);
    if (!policy.empty() && !PolicyCatalog::exists(policy)) {
        throw std::invalid_argument("Unknown policy: " + policy);
    }
    if (title.size() > 120) {
        throw std::invalid_argument("Project title is too long");
    }
    Step step(*this, label("started project " + (title.empty() ? definition : title), ""));
    std::string id = launch(chosen, title, policy, signOff);
    step.commit();
    return id;
}





void TividyMediator::assignThroughPipeline(const std::string& instanceId, const std::string& itemId,
                                   const std::string& requested, bool mustSucceed) {
    LeafTask& leaf = projectFacade_.findLeaf(instanceId, itemId);
    std::string fallback;
    if (leaf.parentItem() != nullptr) fallback = leaf.parentItem()->assignee();
    Runtime& rt = projectFacade_.runtime(instanceId);
    AssignmentPipeline::Result chosen =
        orgFacade_.choose(rt.policy, rt.structured, leaf.data(), leaf.role(), requested, fallback);
    if (chosen.person == nullptr) {
        if (mustSucceed) {
            std::string scope = leaf.unit();
            throw std::logic_error("Nobody " + (scope.empty() ? std::string("") : "in " + scope + " ") +
                                   "can take " + itemId + " right now");
        }
        record(Request(Action::SetData, TaskRef(instanceId, itemId)).with("key", "autoAssign").with("value", "pending"));
        return;
    }
    record(Request(Action::SetData, TaskRef(instanceId, itemId)).with("key", "autoAssign").with("value", "done"));
    record(Request(Action::SetData, TaskRef(instanceId, itemId)).with("key", "assignedVia").with("value", chosen.via));
    assign(instanceId, itemId, chosen.person->name());
}

std::string TividyMediator::addTask(const std::string& instanceId, const TaskSpec& spec) {
    WorkflowInstance& instance = projectFacade_.findInstance(instanceId);
    Runtime& rt = projectFacade_.runtime(instanceId);
    if (instance.state() != WorkflowInstance::RUNNING) {
        throw std::logic_error("Tasks can only be added to a running project");
    }
    if (spec.name.empty() || spec.name.size() > 120) {
        throw std::invalid_argument("A task needs a name of at most 120 characters");
    }
    if (!spec.unit.empty() && orgFacade_.company().findUnit(spec.unit) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + spec.unit);
    }
    if (!spec.priority.empty() && !LeafTask::validPriority(spec.priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    if (!spec.assignee.empty()) orgFacade_.findParticipant(spec.assignee);

    CompositeTask* stage = nullptr;
    if (spec.stageId.empty()) {
        stage = stageOf(instance.currentStage());
    } else {
        stage = stageOf(instance.find(spec.stageId));
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
        LeafTask& blocker = projectFacade_.findLeaf(instanceId, spec.blockedBy[i]);
        if (instance.topLevelIndex(&blocker) > stageOrder) {
            throw std::invalid_argument("Blocker " + blocker.id() + " is in a later stage");
        }
    }

    Step step(*this, label("created task " + spec.name, instanceId));
    rt.structured = true;
    std::string id = record(Request(Action::AddTask, TaskRef(instanceId))
                                .with("parent", stage->id()).with("name", spec.name).with("role", spec.role)
                                .with("unit", spec.unit).withList("approvers", spec.approvers())
                                .with("priority", spec.priority).withList("blockedBy", spec.blockedBy))
                         .data("item");
    publish(WorkflowEvent::ITEM_ADDED, instanceId, id, spec.name);

    if (spec.autoAssign || !spec.assignee.empty()) {
        if (projectFacade_.findItem(instanceId, id).status() == LeafTask::AVAILABLE) {
            assignThroughPipeline(instanceId, id, spec.assignee, false);
        } else {
            record(Request(Action::SetData, TaskRef(instanceId, id)).with("key", "autoAssign").with("value", "pending"));
            if (!spec.assignee.empty()) {
                record(Request(Action::SetData, TaskRef(instanceId, id)).with("key", "requested").with("value", spec.assignee));
            }
        }
    }
    step.commit();
    return id;
}

std::string TividyMediator::addSubtask(const std::string& instanceId, const std::string& parentItemId,
                               const TaskSpec& spec) {
    WorkflowInstance& instance = projectFacade_.findInstance(instanceId);
    Runtime& rt = projectFacade_.runtime(instanceId);
    LeafTask& parent = projectFacade_.findLeaf(instanceId, parentItemId);
    if (instance.state() != WorkflowInstance::RUNNING) {
        throw std::logic_error("Sub-tasks can only be added to a running project");
    }
    if (parent.status() == LeafTask::COMPLETED || parent.status() == LeafTask::CANCELLED) {
        throw std::logic_error(parentItemId + " is already finished");
    }
    int depth = 0;
    for (LeafTask* up = parent.parentItem(); up != nullptr; up = up->parentItem()) ++depth;
    if (depth >= 3) {
        throw std::logic_error("Sub-tasks can only be nested three levels deep");
    }
    if (spec.name.empty() || spec.name.size() > 120) {
        throw std::invalid_argument("A sub-task needs a name of at most 120 characters");
    }
    if (!spec.unit.empty() && orgFacade_.company().findUnit(spec.unit) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + spec.unit);
    }
    if (!spec.priority.empty() && !LeafTask::validPriority(spec.priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    if (!spec.assignee.empty()) orgFacade_.findParticipant(spec.assignee);
    for (std::size_t i = 0; i < spec.blockedBy.size(); ++i) {
        LeafTask& blocker = projectFacade_.findLeaf(instanceId, spec.blockedBy[i]);
        if (instance.topLevelIndex(&blocker) > instance.topLevelIndex(&parent)) {
            throw std::invalid_argument("Blocker " + blocker.id() + " is in a later stage");
        }
        if (projectFacade_.dependsOn(blocker, parent)) {
            throw std::invalid_argument("The sub-task cannot wait for " + blocker.id() + ", which is waiting on " +
                                        parentItemId);
        }
    }

    Step step(*this, label("added sub-task " + spec.name + " to", instanceId, parentItemId));
    rt.structured = true;
    std::string id = record(Request(Action::AddSubtask, TaskRef(instanceId))
                                .with("parent", parentItemId).with("name", spec.name)
                                .with("role", spec.role.empty() ? parent.role() : spec.role)
                                .with("unit", spec.unit.empty() ? parent.unit() : spec.unit)
                                .withList("approvers", spec.approvers())
                                .with("priority", spec.priority.empty() ? parent.priority() : spec.priority)
                                .with("kind", "subtask").withList("blockedBy", spec.blockedBy))
                         .data("item");
    LeafTask& sub = projectFacade_.findLeaf(instanceId, id);
    publish(WorkflowEvent::SUBTASK_ADDED, instanceId, id, parentItemId);

    if (parent.status() == LeafTask::AWAITING_APPROVAL) {
        reject(instanceId, parentItemId, "Sub-task added: " + spec.name);
    }
    if (sub.status() == LeafTask::AVAILABLE) {
        assignThroughPipeline(instanceId, id, spec.assignee, false);
    } else {
        record(Request(Action::SetData, TaskRef(instanceId, id)).with("key", "autoAssign").with("value", "pending"));
        if (!spec.assignee.empty()) {
            record(Request(Action::SetData, TaskRef(instanceId, id)).with("key", "requested").with("value", spec.assignee));
        }
    }
    step.commit();
    return id;
}

void TividyMediator::assign(const std::string& instanceId, const std::string& itemId,
                    const std::string& participant) {
    // 1. Validate up front (so we throw invalid_argument, not logic_error).
    projectFacade_.findItem(instanceId, itemId);
    orgFacade_.findParticipant(participant);
    Step step(*this, label("assigned " + participant + " to", instanceId, itemId));

    // 2. Let the command perform the mutation (single source of truth).
    record(Request(Action::Assign, TaskRef(instanceId, itemId)).with("participant", participant));

    // 3. Side effects AFTER the successful mutation (ProjectFacade's report already updated the loads).
    publish(WorkflowEvent::ITEM_ASSIGNED, instanceId, itemId, participant);
    step.commit();
}

void TividyMediator::assignPending(const std::string& instanceId) {
    WorkflowInstance& instance = projectFacade_.findInstance(instanceId);
    std::vector<std::string> ready;
    FilterIterator it(&instance.root());
    for (it.first(); !it.isDone(); it.next()) {
        LeafTask* item = leafOf(it.currentItem());
        if (item != nullptr && item->getData("autoAssign") == "pending") {
            ready.push_back(item->id());
        }
    }
    for (std::size_t i = 0; i < ready.size(); ++i) {
        std::string requested = projectFacade_.findItem(instanceId, ready[i]).getData("requested");
        assignThroughPipeline(instanceId, ready[i], requested, false);
    }
}

void TividyMediator::autoAssign(const std::string& instanceId, const std::string& itemId) {
    projectFacade_.findItem(instanceId, itemId);
    Step step(*this, label("auto-assigned", instanceId, itemId));
    assignThroughPipeline(instanceId, itemId, "", true);
    step.commit();
}

void TividyMediator::start(const std::string& instanceId, const std::string& itemId) {
    projectFacade_.findItem(instanceId, itemId);
    Step step(*this, label("started", instanceId, itemId));
    // The command drives the transition; if it throws (illegal state),
    // the event never gets published, and undo history stays consistent.
    record(Request(Action::Start, TaskRef(instanceId, itemId)));

    LeafTask& item = projectFacade_.findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_STARTED, instanceId, itemId, item.assignee());
    step.commit();
}

void TividyMediator::complete(const std::string& instanceId, const std::string& itemId) {
    LeafTask& target = projectFacade_.findItem(instanceId, itemId);
    LeafTask* leaf = leafOf(&target);
    if (leaf != nullptr && !leaf->openSubtasks().empty()) {
        throw std::logic_error("Finish the sub-tasks of " + target.name() + " before sending it for approval");
    }
    Step step(*this, label("sent for approval", instanceId, itemId));
    record(Request(Action::Complete, TaskRef(instanceId, itemId)));
    LeafTask& item = projectFacade_.findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_COMPLETED, instanceId, itemId, item.assignee());
    if (projectFacade_.runtime(instanceId).structured) {
        routeApproval(instanceId, itemId);
    }
    step.commit();
}

void TividyMediator::routeApproval(const std::string& instanceId, const std::string& itemId) {
    LeafTask* leaf = leafOf(&projectFacade_.findItem(instanceId, itemId));
    if (leaf == nullptr) return;

    std::vector<std::string> wanted = leaf->approverRoles();
    if (wanted.empty()) wanted.push_back("");

    std::vector<std::string> pending;
    int automatic = 0;
    for (std::size_t i = 0; i < wanted.size(); ++i) {
        ApprovalRequest request(wanted[i]);
        projectFacade_.runtime(instanceId).approvals->handle(request);
        const std::string role = request.getPendingRole();
        if (request.isApproved() && (role.empty() || role == "Auto")) {
            ++automatic;
        } else {
            pending.push_back(role);
        }
    }
    record(Request(Action::RouteApproval, TaskRef(instanceId, itemId)).withList("pending", pending).with("automatic", std::to_string(automatic)));

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

void TividyMediator::withdraw(const std::string& instanceId, const std::string& itemId) {
    projectFacade_.findItem(instanceId, itemId);
    Step step(*this, label("took back", instanceId, itemId));
    record(Request(Action::Withdraw, TaskRef(instanceId, itemId)));
    publish(WorkflowEvent::ITEM_WITHDRAWN, instanceId, itemId, projectFacade_.findItem(instanceId, itemId).assignee());
    step.commit();
}

void TividyMediator::escalate(const std::string& instanceId, const std::string& itemId) {
    projectFacade_.findItem(instanceId, itemId);
    Step step(*this, label("escalated", instanceId, itemId));
    record(Request(Action::Escalate, TaskRef(instanceId, itemId)));
    publish(WorkflowEvent::ITEM_ESCALATED, instanceId, itemId, projectFacade_.findItem(instanceId, itemId).assignee());
    step.commit();
}

void TividyMediator::setPriority(const std::string& instanceId, const std::string& itemId,
                         const std::string& priority) {
    projectFacade_.findLeaf(instanceId, itemId);
    if (!LeafTask::validPriority(priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    Step step(*this, label("set priority " + priority + " on", instanceId, itemId));
    record(Request(Action::SetPriority, TaskRef(instanceId, itemId)).with("priority", priority));
    publish(WorkflowEvent::PRIORITY_CHANGED, instanceId, itemId, priority);
    step.commit();
}

void TividyMediator::approve(const std::string& instanceId, const std::string& itemId) {
    projectFacade_.findItem(instanceId, itemId);
    if (!actor_.empty() && orgFacade_.company().find(actor_) != nullptr) {
        approveAs(instanceId, itemId, actor_);
        return;
    }
    forceApprove(instanceId, itemId);
}

void TividyMediator::forceApprove(const std::string& instanceId, const std::string& itemId) {
    Step step(*this, label("approved", instanceId, itemId));
    finishApproval(instanceId, itemId);
    step.commit();
}

void TividyMediator::finishApproval(const std::string& instanceId, const std::string& itemId) {
    WorkflowInstance& instance = projectFacade_.findInstance(instanceId);

    record(Request(Action::Approve, TaskRef(instanceId, itemId)));

    // Now the item really is approved (ProjectFacade's report updated the loads); the instance may be complete.
    LeafTask& item = projectFacade_.findItem(instanceId, itemId);
    publish(WorkflowEvent::ITEM_APPROVED, instanceId, itemId, item.assignee());
    if (instance.state() == WorkflowInstance::COMPLETED) {
        publish(WorkflowEvent::INSTANCE_COMPLETED, instanceId);
    } else if (instance.state() == WorkflowInstance::AWAITING_SIGNOFF) {
        publish(WorkflowEvent::SIGNOFF_REQUESTED, instanceId);
    } else if (projectFacade_.runtime(instanceId).structured) {
        assignPending(instanceId);
    }
}

void TividyMediator::approveAs(const std::string& instanceId, const std::string& itemId,
                       const std::string& person) {
    LeafTask& leaf = projectFacade_.findLeaf(instanceId, itemId);
    Participant& who = orgFacade_.findParticipant(person);
    if (leaf.status() != LeafTask::AWAITING_APPROVAL) {
        throw std::logic_error(itemId + " is not waiting for approval");
    }
    if (leaf.assignee() == person) {
        throw std::logic_error("Nobody can approve their own work");
    }
    if (leaf.hasSigned(person)) {
        throw std::logic_error(person + " has already signed " + itemId);
    }

    if (leaf.pendingApprovers().empty()) {
        if (!orgFacade_.company().isManager(person)) {
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
    if (!found && orgFacade_.company().isOwner(person)) {
        slot = leaf.pendingApprovers().front();
        found = true;
    }
    if (!found) {
        throw std::logic_error(itemId + " is not waiting for a " + who.role());
    }

    Step step(*this, label("signed off", instanceId, itemId));
    record(Request(Action::Sign, TaskRef(instanceId, itemId)).with("slot", slot).with("person", person));
    publish(WorkflowEvent::ITEM_SIGNED, instanceId, itemId, person);
    if (leaf.pendingApprovers().empty()) {
        finishApproval(instanceId, itemId);
    }
    step.commit();
}

void TividyMediator::reject(const std::string& instanceId, const std::string& itemId,
                    const std::string& reason) {
    projectFacade_.findItem(instanceId, itemId);
    Step step(*this, label("requested changes on", instanceId, itemId));
    record(Request(Action::Reject, TaskRef(instanceId, itemId)).with("reason", reason));

    publish(WorkflowEvent::ITEM_REJECTED, instanceId, itemId, reason);
    step.commit();
}

void TividyMediator::cancel(const std::string& instanceId, const std::string& reason) {
    projectFacade_.findInstance(instanceId);
    Step step(*this, label("cancelled project", instanceId));

    // 2. The command performs the actual cancel (single source of mutation).
    record(Request(Action::CancelProject, TaskRef(instanceId)).with("reason", reason));

    // 3. ProjectFacade's report released the loads.
    publish(WorkflowEvent::INSTANCE_CANCELLED, instanceId, "", reason);
    step.commit();
}

void TividyMediator::signOff(const std::string& instanceId) {
    projectFacade_.findInstance(instanceId);
    Step step(*this, label("signed off project", instanceId));
    record(Request(Action::SignOff, TaskRef(instanceId)));
    publish(WorkflowEvent::INSTANCE_COMPLETED, instanceId);
    step.commit();
}

std::vector<std::string> TividyMediator::flagIssue(const std::string& instanceId, const IssueSpec& spec) {
    WorkflowInstance& instance = projectFacade_.findInstance(instanceId);
    if (spec.name.empty() || spec.name.size() > 120) {
        throw std::invalid_argument("An issue needs a title of at most 120 characters");
    }
    if (spec.units.empty() || spec.units.size() > 10) {
        throw std::invalid_argument("Pick between one and ten teams for the issue");
    }
    for (std::size_t i = 0; i < spec.units.size(); ++i) {
        if (orgFacade_.company().findUnit(spec.units[i]) == nullptr) {
            throw std::invalid_argument("Unknown unit: " + spec.units[i]);
        }
    }
    if (!spec.priority.empty() && !LeafTask::validPriority(spec.priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }

    Step step(*this, label("flagged " + spec.name, instanceId));
    record(Request(Action::ReopenForFixes, TaskRef(instanceId)));
    // The fixes stage is empty until the issue tasks go in, so the project is not Running yet: find it by id.
    CompositeTask& fixes = dynamic_cast<CompositeTask&>(*instance.find(instanceId + "-fixes"));
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < spec.units.size(); ++i) {
        std::string name = spec.name;
        if (spec.units.size() > 1) name += " (" + orgFacade_.company().findUnit(spec.units[i])->name() + ")";
        std::string id = record(Request(Action::AddTask, TaskRef(instanceId))
                                    .with("parent", fixes.id()).with("name", name).with("role", Company::MANAGER_ROLE)
                                    .with("unit", spec.units[i])
                                    .withList("approvers", std::vector<std::string>(1, Company::OWNER_ROLE))
                                    .with("priority", spec.priority.empty() ? std::string("High") : spec.priority)
                                    .with("kind", "issue"))
                             .data("item");
        ids.push_back(id);
        publish(WorkflowEvent::ISSUE_FLAGGED, instanceId, id, name);
        assignThroughPipeline(instanceId, id, "", false);
    }
    step.commit();
    return ids;
}

void TividyMediator::undo() { undoFor(actor_, true); }

void TividyMediator::redo() { redoFor(actor_, true); }

void TividyMediator::undoFor(const std::string& actor, bool anyone) {
    if (historyFacade_.history() == nullptr || !historyFacade_.history()->canUndo()) {
        return;
    }
    Response done = historyFacade_.handle(Request(Action::Undo, TaskRef(), actor).with("anyone", anyone ? "1" : "0"));
    recomputeLoads();
    WorkflowEvent event(WorkflowEvent::UNDONE, "", "", done.data("label"));
    event.actor = actor;
    bus_.notify(event);
}

void TividyMediator::redoFor(const std::string& actor, bool anyone) {
    if (historyFacade_.history() == nullptr || !historyFacade_.history()->canRedo()) {
        return;
    }
    Response done = historyFacade_.handle(Request(Action::Redo, TaskRef(), actor).with("anyone", anyone ? "1" : "0"));
    recomputeLoads();
    WorkflowEvent event(WorkflowEvent::REDONE, "", "", done.data("label"));
    event.actor = actor;
    bus_.notify(event);
}

bool TividyMediator::canUndo(const std::string& actor, bool anyone) const { return historyFacade_.canUndo(actor, anyone); }

bool TividyMediator::canRedo(const std::string& actor, bool anyone) const { return historyFacade_.canRedo(actor, anyone); }

std::vector<std::string> TividyMediator::undoLabels(std::size_t max) const { return historyFacade_.undoLabels(max); }

std::vector<std::string> TividyMediator::redoLabels(std::size_t max) const { return historyFacade_.redoLabels(max); }

void TividyMediator::clearHistory() { return historyFacade_.clearHistory(); }

void TividyMediator::addUnit(const std::string& parentId, const std::string& id, const std::string& name) {
    if (id.size() > 40 || name.size() > 60) {
        throw std::invalid_argument("Unit id or name is too long");
    }
    Step step(*this, label("added team " + name, ""));
    orgFacade_.handle(Request(Action::AddUnit).with("parent", parentId).with("unit", id).with("name", name));
    publish(WorkflowEvent::ORG_CHANGED, "", "", "unit " + id);
    step.commit();
}

void TividyMediator::addPerson(const std::string& name, const std::string& role, const std::string& unitId) {
    if (name.empty() || name.size() > 40 || role.empty() || role.size() > 40) {
        throw std::invalid_argument("A person needs a name and a role of at most 40 characters");
    }
    if (orgFacade_.company().findUnit(unitId) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + unitId);
    }
    if (orgFacade_.company().structure().find(name) != nullptr || orgFacade_.company().find(name) != nullptr) {
        throw std::invalid_argument("Id already in use: " + name);
    }
    Step step(*this, label("added " + name + " as " + role, ""));
    orgFacade_.handle(Request(Action::AddPerson).with("person", name).with("role", role).with("unit", unitId));
    publish(WorkflowEvent::ORG_CHANGED, "", "", "person " + name);
    step.commit();
}

void TividyMediator::moveMember(const std::string& unitId, const std::string& name) {
    orgFacade_.findParticipant(name);
    if (orgFacade_.company().findUnit(unitId) == nullptr) {
        throw std::invalid_argument("Unknown unit: " + unitId);
    }
    Step step(*this, label("moved " + name + " to " + orgFacade_.company().findUnit(unitId)->name(), ""));
    orgFacade_.handle(Request(Action::MoveMember).with("unit", unitId).with("person", name));
    publish(WorkflowEvent::ORG_CHANGED, "", "", "move " + name);
    step.commit();
}

const WorkflowInstance& TividyMediator::instance(const std::string& instanceId) const { return projectFacade_.instance(instanceId); }

std::vector<std::string> TividyMediator::instanceIds() const { return projectFacade_.instanceIds(); }

std::string TividyMediator::policyOf(const std::string& instanceId) const { return projectFacade_.policyOf(instanceId); }

std::vector<ItemRef> TividyMediator::workFor(const std::string& participant) const { return projectFacade_.workFor(participant); }

std::vector<ItemRef> TividyMediator::approvalsFor(const std::string& participant) const {
    std::vector<ItemRef> refs;
    if (orgFacade_.company().find(participant) == nullptr) return refs;
    for (std::size_t i = 0; i < projectFacade_.instanceIds().size(); ++i) {
        FilterIterator it(&projectFacade_.findInstance(projectFacade_.instanceIds()[i]).root(), TaskFilter::awaitingAnyApprover(participant));
        for (it.first(); !it.isDone(); it.next()) {
            if (canApprove(participant, projectFacade_.instanceIds()[i], it.currentItem()->id())) {
                refs.push_back(ItemRef(projectFacade_.instanceIds()[i], it.currentItem()->id()));
            }
        }
    }
    return refs;
}

std::vector<std::string> TividyMediator::signOffsFor(const std::string& participant) const {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < projectFacade_.instanceIds().size(); ++i) {
        if (canSignOff(participant, projectFacade_.instanceIds()[i])) ids.push_back(projectFacade_.instanceIds()[i]);
    }
    return ids;
}

bool TividyMediator::canApprove(const std::string& participant, const std::string& instanceId,
                        const std::string& itemId) const {
    Participant* person = orgFacade_.company().find(participant);
    if (person == nullptr) return false;
    LeafTask* leaf = leafOf(&projectFacade_.findItem(instanceId, itemId));
    if (leaf == nullptr || leaf->status() != LeafTask::AWAITING_APPROVAL) return false;
    if (leaf->assignee() == participant || leaf->hasSigned(participant)) return false;
    const std::vector<std::string>& pending = leaf->pendingApprovers();
    if (pending.empty()) return orgFacade_.company().isManager(participant);
    if (orgFacade_.company().isOwner(participant)) return true;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        if (pending[i] == person->role()) return true;
    }
    return false;
}

bool TividyMediator::canSignOff(const std::string& participant, const std::string& instanceId) const {
    const WorkflowInstance& instance = projectFacade_.findInstance(instanceId);
    if (instance.state() != WorkflowInstance::AWAITING_SIGNOFF) return false;
    if (orgFacade_.company().hasOwner()) return orgFacade_.company().isOwner(participant);
    return orgFacade_.company().isManager(participant);
}

std::vector<std::string> TividyMediator::readyItems(const std::string& instanceId) const { return projectFacade_.readyItems(instanceId); }

double TividyMediator::progress(const std::string& instanceId) const { return projectFacade_.progress(instanceId); }
