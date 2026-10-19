#ifndef SETDATACOMMAND_H
#define SETDATACOMMAND_H

#include "WorkCommand.h"

class SetDataCommand : public WorkCommand {
private:
    std::string nodeId;
    std::string key;
    std::string value;

protected:
    void perform() override;

public:
    std::string describe() const override;
};

#endif // SETDATACOMMAND_H