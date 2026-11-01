#ifndef ASSIGNEDTOITERATOR_H
#define ASSIGNEDTOITERATOR_H

#include <string>
#include "patterns/execution/DepthFirstIterator.h"

class AssignedToIterator : public WorkIterator {
private:
    DepthFirstIterator inner_;
    std::string participant_;

    bool matches(WorkNode* node) const;
    void skip();

public:
    AssignedToIterator(WorkNode* root, const std::string& participant);

    void first() override;
    void next() override;
    bool isDone() const override;
    WorkNode* currentItem() const override;
};

#endif
