#include "decorator/TaskDecorator.h"
#include <stdexcept>
#include "composite/LeafTask.h"

TaskDecorator::TaskDecorator(std::unique_ptr<Task> inner) : inner_(std::move(inner)) {
    if (!inner_) throw std::invalid_argument("TaskDecorator: nothing to decorate");
    LeafTask* work = inner_->leaf();
    if (work != nullptr) work->setOutermost(this);
}

Task& TaskDecorator::inner() const { return *inner_; }

const std::string& TaskDecorator::id() const { return inner_->id(); }

const std::string& TaskDecorator::name() const { return inner_->name(); }

Task* TaskDecorator::parent() const { return inner_->parent(); }

void TaskDecorator::setParent(Task* parent) { inner_->setParent(parent); }

bool TaskDecorator::isComplete() const { return inner_->isComplete(); }

double TaskDecorator::progress() const { return inner_->progress(); }

void TaskDecorator::cancel() { inner_->cancel(); }

bool TaskDecorator::isReady() const { return inner_->isReady(); }

std::size_t TaskDecorator::childCount() const { return inner_->childCount(); }

Task* TaskDecorator::child(std::size_t index) const { return inner_->child(index); }

LeafTask* TaskDecorator::leaf() { return inner_->leaf(); }

ApprovalWork* TaskDecorator::approvalWork() { return inner_->approvalWork(); }

BlockedWork* TaskDecorator::blockedWork() { return inner_->blockedWork(); }

ValidatedWork* TaskDecorator::validatedWork() { return inner_->validatedWork(); }

void TaskDecorator::handle(const Request& request) { inner_->handle(request); }
