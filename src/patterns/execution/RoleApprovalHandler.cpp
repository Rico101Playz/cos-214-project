#include "patterns/execution/RoleApprovalHandler.h"

RoleApprovalHandler::RoleApprovalHandler(std::string roleName) : role(roleName) {}

void RoleApprovalHandler::handle(ApprovalRequest& request) {
    if (request.isResolved()) {
        return;
    }

    if (request.getPendingRole() == role) {
        request.resolve(true, "Approved by " + describe());
    } else {
        ApprovalHandler::handle(request);
    }
}

std::string RoleApprovalHandler::getRole() const {
    return role;
}