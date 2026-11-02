#ifndef CONCRETEDEFINITIONBUILDER_H
#define CONCRETEDEFINITIONBUILDER_H

#include "builder/WorkflowBuilder.h"

/**
 * @class ConcreteDefinitionBuilder
 * @brief A concrete implementation of the WorkflowBuilder interface.
 *
 * This class provides the step-by-step construction logic to assemble a
 * WorkflowDefinition, including its stages, work items, and dependencies.
 */
class ConcreteDefinitionBuilder : public WorkflowBuilder {
private:
    WorkflowDefinition definition; /*!< The internal workflow definition being constructed. */

public:
    /**
     * @brief Resets the builder and initializes a new workflow definition.
     * @param[in] name The name to assign to the new workflow definition.
     */
    void reset(std::string name) override;

    /**
     * @brief Adds a new stage to the workflow definition.
     * @param[in] name The name of the stage to add.
     */
    void addStage(std::string name) override;

    /**
     * @brief Adds a new work item to the workflow definition based on a template.
     * @param[in] item The TaskTemplate containing the details of the work item to add.
     */
    void addWorkItem(TaskTemplate item) override;

    /**
     * @brief Defines a directional dependency between two components in the workflow.
     * @param[in] from The name of the starting node or item of the dependency.
     * @param[in] to The name of the target node or item of the dependency.
     */
    void addDependency(std::string from, std::string to) override;

    /**
     * @brief Finalizes and returns the fully constructed workflow definition.
     * @return WorkflowDefinition The completed workflow definition instance.
     */
    WorkflowDefinition build() override;

    void setPolicy(const std::string& policy) override;
    void setSignOff(bool required) override;
    void addStructuredStage(const StageTemplate& stage) override;
    void addStructuredItem(const std::string& stageId, const TaskTemplate& item) override;
    void addBlocker(const std::string& itemId, const std::string& blockerId) override;
};

#endif // CONCRETEDEFINITIONBUILDER_H
