#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include "WorkItemState.h"

class AvailableState : public WorkItemState {
public:
    std::string name() const override;
    void assign(LeafWorkItem& item, Participant& participant) override;
};

#endif