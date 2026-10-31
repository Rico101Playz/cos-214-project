#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>
#include <memory>
#include "command/Command.h" 

class CommandHistory {
private:
    std::vector<std::unique_ptr<Command>> executed;
    std::size_t limit = 0;

public:
    void push(std::unique_ptr<Command> command);
    std::unique_ptr<Command> pop();
    bool canUndo() const;
    std::size_t size() const;
    // Keeps at most max commands, dropping the oldest first; 0 keeps them all.
    void setLimit(std::size_t max);
};

#endif // COMMANDHISTORY_H