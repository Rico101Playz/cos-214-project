#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>
#include <memory>
#include "patterns/execution/WorkCommand.h" 

/**
 * @class CommandHistory
 * @brief Manages a stack of executed commands to support undo operations.
 * 
 * Part of the Command pattern; records commands that have been executed on workflow
 * instances, allowing the engine to inspect undo availability and retrieve recently
 * executed commands in reverse chronological (LIFO) order.
 */
class CommandHistory {
private:
    std::vector<std::unique_ptr<WorkCommand>> executed; /*!< Stack storing unique pointers to executed commands. */

public:
    /**
     * @brief Pushes an executed command onto the history stack.
     * @param[in] command Unique pointer to the executed WorkCommand to store.
     */
    void push(std::unique_ptr<WorkCommand> command);

    /**
     * @brief Pops and returns the most recently executed command from the stack.
     * @return std::unique_ptr<WorkCommand> The last executed command, or nullptr if the history is empty.
     */
    std::unique_ptr<WorkCommand> pop();

    /**
     * @brief Checks whether there are any commands available in history to undo.
     * @return true if the history stack is not empty, false otherwise.
     */
    bool canUndo() const;
};

#endif // COMMANDHISTORY_H