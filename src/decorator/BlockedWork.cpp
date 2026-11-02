#include "decorator/BlockedWork.h"
#include <stdexcept>
#include "mediator/Request.h"

BlockedWork::BlockedWork(std::unique_ptr<Task> inner) : TaskDecorator(std::move(inner)) {}

const std::vector<std::string>& BlockedWork::blockedBy() const { return blockedBy_; }

void BlockedWork::addBlocker(const std::string& taskId) {
    if (taskId.empty() || taskId == id()) {
        throw std::invalid_argument("An item cannot block itself");
    }
    for (std::size_t i = 0; i < blockedBy_.size(); ++i) {
        if (blockedBy_[i] == taskId) return;
    }
    blockedBy_.push_back(taskId);
}

std::vector<std::string> BlockedWork::openBlockers() const {
    std::vector<std::string> open;
    Task* root = const_cast<BlockedWork*>(this);
    while (root->parent() != nullptr) {
        root = root->parent();
    }
    for (std::size_t i = 0; i < blockedBy_.size(); ++i) {
        Task* blocker = root->find(blockedBy_[i]);
        if (blocker != nullptr && !blocker->isComplete()) {
            open.push_back(blockedBy_[i]);
        }
    }
    return open;
}

bool BlockedWork::isBlocked() const { return !openBlockers().empty(); }

BlockedWork* BlockedWork::blockedWork() { return this; }

void BlockedWork::handle(const Request& request) {
    Action action = request.action();
    if ((action == Action::Assign || action == Action::Start || action == Action::Complete) && isBlocked()) {
        std::vector<std::string> open = openBlockers();
        std::string list;
        for (std::size_t i = 0; i < open.size(); ++i) list += (i ? ", " : "") + open[i];
        throw std::logic_error(id() + " is blocked by " + list);
    }
    inner_->handle(request);
}
