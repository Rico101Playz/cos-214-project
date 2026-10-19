#include "CommandInvoker.h"

void CommandInvoker::invoke(std::unique_ptr<WorkCommand> command) {
    if (command) {
        command->execute();
    }
}

void CommandInvoker::undoLast() {}