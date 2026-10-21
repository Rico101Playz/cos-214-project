#ifndef CREATEDINSTANCE_H
#define CREATEDINSTANCE_H

#include "InstanceState.h"

class CreatedInstance : public InstanceState {
public:
    std::string name() const override;
    void start(WorkflowInstance& instance) override;
    void cancel(WorkflowInstance& instance, const std::string& reason) override;
};

#endif