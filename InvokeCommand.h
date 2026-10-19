#ifndef INVOKECOMMAND_H
#define INVOKECOMMAND_H

#include "WorkCommand.h"

class ExternalConnector;
class Payload;

class InvokeCommand : public WorkCommand {
private:
    std::string itemId_;
    ExternalConnector& connector_;
    std::string operation_;
    Payload arguments_;
    Payload result_;

public:
    InvokeCommand(WorkflowInstance& instance, Participant* actor, 
                  const std::string& itemId, ExternalConnector& connector, 
                  const std::string& operation, const Payload& arguments);
    
    std::string describe() const override;
    
    const Payload& result() const;

protected:
    void perform() override;
};

#endif // INVOKECOMMAND_H