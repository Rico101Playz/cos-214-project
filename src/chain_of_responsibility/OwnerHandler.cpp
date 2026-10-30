#include "chain_of_responsibility/OwnerHandler.h"
#include "chain_of_responsibility/ApprovalRequest.h"

OwnerHandler::OwnerHandler(const std::string& ownerRole) : ownerRole_(ownerRole) {}

void OwnerHandler::handle(ApprovalRequest& request) {
    if (request.isResolved()) return;
    if (!ownerRole_.empty() && request.getPendingRole() == ownerRole_) {
        request.escalateTo(ownerRole_);
        request.resolve(true, "Needs the Owner to approve");
        return;
    }
    ApprovalHandler::handle(request);
}

std::string OwnerHandler::describe() { return "OwnerHandler"; }
