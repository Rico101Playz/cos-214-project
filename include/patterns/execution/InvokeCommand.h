#ifndef INVOKECOMMAND_H
#define INVOKECOMMAND_H

#include "WorkCommand.h"
#include <string>

class InvokeCommand : public WorkCommand {
private:
    std::string itemId;
    std::string operation;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    InvokeCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& operation);
    std::string describe() const override;
};

#endif // INVOKECOMMAND_H