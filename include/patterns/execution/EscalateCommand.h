#ifndef ESCALATECOMMAND_H
#define ESCALATECOMMAND_H

#include "WorkCommand.h"
#include <string>

class EscalateCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform() override;

public:
    explicit EscalateCommand(const std::string& itemId);
    std::string describe() const override;
};

#endif // ESCALATECOMMAND_H