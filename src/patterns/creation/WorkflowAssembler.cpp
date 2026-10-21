#include "patterns/creation/WorkflowAssembler.h"

#include <utility>

std::unique_ptr<CompositeStage> WorkflowAssembler::buildFeatureTree(const WorkflowFactory& factory) {
    std::unique_ptr<CompositeStage> root = factory.createStage("feature", "Feature Delivery");

    std::unique_ptr<CompositeStage> develop = factory.createStage("develop", "Develop");
    develop->add(factory.createWorkItem("implement", "Implement feature", "Developer"));
    develop->add(factory.createWorkItem("write-tests", "Write tests", "QATester"));

    std::unique_ptr<CompositeStage> review = factory.createStage("review", "Review");
    review->add(factory.createWorkItem("code-review", "Code review", "Reviewer"));

    std::unique_ptr<CompositeStage> release = factory.createStage("release", "Release");
    release->add(factory.createWorkItem("deploy", "Deploy", "ReleaseManager"));

    root->add(std::move(develop));
    root->add(std::move(review));
    root->add(std::move(release));
    return root;
}

std::unique_ptr<AssignmentService> WorkflowAssembler::buildAssignmentService(
    const WorkflowFactory& factory, const Organisation& organisation) {
    std::unique_ptr<AssignmentService> service(new AssignmentService(organisation));
    service->setStrategy(factory.createAssignmentStrategy());
    return service;
}
