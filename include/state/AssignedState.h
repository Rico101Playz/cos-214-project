#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "state/TaskState.h"

class AssignedState : public TaskState {
public:
    std::string name() const override;
    void assign(LeafTask& item, const std::string& participant) override;
    void start(LeafTask& item) override;
    void escalate(LeafTask& item) override;
};

#endif