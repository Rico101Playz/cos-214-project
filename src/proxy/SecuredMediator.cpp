#include "proxy/SecuredMediator.h"
#include "mediator/TividyMediator.h"
#include "composite/LeafTask.h"

SecuredMediator::SecuredMediator(TividyMediator& real, const std::string& actor) : real_(real), actor_(actor) {
    if (real_.organisation().find(actor) == nullptr) {
        throw std::invalid_argument("Unknown participant: " + actor);
    }
}

const std::string& SecuredMediator::actor() const { return actor_; }

bool SecuredMediator::isManager() const { return real_.organisation().isManager(actor_); }

bool SecuredMediator::isOwner() const { return real_.organisation().isOwner(actor_); }

LeafTask* SecuredMediator::leaf(const std::string& instanceId, const std::string& itemId) const {
    const WorkflowInstance& instance = real_.instance(instanceId);
    LeafTask* found = leafOf(instance.item(itemId));
    if (found == nullptr) {
        throw std::invalid_argument("Unknown item " + itemId + " in " + instanceId);
    }
    return found;
}

void SecuredMediator::requireManager(const std::string& action) const {
    require(isManager(), action);
}

void SecuredMediator::require(bool allowed, const std::string& action) const {
    if (!allowed || real_.organisation().find(actor_) == nullptr) {
        throw AccessDenied(actor_ + " is not allowed to " + action);
    }
}

bool SecuredMediator::mayClaim(const std::string& instanceId, const std::string& itemId) const {
    LeafTask* item = leaf(instanceId, itemId);
    if (item->status() != LeafTask::AVAILABLE) return false;
    if (isManager()) return true;
    const Company& org = real_.organisation();
    const Participant* me = org.find(actor_);
    if (me == nullptr) return false;
    if (!item->role().empty() && item->role() != me->role()) return false;
    if (item->unit().empty() || org.inUnit(actor_, item->unit())) return true;
    OrgUnit* mine = org.unitOf(actor_);
    OrgUnit* target = org.findUnit(item->unit());
    return mine != nullptr && target != nullptr && mine->contains(target);
}

bool SecuredMediator::mayWork(const std::string& instanceId, const std::string& itemId) const {
    return leaf(instanceId, itemId)->assignee() == actor_;
}

bool SecuredMediator::mayDecide(const std::string& instanceId, const std::string& itemId) const {
    return real_.canApprove(actor_, instanceId, itemId);
}

bool SecuredMediator::mayEscalate(const std::string& instanceId, const std::string& itemId) const {
    return isManager() || mayWork(instanceId, itemId);
}

bool SecuredMediator::mayAddSubtask(const std::string& instanceId, const std::string& itemId) const {
    return isManager() || mayWork(instanceId, itemId);
}

bool SecuredMediator::mayWithdraw(const std::string& instanceId, const std::string& itemId) const {
    LeafTask* item = leaf(instanceId, itemId);
    return item->status() == LeafTask::AWAITING_APPROVAL && (item->assignee() == actor_ || isManager());
}

bool SecuredMediator::maySignOff(const std::string& instanceId) const {
    return real_.canSignOff(actor_, instanceId);
}

std::string SecuredMediator::startProject(const std::string& definition, const std::string& title,
                                        const std::string& policy, SignOffRule signOff) {
    requireManager("start projects");
    TividyMediator::ActorScope scope(real_, actor_);
    return real_.startProject(definition, title, policy, signOff);
}

std::string SecuredMediator::addTask(const std::string& instanceId, const TaskSpec& spec) {
    requireManager("create tasks");
    TividyMediator::ActorScope scope(real_, actor_);
    return real_.addTask(instanceId, spec);
}

std::string SecuredMediator::addSubtask(const std::string& instanceId, const std::string& parentItemId,
                                      const TaskSpec& spec) {
    require(mayAddSubtask(instanceId, parentItemId), "add sub-tasks to " + parentItemId);
    if (!spec.assignee.empty() && spec.assignee != actor_) {
        requireManager("hand a sub-task to someone else");
    }
    if (!spec.approvers().empty()) {
        requireManager("choose who approves a sub-task");
    }
    TividyMediator::ActorScope scope(real_, actor_);
    return real_.addSubtask(instanceId, parentItemId, spec);
}

