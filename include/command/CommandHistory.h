#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>
#include <memory>
#include "command/Command.h"

/**
 * @class CommandHistory
 * @brief Manages a stack of executed commands to support undo operations.
 *
 * Part of the Command pattern; records commands that have been executed,
 * allowing the engine to inspect undo availability and retrieve recently
 * executed commands in reverse chronological (LIFO) order.
 */
class CommandHistory {
private:
    std::vector<std::unique_ptr<Command>> executed; /*!< Stack storing unique pointers to executed commands. */
    std::size_t limit = 0;

public:
    /**
     * @brief Pushes an executed command onto the history stack.
     * @param[in] command Unique pointer to the executed Command to store.
     */
    void push(std::unique_ptr<Command> command);

    /**
     * @brief Pops and returns the most recently executed command from the stack.
     * @return std::unique_ptr<Command> The last executed command, or nullptr if the history is empty.
     */
    std::unique_ptr<Command> pop();

    /**
     * @brief Checks whether there are any commands available in history to undo.
     * @return true if the history stack is not empty, false otherwise.
     */
    bool canUndo() const;
    std::size_t size() const;
    // Keeps at most max commands, dropping the oldest first; 0 keeps them all.
    void setLimit(std::size_t max);
};

#endif // COMMANDHISTORY_H