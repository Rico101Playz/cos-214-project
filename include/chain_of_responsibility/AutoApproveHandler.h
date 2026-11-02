#ifndef AUTO_APPROVE_HANDLER_H
#define AUTO_APPROVE_HANDLER_H

#include "chain_of_responsibility/ApprovalHandler.h"

/**
 * @class AutoApproveHandler
 * @brief Concrete approval handler that automatically resolves low-risk or unassigned approval requests.
 * 
 * Sits at the head of the approval chain to short-circuit requests requiring an "Auto" role 
 * or an empty role, resolving them immediately without routing them to human reviewers.
 */
class AutoApproveHandler : public ApprovalHandler {
public:
    /**
     * @brief Evaluates an approval request and resolves it if eligible for automated approval.
     * 
     * If the request is already resolved, no action is taken. If the pending role is "Auto" 
     * or empty, the request is resolved as approved; otherwise, it is passed down the chain.
     * 
     * @param[in,out] request The approval request being processed through the chain.
     */
    void handle(ApprovalRequest& request) override;

    /**
     * @brief Retrieves the descriptive name of the handler.
     * @return std::string The handler's class name ("AutoApproveHandler").
     */
    std::string describe() override;
};

#endif