#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <memory>
#include "mediator/Response.h"
#include "mediator/TaskRef.h"

class Colleague;
class Memento;
class SnapshotStore;

// COMMAND pattern - the COMMAND.
//
// Bound to the Colleague that performs it (the receiver) and to the scope it
// changes. execute() saves the receiver's state for that scope, performs the
// command and files the saved state in the SnapshotStore (when there is one);
// undo() puts that state back.
class Command {
protected:
    Colleague* receiver_;
    TaskRef scope_;
    SnapshotStore* store_;
    std::shared_ptr<Memento> before_;

public:
    Command(Colleague* receiver, const TaskRef& scope, SnapshotStore* store = nullptr);
    virtual ~Command() = default;

    // Throws std::invalid_argument when there is no receiver, and whatever perform() throws.
    Response execute();
    void undo();

    virtual std::string describe() const = 0;

protected:
    virtual Response perform() = 0;
};

#endif // COMMAND_H
