#ifndef AWAITINGAPPROVALSTATE_H
#define AWAITINGAPPROVALSTATE_H

#include "state/TaskState.h"

/**
 * @class AwaitingApprovalState
 * @brief Represents the lifecycle state where a completed work item is pending review or sign-off.
 *
 * In this state, work execution is finished, but approval is required to proceed.
 * It permits approving the item (transitioning it to CompletedState) or rejecting
 * it with a recorded reason (transitioning it to RejectedState).
 */
class AwaitingApprovalState : public TaskState {
public:
    /**
     * @brief Gets the name of the state.
     * @return std::string The string identifier ("AwaitingApproval").
     */
    std::string name() const override;

    /**
     * @brief Approves the work item and transitions it into CompletedState.
     * @param[in,out] item Reference to the target LeafTask being approved.
     */
    void approve(LeafTask& item) override;

    /**
     * @brief Rejects the work item with an explanatory reason and transitions it into RejectedState.
     * @param[in,out] item Reference to the target LeafTask being rejected.
     * @param[in] reason Explanation or justification for rejecting the work item.
     */
    void reject(LeafTask& item, const std::string& reason) override;
    void withdraw(LeafTask& item) override;
};

#endif