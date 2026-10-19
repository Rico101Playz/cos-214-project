#ifndef ADDWORKCOMMAND_H
#define ADDWORKCOMMAND_H

#include "WorkCommand.h"
#include "ItemTemplate.h"

class AddWorkCommand : public WorkCommand {
private:
    std::string stageId_;
    ItemTemplate spec_;

public:
    AddWorkCommand(WorkflowInstance& instance, Participant* actor, 
                   const std::string& stageId, const ItemTemplate& spec);
    
    std::string describe() const override;

protected:
    void _perform() override;
};

#endif // ADDWORKCOMMAND_H