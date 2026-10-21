#include "patterns/execution/TimeoutEscalationHandler.h"

TimeoutEscalationHandler::TimeoutEscalationHandler(std::string targetRole)
    : targetRole_(targetRole) {}

void TimeoutEscalationHandler::handle(ApprovalRequest& request) {

    
    if (request.isResolved()) {
        return;
    }

    if (request.getPendingRole() == "Timeout" && !targetRole_.empty()) {
        request.escalateTo(targetRole_);
    }

    ApprovalHandler::handle(request);
}

std::string TimeoutEscalationHandler::describe() {
    return "TimeoutEscalationHandler";
}