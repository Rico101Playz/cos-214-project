#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>
#include <vector>
#include <iostream>

/**
 * @class WorkflowDefinition
 * @brief Defines the structure and configuration of a workflow.
 * 
 * This class stores the blueprint for a workflow, including its name, 
 * the stages it contains, the items involved, and any dependencies.
 */
class WorkflowDefinition {
public:
    std::string name;                      /*!< The name of the workflow definition. */
    std::vector<std::string> stages;       /*!< A collection of stages included in the workflow. */
    std::vector<std::string> items;        /*!< A collection of items associated with this workflow. */
    std::vector<std::string> dependencies; /*!< A list of dependencies required for the workflow to execute. */

    /**
     * @brief Prints a summary of the workflow definition to standard output.
     * 
     * Outputs the workflow's name along with the total number of stages and items it contains.
     */
    void print() const {
        std::cout << "Workflow: " << name << "\n";
        std::cout << "Stages: " << stages.size() << ", Items: " << items.size() << "\n";
    }
};

/**
 * @class ItemTemplate
 * @brief A template representing a basic item within a workflow definition.
 */
class ItemTemplate {
public:
    std::string name; /*!< The name assigned to the item template. */
    
    /**
     * @brief Constructs an ItemTemplate with the specified name.
     * @param[in] n The name to assign to the new item template.
     */
    ItemTemplate(std::string n) : name(n) {}
};

#endif // WORKFLOWDEFINITION_H