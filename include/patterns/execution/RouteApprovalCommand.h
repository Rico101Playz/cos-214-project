#ifndef ROUTEAPPROVALCOMMAND_H
#define ROUTEAPPROVALCOMMAND_H

#include "WorkCommand.h"
#include <string>
#include <vector>

class RouteApprovalCommand : public WorkCommand {
private:
    std::string itemId;
    std::vector<std::string> pending;
    int automaticSignatures;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    RouteApprovalCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId,
                         const std::vector<std::string>& pending, int automaticSignatures);
    std::string describe() const override;
};

#endif
