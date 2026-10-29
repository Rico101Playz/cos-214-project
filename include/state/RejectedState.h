#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "state/TaskState.h"

class RejectedState : public TaskState {
public:
    std::string name() const override;
    void assign(LeafTask& item, const std::string& participant) override;
    void start(LeafTask& item) override;
};

#endif