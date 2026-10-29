#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include <vector>
#include "iterator/WorkIterator.h"

// ITERATOR pattern - a CONCRETE ITERATOR.
//
// Visits a node, then each of its children's subtrees from first to last
// (pre-order), e.g. Delivery, Build, Implement, Test, Release, Deploy.
// This matches the order the work is laid out in the definition.
class DepthFirstIterator : public WorkIterator {
private:
    Task* root_;
    std::vector<Task*> pending_;    // stack; the back is the current node

public:
    // A null root gives an iterator that is immediately done.
    explicit DepthFirstIterator(Task* root);

    void first() override;
    void next() override;
    bool isDone() const override;
    Task* currentItem() const override;
};

#endif // DEPTHFIRSTITERATOR_H
