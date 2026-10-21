#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "WorkItemState.h"

class RejectedState : public WorkItemState {
public:
    std::string name() const override;
    void assign(LeafWorkItem& item, const std::string& participant) override;
    void start(LeafWorkItem& item) override;
};

#endif