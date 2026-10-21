#ifndef APPROVAL_REQUEST_H // passed along chain
#define APPROVAL_REQUEST_H

#include <string>
#include <vector>
#include "patterns/execution/ApprovalDecision.h"

class ApprovalRequest {
private:
    std::string pendingRole;
    bool approved;
    bool resolved;
    std::vector<ApprovalDecision> decisions_;

public:
    ApprovalRequest(std::string role); //

    void record(ApprovalDecision decision);
    void resolve(bool isApproved, std::string comment);
    bool isResolved() const;
    void escalateTo(std::string role);

    // helpers
    std::string getPendingRole() const;
    bool isApproved() const;
    const std::vector<ApprovalDecision>& getDecisions() const;
};

#endif