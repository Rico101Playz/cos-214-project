#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include "WorkCommand.h"
#include <string>

class ApproveCommand : public WorkCommand {
private:
    std::string itemId;
    bool approved;
    std::string comment;

protected:
    void perform() override;

public:
    ApproveCommand(const std::string& itemId, bool approved, const std::string& comment);
    std::string describe() const override;
};

#endif // APPROVECOMMAND_H