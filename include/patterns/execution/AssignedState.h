#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "WorkItemState.h"

class AssignedState : public WorkItemState {
public:
    std::string name() const override;
    void start(LeafWorkItem& item) override;
    void escalate(LeafWorkItem& item) override;
};

#endif