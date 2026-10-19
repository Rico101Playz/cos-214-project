#ifndef ESCALATECOMMAND_H
#define ESCALATECOMMAND_H

#include "WorkCommand.h"

class EscalateCommand : public WorkCommand {
private:
    std::string itemId_;
    std::string target_;

public:
    EscalateCommand(WorkflowInstance& instance, Participant* actor, const std::string& itemId);
    
    std::string describe() const override;

    std::string target() const;

protected:
    void perform() override;
};

#endif // ESCALATECOMMAND_H