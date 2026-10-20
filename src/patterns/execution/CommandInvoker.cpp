#include "patterns/execution/CommandInvoker.h"

void CommandInvoker::invoke(std::unique_ptr<WorkCommand> command, WorkflowInstance* instance) {
    if (command) {
        command->execute(instance);
        history.push_back(std::move(command));
    }
}

void CommandInvoker::undoLast(WorkflowInstance* instance) {
    if (!history.empty()) {
        auto cmd = std::move(history.back());
        history.pop_back();
        cmd->undo(instance);
    }
}