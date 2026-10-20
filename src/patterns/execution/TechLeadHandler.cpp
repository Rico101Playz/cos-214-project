#include "patterns/execution/TechLeadHandler.h"

TechLeadHandler::TechLeadHandler() : RoleApprovalHandler("TechLead") {}

std::string TechLeadHandler::describe() {
    return "TechLeadHandler";
}