#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "state/TaskState.h"

class EscalatedState : public TaskState {
public:
    std::string name() const override;
    void assign(LeafTask& item, const std::string& participant) override;
};

#endif