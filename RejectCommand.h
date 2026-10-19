#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include "WorkCommand.h"

class RejectCommand : public WorkCommand {
private:
    std::string itemId_;
    std::string reason_;

public:
    RejectCommand(WorkflowInstance& instance, Participant* actor, 
                  const std::string& itemId, const std::string& reason);
    
    std::string describe() const override;

protected:
    void perform() override;
};

#endif // REJECTCOMMAND_H