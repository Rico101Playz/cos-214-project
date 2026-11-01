#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>
#include <vector>
#include <iostream>

/**
 * @class TaskTemplate
 * @brief A template representing a basic item within a workflow definition.
 */
class TaskTemplate {
public:
    std::string id;
    std::string name; /*!< The name assigned to the item template. */
    std::string stage;
    std::string role;
    std::vector<std::string> approverRoles;
    std::string unit;
    std::string priority;
    std::vector<std::string> blockedBy;

    /**
     * @brief Constructs a TaskTemplate with the specified name.
     * @param[in] n The name to assign to the new item template.
     */
    TaskTemplate(std::string n) : id(n), name(n), priority("Medium") {}
    TaskTemplate(const std::string& itemId, const std::string& itemName,
                 const std::string& itemRole, const std::string& itemUnit = "",
                 const std::string& approver = "", const std::string& itemPriority = "Medium")
        : id(itemId), name(itemName), role(itemRole), unit(itemUnit), priority(itemPriority) {
        if (!approver.empty()) approverRoles.push_back(approver);
    }

    TaskTemplate& alsoApprovedBy(const std::string& approver) {
        approverRoles.push_back(approver);
        return *this;
    }
};

class StageTemplate {
public:
    std::string id;
    std::string name;
    std::string parent;

    StageTemplate(const std::string& stageId, const std::string& stageName,
                  const std::string& parentId = "")
        : id(stageId), name(stageName), parent(parentId) {}
};

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

    std::string policy;
    bool signOff = false;
    std::vector<StageTemplate> layout;
    std::vector<TaskTemplate> templates;

    bool structured() const { return !layout.empty() || !templates.empty(); }

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

#endif // WORKFLOWDEFINITION_H
