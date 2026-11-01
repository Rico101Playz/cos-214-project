#include "iterator/FilterIterator.h"
#include "composite/Task.h"

FilterIterator::FilterIterator(Task* root, const TaskFilter& filter) : inner_(root), filter_(filter) {
    skipToMatch();
}

void FilterIterator::skipToMatch() {
    while (!inner_.isDone() && !filter_.matches(*inner_.currentItem())) {
        inner_.next();
    }
}

void FilterIterator::first() {
    inner_.first();
    skipToMatch();
}

void FilterIterator::next() {
    inner_.next();
    skipToMatch();
}

bool FilterIterator::isDone() const { return inner_.isDone(); }

Task* FilterIterator::currentItem() const { return inner_.currentItem(); }
