#ifndef TIMEOUT_ESCALATION_HANDLER_H
#define TIMEOUT_ESCALATION_HANDLER_H

#include <string>
#include "patterns/execution/ApprovalHandler.h"

class TimeoutEscalationHandler : public ApprovalHandler {
private:
    std::string targetRole_;

public:
    TimeoutEscalationHandler(std::string targetRole = "");

    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif