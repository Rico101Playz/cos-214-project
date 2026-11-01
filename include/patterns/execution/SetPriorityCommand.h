#ifndef SETPRIORITYCOMMAND_H
#define SETPRIORITYCOMMAND_H

#include "WorkCommand.h"
#include <string>

class SetPriorityCommand : public WorkCommand {
private:
    std::string itemId;
    std::string priority;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    SetPriorityCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId,
                       const std::string& priority);
    std::string describe() const override;
};

#endif
