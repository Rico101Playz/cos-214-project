#ifndef ESCALATECOMMAND_H
#define ESCALATECOMMAND_H

#include "WorkCommand.h"

class EscalateCommand : public WorkCommand {
private:
    std::string itemId;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // ESCALATECOMMAND_H