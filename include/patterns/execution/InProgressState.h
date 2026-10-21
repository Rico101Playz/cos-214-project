#ifndef INPROGRESSSTATE_H
#define INPROGRESSSTATE_H

#include "WorkItemState.h"

class InProgressState : public WorkItemState {
public:
    std::string name() const override;
    void complete(LeafWorkItem& item) override;
    void reject(LeafWorkItem& item, const std::string& reason) override;
    void escalate(LeafWorkItem& item) override;
};

#endif