#ifndef ADDWORKCOMMAND_H
#define ADDWORKCOMMAND_H

#include "WorkCommand.h"
#include "core/WorkItem.h"
#include <string>

class AddWorkCommand : public WorkCommand {
private:
    std::string stageId;
    std::string itemId;
    std::shared_ptr<WorkItem> itemTemplate;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    explicit AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& stageId, std::shared_ptr<WorkItem> itemTemplate);
    std::string describe() const override;
};

#endif // ADDWORKCOMMAND_H