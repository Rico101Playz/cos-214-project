#include "patterns/execution/ReadyOnlyIterator.h"
#include "patterns/structural/WorkNode.h"

ReadyOnlyIterator::ReadyOnlyIterator(WorkNode* root) : inner_(root) {
    skipToReady();
}

void ReadyOnlyIterator::skipToReady() {
    while (!inner_.isDone() && !inner_.currentItem()->isReady()) {
        inner_.next();
    }
}

void ReadyOnlyIterator::first() {
    inner_.first();
    skipToReady();
}

void ReadyOnlyIterator::next() {
    inner_.next();
    skipToReady();
}

bool ReadyOnlyIterator::isDone() const { return inner_.isDone(); }

WorkNode* ReadyOnlyIterator::currentItem() const { return inner_.currentItem(); }
