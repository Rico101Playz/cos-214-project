#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include "WorkCommand.h"
#include <string>

class AssignCommand : public WorkCommand {
private:
    std::string itemId;
    std::string participantName;

protected:
    void perform() override;

public:
    AssignCommand(const std::string& itemId, const std::string& participantName);
    std::string describe() const override;
};

#endif // ASSIGNCOMMAND_H