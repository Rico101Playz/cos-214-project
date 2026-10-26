#ifndef LEASTLOADEDASSIGNMENT_H
#define LEASTLOADEDASSIGNMENT_H

#include "patterns/execution/AssignmentStrategy.h"

// Picks the available participant with the role who currently has the fewest
// assigned items, so work spreads across a busy team. When two people have the
// same load, the one registered first wins (deterministic, easy to test).
class LeastLoadedAssignment : public AssignmentStrategy {
public:
    Participant* assign(const WorkItem& item,
                        const std::string& role,
                        const Organisation& organisation) const override;

    std::string name() const override;
};

#endif // LEASTLOADEDASSIGNMENT_H
