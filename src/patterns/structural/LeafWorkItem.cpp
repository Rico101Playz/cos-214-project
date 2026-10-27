#include "patterns/structural/LeafWorkItem.h"
#include "patterns/structural/CompositeStage.h"

#include "patterns/execution/CreatedState.h"
#include "patterns/execution/AvailableState.h"
#include "patterns/execution/AssignedState.h"
#include "patterns/execution/InProgressState.h"
#include "patterns/execution/AwaitingApprovalState.h"
#include "patterns/execution/CompletedState.h"
#include "patterns/execution/RejectedState.h"
#include "patterns/execution/CancelledState.h"
#include "patterns/execution/EscalatedState.h"
#include "patterns/creation/StateFactory.h"
#include "patterns/execution/DepthFirstIterator.h"

#include <stdexcept>
#include <utility>

const char* const LeafWorkItem::PRIORITIES[4] = { "Low", "Medium", "High", "Urgent" };

bool LeafWorkItem::validPriority(const std::string& priority) {
    return priorityRank(priority) >= 0;
}

int LeafWorkItem::priorityRank(const std::string& priority) {
    for (int i = 0; i < 4; ++i) {
        if (priority == PRIORITIES[i]) return i;
    }
    return -1;
}

LeafWorkItem::LeafWorkItem(const std::string& id, const std::string& name,
                           const std::string& role, int dueHours)
    : WorkItem(id, name, role),
      dueHours_(dueHours),
      state_(new CreatedState()),
      priority_("Medium"),
      released_(false) {
    itemId = id;     
    state = status_;
}

void LeafWorkItem::makeAvailable() {
    released_ = true;
    state_->makeAvailable(*this);
}
void LeafWorkItem::assign(const std::string& participant) { state_->assign(*this, participant); }
void LeafWorkItem::start()                                { state_->start(*this); }
void LeafWorkItem::complete() {
    std::vector<std::string> open = openSubtasks();
    if (!open.empty()) {
        throw std::logic_error("Cannot send " + id() + " for approval while " +
                               std::to_string(open.size()) + " sub-task(s) are unfinished");
    }
    state_->complete(*this);
}
void LeafWorkItem::approve()                              { state_->approve(*this); }
void LeafWorkItem::reject(const std::string& reason)      { state_->reject(*this, reason); }
void LeafWorkItem::escalate()                             { state_->escalate(*this); }

void LeafWorkItem::withdraw() {
    state_->withdraw(*this);
    clearApprovals();
}

// ---- Cancel: any non-terminal state -> Cancelled (no-op on finished items) ----

void LeafWorkItem::cancel() {
    for (std::size_t i = 0; i < subtasks_.size(); ++i) {
        subtasks_[i]->cancel();
    }
    if (status_ == COMPLETED || status_ == CANCELLED) {
        return;
    }
    transitionTo(std::unique_ptr<WorkItemState>(new CancelledState()));
}

// ---- State support ----

void LeafWorkItem::transitionTo(std::unique_ptr<WorkItemState> next) {
    if (!next) {
        throw std::invalid_argument("LeafWorkItem::transitionTo: next state is null");
    }
    state_ = std::move(next);    
    status_ = state_->name();
    state = status_;               

    if (status_ == COMPLETED) {   
        CompositeStage* stage = dynamic_cast<CompositeStage*>(parent());
        if (stage != nullptr) {
            stage->childCompleted(*this);
        }
        releaseWaiting();
    }
}
void LeafWorkItem::setAssignee(const std::string& participant) {
    WorkItem::setAssignee(participant);
    reason_.clear();
}

void LeafWorkItem::setReason(const std::string& reason) { reason_ = reason; }

void LeafWorkItem::restore(const std::string& stateName, const std::string& assigneeName,
                           const std::string& reasonText) {
    state_ = createWorkItemState(stateName);
    status_ = state_->name();
    state = status_;
    WorkItem::setAssignee(assigneeName);
    reason_ = reasonText;
}

int LeafWorkItem::dueHours() const { return dueHours_; }

WorkNode* LeafWorkItem::treeRoot() const {
    WorkNode* node = const_cast<LeafWorkItem*>(this);
    while (node->parent() != nullptr) {
        node = node->parent();
    }
    return node;
}

void LeafWorkItem::releaseWaiting() {
    DepthFirstIterator it(treeRoot());
    for (it.first(); !it.isDone(); it.next()) {
        LeafWorkItem* leaf = dynamic_cast<LeafWorkItem*>(it.currentItem());
        if (leaf != nullptr && leaf != this && leaf->isWaiting() && !leaf->isBlocked()) {
            leaf->makeAvailable();
        }
    }
}

std::string LeafWorkItem::unit() const { return getData("unit"); }

