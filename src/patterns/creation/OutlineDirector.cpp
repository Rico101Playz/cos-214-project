#include "patterns/creation/OutlineDirector.h"

OutlineDirector::OutlineDirector(std::string outline, WorkflowBuilder* builder)
    : outline(outline), builder(builder) {}

void OutlineDirector::construct() {
    if (!builder) return;

    builder->reset("Workflow based on: " + outline);
    
    builder->addStage("Initialization");
    builder->addStage("Processing");
    builder->addStage("Completion");

    builder->addWorkItem(ItemTemplate("Task A"));
    builder->addWorkItem(ItemTemplate("Task B"));

    builder->addDependency("Initialization", "Processing");
    builder->addDependency("Processing", "Completion");
}