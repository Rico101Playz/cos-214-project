#include "patterns/execution/AwaitingApprovalIterator.h"
#include "patterns/structural/LeafWorkItem.h"

AwaitingApprovalIterator::AwaitingApprovalIterator(WorkNode* root, const std::string& approverRole,
                                                   const std::string& excludeAssignee)
    : inner_(root), approverRole_(approverRole), excludeAssignee_(excludeAssignee), anyRole_(false) {
    skip();
}

AwaitingApprovalIterator AwaitingApprovalIterator::anyApprover(WorkNode* root,
                                                               const std::string& excludeAssignee) {
    AwaitingApprovalIterator it(root, "", excludeAssignee);
    it.anyRole_ = true;
    it.first();
    return it;
}

bool AwaitingApprovalIterator::matches(WorkNode* node) const {
    LeafWorkItem* item = dynamic_cast<LeafWorkItem*>(node);
    if (item == nullptr || item->status() != WorkItem::AWAITING_APPROVAL) return false;
    if (!excludeAssignee_.empty() && item->assignee() == excludeAssignee_) return false;
    if (anyRole_) return true;
    const std::vector<std::string>& pending = item->pendingApprovers();
    for (std::size_t i = 0; i < pending.size(); ++i) {
        if (pending[i] == approverRole_) return true;
    }
    return false;
}

void AwaitingApprovalIterator::skip() {
    while (!inner_.isDone() && !matches(inner_.currentItem())) {
        inner_.next();
    }
}

void AwaitingApprovalIterator::first() {
    inner_.first();
    skip();
}

void AwaitingApprovalIterator::next() {
    inner_.next();
    skip();
}

bool AwaitingApprovalIterator::isDone() const { return inner_.isDone(); }

WorkNode* AwaitingApprovalIterator::currentItem() const { return inner_.currentItem(); }
