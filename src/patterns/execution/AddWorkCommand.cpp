#include "patterns/execution/AddWorkCommand.h"

AddWorkCommand::AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& stageId, std::shared_ptr<WorkItem> itemTemplate) 
    : WorkCommand(store, std::move(instanceId)), stageId(stageId) {}

void AddWorkCommand::perform(WorkflowInstance* instance) {}

std::string AddWorkCommand::describe() const {
    return "AddWorkCommand: stage=" + stageId;
}