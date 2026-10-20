#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include "WorkCommand.h"
#include <string>

class WorkflowInstance;

class ApproveCommand : public WorkCommand {
private:
    std::string itemId;
    bool approved;
    std::string comment;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    ApproveCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, bool approved, const std::string& comment);
                   
    std::string describe() const override;
};

#endif // APPROVECOMMAND_H