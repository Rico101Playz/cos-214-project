#include "composite/Task.h"
#include <stdexcept>
#include "iterator/DepthFirstIterator.h"

LeafTask* Task::leaf() { return nullptr; }

const LeafTask* Task::leaf() const { return const_cast<Task*>(this)->leaf(); }

ApprovalWork* Task::approvalWork() { return nullptr; }

BlockedWork* Task::blockedWork() { return nullptr; }

ValidatedWork* Task::validatedWork() { return nullptr; }

void Task::handle(const Request&) {
    throw std::invalid_argument(id() + " is a stage, not a task that can take this request");
}

Task* Task::find(const std::string& id) {
    if (this->id() == id) {
        return this;
    }
    for (std::size_t i = 0; i < childCount(); ++i) {
        Task* found = child(i)->find(id);
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

std::unique_ptr<WorkIterator> Task::createIterator() {
    return std::unique_ptr<WorkIterator>(new DepthFirstIterator(this));
}

LeafTask* leafOf(Task* task) { return task == nullptr ? nullptr : task->leaf(); }

const LeafTask* leafOf(const Task* task) { return task == nullptr ? nullptr : task->leaf(); }
