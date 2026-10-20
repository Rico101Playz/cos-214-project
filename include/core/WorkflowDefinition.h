#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <string>
#include <vector>
#include <iostream>

class WorkflowDefinition {
public:
    std::string name;
    std::vector<std::string> stages;
    std::vector<std::string> items;
    std::vector<std::string> dependencies;

    void print() const {
        std::cout << "Workflow: " << name << "\n";
        std::cout << "Stages: " << stages.size() << ", Items: " << items.size() << "\n";
    }
};

class ItemTemplate {
public:
    std::string name;
    ItemTemplate(std::string n) : name(n) {}
};

#endif // WORKFLOWDEFINITION_H