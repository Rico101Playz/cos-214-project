#ifndef AWAITINGAPPROVALSTATE_H
#define AWAITINGAPPROVALSTATE_H

#include "WorkItemState.h"

class AwaitingApprovalState : public WorkItemState {
public:
    std::string name() const override;
    void approve(LeafWorkItem& item) override;
    void reject(LeafWorkItem& item, const std::string& reason) override;
    void withdraw(LeafWorkItem& item) override;
};

#endif