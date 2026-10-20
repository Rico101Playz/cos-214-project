#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "WorkItemState.h"

// terminal w/no transitions
class CancelledState : public WorkItemState {
public:
    std::string name() const override;
};

#endif