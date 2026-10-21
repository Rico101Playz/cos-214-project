#ifndef TECH_LEAD_HANDLER_H
#define TECH_LEAD_HANDLER_H

#include "patterns/execution/RoleApprovalHandler.h"

class TechLeadHandler : public RoleApprovalHandler {
public:
    TechLeadHandler();
    std::string describe() override;
};

#endif