#ifndef FEATUREDELIVERYPROCESS_H
#define FEATUREDELIVERYPROCESS_H

#include "patterns/creation/WorkflowBuilder.h"

/**
 * @class FeatureDeliveryProcess
 * @brief Directs the construction of a Software Feature Delivery workflow.
 * 
 * This class encapsulates the step-by-step process of building a specific 
 * feature delivery workflow configuration by utilizing a provided WorkflowBuilder.
 */
class FeatureDeliveryProcess {
public:
    /**
     * @brief Constructs the feature delivery workflow using the provided builder.
     * 
     * @param[in,out] builder A pointer to the WorkflowBuilder instance used to assemble the workflow.
     */
    void construct(WorkflowBuilder* builder);
};

#endif // FEATUREDELIVERYPROCESS_H