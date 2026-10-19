#include "CommandHistory.h"
#include "WorkCommand.h" 

size_t CommandHistory::log(std::string line) {
    log_.push_back(std::move(line));
    return log_.size();
}


void CommandHistory::relabel(size_t index, std::string line) {
    if (index < log_.size()) {
        log_[index] = std::move(line);
    }
}

void CommandHistory::push(std::unique_ptr<WorkCommand> command) {
    if (command) {
        log_.push_back(command->describe());
        executed_.push_back(std::move(command));
    }
}

std::unique_ptr<WorkCommand> CommandHistory::pop() {
    if (executed_.empty()) {
        return nullptr;
    }
    std::unique_ptr<WorkCommand> cmd = std::move(executed_.back());
    executed_.pop_back();
    return cmd;
}

void CommandHistory::truncate(size_t commandCount, size_t logCount) {
    if (commandCount < executed_.size()) {
        executed_.erase(executed_.begin() + commandCount, executed_.end());
    }

    if (logCount < log_.size()) {
        log_.erase(log_.begin() + logCount, log_.end());
    }
}

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