#ifndef AUTO_APPROVE_HANDLER_H
#define AUTO_APPROVE_HANDLER_H

#include "chain_of_responsibility/ApprovalHandler.h"

class AutoApproveHandler : public ApprovalHandler {
public:
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif