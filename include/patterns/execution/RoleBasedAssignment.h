#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include "patterns/execution/AssignmentStrategy.h"

// Picks the first available participant who holds the role ("who must handle
// me" - the Trello-style approach from the research). Ignores workload.
class RoleBasedAssignment : public AssignmentStrategy {
public:
    Participant* assign(const WorkItem& item,
                        const std::string& role,
                        const Organisation& organisation) const override;

    std::string name() const override;
};

#endif // ROLEBASEDASSIGNMENT_H
