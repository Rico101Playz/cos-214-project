#include "InvokeCommand.h"

InvokeCommand::InvokeCommand(const std::string& itemId, const std::string& operation)
    : itemId(itemId), operation(operation) {}

void InvokeCommand::perform() {}

std::string InvokeCommand::describe() const {
    return "InvokeCommand: item=" + itemId + ", operation=" + operation;
}