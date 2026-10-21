#include "patterns/execution/ReleaseManagerHandler.h"

ReleaseManagerHandler::ReleaseManagerHandler() : RoleApprovalHandler("ReleaseManager") {}

std::string ReleaseManagerHandler::describe() {
    return "ReleaseManagerHandler";
}