#include "mediator/Colleague.h"
#include "mediator/Mediator.h"
#include "memento/Memento.h"

Colleague::Colleague() : mediator_(nullptr) {}

void Colleague::setMediator(Mediator* mediator) { mediator_ = mediator; }

Mediator* Colleague::mediator() const { return mediator_; }

void Colleague::changed(const Response& change) {
    if (mediator_ != nullptr) mediator_->notify(this, change);
}

std::shared_ptr<Memento> Colleague::createMemento(const TaskRef&, const std::string&) const {
    return std::shared_ptr<Memento>();
}

void Colleague::restore(const TaskRef&, Memento&) {}
