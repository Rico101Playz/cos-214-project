#include "ApproveCommand.h"

ApproveCommand::ApproveCommand(const std::string& itemId, bool approved, const std::string& comment)
    : itemId(itemId), approved(approved), comment(comment) {}

void ApproveCommand::perform() {}

std::string ApproveCommand::describe() const {
    return "ApproveCommand: item=" + itemId + ", approved=" + (approved ? "true" : "false") + ", comment=" + comment;
}