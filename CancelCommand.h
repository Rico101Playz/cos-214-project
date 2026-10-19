#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include "WorkCommand.h"

class CancelCommand : public WorkCommand {
private:
    std::string nodeId_;
    std::string reason_;

public:
    CancelCommand(WorkflowInstance& instance, Participant* actor, 
                  const std::string& nodeId, const std::string& reason);
    
    std::string describe() const override;

protected:
    void perform() override;
};

#endif // CANCELCOMMAND_H