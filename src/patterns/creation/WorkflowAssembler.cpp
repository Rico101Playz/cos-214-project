#include "patterns/creation/WorkflowAssembler.h"

#include <utility>
#include <vector>

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

std::unique_ptr<CompositeStage> WorkflowAssembler::buildFromDefinition(
    const WorkflowFactory& factory, const WorkflowDefinition& definition,
    const std::string& itemRole) {
    std::unique_ptr<CompositeStage> root = factory.createStage(definition.name, definition.name);

    std::vector<CompositeStage*> stages;   // non-owning; the root owns them
    for (std::size_t i = 0; i < definition.stages.size(); ++i) {
        std::unique_ptr<CompositeStage> stage =
            factory.createStage(definition.stages[i], definition.stages[i]);
        stages.push_back(stage.get());
        root->add(std::move(stage));
    }

    CompositeStage* target = stages.empty() ? root.get() : stages[0];
    for (std::size_t i = 0; i < definition.items.size(); ++i) {
        target->add(factory.createWorkItem(definition.items[i], definition.items[i], itemRole));
    }
    return root;
}

std::unique_ptr<AssignmentService> WorkflowAssembler::buildAssignmentService(
    const WorkflowFactory& factory, const Organisation& organisation) {
    std::unique_ptr<AssignmentService> service(new AssignmentService(organisation));
    service->setStrategy(factory.createAssignmentStrategy());
    return service;
}
