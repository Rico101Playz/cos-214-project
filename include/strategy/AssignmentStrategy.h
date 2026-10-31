#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <map>
#include <string>
#include "core/Organisation.h"
#include "org_composite/Participant.h"

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
// What a strategy may know about the work it assigns: the task's data
// (unit, risk, kind, ...). Strategies never see the task itself, so the
// organisation side shares no class with the project side.
typedef std::map<std::string, std::string> TaskFacts;

class AssignmentStrategy {
public:
    virtual ~AssignmentStrategy() = default;

    virtual Participant* assign(const TaskFacts& facts,
                                const std::string& role,
                                const Organisation& organisation) const = 0;

    virtual std::string name() const = 0;
};

#endif // ASSIGNMENTSTRATEGY_H
