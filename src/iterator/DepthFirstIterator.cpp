#include "iterator/DepthFirstIterator.h"
#include "composite/Task.h"

DepthFirstIterator::DepthFirstIterator(Task* root) : root_(root) {
    first();
}

void DepthFirstIterator::first() {
    pending_.clear();
    if (root_ != nullptr) {
        pending_.push_back(root_);
    }
}

void DepthFirstIterator::next() {
    if (pending_.empty()) {
        return;
    }
    Task* current = pending_.back();
    pending_.pop_back();

    // Push children last-to-first so the first child is visited next.
    for (std::size_t i = current->childCount(); i > 0; --i) {
        pending_.push_back(current->child(i - 1));
    }
}

bool DepthFirstIterator::isDone() const { return pending_.empty(); }

Task* DepthFirstIterator::currentItem() const {
    return pending_.empty() ? nullptr : pending_.back();
}
