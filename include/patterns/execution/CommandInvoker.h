#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <vector>
#include <memory>
#include "patterns/execution/WorkCommand.h"

class WorkflowInstance;

class CommandInvoker {
private:
    std::vector<std::unique_ptr<WorkCommand>> history;
public:
    void invoke(std::unique_ptr<WorkCommand> command, WorkflowInstance* instance);
    void undoLast(WorkflowInstance* instance);
};

#endif // COMMANDINVOKER_H