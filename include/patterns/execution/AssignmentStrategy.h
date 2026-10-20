#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <string>
#include "core/Organisation.h"
#include "core/Participant.h"
#include "core/WorkItem.h"

// STRATEGY pattern - the interface every assignment policy implements.
//
// A workflow definition only says WHICH ROLE must do an item. How a concrete
// person is picked for that role differs between organisations (first
// available, least busy, rule driven), so each policy is a swappable object.
//
// assign() never changes anyone's load. It only CHOOSES; recording the
// assignment (and adding load) is the job of whoever performs the assignment.
// Returns nullptr when nobody is eligible, so the caller can wait for the
// role to become available (activity A04).
class AssignmentStrategy {
public:
    virtual ~AssignmentStrategy() = default;

    virtual Participant* assign(const WorkItem& item,
                                const std::string& role,
                                const Organisation& organisation) const = 0;

    virtual std::string name() const = 0;
};

#endif // ASSIGNMENTSTRATEGY_H
