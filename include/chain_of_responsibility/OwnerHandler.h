#ifndef OWNER_HANDLER_H
#define OWNER_HANDLER_H

#include <string>
#include "chain_of_responsibility/ApprovalHandler.h"

// CHAIN OF RESPONSIBILITY pattern - a CONCRETE HANDLER.
//
// Sends a request for the owner's role to the owner. It knows only the role's
// name; an empty role means the company has no owner, so the request passes on.
class OwnerHandler : public ApprovalHandler {
private:
    std::string ownerRole_;

public:
    explicit OwnerHandler(const std::string& ownerRole);
    void handle(ApprovalRequest& request) override;
    std::string describe() override;
};

#endif
