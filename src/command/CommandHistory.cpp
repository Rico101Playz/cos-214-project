#include "command/CommandHistory.h"

void CommandHistory::push(std::unique_ptr<Command> command) {
    executed.push_back(std::move(command));
    setLimit(limit);
}

std::unique_ptr<Command> CommandHistory::pop() {
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

std::size_t CommandHistory::size() const {
    return executed.size();
}

void CommandHistory::setLimit(std::size_t max) {
    limit = max;
    if (limit > 0 && executed.size() > limit) {
        executed.erase(executed.begin(), executed.begin() + (executed.size() - limit));
    }
}
