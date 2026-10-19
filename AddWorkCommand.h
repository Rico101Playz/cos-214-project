#ifndef ADDWORKCOMMAND_H
#define ADDWORKCOMMAND_H

#include "WorkCommand.h"

class AddWorkCommand : public WorkCommand {
private:
    std::string stageId;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // ADDWORKCOMMAND_H