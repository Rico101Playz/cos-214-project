#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include <vector>
#include <memory>
#include "command/Command.h"
#include "command/CommandHistory.h"

// COMMAND pattern - the INVOKER.
//
// Runs commands and records them in a CommandHistory: its own, or one kept
// elsewhere (TividyMediator is the engine's invoker; HistoryFacade keeps the
// history).
/**
 * @class CommandInvoker
 * @brief Invoker class in the Command pattern responsible for executing and rolling back commands.
 *
 * Executes commands and records them in a CommandHistory, its own or one kept
 * elsewhere, so that they can be rolled back in reverse order (undo).
 */
class CommandInvoker {
private:
    CommandHistory own_;
    CommandHistory* history_;

public:
    CommandInvoker();
    explicit CommandInvoker(CommandHistory& history);
    CommandInvoker(const CommandInvoker&) = delete;
    CommandInvoker& operator=(const CommandInvoker&) = delete;

    void setLimit(std::size_t max);
    std::size_t size() const;
    // Runs the command and records it; a command that throws is not recorded.
    /**
     * @brief Executes a command and logs it in the history stack.
     *
     * @param[in] command Unique pointer to the Command to execute.
     * @return Response The result returned by the command.
     */
    Response invoke(std::unique_ptr<Command> command);

    /**
     * @brief Reverses the most recently executed command.
     *
     * Pops the last command from the history stack and invokes its undo method.
     */
    void undoLast();
    // The commands run so far, most recent last.
    const CommandHistory& history() const;
};

#endif // COMMANDINVOKER_H
