#ifndef ROLE_APPROVAL_HANDLER_H
#define ROLE_APPROVAL_HANDLER_H

#include <string>
#include "chain_of_responsibility/ApprovalHandler.h"

class RoleApprovalHandler : public ApprovalHandler {
private:
    std::string role;

public:
    RoleApprovalHandler(std::string roleName);
    virtual ~RoleApprovalHandler() = default;

    void handle(ApprovalRequest& request) override;
    std::string describe() override;
    std::string getRole() const;
};

#endif