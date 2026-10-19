#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include "WorkCommand.h"

class ApproveCommand : public WorkCommand {
private:
    std::string itemId_;
    bool approved_;
    std::string comment_;

public:
    ApproveCommand(WorkflowInstance& instance, Participant* approver, 
                   const std::string& itemId, bool approved, const std::string& comment);
    
    std::string describe() const override;

protected:
    void perform() override;
};

#endif // APPROVECOMMAND_H