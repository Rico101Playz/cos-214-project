#ifndef MANAGER_FALLBACK_HANDLER_H
#define MANAGER_FALLBACK_HANDLER_H

#include "chain_of_responsibility/ApprovalHandler.h"

class ManagerFallbackHandler : public ApprovalHandler {
public:
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif
