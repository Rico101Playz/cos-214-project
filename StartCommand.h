#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

#include "WorkCommand.h"

class StartCommand : public WorkCommand {
private:
    std::string itemId_;

public:
    StartCommand(WorkflowInstance& instance, Participant* actor, const std::string& itemId);
    
    std::string describe() const override;

protected:
    void perform() override;
};

#endif // STARTCOMMAND_H