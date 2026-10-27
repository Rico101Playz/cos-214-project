#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>
#include <vector>
#include <iostream>

class ItemTemplate {
public:
    std::string id;
    std::string name;
    std::string stage;
    std::string role;
    std::vector<std::string> approverRoles;
    std::string unit;
    std::string priority;
    std::vector<std::string> blockedBy;

    ItemTemplate(std::string n) : id(n), name(n), priority("Medium") {}
    ItemTemplate(const std::string& itemId, const std::string& itemName,
                 const std::string& itemRole, const std::string& itemUnit = "",
                 const std::string& approver = "", const std::string& itemPriority = "Medium")
        : id(itemId), name(itemName), role(itemRole), unit(itemUnit), priority(itemPriority) {
        if (!approver.empty()) approverRoles.push_back(approver);
    }

    ItemTemplate& alsoApprovedBy(const std::string& approver) {
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

class WorkflowDefinition {
public:
    std::string name;
    std::vector<std::string> stages;
    std::vector<std::string> items;
    std::vector<std::string> dependencies;

    std::string policy;
    bool signOff = false;
    std::vector<StageTemplate> layout;
    std::vector<ItemTemplate> templates;

    bool structured() const { return !layout.empty() || !templates.empty(); }

    void print() const {
        std::cout << "Workflow: " << name << "\n";
        std::cout << "Stages: " << stages.size() << ", Items: " << items.size() << "\n";
    }
};

#endif // WORKFLOWDEFINITION_H
