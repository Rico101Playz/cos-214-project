#include "CommandHistory.h"
#include "WorkCommand.h" 

size_t CommandHistory::log(std::string line) {}


void CommandHistory::relabel(size_t index, std::string line) {}

void CommandHistory::push(std::unique_ptr<WorkCommand> command) {}

std::unique_ptr<WorkCommand> CommandHistory::pop() {}

void CommandHistory::truncate(size_t commandCount, size_t logCount) {}

std::vector<std::string> CommandHistory::entries() {
    return log_;
}

size_t CommandHistory::logSize() {
    return log_.size();
}

size_t CommandHistory::size() {
    return executed_.size();
}

bool CommandHistory::canUndo() {
    return !executed_.empty();
}