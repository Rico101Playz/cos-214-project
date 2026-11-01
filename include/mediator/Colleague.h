#ifndef COLLEAGUE_H
#define COLLEAGUE_H

#include <memory>
#include <string>
#include "mediator/Request.h"
#include "mediator/Response.h"

class Mediator;
class Memento;

// MEDIATOR pattern - the COLLEAGUE.
//
// One of the facades behind the mediator. It performs the Requests aimed at
// its own part of the engine (handle) and reports what changed to the
// mediator (changed -> Mediator::notify); it never talks to another
// colleague. COMMAND pattern: a colleague is also the receiver a
// RequestCommand runs against, so it can save and put back the state a
// Request in a given scope may change.
class Colleague {
private:
    Mediator* mediator_;

protected:
    // Tells the mediator what a Request changed (nothing happens before setMediator()).
    void changed(const Response& change);

public:
    Colleague();
    virtual ~Colleague() = default;

    void setMediator(Mediator* mediator);
    Mediator* mediator() const;

    // Performs a Request. Throws std::invalid_argument when it names something
    // unknown and std::logic_error when the action is not allowed right now.
    virtual Response handle(const Request& request) = 0;

    // The state a Request in `scope` may change, and putting it back; a
    // colleague keeps none by default.
    virtual std::shared_ptr<Memento> createMemento(const TaskRef& scope, const std::string& label) const;
    virtual void restore(const TaskRef& scope, Memento& memento);
};

#endif // COLLEAGUE_H
