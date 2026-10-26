#ifndef WORKITERATOR_H
#define WORKITERATOR_H

class WorkNode;

// ITERATOR pattern - the ITERATOR interface.
//
// Walks the nodes of a work tree in some order without exposing how the tree
// stores its children. Usage:
//
//     for (it.first(); !it.isDone(); it.next()) { use(it.currentItem()); }
//
// The tree must not be changed (nodes added or removed) while it is walked.
class WorkIterator {
public:
    virtual ~WorkIterator() = default;

    // Moves to the first node of the walk.
    virtual void first() = 0;
    // Moves to the following node. Does nothing once the walk is done.
    virtual void next() = 0;
    // True when every node has been visited.
    virtual bool isDone() const = 0;
    // The node at the current position; nullptr once the walk is done.
    virtual WorkNode* currentItem() const = 0;
};

#endif // WORKITERATOR_H
