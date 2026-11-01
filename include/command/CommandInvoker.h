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
    Response invoke(std::unique_ptr<Command> command);
    void undoLast();
    // The commands run so far, most recent last.
    const CommandHistory& history() const;
};

#endif // COMMANDINVOKER_H
