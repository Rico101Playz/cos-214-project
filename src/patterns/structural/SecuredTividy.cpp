#include "patterns/structural/SecuredTividy.h"
#include "patterns/structural/Tividy.h"
#include "patterns/structural/LeafWorkItem.h"

SecuredTividy::SecuredTividy(Tividy& real, const std::string& actor) : real_(real), actor_(actor) {
    if (real_.organisation().find(actor) == nullptr) {
        throw std::invalid_argument("Unknown participant: " + actor);
    }
}

const std::string& SecuredTividy::actor() const { return actor_; }

bool SecuredTividy::isManager() const { return real_.organisation().isManager(actor_); }

bool SecuredTividy::isOwner() const { return real_.organisation().isOwner(actor_); }

LeafWorkItem* SecuredTividy::leaf(const std::string& instanceId, const std::string& itemId) const {
    const WorkflowInstance& instance = real_.instance(instanceId);
    LeafWorkItem* found = dynamic_cast<LeafWorkItem*>(instance.item(itemId));
    if (found == nullptr) {
        throw std::invalid_argument("Unknown item " + itemId + " in " + instanceId);
    }
    return found;
}

void SecuredTividy::requireManager(const std::string& action) const {
    require(isManager(), action);
}

void SecuredTividy::require(bool allowed, const std::string& action) const {
    if (!allowed || real_.organisation().find(actor_) == nullptr) {
        throw AccessDenied(actor_ + " is not allowed to " + action);
    }
}

