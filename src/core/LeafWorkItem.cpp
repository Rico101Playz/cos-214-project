#include "core/LeafWorkItem.h"
#include "patterns/execution/CreatedState.h"
#include "patterns/execution/CompletedState.h"
#include "patterns/execution/CancelledState.h"

#include <stdexcept>
#include <utility>

LeafWorkItem::LeafWorkItem(const std::string& id, const std::string& name,
                           const std::string& role, int dueHours)
    : WorkItem(id, name, role),
      dueHours_(dueHours),
      assignee_(nullptr),
      reason_(),
      state_(new CreatedState()) {}

void LeafWorkItem::makeAvailable()                   { state_->makeAvailable(*this); }
void LeafWorkItem::assign(Participant& participant)  { state_->assign(*this, participant); }
void LeafWorkItem::start()                           { state_->start(*this); }
void LeafWorkItem::complete()                        { state_->complete(*this); }
void LeafWorkItem::reject(const std::string& reason) { state_->reject(*this, reason); }
void LeafWorkItem::escalate()                        { state_->escalate(*this); }

void LeafWorkItem::cancel() {
    if (dynamic_cast<CompletedState*>(state_.get()) != nullptr ||
        dynamic_cast<CancelledState*>(state_.get()) != nullptr) {
        return;
    }
    transitionTo(std::unique_ptr<WorkItemState>(new CancelledState()));
}

bool LeafWorkItem::isComplete() const {
    return dynamic_cast<CompletedState*>(state_.get()) != nullptr;
}

double LeafWorkItem::progress() const {
    return isComplete() ? 1.0 : 0.0;
}

void LeafWorkItem::transitionTo(std::unique_ptr<WorkItemState> next) {
    if (!next) {
        throw std::invalid_argument("LeafWorkItem::transitionTo: next state is null");
    }
    state_ = std::move(next);
}

void LeafWorkItem::setAssignee(Participant& participant) { assignee_ = &participant; }
void LeafWorkItem::setReason(const std::string& reason)   { reason_ = reason; }

int LeafWorkItem::dueHours() const              { return dueHours_; }
Participant* LeafWorkItem::assignee() const     { return assignee_; }
const std::string& LeafWorkItem::reason() const { return reason_; }
std::string LeafWorkItem::stateName() const     { return state_->name(); }