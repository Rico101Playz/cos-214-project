#ifndef WORKFLOWBUILDER_H
#define WORKFLOWBUILDER_H

#include <string>
#include "core/WorkflowDefinition.h"

/**
 * @class WorkflowBuilder
 * @brief Abstract interface for building a workflow definition step by step.
 * 
 * Defines the standard operations required to construct the various parts of a 
 * WorkflowDefinition, such as stages, work items, and their dependencies.
 */
class WorkflowBuilder {
public:
    /**
     * @brief Default virtual destructor.
     */
    virtual ~WorkflowBuilder() = default;

    /**
     * @brief Resets the builder to start constructing a new workflow definition.
     * @param[in] name The name of the new workflow definition.
     */
    virtual void reset(std::string name) = 0;
    
    /**
     * @brief Adds a new stage to the workflow being built.
     * @param[in] name The name of the stage to add.
     */
    virtual void addStage(std::string name) = 0;
    
    /**
     * @brief Adds a new work item to the workflow being built.
     * @param[in] item The template detailing the work item to add.
     */
    virtual void addWorkItem(ItemTemplate item) = 0;
    
    /**
     * @brief Adds a dependency between two components in the workflow.
     * @param[in] from The name of the dependent node's origin.
     * @param[in] to The name of the dependent node's target.
     */
    virtual void addDependency(std::string from, std::string to) = 0;
    
    /**
     * @brief Finalizes and retrieves the constructed workflow definition.
     * @return WorkflowDefinition The fully assembled workflow definition.
     */
    virtual WorkflowDefinition build() = 0;
};

#endif // WORKFLOWBUILDER_H