#ifndef ADDWORKCOMMAND_H
#define ADDWORKCOMMAND_H

#include "WorkCommand.h"
#include "core/WorkItem.h"
#include <memory>
#include <string>

class LeafWorkItem;

class AddWorkCommand : public WorkCommand {
private:
    std::string stageId;
    std::string itemId;
    std::shared_ptr<WorkItem> itemTemplate;
    std::unique_ptr<LeafWorkItem> prepared;
    std::string preparedId;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    explicit AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& stageId, std::shared_ptr<WorkItem> itemTemplate);
    AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& parentId,
                   std::unique_ptr<LeafWorkItem> item);
    ~AddWorkCommand() override;
    std::string describe() const override;
};

#endif // ADDWORKCOMMAND_H