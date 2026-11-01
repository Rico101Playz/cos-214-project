#ifndef FILTERITERATOR_H
#define FILTERITERATOR_H

#include "iterator/DepthFirstIterator.h"
#include "iterator/TaskFilter.h"

// ITERATOR pattern - a CONCRETE ITERATOR that filters another.
//
// Walks the tree in depth-first order but only stops on nodes the TaskFilter
// matches. The default filter keeps nodes that are ready to be picked up
// (Task::isReady), e.g. to answer "what can the team work on right now?".
// It owns a DepthFirstIterator and skips past everything else.
class FilterIterator : public WorkIterator {
private:
    DepthFirstIterator inner_;
    TaskFilter filter_;

    // Advances inner_ until it sits on a matching node or is done.
    void skipToMatch();

public:
    explicit FilterIterator(Task* root, const TaskFilter& filter = TaskFilter::ready());

    void first() override;
    void next() override;
    bool isDone() const override;
    Task* currentItem() const override;
};

#endif // FILTERITERATOR_H
