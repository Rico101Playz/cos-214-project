#ifndef CREATEDINSTANCE_H
#define CREATEDINSTANCE_H

#include "InstanceState.h"

class WorkflowInstance;

/**
 * @class CreatedInstance
 * @brief Represents the initial lifecycle state of a newly created workflow instance.
 * 
 * Part of the State pattern governing WorkflowInstance execution. In this initial state,
 * the workflow structure and definition are loaded, but execution has not begun.
 * It permits transitioning the workflow into the active execution state via start(),
 * or aborting the process prior to execution via cancel().
 */
class CreatedInstance : public InstanceState {
public:
    /**
     * @brief Retrieves the descriptive name of the instance state.
     * @return std::string The state name ("Created").
     */
    std::string name() const override;

    /**
     * @brief Initiates execution of the workflow instance.
     * 
     * Transitions the instance to RunningInstance, opens the first stage,
     * and advances the instance workflow tree.
     * 
     * @param[in,out] instance Reference to the target WorkflowInstance being started.
     */
    void start(WorkflowInstance& instance) override;

    /**
     * @brief Cancels the workflow instance prior to execution.
     * 
     * Aborts the root workflow node, records the cancellation reason, and
     * transitions the instance to CancelledInstance.
     * 
     * @param[in,out] instance Reference to the target WorkflowInstance being cancelled.
     * @param[in] reason Explanatory note or justification for cancelling the instance.
     */
    void cancel(WorkflowInstance& instance, const std::string& reason) override;
};

#endif