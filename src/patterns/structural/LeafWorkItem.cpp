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

#include <stdexcept>
#include <utility>

LeafWorkItem::LeafWorkItem(const std::string& id, const std::string& name,
                           const std::string& role, int dueHours)
    : WorkItem(id, name, role),
      dueHours_(dueHours),
      state_(new CreatedState()) {
    itemId = id;     
    state = status_;
}

void LeafWorkItem::makeAvailable()                        { state_->makeAvailable(*this); }
void LeafWorkItem::assign(const std::string& participant) { state_->assign(*this, participant); }
void LeafWorkItem::start()                                { state_->start(*this); }
void LeafWorkItem::complete()                             { state_->complete(*this); }
void LeafWorkItem::approve()                              { state_->approve(*this); }
void LeafWorkItem::reject(const std::string& reason)      { state_->reject(*this, reason); }
void LeafWorkItem::escalate()                             { state_->escalate(*this); }

// ---- Cancel: any non-terminal state -> Cancelled (no-op on finished items) ----

void LeafWorkItem::cancel() {
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