#include "patterns/execution/PolicyChainHandler.h"
#include "patterns/execution/ApprovalRequest.h"
#include <utility>

PolicyChainHandler::PolicyChainHandler(std::unique_ptr<ApprovalHandler> policy) : policy_(std::move(policy)) {}

void PolicyChainHandler::handle(ApprovalRequest& request) {
    if (!request.isResolved() && policy_) policy_->handle(request);
    ApprovalHandler::handle(request);
}

std::string PolicyChainHandler::describe() { return "PolicyChainHandler"; }
