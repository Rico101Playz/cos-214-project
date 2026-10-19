#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include "WorkCommand.h"

class CompleteCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // COMPLETECOMMAND_H