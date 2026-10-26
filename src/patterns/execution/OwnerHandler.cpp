#include "patterns/execution/OwnerHandler.h"
#include "core/Company.h"
#include "patterns/execution/ApprovalRequest.h"

OwnerHandler::OwnerHandler(const Company& organisation) : organisation_(organisation) {}

void OwnerHandler::handle(ApprovalRequest& request) {
    if (request.isResolved()) return;
    if (request.getPendingRole() == Company::OWNER_ROLE && organisation_.hasOwner()) {
        request.escalateTo(Company::OWNER_ROLE);
        request.resolve(true, "Needs the Owner to approve");
        return;
    }
    ApprovalHandler::handle(request);
}

std::string OwnerHandler::describe() { return "OwnerHandler"; }
