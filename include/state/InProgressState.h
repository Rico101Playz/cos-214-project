#ifndef INPROGRESSSTATE_H
#define INPROGRESSSTATE_H

#include "state/TaskState.h"

class InProgressState : public TaskState {
public:
    std::string name() const override;
    void complete(LeafTask& item) override;
    void reject(LeafTask& item, const std::string& reason) override;
    void escalate(LeafTask& item) override;
};

#endif