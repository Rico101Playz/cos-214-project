#include "patterns/structural/CompositeStage.h"
#include "patterns/execution/ParallelMode.h"
#include <stdexcept>

CompositeStage::CompositeStage(const std::string& id, const std::string& name)
    : WorkNode(id, name) {}

WorkNode& CompositeStage::add(std::unique_ptr<WorkNode> child) {
    if (!child) {
        throw std::invalid_argument("Cannot add a null child to stage " + id());
    }
    child->setParent(this);
    children_.push_back(std::move(child));
    return *children_.back();
}

bool CompositeStage::remove(const std::string& id) {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i]->id() == id) {
            children_.erase(children_.begin() + i);
            return true;
        }
    }
    return false;
}

std::size_t CompositeStage::childCount() const { return children_.size(); }

WorkNode* CompositeStage::child(std::size_t index) const {
    if (index >= children_.size()) {
        return nullptr;
    }
    return children_[index].get();
}

bool CompositeStage::isComplete() const {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (!children_[i]->isComplete()) {
            return false;
        }
    }
    return true;
}

double CompositeStage::progress() const {
    if (children_.empty()) {
        return 1.0;
    }
    double total = 0.0;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        total += children_[i]->progress();
    }
    return total / children_.size();
}

void CompositeStage::cancel() {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        children_[i]->cancel();
    }
}

StageMode& CompositeStage::mode() {
    if (!mode_) mode_.reset(new ParallelMode());   
    return *mode_;
}

void CompositeStage::setMode(std::unique_ptr<StageMode> mode) { mode_ = std::move(mode); }

void CompositeStage::activate() { mode().activate(*this); }

void CompositeStage::childCompleted(WorkNode& child) {
    mode().childCompleted(*this, child);
    if (isComplete()) {                          
        CompositeStage* up = dynamic_cast<CompositeStage*>(parent());
        if (up != nullptr) up->childCompleted(*this);
    }
}
