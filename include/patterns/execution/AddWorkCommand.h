#ifndef ADDWORKCOMMAND_H
#define ADDWORKCOMMAND_H

#include "WorkCommand.h"
#include <string>

class AddWorkCommand : public WorkCommand {
private:
    std::string stageId;

protected:
    void perform() override;

public:
    explicit AddWorkCommand(const std::string& stageId);
    std::string describe() const override;
};

#endif // ADDWORKCOMMAND_H