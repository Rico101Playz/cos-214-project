#include "FeatureDeliveryProcess.h"
#include "OutlineDirector.h"

void FeatureDeliveryProcess::construct(WorkflowBuilder* builder) {
    OutlineDirector director("Feature Delivery Outline", builder);
    director.construct();
}