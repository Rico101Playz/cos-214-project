#include "patterns/execution/ReviewerHandler.h"

ReviewerHandler::ReviewerHandler() : RoleApprovalHandler("Reviewer") {}

std::string ReviewerHandler::describe() {
    return "ReviewerHandler";
}