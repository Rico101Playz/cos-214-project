#include "patterns/execution/ManagerFallbackHandler.h"
#include "core/Company.h"
#include "patterns/execution/ApprovalRequest.h"

void ManagerFallbackHandler::handle(ApprovalRequest& request) {
    if (request.isResolved()) return;
    request.escalateTo(Company::MANAGER_ROLE);
    request.resolve(true, "Needs a Manager to approve");
}

std::string ManagerFallbackHandler::describe() { return "ManagerFallbackHandler"; }
