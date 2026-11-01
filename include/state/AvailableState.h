#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include "state/TaskState.h"

class AvailableState : public TaskState {
public:
    std::string name() const override;
    void assign(LeafTask& item, const std::string& participant) override;
};

#endif