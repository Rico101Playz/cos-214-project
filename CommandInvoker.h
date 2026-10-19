#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <memory>
#include <vector>
#include "WorkCommand.h"

class CommandInvoker {
private:
    std::vector<std::unique_ptr<WorkCommand>> history;

public:
    void invoke(std::unique_ptr<WorkCommand> command);
    void undoLast();
};

#endif // COMMANDINVOKER_H