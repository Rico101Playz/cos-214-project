#ifndef MANAGER_FALLBACK_HANDLER_H
#define MANAGER_FALLBACK_HANDLER_H

#include "patterns/execution/ApprovalHandler.h"

class ManagerFallbackHandler : public ApprovalHandler {
public:
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif
