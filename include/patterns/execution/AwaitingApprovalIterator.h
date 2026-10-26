#ifndef AWAITINGAPPROVALITERATOR_H
#define AWAITINGAPPROVALITERATOR_H

#include <string>
#include "patterns/execution/DepthFirstIterator.h"

class AwaitingApprovalIterator : public WorkIterator {
private:
    DepthFirstIterator inner_;
    std::string approverRole_;
    std::string excludeAssignee_;
    bool anyRole_;

    bool matches(WorkNode* node) const;
    void skip();

public:
    AwaitingApprovalIterator(WorkNode* root, const std::string& approverRole,
                             const std::string& excludeAssignee = "");
    static AwaitingApprovalIterator anyApprover(WorkNode* root, const std::string& excludeAssignee = "");

    void first() override;
    void next() override;
    bool isDone() const override;
    WorkNode* currentItem() const override;
};

#endif
