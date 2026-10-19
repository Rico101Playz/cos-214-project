#include "CommandInvoker.h"

CommandInvoker::CommandInvoker(CommandHistory& history) : history_(history) {}

void CommandInvoker::invoke(std::unique_ptr<WorkCommand> command) {

    if (!command) return;

    command->execute();
    history_.push(std::move(command));
}

std::string CommandInvoker::undoLast() {

    if (!history_.canUndo()) {
        return "";
    }
    
    auto cmd = history_.pop();
    if (cmd) {
        cmd->undo();
        return cmd->describe(); 
    }
    return "";
    
}