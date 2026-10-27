#ifndef SIGNOFFCOMMAND_H
#define SIGNOFFCOMMAND_H

#include "WorkCommand.h"
#include <string>

class WorkflowFactory;

class SignOffCommand : public WorkCommand {
public:
    enum Decision { SIGN_OFF, REOPEN_FOR_FIXES };

private:
    Decision decision;
    const WorkflowFactory* factory;

protected:
    void perform(WorkflowInstance* instance) override;

public:
    SignOffCommand(SnapshotStore* store, std::string instanceId, Decision decision,
                   const WorkflowFactory* factory = nullptr);
    std::string describe() const override;
};

#endif
