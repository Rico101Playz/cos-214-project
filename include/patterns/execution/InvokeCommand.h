#ifndef INVOKECOMMAND_H
#define INVOKECOMMAND_H

#include "WorkCommand.h"
#include <string>

class InvokeCommand : public WorkCommand {
private:
    std::string itemId;
    std::string operation;

protected:
    void perform() override;

public:
    InvokeCommand(const std::string& itemId, const std::string& operation);
    std::string describe() const override;
};

#endif // INVOKECOMMAND_H