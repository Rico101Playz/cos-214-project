#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

// terminal w/no transitions
/**
 * @class CompletedState
 * @brief Represents the terminal completion state of a work item.
 * 
 * Part of the State pattern governing LeafWorkItem lifecycles.
 * In this terminal state, work execution and approvals have finished successfully;
 * no further state transitions or operational commands are permitted.
 */
class CompletedState : public WorkItemState {
public:
    /**
     * @brief Retrieves the descriptive name of the work item state.
     * @return std::string The state name ("Completed").
     */
    std::string name() const override;
};

#endif