void SecuredMediator::assign(const std::string& instanceId, const std::string& itemId,
                           const std::string& participant) {
    if (participant == actor_ && !isManager()) {
        require(mayClaim(instanceId, itemId), "claim " + itemId);
    } else {
        requireManager("assign work to others");
    }
    TividyMediator::ActorScope scope(real_, actor_);
    real_.assign(instanceId, itemId, participant);
}

void SecuredMediator::autoAssign(const std::string& instanceId, const std::string& itemId) {
    requireManager("auto-assign work");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.autoAssign(instanceId, itemId);
}

void SecuredMediator::start(const std::string& instanceId, const std::string& itemId) {
    require(mayWork(instanceId, itemId), "start " + itemId + ", it is not assigned to them");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.start(instanceId, itemId);
}

void SecuredMediator::complete(const std::string& instanceId, const std::string& itemId) {
    require(mayWork(instanceId, itemId), "send " + itemId + " for approval, it is not assigned to them");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.complete(instanceId, itemId);
}

void SecuredMediator::withdraw(const std::string& instanceId, const std::string& itemId) {
    require(mayWithdraw(instanceId, itemId), "take " + itemId + " back out of approval");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.withdraw(instanceId, itemId);
}

void SecuredMediator::approve(const std::string& instanceId, const std::string& itemId) {
    require(mayDecide(instanceId, itemId), "approve " + itemId);
    TividyMediator::ActorScope scope(real_, actor_);
    real_.approve(instanceId, itemId);
}

void SecuredMediator::reject(const std::string& instanceId, const std::string& itemId,
                           const std::string& reason) {
    require(mayDecide(instanceId, itemId), "request changes on " + itemId);
    TividyMediator::ActorScope scope(real_, actor_);
    real_.reject(instanceId, itemId, reason);
}

void SecuredMediator::escalate(const std::string& instanceId, const std::string& itemId) {
    require(mayEscalate(instanceId, itemId), "escalate " + itemId);
    TividyMediator::ActorScope scope(real_, actor_);
    real_.escalate(instanceId, itemId);
}

void SecuredMediator::setPriority(const std::string& instanceId, const std::string& itemId,
                                const std::string& priority) {
    requireManager("change priorities");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.setPriority(instanceId, itemId, priority);
}

void SecuredMediator::cancel(const std::string& instanceId, const std::string& reason) {
    requireManager("cancel projects");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.cancel(instanceId, reason);
}

void SecuredMediator::signOff(const std::string& instanceId) {
    require(maySignOff(instanceId), "sign off this project");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.signOff(instanceId);
}

std::vector<std::string> SecuredMediator::flagIssue(const std::string& instanceId, const IssueSpec& spec) {
    require(maySignOff(instanceId), "flag issues on this project");
    TividyMediator::ActorScope scope(real_, actor_);
    return real_.flagIssue(instanceId, spec);
}

bool SecuredMediator::mayUndo() const { return real_.canUndo(actor_, isManager()); }

bool SecuredMediator::mayRedo() const { return real_.canRedo(actor_, isManager()); }

void SecuredMediator::undo() {
    require(mayUndo(), "undo the last action, it was not theirs");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.undoFor(actor_, isManager());
}

void SecuredMediator::redo() {
    require(mayRedo(), "redo that action, it was not theirs");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.redoFor(actor_, isManager());
}

std::shared_ptr<const EngineMemento> SecuredMediator::createMemento(const std::string& label) const {
    return real_.createMemento(label);
}

void SecuredMediator::restoreMemento(const EngineMemento& memento) {
    requireManager("restore an earlier state of the engine");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.restoreMemento(memento);
}

void SecuredMediator::addUnit(const std::string& parentId, const std::string& id,
                            const std::string& name) {
    requireManager("change the company layout");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.addUnit(parentId, id, name);
}

void SecuredMediator::addPerson(const std::string& name, const std::string& role,
                              const std::string& unitId) {
    requireManager("add people");
    if (role == Company::OWNER_ROLE) {
        require(isOwner(), "make someone an owner");
    }
    TividyMediator::ActorScope scope(real_, actor_);
    real_.addPerson(name, role, unitId);
}

void SecuredMediator::moveMember(const std::string& unitId, const std::string& name) {
    requireManager("move people between teams");
    TividyMediator::ActorScope scope(real_, actor_);
    real_.moveMember(unitId, name);
}
