#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <memory>
#include "WorkCommand.h"

class CommandInvoker {
public:
    void invoke(std::unique_ptr<WorkCommand> command);
    void undoLast();
};

#endif // COMMANDINVOKER_H