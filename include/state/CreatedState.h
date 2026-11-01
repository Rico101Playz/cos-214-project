#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include "state/TaskState.h"

class CreatedState : public TaskState {
public:
    std::string name() const override;
    void makeAvailable(LeafTask& item) override;
};

#endif