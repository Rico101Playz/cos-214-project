#ifndef RUNNINGINSTANCE_H
#define RUNNINGINSTANCE_H

#include "InstanceState.h"

class RunningInstance : public InstanceState {
public:
    std::string name() const override;
    void advance(WorkflowInstance& instance) override;
    void cancel(WorkflowInstance& instance, const std::string& reason) override;
};

#endif