#include "builder/FeatureDeliveryDirector.h"

FeatureDeliveryDirector::FeatureDeliveryDirector(WorkflowBuilder* builder) : builder_(builder) {}

void FeatureDeliveryDirector::construct(const std::string& policy) {
    if (!builder_) return;

    builder_->reset("Feature Delivery");
    builder_->setPolicy(policy);
    builder_->setSignOff(true);

    builder_->addStructuredStage(StageTemplate("intake", "Intake"));
    builder_->addStructuredStage(StageTemplate("build", "Build"));
    builder_->addStructuredStage(StageTemplate("build-backend", "Back-end work", "build"));
    builder_->addStructuredStage(StageTemplate("build-frontend", "Front-end work", "build"));
    builder_->addStructuredStage(StageTemplate("verify", "Verify"));
    builder_->addStructuredStage(StageTemplate("release", "Release"));

    builder_->addStructuredItem("intake", TaskTemplate("spec", "Write technical spec", "TechLead", "engineering", "Manager", "High"));

    builder_->addStructuredItem("build-backend", TaskTemplate("api", "Build the API endpoint", "Developer", "backend", "Reviewer", "High"));
    builder_->addStructuredItem("build-backend", TaskTemplate("migration", "Database migration", "Developer", "backend", "Reviewer", "Urgent"));
    builder_->addStructuredItem("build-frontend", TaskTemplate("mockups", "Design the screens", "Designer", "frontend", "Manager", "Medium"));
    builder_->addStructuredItem("build-frontend", TaskTemplate("ui", "Build the UI", "Developer", "frontend", "Reviewer", "Low"));

    builder_->addStructuredItem("verify", TaskTemplate("tests", "Write integration tests", "QA", "qa", "Reviewer", "Medium"));
    builder_->addStructuredItem("verify", TaskTemplate("code-review", "Code review", "Reviewer", "engineering", "TechLead", "High").alsoApprovedBy("Manager"));

    builder_->addStructuredItem("release", TaskTemplate("deploy", "Deploy to production", "ReleaseManager", "platform", "Auto", "Urgent"));

    builder_->addBlocker("ui", "mockups");
    builder_->addBlocker("ui", "api");
    builder_->addBlocker("code-review", "tests");
}
