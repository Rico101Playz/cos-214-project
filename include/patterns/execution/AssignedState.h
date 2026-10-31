#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "WorkItemState.h"

/**
 * @class AssignedState
 * @brief Represents the state where a work item has been assigned to a participant.
 * 
 * In this state, the work item has an owner but execution has not begun.
 * It permits reassigning to another participant, starting execution to move to 
 * InProgressState, or escalating the item to EscalatedState.
 */
class AssignedState : public WorkItemState {
public:
    /**
     * @brief Gets the name of the state.
     * @return std::string The string identifier ("Assigned").
     */
    std::string name() const override;

    /**
     * @brief Reassigns the work item to a different participant while remaining in AssignedState.
     * @param[in,out] item Reference to the target LeafWorkItem being reassigned.
     * @param[in] participant Name or ID of the new participant.
     */
    void assign(LeafWorkItem& item, const std::string& participant) override;

    /**
     * @brief Transitions the work item into InProgressState to begin execution.
     * @param[in,out] item Reference to the target LeafWorkItem.
     */
    void start(LeafWorkItem& item) override;

    /**
     * @brief Escalates the work item, transitioning it into EscalatedState.
     * @param[in,out] item Reference to the target LeafWorkItem.
     */
    void escalate(LeafWorkItem& item) override;
};

#endif