#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

// terminal w/no transitions
class CompletedState : public WorkItemState {
public:
    std::string name() const override;
};

#endif