#ifndef ASSIGNMENTSERVICE_H
#define ASSIGNMENTSERVICE_H

#include <memory>
#include <string>
#include "core/Organisation.h"
#include "patterns/execution/AssignmentStrategy.h"

// STRATEGY pattern - the CONTEXT.
//
// The rest of the engine asks this service "who should do this item?" and never
// touches a concrete strategy. The strategy can be swapped while workflows are
// running (setStrategy), e.g. when a team grows from role-based to least-loaded
// assignment. It looks people up in the Organisation it was created for.
//
// Class diagram: WorkflowInstance owns one AssignmentService (assignment_) and
// the service owns its current strategy (strategy_).
class AssignmentService {
private:
    const Organisation& organisation_;
    std::unique_ptr<AssignmentStrategy> strategy_;

public:
    // Starts with RoleBasedAssignment so a strategy is always present.
    explicit AssignmentService(const Organisation& organisation);

    // Throws std::invalid_argument for a null strategy (the current one is kept).
    void setStrategy(std::unique_ptr<AssignmentStrategy> strategy);

    const AssignmentStrategy& strategy() const;

    // Chooses a participant for the role; nullptr when nobody is eligible.
    // Does not change anyone's load - the caller records the assignment.
    Participant* chooseAssignee(const WorkItem& item, const std::string& role) const;
};

#endif // ASSIGNMENTSERVICE_H
