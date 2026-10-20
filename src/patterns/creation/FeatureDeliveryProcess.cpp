#include "patterns/creation/FeatureDeliveryProcess.h"
#include "patterns/creation/OutlineDirector.h"

void FeatureDeliveryProcess::construct(WorkflowBuilder* builder) {
    OutlineDirector director("Feature Delivery Outline", builder);
    director.construct();
}