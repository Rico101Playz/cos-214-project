#include "CompleteCommand.h"

CompleteCommand::CompleteCommand(const std::string& itemId) : itemId(itemId) {}

void CompleteCommand::perform() {}

std::string CompleteCommand::describe() const {
    return "CompleteCommand: item=" + itemId;
}