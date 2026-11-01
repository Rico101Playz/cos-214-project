#ifndef WORKFLOWBUILDER_H
#define WORKFLOWBUILDER_H

#include <string>
#include "core/WorkflowDefinition.h"

class WorkflowBuilder {
public:
    virtual ~WorkflowBuilder() = default;

    virtual void reset(std::string name) = 0;
    virtual void addStage(std::string name) = 0;
    virtual void addWorkItem(ItemTemplate item) = 0;
    virtual void addDependency(std::string from, std::string to) = 0;
    virtual WorkflowDefinition build() = 0;

    virtual void setPolicy(const std::string&) {}
    virtual void setSignOff(bool) {}
    virtual void addStructuredStage(const StageTemplate&) {}
    virtual void addStructuredItem(const std::string&, const ItemTemplate&) {}
    virtual void addBlocker(const std::string&, const std::string&) {}
};

#endif // WORKFLOWBUILDER_H
