#include "composite/LeafTask.h"
#include "composite/CompositeTask.h"

#include "state/CreatedState.h"
#include "state/AvailableState.h"
#include "state/AssignedState.h"
#include "state/InProgressState.h"
#include "state/AwaitingApprovalState.h"
#include "state/CompletedState.h"
#include "state/RejectedState.h"
#include "state/CancelledState.h"
#include "state/EscalatedState.h"
#include "state/StateFactory.h"
#include "iterator/DepthFirstIterator.h"

#include <cstdlib>
#include <stdexcept>
#include "mediator/Request.h"
#include "decorator/ApprovalWork.h"
#include "decorator/BlockedWork.h"
#include <utility>

const char* const LeafTask::CREATED = "Created";
const char* const LeafTask::AVAILABLE = "Available";
const char* const LeafTask::ASSIGNED = "Assigned";
const char* const LeafTask::IN_PROGRESS = "InProgress";
const char* const LeafTask::AWAITING_APPROVAL = "AwaitingApproval";
const char* const LeafTask::COMPLETED = "Completed";
const char* const LeafTask::REJECTED = "Rejected";
const char* const LeafTask::CANCELLED = "Cancelled";
const char* const LeafTask::ESCALATED = "Escalated";

const char* const LeafTask::PRIORITIES[4] = { "Low", "Medium", "High", "Urgent" };

bool LeafTask::validPriority(const std::string& priority) {
    return priorityRank(priority) >= 0;
}

int LeafTask::priorityRank(const std::string& priority) {
    for (int i = 0; i < 4; ++i) {
        if (priority == PRIORITIES[i]) return i;
    }
    return -1;
}

LeafTask::LeafTask(const std::string& id) : LeafTask(id, id, "") {}

LeafTask::LeafTask(const std::string& id, const std::string& name,
                           const std::string& role, int dueHours)
    : TaskNode(id, name),
      outermost_(this),
      role_(role),
      dueHours_(dueHours),
      state_(new CreatedState()),
      priority_("Medium"),
      released_(false) {}

void LeafTask::makeAvailable() {
    released_ = true;
    state_->makeAvailable(*this);
}
void LeafTask::assign(const std::string& participant) { state_->assign(*this, participant); }
void LeafTask::start()                                { state_->start(*this); }
void LeafTask::complete() {
    std::vector<std::string> open = openSubtasks();
    if (!open.empty()) {
        throw std::logic_error("Cannot send " + id() + " for approval while " +
                               std::to_string(open.size()) + " sub-task(s) are unfinished");
    }
    state_->complete(*this);
}
void LeafTask::approve()                              { state_->approve(*this); }
void LeafTask::reject(const std::string& reason)      { state_->reject(*this, reason); }
void LeafTask::escalate()                             { state_->escalate(*this); }

void LeafTask::withdraw() {
    state_->withdraw(*this);
}

// ---- Cancel: any non-terminal state -> Cancelled (no-op on finished items) ----

void LeafTask::cancel() {
    for (std::size_t i = 0; i < outermost_->childCount(); ++i) {
        outermost_->child(i)->cancel();
    }
    if (status() == COMPLETED || status() == CANCELLED) {
        return;
    }
    transitionTo(std::unique_ptr<TaskState>(new CancelledState()));
}

// ---- State support ----

void LeafTask::transitionTo(std::unique_ptr<TaskState> next) {
    if (!next) {
        throw std::invalid_argument("LeafTask::transitionTo: next state is null");
    }
    state_ = std::move(next);

    if (status() == COMPLETED) {
        CompositeTask* stage = stageOf(parent());
        if (stage != nullptr) {
            stage->childCompleted(*outermost_);
        }
        releaseWaiting();
    }
}
void LeafTask::setAssignee(const std::string& participant) {
    assignee_ = participant;
    reason_.clear();
}

void LeafTask::setReason(const std::string& reason) { reason_ = reason; }

void LeafTask::restore(const std::string& stateName, const std::string& assigneeName,
                           const std::string& reasonText) {
    state_ = createWorkItemState(stateName);
    assignee_ = assigneeName;
    reason_ = reasonText;
}

int LeafTask::dueHours() const { return dueHours_; }

Task* LeafTask::treeRoot() const {
    Task* node = const_cast<LeafTask*>(this);
    while (node->parent() != nullptr) {
        node = node->parent();
    }
    return node;
}

void LeafTask::releaseWaiting() {
    DepthFirstIterator it(treeRoot());
    for (it.first(); !it.isDone(); it.next()) {
        LeafTask* leaf = leafOf(it.currentItem());
        if (leaf != nullptr && leaf != this && leaf->isWaiting() && !leaf->isBlocked()) {
            leaf->makeAvailable();
        }
    }
}

std::string LeafTask::unit() const { return getData("unit"); }

void LeafTask::setUnit(const std::string& unitId) { setData("unit", unitId); }

