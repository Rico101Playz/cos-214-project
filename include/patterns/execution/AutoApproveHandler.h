#ifndef AUTO_APPROVE_HANDLER_H
#define AUTO_APPROVE_HANDLER_H

#include "patterns/execution/ApprovalHandler.h"

class AutoApproveHandler : public ApprovalHandler {
public:
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif