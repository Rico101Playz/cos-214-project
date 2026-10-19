#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

#include "WorkCommand.h"
#include <string>

class StartCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform() override;

public:
    explicit StartCommand(const std::string& itemId);
    std::string describe() const override;
};

#endif // STARTCOMMAND_H