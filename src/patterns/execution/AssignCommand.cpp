#include "patterns/execution/AssignCommand.h"

AssignCommand::AssignCommand(const std::string& itemId, const std::string& participantName) 
    : itemId(itemId), participantName(participantName) {}

void AssignCommand::perform() {}

std::string AssignCommand::describe() const {
    return "AssignCommand: item=" + itemId + ", participant=" + participantName;
}