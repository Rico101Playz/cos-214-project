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

    void setPolicy(const std::string& policy) override;
    void setSignOff(bool required) override;
    void addStructuredStage(const StageTemplate& stage) override;
    void addStructuredItem(const std::string& stageId, const ItemTemplate& item) override;
    void addBlocker(const std::string& itemId, const std::string& blockerId) override;
};

#endif // CONCRETEDEFINITIONBUILDER_H
