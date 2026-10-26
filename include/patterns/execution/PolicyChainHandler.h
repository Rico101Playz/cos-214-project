#ifndef POLICY_CHAIN_HANDLER_H
#define POLICY_CHAIN_HANDLER_H

#include <memory>
#include "patterns/execution/ApprovalHandler.h"

class PolicyChainHandler : public ApprovalHandler {
private:
    std::unique_ptr<ApprovalHandler> policy_;

public:
    explicit PolicyChainHandler(std::unique_ptr<ApprovalHandler> policy);
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif
