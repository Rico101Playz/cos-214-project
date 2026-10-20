#include "patterns/execution/EscalateCommand.h"

EscalateCommand::EscalateCommand(const std::string& itemId) : itemId(itemId) {}

void EscalateCommand::perform() {}

std::string EscalateCommand::describe() const {
    return "EscalateCommand: item=" + itemId;
}