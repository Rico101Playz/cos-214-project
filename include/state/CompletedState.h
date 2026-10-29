#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "state/TaskState.h"

// terminal w/no transitions
class CompletedState : public TaskState {
public:
    std::string name() const override;
};

#endif