#ifndef SIGNCOMMAND_H
#define SIGNCOMMAND_H

#include "WorkCommand.h"
#include <string>

class SignCommand : public WorkCommand {
private:
    std::string itemId;
    std::string slot;
    std::string person;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    SignCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId,
                const std::string& slot, const std::string& person);
    std::string describe() const override;
};

#endif
