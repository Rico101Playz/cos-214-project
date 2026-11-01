#include "core/Company.h"
#include "patterns/structural/OrgMember.h"
#include <stdexcept>
#include <utility>

const char* const Company::MANAGER_ROLE = "Manager";
const char* const Company::OWNER_ROLE = "Owner";

Company::Company(const std::string& name)
    : people_(new Organisation(name)), structure_(new OrgUnit("org", name)) {}

void Company::swap(Company& other) {
    people_.swap(other.people_);
    structure_.swap(other.structure_);
    order_.swap(other.order_);
}

void Company::reset(const std::string& name) {
    Company fresh(name);
    swap(fresh);
}

const std::string& Company::name() const { return people_->name(); }

std::size_t Company::size() const { return people_->size(); }

Organisation& Company::people() { return *people_; }

const Organisation& Company::people() const { return *people_; }

Participant& Company::addParticipant(const std::string& name, const std::string& role) {
    Participant& added = people_->addParticipant(name, role);
    order_.push_back(name);
    return added;
}

Participant* Company::find(const std::string& name) const { return people_->find(name); }

std::vector<Participant*> Company::participants() const {
    std::vector<Participant*> all;
    for (std::size_t i = 0; i < order_.size(); ++i) {
        all.push_back(people_->find(order_[i]));
    }
    return all;
}

bool Company::isBossRole(const std::string& role) {
    return role == MANAGER_ROLE || role == OWNER_ROLE;
}

bool Company::isOwner(const std::string& participantName) const {
    Participant* participant = find(participantName);
    return participant != nullptr && participant->role() == OWNER_ROLE;
}

bool Company::hasOwner() const {
    std::vector<Participant*> all = participants();
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i]->role() == OWNER_ROLE) return true;
    }
    return false;
}

bool Company::isManager(const std::string& participantName) const {
    Participant* participant = find(participantName);
    return participant != nullptr && isBossRole(participant->role());
}

OrgUnit& Company::structure() const { return *structure_; }

OrgUnit& Company::addUnit(const std::string& parentId, const std::string& id, const std::string& name) {
    if (id.empty() || name.empty()) {
        throw std::invalid_argument("A unit needs an id and a name");
    }
    if (structure_->find(id) != nullptr || find(id) != nullptr) {
        throw std::invalid_argument("Id already in use: " + id);
    }
    OrgUnit* parent = parentId.empty() ? structure_.get() : findUnit(parentId);
    if (parent == nullptr) {
        throw std::invalid_argument("Unknown unit: " + parentId);
    }
    return static_cast<OrgUnit&>(parent->add(std::unique_ptr<OrgComponent>(new OrgUnit(id, name))));
}

void Company::placeMember(const std::string& unitId, const std::string& participantName) {
    Participant* participant = find(participantName);
    if (participant == nullptr) {
        throw std::invalid_argument("Unknown participant: " + participantName);
    }
    OrgUnit* target = findUnit(unitId);
    if (target == nullptr) {
        throw std::invalid_argument("Unknown unit: " + unitId);
    }
    std::unique_ptr<OrgComponent> member;
    OrgUnit* current = unitOf(participantName);
    if (current != nullptr) {
        member = current->remove(participantName);
    } else {
        member.reset(new OrgMember(participant));
    }
    target->add(std::move(member));
}

OrgUnit* Company::findUnit(const std::string& id) const {
    OrgComponent* found = structure_->find(id);
    return (found != nullptr && found->isUnit()) ? static_cast<OrgUnit*>(found) : nullptr;
}

OrgUnit* Company::unitOf(const std::string& participantName) const {
    OrgComponent* found = structure_->find(participantName);
    return (found != nullptr && !found->isUnit()) ? found->parent() : nullptr;
}

bool Company::inUnit(const std::string& participantName, const std::string& unitId) const {
    OrgUnit* unit = findUnit(unitId);
    OrgComponent* member = structure_->find(participantName);
    return unit != nullptr && member != nullptr && !member->isUnit() && unit->contains(member);
}

std::vector<Participant*> Company::eligible(const std::string& role) const {
    return people_->eligible(role);
}

std::vector<Participant*> Company::membersOf(const std::string& unitId) const {
    std::vector<Participant*> pool;
    if (unitId.empty()) return participants();
    OrgUnit* unit = findUnit(unitId);
    if (unit != nullptr) unit->collectMembers(pool);
    return pool;
}

std::vector<Participant*> Company::eligible(const std::string& role, const std::string& unitId) const {
    std::vector<Participant*> pool = membersOf(unitId);
    std::vector<Participant*> result;
    for (std::size_t i = 0; i < pool.size(); ++i) {
        Participant* p = pool[i];
        if (!p->isAvailable()) continue;
        if (role.empty() ? !isBossRole(p->role()) : p->role() == role) {
            result.push_back(p);
        }
    }
    return result;
}
