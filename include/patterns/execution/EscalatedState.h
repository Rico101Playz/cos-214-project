#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "WorkItemState.h"

class EscalatedState : public WorkItemState {
public:
    std::string name() const override;
    void assign(LeafWorkItem& item, Participant& participant) override;
};

#endif