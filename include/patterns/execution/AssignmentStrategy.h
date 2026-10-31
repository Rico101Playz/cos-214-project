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
/**
 * @class AssignmentStrategy
 * @brief Abstract Strategy interface for selecting an eligible participant for a work item.
 * 
 * Defines the algorithm contract used by AssignmentService to select a concrete 
 * Participant based on required roles, operational policies, and organisational context.
 */
class AssignmentStrategy {
public:
    /**
     * @brief Virtual destructor for polymorphic cleanup.
     */
    virtual ~AssignmentStrategy() = default;

    /**
     * @brief Selects an eligible participant to handle a work item.
     * 
     * Determines a candidate participant matching the role without altering 
     * any participant's current workload.
     * 
     * @param[in] item The work item requiring assignment.
     * @param[in] role The role required to perform the item.
     * @param[in] organisation The organisation pool containing participants and roles.
     * @return Participant* Pointer to the chosen participant, or nullptr if no eligible candidate is available.
     */
    virtual Participant* assign(const WorkItem& item,
                                const std::string& role,
                                const Organisation& organisation) const = 0;

    /**
     * @brief Retrieves the descriptive name of the assignment strategy.
     * @return std::string The name of the concrete strategy policy.
     */
    virtual std::string name() const = 0;
};

#endif // ASSIGNMENTSTRATEGY_H
