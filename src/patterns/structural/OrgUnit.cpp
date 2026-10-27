#include "patterns/structural/OrgUnit.h"
#include <stdexcept>
#include <utility>

OrgUnit::OrgUnit(const std::string& id, const std::string& name) : OrgComponent(id, name) {}

bool OrgUnit::isUnit() const { return true; }

void OrgUnit::collectMembers(std::vector<Participant*>& out) const {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        children_[i]->collectMembers(out);
    }
}

std::size_t OrgUnit::childCount() const { return children_.size(); }

OrgComponent* OrgUnit::child(std::size_t index) const {
    return index < children_.size() ? children_[index].get() : nullptr;
}

OrgComponent& OrgUnit::add(std::unique_ptr<OrgComponent> child) {
    if (!child) {
        throw std::invalid_argument("Cannot add a null component to unit " + id());
    }
    child->setParent(this);
    children_.push_back(std::move(child));
    return *children_.back();
}

std::unique_ptr<OrgComponent> OrgUnit::remove(const std::string& id) {
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i]->id() == id) {
            std::unique_ptr<OrgComponent> removed = std::move(children_[i]);
            children_.erase(children_.begin() + i);
            removed->setParent(nullptr);
            return removed;
        }
    }
    return std::unique_ptr<OrgComponent>();
}

std::vector<OrgUnit*> OrgUnit::subUnits() const {
    std::vector<OrgUnit*> units;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i]->isUnit()) {
            units.push_back(static_cast<OrgUnit*>(children_[i].get()));
        }
    }
    return units;
}
