#include "patterns/execution/ApprovalRequest.h"

ApprovalRequest::ApprovalRequest(std::string role)
    : pendingRole(role), approved(false), resolved(false) {}

void ApprovalRequest::record(ApprovalDecision decision) {
    decisions_.push_back(decision);
}

void ApprovalRequest::resolve(bool isApproved, std::string comment) {
    approved = isApproved;
    resolved = true;
    record(ApprovalDecision("System", pendingRole, isApproved, comment));
}

bool ApprovalRequest::isResolved() const {
    return resolved;
}

void ApprovalRequest::escalateTo(std::string role) {
    pendingRole = role;
    resolved = false;
}

std::string ApprovalRequest::getPendingRole() const {
    return pendingRole;
}

bool ApprovalRequest::isApproved() const {
    return approved;
}

const std::vector<ApprovalDecision>& ApprovalRequest::getDecisions() const {
    return decisions_;
}