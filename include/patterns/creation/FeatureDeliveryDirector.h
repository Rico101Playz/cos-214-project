#ifndef FEATUREDELIVERYDIRECTOR_H
#define FEATUREDELIVERYDIRECTOR_H

#include <string>
#include "patterns/creation/WorkflowBuilder.h"

class FeatureDeliveryDirector {
private:
    WorkflowBuilder* builder_;

public:
    explicit FeatureDeliveryDirector(WorkflowBuilder* builder);
    void construct(const std::string& policy);
};

#endif
