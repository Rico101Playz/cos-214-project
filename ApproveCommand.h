#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include "WorkCommand.h"

class ApproveCommand : public WorkCommand {
private:
    std::string itemId;
    bool approved;
    std::string comment;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // APPROVECOMMAND_H