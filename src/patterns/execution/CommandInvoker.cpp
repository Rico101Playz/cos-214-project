#include "patterns/execution/CommandInvoker.h"

void CommandInvoker::invoke(std::unique_ptr<WorkCommand> command, WorkflowInstance* instance) {
    if (command) {
        command->execute(instance);
        history.push_back(std::move(command));
        if (limit > 0 && history.size() > limit) {
            history.erase(history.begin(), history.begin() + (history.size() - limit));
        }
    }
}

void CommandInvoker::setLimit(std::size_t max) {
    limit = max;
    if (limit > 0 && history.size() > limit) {
        history.erase(history.begin(), history.begin() + (history.size() - limit));
    }
}

std::size_t CommandInvoker::size() const {
    return history.size();
}

void CommandInvoker::undoLast(WorkflowInstance* instance) {
    if (!history.empty()) {
        auto cmd = std::move(history.back());
        history.pop_back();
        cmd->undo(instance);
    }
}