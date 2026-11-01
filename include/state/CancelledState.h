#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "state/TaskState.h"

// terminal w/no transitions
class CancelledState : public TaskState {
public:
    std::string name() const override;
};

#endif