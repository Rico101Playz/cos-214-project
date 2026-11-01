#ifndef WITHDRAWCOMMAND_H
#define WITHDRAWCOMMAND_H

#include "WorkCommand.h"
#include <string>

class WithdrawCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    WithdrawCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId);
    std::string describe() const override;
};

#endif
