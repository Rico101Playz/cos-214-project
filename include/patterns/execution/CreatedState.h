#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include "WorkItemState.h"

class CreatedState : public WorkItemState {
public:
    std::string name() const override;
    void makeAvailable(LeafWorkItem& item) override;
};

#endif