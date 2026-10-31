#ifndef APPROVAL_REQUEST_H // passed along chain
#define APPROVAL_REQUEST_H

#include <string>
#include <vector>
#include "patterns/execution/ApprovalDecision.h"

/**
 * @class ApprovalRequest
 * @brief Represents an approval request passed along a Chain of Responsibility.
 * 
 * Encapsulates the state, the required approving role, resolution outcome, 
 * and an audit history of approval decisions as the request traverses the handler chain.
 */
class ApprovalRequest {
private:
    std::string pendingRole;                  /*!< The role currently required to evaluate this request. */
    bool approved;                            /*!< Indicates whether the request has been approved. */
    bool resolved;                            /*!< Indicates whether the request has reached a terminal resolution. */
    std::vector<ApprovalDecision> decisions_; /*!< Chronological audit trail of decisions made on this request. */

public:
    /**
     * @brief Constructs an ApprovalRequest targeted at a specific initial role.
     * @param[in] role The role initially required to approve the request.
     */
    ApprovalRequest(std::string role); //

    /**
     * @brief Appends a decision record to the audit trail history.
     * @param[in] decision The ApprovalDecision record to log.
     */
    void record(ApprovalDecision decision);

    /**
     * @brief Resolves the approval request with a final status and comment.
     * @param[in] isApproved The resolution status (true if approved, false if rejected).
     * @param[in] comment Accompanying explanation or feedback for the resolution.
     */
    void resolve(bool isApproved, std::string comment);

    /**
     * @brief Checks if the request has been resolved.
     * @return true if the request is resolved, false otherwise.
     */
    bool isResolved() const;
    
    /**
     * @brief Escalates the request by reassigning the pending role to a higher authority.
     * @param[in] role The new target role to which the request is escalated.
     */
    void escalateTo(std::string role);

    // helpers
    /**
     * @brief Retrieves the role currently required to handle the request.
     * @return std::string The pending role string.
     */
    std::string getPendingRole() const;

    /**
     * @brief Checks the current approval outcome of the request.
     * @return true if approved, false otherwise.
     */
    bool isApproved() const;

    /**
     * @brief Retrieves the full history of recorded approval decisions.
     * @return const std::vector<ApprovalDecision>& A constant reference to the collection of decisions.
     */
    const std::vector<ApprovalDecision>& getDecisions() const;
};

#endif