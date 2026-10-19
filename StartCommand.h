#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

#include "WorkCommand.h"

class StartCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // STARTCOMMAND_H