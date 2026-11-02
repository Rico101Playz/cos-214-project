#ifndef ASSIGNMENTSERVICE_H
#define ASSIGNMENTSERVICE_H

#include <map>
#include <memory>
#include <string>
#include "core/Organisation.h"
#include "strategy/AssignmentStrategy.h"

// STRATEGY pattern - the CONTEXT.
//
// The rest of the engine asks this service "who should do this item?" and never
// touches a concrete strategy. The strategy can be swapped while workflows are
// running (setStrategy), e.g. when a team grows from role-based to least-loaded
// assignment. It looks people up in the Organisation it was created for.
//
// Class diagram: WorkflowInstance owns one AssignmentService (assignment_) and
// the service owns its current strategy (strategy_).
/**
 * @class AssignmentService
 * @brief Context class for the Strategy pattern that manages participant assignment.
 *
 * Provides an interface for the workflow engine to query eligible assignees for tasks
 * without binding directly to concrete assignment algorithms. Looks people up in the
 * target Organisation and owns a swappable AssignmentStrategy instance.
 */
class AssignmentService {
private:
    const Organisation* organisation_;      // who chooseAssignee() looks at; null when only strategies are kept
    std::unique_ptr<AssignmentStrategy> strategy_; /*!< The current assignment algorithm strategy. */
    std::map<std::string, std::unique_ptr<AssignmentStrategy> > policies_;

public:
    // Starts with RoleBasedAssignment so a strategy is always present.
    AssignmentService();
    /**
     * @brief Constructs an AssignmentService initialized with a default RoleBasedAssignment strategy.
     * @param[in] organisation The organisation containing participants and roles.
     */
    explicit AssignmentService(const Organisation& organisation);

    // Throws std::invalid_argument for a null strategy (the current one is kept).
    /**
     * @brief Swaps the active assignment strategy at runtime.
     * @param[in] strategy Unique pointer to the new AssignmentStrategy to apply.
     * @throws std::invalid_argument if the provided strategy pointer is null.
     */
    void setStrategy(std::unique_ptr<AssignmentStrategy> strategy);

    /**
     * @brief Retrieves the active assignment strategy.
     * @return const AssignmentStrategy& Reference to the current assignment strategy.
     */
    const AssignmentStrategy& strategy() const;
    // The strategy a policy uses (e.g. "BusyTeam"); the current strategy for a
    // policy with none of its own. Throws std::invalid_argument for a null strategy.
    void setStrategy(const std::string& policy, std::unique_ptr<AssignmentStrategy> strategy);
    const AssignmentStrategy& strategyFor(const std::string& policy) const;

    // Chooses a participant for the role; nullptr when nobody is eligible.
    // Does not change anyone's load - the caller records the assignment.
    /**
     * @brief Selects an eligible participant for a given work item and required role.
     *
     * Delegates candidate selection to the active strategy without altering participant workloads.
     *
     * @param[in] facts The data of the work item requiring assignment.
     * @param[in] role The role required to perform the item.
     * @return Participant* Pointer to the selected participant, or nullptr if no candidate is eligible.
     */
    Participant* chooseAssignee(const TaskFacts& facts, const std::string& role) const;
};

#endif // ASSIGNMENTSERVICE_H
