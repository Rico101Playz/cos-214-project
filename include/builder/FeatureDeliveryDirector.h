#ifndef FEATUREDELIVERYDIRECTOR_H
#define FEATUREDELIVERYDIRECTOR_H

#include <string>
#include "builder/WorkflowBuilder.h"

class FeatureDeliveryDirector {
private:
    WorkflowBuilder* builder_;

public:
    explicit FeatureDeliveryDirector(WorkflowBuilder* builder);
    void construct(const std::string& policy);
};

#endif
