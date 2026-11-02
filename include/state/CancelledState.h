#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "state/TaskState.h"

// terminal w/no transitions
/**
 * @class CancelledState
 * @brief Represents the terminal cancelled state of a work item.
 *
 * Part of the State pattern governing LeafTask lifecycles.
 * In this terminal state, the work item has been cancelled and does not
 * permit any further state transitions or operational commands.
 */
class CancelledState : public TaskState {
public:
    /**
     * @brief Retrieves the descriptive name of the work item state.
     * @return std::string The state name ("Cancelled").
     */
    std::string name() const override;
};

#endif