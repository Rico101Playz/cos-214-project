#include "RejectCommand.h"

RejectCommand::RejectCommand(const std::string& itemId, const std::string& reason)
    : itemId(itemId), reason(reason) {}

void RejectCommand::perform() {}

std::string RejectCommand::describe() const {
    return "RejectCommand: item=" + itemId + ", reason=" + reason;
}