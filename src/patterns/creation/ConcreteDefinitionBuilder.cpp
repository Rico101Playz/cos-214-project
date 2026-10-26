#include "patterns/creation/ConcreteDefinitionBuilder.h"

void ConcreteDefinitionBuilder::reset(std::string name) {
    definition = WorkflowDefinition();
    definition.name = name;
}

void ConcreteDefinitionBuilder::addStage(std::string name) {
    definition.stages.push_back(name);
}

void ConcreteDefinitionBuilder::addWorkItem(ItemTemplate item) {
    definition.items.push_back(item.name);
}

void ConcreteDefinitionBuilder::addDependency(std::string from, std::string to) {
    definition.dependencies.push_back(from + " -> " + to);
}

WorkflowDefinition ConcreteDefinitionBuilder::build() {
    return definition;
}