#include "patterns/execution/AddWorkCommand.h"
#include "core/WorkflowInstance.h"

#include <stdexcept>
#include <utility>

AddWorkCommand::AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& stageId, std::shared_ptr<WorkItem> itemTemplate) 
    : WorkCommand(store, std::move(instanceId)), stageId(stageId), itemTemplate(std::move(itemTemplate)) {}

void AddWorkCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("AddWorkCommand: instance is null");
    if (!itemTemplate)
        throw std::invalid_argument("AddWorkCommand: item template is null");

    if (!instance->addWorkItem(stageId, itemTemplate->id(), itemTemplate)) {
        throw std::logic_error(
            "AddWorkCommand: cannot add item '" + itemTemplate->id() +
            "' to stage '" + stageId + "'");
    }
}

std::string AddWorkCommand::describe() const {
    return "AddWorkCommand: stage=" + stageId;
}