const std::string& LeafTask::priority() const { return priority_; }

void LeafTask::setPriority(const std::string& priority) {
    if (!validPriority(priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    priority_ = priority;
}

namespace {
const std::vector<std::string> NO_ROLES;
const std::vector<LeafTask::Signature> NO_SIGNATURES;
}

ApprovalWork* LeafTask::approvals() const { return outermost_->approvalWork(); }

std::string LeafTask::approverRole() const { return approvals() ? approvals()->approverRole() : std::string(); }

const std::vector<std::string>& LeafTask::approverRoles() const { return approvals() ? approvals()->approverRoles() : NO_ROLES; }

std::string LeafTask::pendingApprover() const { return approvals() ? approvals()->pendingApprover() : std::string(); }

const std::vector<std::string>& LeafTask::pendingApprovers() const {
    return approvals() ? approvals()->pendingApprovers() : NO_ROLES;
}

const std::vector<LeafTask::Signature>& LeafTask::signatures() const {
    return approvals() ? approvals()->signatures() : NO_SIGNATURES;
}

bool LeafTask::hasSigned(const std::string& person) const { return approvals() && approvals()->hasSigned(person); }

BlockedWork* LeafTask::blocking() const { return outermost_->blockedWork(); }

ValidatedWork* LeafTask::validation() const { return outermost_->validatedWork(); }

const std::vector<std::string>& LeafTask::blockedBy() const { return blocking() ? blocking()->blockedBy() : NO_ROLES; }

std::vector<std::string> LeafTask::openBlockers() const {
    return blocking() ? blocking()->openBlockers() : std::vector<std::string>();
}

bool LeafTask::isBlocked() const { return blocking() && blocking()->isBlocked(); }

bool LeafTask::isWaiting() const { return released_ && status() == CREATED; }

bool LeafTask::isReleased() const { return released_; }

void LeafTask::setReleased(bool released) { released_ = released; }

std::vector<std::string> LeafTask::openSubtasks() const {
    std::vector<std::string> open;
    for (std::size_t i = 0; i < outermost_->childCount(); ++i) {
        Task* subtask = outermost_->child(i);
        const std::string status = leafOf(subtask)->status();
        if (status != COMPLETED && status != CANCELLED) {
            open.push_back(subtask->id());
        }
    }
    return open;
}

LeafTask* LeafTask::parentItem() const {
    return leafOf(parent());
}

const std::string& LeafTask::role() const { return role_; }

std::string LeafTask::status() const { return state_->name(); }

const std::string& LeafTask::reason() const { return reason_; }

const std::string& LeafTask::assignee() const { return assignee_; }

void LeafTask::setData(const std::string& key, const std::string& value) { data_[key] = value; }

bool LeafTask::hasData(const std::string& key) const { return data_.find(key) != data_.end(); }

std::string LeafTask::getData(const std::string& key, const std::string& fallback) const {
    std::map<std::string, std::string>::const_iterator it = data_.find(key);
    return it == data_.end() ? fallback : it->second;
}

void LeafTask::removeData(const std::string& key) { data_.erase(key); }

const std::map<std::string, std::string>& LeafTask::data() const { return data_; }

bool LeafTask::holdsLoad() const {
    std::string s = status();
    return !assignee_.empty() && (s == ASSIGNED || s == IN_PROGRESS || s == AWAITING_APPROVAL || s == ESCALATED);
}

bool LeafTask::isComplete() const { return status() == COMPLETED; }

double LeafTask::progress() const { return isComplete() ? 1.0 : 0.0; }

bool LeafTask::isReady() const { return status() == AVAILABLE; }

LeafTask* LeafTask::leaf() { return this; }

void LeafTask::setOutermost(Task* outermost) { outermost_ = outermost == nullptr ? this : outermost; }

Task& LeafTask::outermost() const { return *outermost_; }

void LeafTask::handle(const Request& request) {
    switch (request.action()) {
        case Action::Assign: assign(request.arg("participant")); break;
        case Action::Start: start(); break;
        case Action::Complete: complete(); break;
        case Action::Approve: approve(); break;
        case Action::Reject: reject(request.arg("reason")); break;
        case Action::Escalate: escalate(); break;
        case Action::Withdraw: withdraw(); break;
        case Action::SetPriority: setPriority(request.arg("priority")); break;
        case Action::CancelProject:
            // cancel() is a no-op on finished work, so refuse it here instead.
            if (status() == COMPLETED || status() == CANCELLED) {
                throw std::logic_error("item '" + id() + "' is already " + status());
            }
            cancel();
            break;
        case Action::SetData: setData(request.arg("key"), request.arg("value")); break;
        case Action::Invoke: setData("last_operation", request.arg("operation")); break;
        default:
            throw std::invalid_argument(toString(request.action()) + " is not something a task does");
    }
}
