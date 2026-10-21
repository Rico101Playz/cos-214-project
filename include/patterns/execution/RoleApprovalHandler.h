#ifndef ROLE_APPROVAL_HANDLER_H
#define ROLE_APPROVAL_HANDLER_H

#include <string>
#include "patterns/execution/ApprovalHandler.h"

class RoleApprovalHandler : public ApprovalHandler {
private:
    std::string role;

public:
    RoleApprovalHandler(std::string roleName);
    virtual ~RoleApprovalHandler() = default;

    void handle(ApprovalRequest& request) override;
    std::string getRole() const;
};

#endif