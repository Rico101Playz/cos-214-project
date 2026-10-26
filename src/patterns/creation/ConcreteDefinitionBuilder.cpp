#include "patterns/creation/ConcreteDefinitionBuilder.h"
#include <stdexcept>

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

void ConcreteDefinitionBuilder::setPolicy(const std::string& policy) {
    definition.policy = policy;
}

void ConcreteDefinitionBuilder::setSignOff(bool required) {
    definition.signOff = required;
}

void ConcreteDefinitionBuilder::addStructuredStage(const StageTemplate& stage) {
    for (std::size_t i = 0; i < definition.layout.size(); ++i) {
        if (definition.layout[i].id == stage.id) {
            throw std::invalid_argument("Duplicate stage id: " + stage.id);
        }
    }
    bool parentFound = stage.parent.empty();
    for (std::size_t i = 0; i < definition.layout.size() && !parentFound; ++i) {
        parentFound = definition.layout[i].id == stage.parent;
    }
    if (!parentFound) {
        throw std::invalid_argument("Unknown parent stage: " + stage.parent);
    }
    definition.layout.push_back(stage);
}

void ConcreteDefinitionBuilder::addStructuredItem(const std::string& stageId, const ItemTemplate& item) {
    bool stageFound = false;
    for (std::size_t i = 0; i < definition.layout.size() && !stageFound; ++i) {
        stageFound = definition.layout[i].id == stageId;
    }
    if (!stageFound) {
        throw std::invalid_argument("Unknown stage: " + stageId);
    }
    for (std::size_t i = 0; i < definition.templates.size(); ++i) {
        if (definition.templates[i].id == item.id) {
            throw std::invalid_argument("Duplicate item id: " + item.id);
        }
    }
    ItemTemplate copy = item;
    copy.stage = stageId;
    definition.templates.push_back(copy);
}

void ConcreteDefinitionBuilder::addBlocker(const std::string& itemId, const std::string& blockerId) {
    int target = -1;
    int blocker = -1;
    for (std::size_t i = 0; i < definition.templates.size(); ++i) {
        if (definition.templates[i].id == itemId) target = static_cast<int>(i);
        if (definition.templates[i].id == blockerId) blocker = static_cast<int>(i);
    }
    if (target < 0 || blocker < 0 || blocker >= target) {
        throw std::invalid_argument("Invalid blocker " + blockerId + " for " + itemId);
    }
    definition.templates[target].blockedBy.push_back(blockerId);
}
