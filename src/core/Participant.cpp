#include "core/Participant.h"

Participant::Participant(const std::string& name, const std::string& role)
    : name_(name), role_(role), load_(0), available_(true) {}

const std::string& Participant::name() const { return name_; }

const std::string& Participant::role() const { return role_; }

int Participant::load() const { return load_; }

bool Participant::isAvailable() const { return available_; }

void Participant::setAvailable(bool available) { available_ = available; }

void Participant::addLoad(int delta) {
    load_ += delta;
    if (load_ < 0) {
        load_ = 0;
    }
}
