#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include "state/TaskState.h"

/**
 * @class AvailableState
 * @brief Represents the lifecycle state where a work item is ready to be assigned.
 *
 * In this state, prerequisite dependencies have been met and the work item is
 * available in the pool. It allows assigning the item to a participant, which
 * sets the assignee and transitions the item to AssignedState.
 */
class AvailableState : public TaskState {
public:
    /**
     * @brief Gets the name of the state.
     * @return std::string The string identifier ("Available").
     */
    std::string name() const override;

    /**
     * @brief Assigns the work item to a participant and transitions to AssignedState.
     * @param[in,out] item Reference to the target LeafTask being assigned.
     * @param[in] participant Name or identifier of the participant receiving the assignment.
     */
    void assign(LeafTask& item, const std::string& participant) override;
};

#endif