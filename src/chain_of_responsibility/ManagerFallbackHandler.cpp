#include "chain_of_responsibility/ManagerFallbackHandler.h"
#include "org_composite/Company.h"
#include "chain_of_responsibility/ApprovalRequest.h"

void ManagerFallbackHandler::handle(ApprovalRequest& request) {
    if (request.isResolved()) return;
    request.escalateTo(Company::MANAGER_ROLE);
    request.resolve(true, "Needs a Manager to approve");
}

std::string ManagerFallbackHandler::describe() { return "ManagerFallbackHandler"; }
