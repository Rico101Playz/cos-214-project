#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include "WorkCommand.h"

class AssignCommand : public WorkCommand {
private:
    std::string itemId;
    std::string participantName;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // ASSIGNCOMMAND_H