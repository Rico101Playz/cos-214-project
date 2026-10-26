#ifndef CONCRETEDEFINITIONBUILDER_H
#define CONCRETEDEFINITIONBUILDER_H

#include "patterns/creation/WorkflowBuilder.h"

class ConcreteDefinitionBuilder : public WorkflowBuilder {
private:
    WorkflowDefinition definition;

public:
    void reset(std::string name) override;
    void addStage(std::string name) override;
    void addWorkItem(ItemTemplate item) override;
    void addDependency(std::string from, std::string to) override;
    WorkflowDefinition build() override;
};

#endif // CONCRETEDEFINITIONBUILDER_H