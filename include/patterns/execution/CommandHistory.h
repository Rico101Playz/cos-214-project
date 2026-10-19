#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>
#include <memory>
#include "patterns/execution/WorkCommand.h" 

class CommandHistory {
private:
    std::vector<std::unique_ptr<WorkCommand>> executed;

public:
    void push(std::unique_ptr<WorkCommand> command);
    std::unique_ptr<WorkCommand> pop();
    bool canUndo() const;
};

#endif // COMMANDHISTORY_H