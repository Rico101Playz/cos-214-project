#include "command/CommandInvoker.h"

CommandInvoker::CommandInvoker() : history_(&own_) {}

CommandInvoker::CommandInvoker(CommandHistory& history) : history_(&history) {}

Response CommandInvoker::invoke(std::unique_ptr<Command> command) {
    if (!command) return Response();
    Response response = command->execute();
    history_->push(std::move(command));
    return response;
}

void CommandInvoker::setLimit(std::size_t max) { history_->setLimit(max); }

std::size_t CommandInvoker::size() const { return history_->size(); }

void CommandInvoker::undoLast() {
    std::unique_ptr<Command> cmd = history_->pop();
    if (cmd) {
        cmd->undo();
    }
}

const CommandHistory& CommandInvoker::history() const { return *history_; }
