#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <memory>
#include <string>
#include "CommandHistory.h"
#include "WorkCommand.h"

class CommandInvoker {
private:
    CommandHistory& history_;

public:
    explicit CommandInvoker(CommandHistory& history);
    
    void invoke(std::unique_ptr<WorkCommand> command);
    
    std::string undoLast();
};

#endif // COMMANDINVOKER_H