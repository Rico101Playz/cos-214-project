#ifndef AWAITINGAPPROVALSTATE_H
#define AWAITINGAPPROVALSTATE_H

#include "WorkItemState.h"

class AwaitingApprovalState : public WorkItemState {
public:
    std::string name() const override;
    void complete(LeafWorkItem& item) override;
    void reject(LeafWorkItem& item, const std::string& reason) override;
};

#endif