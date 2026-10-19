#include "patterns/execution/AddWorkCommand.h"

AddWorkCommand::AddWorkCommand(const std::string& stageId) : stageId(stageId) {}

void AddWorkCommand::perform() {}

std::string AddWorkCommand::describe() const {
    return "AddWorkCommand: stage=" + stageId;
}