#ifndef READYONLYITERATOR_H
#define READYONLYITERATOR_H

#include "patterns/execution/DepthFirstIterator.h"

// ITERATOR pattern - a CONCRETE ITERATOR that filters another.
//
// Walks the tree in depth-first order but only stops on nodes that are ready
// to be picked up (WorkNode::isReady), e.g. to answer "what can the team work
// on right now?". It owns a DepthFirstIterator and skips past everything else.
class ReadyOnlyIterator : public WorkIterator {
private:
    DepthFirstIterator inner_;

    // Advances inner_ until it sits on a ready node or is done.
    void skipToReady();

public:
    explicit ReadyOnlyIterator(WorkNode* root);

    void first() override;
    void next() override;
    bool isDone() const override;
    WorkNode* currentItem() const override;
};

#endif // READYONLYITERATOR_H
