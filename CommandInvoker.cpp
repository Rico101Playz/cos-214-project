#include "CommandInvoker.h"

CommandInvoker::CommandInvoker(CommandHistory& history) : history_(history) {}

void CommandInvoker::invoke(std::unique_ptr<WorkCommand> command) {}

std::string CommandInvoker::undoLast() {}