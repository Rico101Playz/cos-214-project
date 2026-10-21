#ifndef REVIEWER_HANDLER_H
#define REVIEWER_HANDLER_H

#include "patterns/execution/RoleApprovalHandler.h"

class ReviewerHandler : public RoleApprovalHandler {
public:
    ReviewerHandler();
    std::string describe() override;
};

#endif