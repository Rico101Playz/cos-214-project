#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include "WorkCommand.h"
#include <string>

class CancelCommand : public WorkCommand {
private:
    std::string nodeId;
    std::string reason;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    CancelCommand(SnapshotStore* store, std::string instanceId, const std::string& nodeId, const std::string& reason);
    std::string describe() const override;
};

#endif // CANCELCOMMAND_H