#include "CancelCommand.h"

CancelCommand::CancelCommand(const std::string& nodeId, const std::string& reason)
    : nodeId(nodeId), reason(reason) {}

void CancelCommand::perform() {}

std::string CancelCommand::describe() const {
    return "CancelCommand: node=" + nodeId + ", reason=" + reason;
}