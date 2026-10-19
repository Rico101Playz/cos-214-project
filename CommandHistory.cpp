#include "CommandHistory.h"

void CommandHistory::push(std::unique_ptr<WorkCommand> command) {
    executed.push_back(std::move(command));
}

std::unique_ptr<WorkCommand> CommandHistory::pop() {
    if (executed.empty()) {
        return nullptr;
    }
    auto cmd = std::move(executed.back());
    executed.pop_back();
    return cmd;
}

bool CommandHistory::canUndo() const {
    return !executed.empty();
}