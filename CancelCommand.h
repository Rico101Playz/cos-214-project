#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include "WorkCommand.h"

class CancelCommand : public WorkCommand {
private:
    std::string nodeId;
    std::string reason;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // CANCELCOMMAND_H