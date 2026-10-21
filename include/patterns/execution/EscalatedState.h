#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "WorkItemState.h"

class EscalatedState : public WorkItemState {
public:
    std::string name() const override;
    void assign(LeafWorkItem& item, const std::string& participant) override;
};

#endif