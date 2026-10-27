#include "patterns/execution/AddWorkCommand.h"
#include "core/WorkflowInstance.h"
#include "patterns/structural/LeafWorkItem.h"

#include <stdexcept>
#include <utility>

AddWorkCommand::AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& stageId, std::shared_ptr<WorkItem> itemTemplate) 
    : WorkCommand(store, std::move(instanceId)), stageId(stageId), itemTemplate(std::move(itemTemplate)) {}

AddWorkCommand::AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& parentId,
                               std::unique_ptr<LeafWorkItem> item)
    : WorkCommand(store, std::move(instanceId)), stageId(parentId), prepared(std::move(item)) {
    if (prepared) preparedId = prepared->id();
}

AddWorkCommand::~AddWorkCommand() = default;

void AddWorkCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr)
        throw std::invalid_argument("AddWorkCommand: instance is null");
    if (prepared) {
        if (!instance->adoptItem(stageId, std::move(prepared))) {
            throw std::logic_error("AddWorkCommand: cannot add item '" + preparedId + "' under '" + stageId + "'");
        }
        return;
    }
    if (!preparedId.empty())
        throw std::logic_error("AddWorkCommand: item '" + preparedId + "' was already added");
    if (!itemTemplate)
        throw std::invalid_argument("AddWorkCommand: item template is null");

    if (!instance->addWorkItem(stageId, itemTemplate->id(), itemTemplate)) {
        throw std::logic_error(
            "AddWorkCommand: cannot add item '" + itemTemplate->id() +
            "' to stage '" + stageId + "'");
    }
}

std::string AddWorkCommand::describe() const {
    if (!preparedId.empty()) return "AddWorkCommand: stage=" + stageId + ", item=" + preparedId;
    return "AddWorkCommand: stage=" + stageId;
}