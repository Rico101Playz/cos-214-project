#include "patterns/creation/FeatureDeliveryDirector.h"

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

    builder_->addStructuredItem("intake", ItemTemplate("spec", "Write technical spec", "TechLead", "engineering", "Manager", "High"));

    builder_->addStructuredItem("build-backend", ItemTemplate("api", "Build the API endpoint", "Developer", "backend", "Reviewer", "High"));
    builder_->addStructuredItem("build-backend", ItemTemplate("migration", "Database migration", "Developer", "backend", "Reviewer", "Urgent"));
    builder_->addStructuredItem("build-frontend", ItemTemplate("mockups", "Design the screens", "Designer", "frontend", "Manager", "Medium"));
    builder_->addStructuredItem("build-frontend", ItemTemplate("ui", "Build the UI", "Developer", "frontend", "Reviewer", "Low"));

    builder_->addStructuredItem("verify", ItemTemplate("tests", "Write integration tests", "QA", "qa", "Reviewer", "Medium"));
    builder_->addStructuredItem("verify", ItemTemplate("code-review", "Code review", "Reviewer", "engineering", "TechLead", "High").alsoApprovedBy("Manager"));

    builder_->addStructuredItem("release", ItemTemplate("deploy", "Deploy to production", "ReleaseManager", "platform", "Auto", "Urgent"));

    builder_->addBlocker("ui", "mockups");
    builder_->addBlocker("ui", "api");
    builder_->addBlocker("code-review", "tests");
}
