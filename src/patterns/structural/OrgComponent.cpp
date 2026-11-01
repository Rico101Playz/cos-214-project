#include "patterns/structural/OrgComponent.h"
#include "patterns/structural/OrgUnit.h"

OrgComponent::OrgComponent(const std::string& id, const std::string& name)
    : id_(id), name_(name), parent_(nullptr) {}

const std::string& OrgComponent::id() const { return id_; }

const std::string& OrgComponent::name() const { return name_; }

OrgUnit* OrgComponent::parent() const { return parent_; }

void OrgComponent::setParent(OrgUnit* parent) { parent_ = parent; }

std::size_t OrgComponent::childCount() const { return 0; }

OrgComponent* OrgComponent::child(std::size_t) const { return nullptr; }

OrgComponent* OrgComponent::find(const std::string& id) {
    if (id_ == id) return this;
    for (std::size_t i = 0; i < childCount(); ++i) {
        OrgComponent* found = child(i)->find(id);
        if (found != nullptr) return found;
    }
    return nullptr;
}

bool OrgComponent::contains(const OrgComponent* other) const {
    for (const OrgComponent* cursor = other; cursor != nullptr; cursor = cursor->parent()) {
        if (cursor == this) return true;
    }
    return false;
}

std::string OrgComponent::path() const {
    if (parent_ == nullptr || parent_->parent() == nullptr) return name_;
    return parent_->path() + " / " + name_;
}
