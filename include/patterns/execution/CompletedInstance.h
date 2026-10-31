#ifndef COMPLETEDINSTANCE_H
#define COMPLETEDINSTANCE_H

#include "InstanceState.h"

/**
 * @class CompletedInstance
 * @brief Represents the terminal lifecycle state of a successfully completed workflow instance.
 * 
 * Part of the State pattern governing WorkflowInstance execution. In this terminal state,
 * all required stages, sub-processes, and leaf work items have concluded successfully,
 * and no further commands or state transitions are permitted.
 */
class CompletedInstance : public InstanceState {
public:
    /**
     * @brief Retrieves the descriptive name of the instance state.
     * @return std::string The state name ("Completed").
     */
    std::string name() const override;
};

#endif