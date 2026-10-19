#ifndef SETDATACOMMAND_H
#define SETDATACOMMAND_H

#include "WorkCommand.h"

class SetDataCommand : public WorkCommand {
private:
    std::string nodeId_;
    std::string key_;
    std::string value_;

public:
    SetDataCommand(WorkflowInstance& instance, Participant* actor, 
        const std::string& nodeId, const std::string& key, const std::string& value);
    
    std::string describe() const override;

protected:
    void perform() override;
};

#endif // SETDATACOMMAND_H