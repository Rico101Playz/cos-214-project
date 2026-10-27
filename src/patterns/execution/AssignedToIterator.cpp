#include "patterns/execution/AssignedToIterator.h"
#include "core/WorkItem.h"

AssignedToIterator::AssignedToIterator(WorkNode* root, const std::string& participant)
    : inner_(root), participant_(participant) {
    skip();
}

bool AssignedToIterator::matches(WorkNode* node) const {
    WorkItem* item = dynamic_cast<WorkItem*>(node);
    if (item == nullptr || participant_.empty() || item->assignee() != participant_) return false;
    return item->status() != WorkItem::COMPLETED && item->status() != WorkItem::CANCELLED;
}

void AssignedToIterator::skip() {
    while (!inner_.isDone() && !matches(inner_.currentItem())) {
        inner_.next();
    }
}

void AssignedToIterator::first() {
    inner_.first();
    skip();
}

void AssignedToIterator::next() {
    inner_.next();
    skip();
}

bool AssignedToIterator::isDone() const { return inner_.isDone(); }

WorkNode* AssignedToIterator::currentItem() const { return inner_.currentItem(); }
