#include "patterns/execution/AutoApproveHandler.h"

void AutoApproveHandler::handle(ApprovalRequest& request) {
    if (request.isResolved()) {
        return;
    }

    if (request.getPendingRole() == "Auto" || request.getPendingRole().empty()) {
        request.resolve(true, "Automatically approved by AutoApproveHandler");
    } else {
        // pass it to the next handler in the chain
        ApprovalHandler::handle(request);
    }
}

std::string AutoApproveHandler::describe() {
    return "AutoApproveHandler";
}