bool SecuredTividy::mayClaim(const std::string& instanceId, const std::string& itemId) const {
    LeafWorkItem* item = leaf(instanceId, itemId);
    if (item->status() != WorkItem::AVAILABLE) return false;
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

bool SecuredTividy::mayWork(const std::string& instanceId, const std::string& itemId) const {
    return leaf(instanceId, itemId)->assignee() == actor_;
}

bool SecuredTividy::mayDecide(const std::string& instanceId, const std::string& itemId) const {
    return real_.canApprove(actor_, instanceId, itemId);
}

bool SecuredTividy::mayEscalate(const std::string& instanceId, const std::string& itemId) const {
    return isManager() || mayWork(instanceId, itemId);
}

bool SecuredTividy::mayAddSubtask(const std::string& instanceId, const std::string& itemId) const {
    return isManager() || mayWork(instanceId, itemId);
}

bool SecuredTividy::mayWithdraw(const std::string& instanceId, const std::string& itemId) const {
    LeafWorkItem* item = leaf(instanceId, itemId);
    return item->status() == WorkItem::AWAITING_APPROVAL && (item->assignee() == actor_ || isManager());
}

bool SecuredTividy::maySignOff(const std::string& instanceId) const {
    return real_.canSignOff(actor_, instanceId);
}

std::string SecuredTividy::startProject(const std::string& definition, const std::string& title,
                                        const std::string& policy, SignOffRule signOff) {
    requireManager("start projects");
    Tividy::ActorScope scope(real_, actor_);
    return real_.startProject(definition, title, policy, signOff);
}

std::string SecuredTividy::addTask(const std::string& instanceId, const TaskSpec& spec) {
    requireManager("create tasks");
    Tividy::ActorScope scope(real_, actor_);
    return real_.addTask(instanceId, spec);
}

std::string SecuredTividy::addSubtask(const std::string& instanceId, const std::string& parentItemId,
                                      const TaskSpec& spec) {
    require(mayAddSubtask(instanceId, parentItemId), "add sub-tasks to " + parentItemId);
    if (!spec.assignee.empty() && spec.assignee != actor_) {
        requireManager("hand a sub-task to someone else");
    }
    if (!spec.approvers().empty()) {
        requireManager("choose who approves a sub-task");
    }
    Tividy::ActorScope scope(real_, actor_);
    return real_.addSubtask(instanceId, parentItemId, spec);
}

void SecuredTividy::assign(const std::string& instanceId, const std::string& itemId,
                           const std::string& participant) {
    if (participant == actor_ && !isManager()) {
        require(mayClaim(instanceId, itemId), "claim " + itemId);
    } else {
        requireManager("assign work to others");
    }
    Tividy::ActorScope scope(real_, actor_);
    real_.assign(instanceId, itemId, participant);
}

void SecuredTividy::autoAssign(const std::string& instanceId, const std::string& itemId) {
    requireManager("auto-assign work");
    Tividy::ActorScope scope(real_, actor_);
    real_.autoAssign(instanceId, itemId);
}

void SecuredTividy::start(const std::string& instanceId, const std::string& itemId) {
    require(mayWork(instanceId, itemId), "start " + itemId + ", it is not assigned to them");
    Tividy::ActorScope scope(real_, actor_);
    real_.start(instanceId, itemId);
}

void SecuredTividy::complete(const std::string& instanceId, const std::string& itemId) {
    require(mayWork(instanceId, itemId), "send " + itemId + " for approval, it is not assigned to them");
    Tividy::ActorScope scope(real_, actor_);
    real_.complete(instanceId, itemId);
}

void SecuredTividy::withdraw(const std::string& instanceId, const std::string& itemId) {
    require(mayWithdraw(instanceId, itemId), "take " + itemId + " back out of approval");
    Tividy::ActorScope scope(real_, actor_);
    real_.withdraw(instanceId, itemId);
}

void SecuredTividy::approve(const std::string& instanceId, const std::string& itemId) {
    require(mayDecide(instanceId, itemId), "approve " + itemId);
    Tividy::ActorScope scope(real_, actor_);
    real_.approve(instanceId, itemId);
}

void SecuredTividy::reject(const std::string& instanceId, const std::string& itemId,
                           const std::string& reason) {
    require(mayDecide(instanceId, itemId), "request changes on " + itemId);
    Tividy::ActorScope scope(real_, actor_);
    real_.reject(instanceId, itemId, reason);
}

void SecuredTividy::escalate(const std::string& instanceId, const std::string& itemId) {
    require(mayEscalate(instanceId, itemId), "escalate " + itemId);
    Tividy::ActorScope scope(real_, actor_);
    real_.escalate(instanceId, itemId);
}

void SecuredTividy::setPriority(const std::string& instanceId, const std::string& itemId,
                                const std::string& priority) {
    requireManager("change priorities");
    Tividy::ActorScope scope(real_, actor_);
    real_.setPriority(instanceId, itemId, priority);
}

void SecuredTividy::cancel(const std::string& instanceId, const std::string& reason) {
    requireManager("cancel projects");
    Tividy::ActorScope scope(real_, actor_);
    real_.cancel(instanceId, reason);
}

void SecuredTividy::signOff(const std::string& instanceId) {
    require(maySignOff(instanceId), "sign off this project");
    Tividy::ActorScope scope(real_, actor_);
    real_.signOff(instanceId);
}

std::vector<std::string> SecuredTividy::flagIssue(const std::string& instanceId, const IssueSpec& spec) {
    require(maySignOff(instanceId), "flag issues on this project");
    Tividy::ActorScope scope(real_, actor_);
    return real_.flagIssue(instanceId, spec);
}

bool SecuredTividy::mayUndo() const { return real_.canUndo(actor_, isManager()); }

bool SecuredTividy::mayRedo() const { return real_.canRedo(actor_, isManager()); }

void SecuredTividy::undo() {
    require(mayUndo(), "undo the last action, it was not theirs");
    Tividy::ActorScope scope(real_, actor_);
    real_.undoFor(actor_, isManager());
}

void SecuredTividy::redo() {
    require(mayRedo(), "redo that action, it was not theirs");
    Tividy::ActorScope scope(real_, actor_);
    real_.redoFor(actor_, isManager());
}

void SecuredTividy::addUnit(const std::string& parentId, const std::string& id,
                            const std::string& name) {
    requireManager("change the company layout");
    Tividy::ActorScope scope(real_, actor_);
    real_.addUnit(parentId, id, name);
}

void SecuredTividy::addPerson(const std::string& name, const std::string& role,
                              const std::string& unitId) {
    requireManager("add people");
    if (role == Company::OWNER_ROLE) {
        require(isOwner(), "make someone an owner");
    }
    Tividy::ActorScope scope(real_, actor_);
    real_.addPerson(name, role, unitId);
}

void SecuredTividy::moveMember(const std::string& unitId, const std::string& name) {
    requireManager("move people between teams");
    Tividy::ActorScope scope(real_, actor_);
    real_.moveMember(unitId, name);
}