void LeafWorkItem::setUnit(const std::string& unitId) { setData("unit", unitId); }

const std::string& LeafWorkItem::priority() const { return priority_; }

void LeafWorkItem::setPriority(const std::string& priority) {
    if (!validPriority(priority)) {
        throw std::invalid_argument("Priority must be Low, Medium, High or Urgent");
    }
    priority_ = priority;
}

std::string LeafWorkItem::approverRole() const {
    return approverRoles_.empty() ? std::string() : approverRoles_.front();
}

void LeafWorkItem::setApproverRole(const std::string& role) {
    approverRoles_.clear();
    if (!role.empty()) approverRoles_.push_back(role);
}

const std::vector<std::string>& LeafWorkItem::approverRoles() const { return approverRoles_; }

void LeafWorkItem::setApproverRoles(const std::vector<std::string>& roles) {
    approverRoles_.clear();
    for (std::size_t i = 0; i < roles.size(); ++i) {
        if (!roles[i].empty()) approverRoles_.push_back(roles[i]);
    }
}

std::string LeafWorkItem::pendingApprover() const {
    return pendingApprovers_.empty() ? std::string() : pendingApprovers_.front();
}

void LeafWorkItem::setPendingApprover(const std::string& role) {
    pendingApprovers_.clear();
    if (!role.empty()) pendingApprovers_.push_back(role);
}

const std::vector<std::string>& LeafWorkItem::pendingApprovers() const { return pendingApprovers_; }

void LeafWorkItem::setPendingApprovers(const std::vector<std::string>& roles) { pendingApprovers_ = roles; }

bool LeafWorkItem::takePendingSlot(const std::string& role) {
    for (std::size_t i = 0; i < pendingApprovers_.size(); ++i) {
        if (pendingApprovers_[i] == role) {
            pendingApprovers_.erase(pendingApprovers_.begin() + i);
            return true;
        }
    }
    return false;
}

const std::vector<LeafWorkItem::Signature>& LeafWorkItem::signatures() const { return signatures_; }

void LeafWorkItem::addSignature(const std::string& role, const std::string& person) {
    signatures_.push_back(Signature(role, person));
}

bool LeafWorkItem::hasSigned(const std::string& person) const {
    for (std::size_t i = 0; i < signatures_.size(); ++i) {
        if (signatures_[i].second == person) return true;
    }
    return false;
}

void LeafWorkItem::clearApprovals() {
    pendingApprovers_.clear();
    signatures_.clear();
}

const std::vector<std::string>& LeafWorkItem::blockedBy() const { return blockedBy_; }

void LeafWorkItem::addBlocker(const std::string& itemId) {
    if (itemId.empty() || itemId == id()) {
        throw std::invalid_argument("An item cannot block itself");
    }
    for (std::size_t i = 0; i < blockedBy_.size(); ++i) {
        if (blockedBy_[i] == itemId) return;
    }
    blockedBy_.push_back(itemId);
}

std::vector<std::string> LeafWorkItem::openBlockers() const {
    std::vector<std::string> open;
    WorkNode* root = treeRoot();
    for (std::size_t i = 0; i < blockedBy_.size(); ++i) {
        WorkNode* blocker = root->find(blockedBy_[i]);
        if (blocker != nullptr && !blocker->isComplete()) {
            open.push_back(blockedBy_[i]);
        }
    }
    return open;
}

bool LeafWorkItem::isBlocked() const { return !openBlockers().empty(); }

bool LeafWorkItem::isWaiting() const { return released_ && status_ == CREATED; }

bool LeafWorkItem::isReleased() const { return released_; }

void LeafWorkItem::setReleased(bool released) { released_ = released; }

LeafWorkItem& LeafWorkItem::addSubtask(std::unique_ptr<LeafWorkItem> subtask) {
    if (!subtask) {
        throw std::invalid_argument("Cannot add a null sub-task to " + id());
    }
    subtask->setParent(this);
    subtasks_.push_back(std::move(subtask));
    return *subtasks_.back();
}

std::size_t LeafWorkItem::childCount() const { return subtasks_.size(); }

WorkNode* LeafWorkItem::child(std::size_t index) const {
    return index < subtasks_.size() ? subtasks_[index].get() : nullptr;
}

std::vector<std::string> LeafWorkItem::openSubtasks() const {
    std::vector<std::string> open;
    for (std::size_t i = 0; i < subtasks_.size(); ++i) {
        const std::string& status = subtasks_[i]->status();
        if (status != COMPLETED && status != CANCELLED) {
            open.push_back(subtasks_[i]->id());
        }
    }
    return open;
}

LeafWorkItem* LeafWorkItem::parentItem() const {
    return dynamic_cast<LeafWorkItem*>(parent());
}
