#include "patterns/execution/CommandInvoker.h"

void CommandInvoker::invoke(std::unique_ptr<WorkCommand> command) {
    if (command) {
        command->execute();
        history.push_back(std::move(command));
    }
}

void CommandInvoker::undoLast() {
    if (!history.empty()) {

        auto cmd = std::move(history.back());
        history.pop_back();
        
        cmd->undo(); 
    }
}