#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include "WorkCommand.h"
#include <string>

class CompleteCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform() override;

public:
    explicit CompleteCommand(const std::string& itemId);
    std::string describe() const override;
};

#endif // COMPLETECOMMAND_H