#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <vector>
#include <memory>
#include "patterns/execution/WorkCommand.h"

class WorkflowInstance;

class CommandInvoker {
private:
    std::vector<std::unique_ptr<WorkCommand>> history;
    std::size_t limit = 0;
public:
    void setLimit(std::size_t max);
    std::size_t size() const;
    void invoke(std::unique_ptr<WorkCommand> command, WorkflowInstance* instance);
    void undoLast(WorkflowInstance* instance);
};

#endif // COMMANDINVOKER_H