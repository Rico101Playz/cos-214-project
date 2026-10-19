#ifndef INVOKECOMMAND_H
#define INVOKECOMMAND_H

#include "WorkCommand.h"

class InvokeCommand : public WorkCommand {
private:
    std::string itemId;
    std::string operation;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // INVOKECOMMAND_H