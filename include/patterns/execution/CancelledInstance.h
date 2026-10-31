#ifndef CANCELLEDINSTANCE_H
#define CANCELLEDINSTANCE_H

#include "InstanceState.h"

/**
 * @class CancelledInstance
 * @brief Represents the terminal lifecycle state of a cancelled workflow instance.
 * 
 * Part of the State pattern governing WorkflowInstance execution. In this state,
 * the entire workflow instance has been aborted or terminated prematurely,
 * preventing any further command execution, state transitions, or work processing.
 */
class CancelledInstance : public InstanceState {
public:
    /**
     * @brief Retrieves the descriptive name of the instance state.
     * @return std::string The state name ("Cancelled").
     */
    std::string name() const override;
};

#endif