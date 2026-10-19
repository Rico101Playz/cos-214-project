#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include "WorkCommand.h"

class RejectCommand : public WorkCommand {
private:
    std::string itemId;
    std::string reason;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // REJECTCOMMAND_H