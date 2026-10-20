#ifndef ESCALATECOMMAND_H
#define ESCALATECOMMAND_H

#include "WorkCommand.h"
#include <string>

class EscalateCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    explicit EscalateCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId);
    std::string describe() const override;
};

#endif // ESCALATECOMMAND_H