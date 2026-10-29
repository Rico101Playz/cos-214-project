#ifndef AWAITINGAPPROVALSTATE_H
#define AWAITINGAPPROVALSTATE_H

#include "state/TaskState.h"

class AwaitingApprovalState : public TaskState {
public:
    std::string name() const override;
    void approve(LeafTask& item) override;
    void reject(LeafTask& item, const std::string& reason) override;
    void withdraw(LeafTask& item) override;
};

#endif