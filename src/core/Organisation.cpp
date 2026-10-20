#include "core/Organisation.h"
#include <stdexcept>

Organisation::Organisation(const std::string& name) : name_(name) {}

const std::string& Organisation::name() const { return name_; }

std::size_t Organisation::size() const { return participants_.size(); }

Participant& Organisation::addParticipant(const std::string& name, const std::string& role) {
    if (find(name) != nullptr) {
        throw std::invalid_argument("Participant already exists: " + name);
    }
    participants_.push_back(std::unique_ptr<Participant>(new Participant(name, role)));
    return *participants_.back();
}

Participant* Organisation::find(const std::string& name) const {
    for (std::size_t i = 0; i < participants_.size(); ++i) {
        if (participants_[i]->name() == name) {
            return participants_[i].get();
        }
    }
    return nullptr;
}

std::vector<Participant*> Organisation::eligible(const std::string& role) const {
    std::vector<Participant*> result;
    for (std::size_t i = 0; i < participants_.size(); ++i) {
        Participant* p = participants_[i].get();
        if (p->role() == role && p->isAvailable()) {
            result.push_back(p);
        }
    }
    return result;
}
