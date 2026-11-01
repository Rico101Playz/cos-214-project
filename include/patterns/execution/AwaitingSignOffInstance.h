#ifndef AWAITINGSIGNOFFINSTANCE_H
#define AWAITINGSIGNOFFINSTANCE_H

#include "InstanceState.h"

class AwaitingSignOffInstance : public InstanceState {
public:
    std::string name() const override;
    void cancel(WorkflowInstance& instance, const std::string& reason) override;
    void signOff(WorkflowInstance& instance) override;
    void reopen(WorkflowInstance& instance) override;
};

#endif
