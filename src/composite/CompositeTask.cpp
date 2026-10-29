#include "composite/CompositeTask.h"
#include "strategy/ParallelMode.h"
#include "composite/LeafTask.h"
#include <stdexcept>

CompositeTask::CompositeTask(const std::string& id, const std::string& name)
    : TaskNode(id, name) {}

CompositeTask::CompositeTask(std::unique_ptr<Task> work)
    : TaskNode(work ? work->id() : std::string(), work ? work->name() : std::string()), work_(std::move(work)) {
    if (!work_ || work_->leaf() == nullptr) {
        throw std::invalid_argument("Only a task can get sub-tasks");
    }
    TaskNode::setParent(work_->parent());
    work_->leaf()->setOutermost(this);
}

bool CompositeTask::isStage() const { return !work_; }

Task& CompositeTask::addSubtask(LeafTask& parent, std::unique_ptr<Task> subtask) {
    Task& node = parent.outermost();
    CompositeTask* group = dynamic_cast<CompositeTask*>(&node);
    if (group == nullptr) {
        CompositeTask* holder = dynamic_cast<CompositeTask*>(node.parent());
        if (holder == nullptr) {
            throw std::logic_error(parent.id() + " is not in a project, so it cannot get sub-tasks");
        }
        group = &holder->groupChild(node);
    }
    return group->add(std::move(subtask));
}

Task& CompositeTask::add(std::unique_ptr<Task> child) {
    if (!child) {
        throw std::invalid_argument("Cannot add a null child to stage " + id());
    }
    child->setParent(this);
    children_.push_back(std::move(child));
    return *children_.back();
}

bool CompositeTask::remove(const std::string& id) {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i]->id() == id) {
            children_.erase(children_.begin() + i);
            return true;
        }
    }
    return false;
}

CompositeTask& CompositeTask::groupChild(Task& child) {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i].get() == &child) {
            std::unique_ptr<Task> work(std::move(children_[i]));
            children_[i].reset(new CompositeTask(std::move(work)));
            children_[i]->setParent(this);
            return static_cast<CompositeTask&>(*children_[i]);
        }
    }
    throw std::invalid_argument(child.id() + " is not a child of " + id());
}

void CompositeTask::setParent(Task* parent) {
    TaskNode::setParent(parent);
    if (work_) work_->setParent(parent);
}

bool CompositeTask::isReady() const { return work_ ? work_->isReady() : false; }

LeafTask* CompositeTask::leaf() { return work_ ? work_->leaf() : nullptr; }

ApprovalWork* CompositeTask::approvalWork() { return work_ ? work_->approvalWork() : nullptr; }

BlockedWork* CompositeTask::blockedWork() { return work_ ? work_->blockedWork() : nullptr; }

ValidatedWork* CompositeTask::validatedWork() { return work_ ? work_->validatedWork() : nullptr; }

void CompositeTask::handle(const Request& request) {
    if (!work_) {
        Task::handle(request);
        return;
    }
    work_->handle(request);
}

std::size_t CompositeTask::childCount() const { return children_.size(); }

Task* CompositeTask::child(std::size_t index) const {
    if (index >= children_.size()) {
        return nullptr;
    }
    return children_[index].get();
}

bool CompositeTask::isComplete() const {
    if (work_) return work_->isComplete();
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (!children_[i]->isComplete()) {
            return false;
        }
    }
    return true;
}

double CompositeTask::progress() const {
    if (work_) return work_->progress();
    if (children_.empty()) {
        return 1.0;
    }
    double total = 0.0;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        total += children_[i]->progress();
    }
    return total / children_.size();
}

void CompositeTask::cancel() {
    if (work_) {
        work_->cancel();
        return;
    }
    for (std::size_t i = 0; i < children_.size(); ++i) {
        children_[i]->cancel();
    }
}

StageMode& CompositeTask::mode() {
    if (!mode_) mode_.reset(new ParallelMode());   
    return *mode_;
}

void CompositeTask::setMode(std::unique_ptr<StageMode> mode) { mode_ = std::move(mode); }

void CompositeTask::activate() { mode().activate(*this); }

void CompositeTask::childCompleted(Task& child) {
    mode().childCompleted(*this, child);
    if (isComplete()) {                          
        CompositeTask* up = stageOf(parent());
        if (up != nullptr) up->childCompleted(*this);
    }
}

CompositeTask* stageOf(Task* node) {
    CompositeTask* stage = dynamic_cast<CompositeTask*>(node);
    return stage != nullptr && stage->isStage() ? stage : nullptr;
}
