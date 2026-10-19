#include "StartCommand.h"

StartCommand::StartCommand(const std::string& itemId) : itemId(itemId) {}

void StartCommand::perform() {}

std::string StartCommand::describe() const {
    return "StartCommand: item=" + itemId;
}