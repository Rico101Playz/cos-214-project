#ifndef APPROVAL_WORK_H
#define APPROVAL_WORK_H

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "decorator/TaskDecorator.h"

// DECORATOR pattern - a CONCRETE DECORATOR.
//
// Adds sign-off to a task: the roles that must approve it, the approvals it
// is still waiting for, and the signatures it has collected. Requests pass
// through it to the task:
//   RouteApproval  sets the roles still pending (and any automatic signatures);
//   Sign           records one role's signature;
//   Approve        lets the task approve, then clears what was pending;
//   Reject/Withdraw lets the task act, then clears the approval round.
// Every task in a project wears one, so its approval data never lives on the
// task itself.
class ApprovalWork : public TaskDecorator {
public:
    typedef std::pair<std::string, std::string> Signature;   // role, person

private:
    std::vector<std::string> approverRoles_;
    std::vector<std::string> pendingApprovers_;
    std::vector<Signature> signatures_;

public:
    explicit ApprovalWork(std::unique_ptr<Task> inner);

    std::string approverRole() const;
    void setApproverRole(const std::string& role);
    const std::vector<std::string>& approverRoles() const;
    void setApproverRoles(const std::vector<std::string>& roles);
    std::string pendingApprover() const;
    void setPendingApprover(const std::string& role);
    const std::vector<std::string>& pendingApprovers() const;
    void setPendingApprovers(const std::vector<std::string>& roles);
    // Takes the first pending slot for the role; false when none is pending.
    bool takePendingSlot(const std::string& role);
    const std::vector<Signature>& signatures() const;
    void addSignature(const std::string& role, const std::string& person);
    bool hasSigned(const std::string& person) const;
    void clearApprovals();

    ApprovalWork* approvalWork() override;
    void handle(const Request& request) override;
};

#endif // APPROVAL_WORK_H
