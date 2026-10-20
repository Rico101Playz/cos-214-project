#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include "WorkCommand.h"
#include <string>

class RejectCommand : public WorkCommand {
private:
    std::string itemId;
    std::string reason;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    RejectCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& reason);
    std::string describe() const override;
};

#endif // REJECTCOMMAND_H