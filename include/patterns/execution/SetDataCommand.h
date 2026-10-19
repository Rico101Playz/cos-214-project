#ifndef SETDATACOMMAND_H
#define SETDATACOMMAND_H

#include "WorkCommand.h"
#include <string>

class SetDataCommand : public WorkCommand {
private:
    std::string nodeId;
    std::string key;
    std::string value;

protected:
    void perform() override;

public:
    SetDataCommand(const std::string& nodeId, const std::string& key, const std::string& value);
    std::string describe() const override;
};

#endif // SETDATACOMMAND_H