#ifndef RELEASE_MANAGER_HANDLER_H
#define RELEASE_MANAGER_HANDLER_H

#include "patterns/execution/RoleApprovalHandler.h"

class ReleaseManagerHandler : public RoleApprovalHandler {
public:
    ReleaseManagerHandler();
    std::string describe() override;
};

#endif