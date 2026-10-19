#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include "WorkCommand.h"

class CompleteCommand : public WorkCommand {
private:
    std::string itemId_;

public:
    CompleteCommand(WorkflowInstance& instance, Participant* actor, const std::string& itemId);
    
    std::string describe() const override;

protected:
    void perform() override;
};

#endif // COMPLETECOMMAND_H