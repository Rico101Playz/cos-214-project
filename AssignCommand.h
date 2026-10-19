#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include "WorkCommand.h"

class AssignCommand : public WorkCommand {
private:
    std::string itemId_;
    std::string participantName_;
    std::string chosen_;

public:
    AssignCommand(WorkflowInstance& instance, Participant* actor, 
                  const std::string& itemId, const std::string& participantName);
    
    std::string describe() const override;
    
    std::string chosen() const;

protected:
    void perform() override;
};

#endif // ASSIGNCOMMAND_H