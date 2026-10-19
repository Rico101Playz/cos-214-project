#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include "WorkCommand.h"
#include <string>

class CancelCommand : public WorkCommand {
private:
    std::string nodeId;
    std::string reason;

protected:
    void perform() override;

public:
    CancelCommand(const std::string& nodeId, const std::string& reason);
    std::string describe() const override;
};

#endif // CANCELCOMMAND_H