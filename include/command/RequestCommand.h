#ifndef REQUEST_COMMAND_H
#define REQUEST_COMMAND_H

#include "command/Command.h"
#include "mediator/Request.h"

// COMMAND pattern - the CONCRETE COMMAND.
//
// Carries one Request to the Colleague that performs it, e.g. ProjectFacade
// for "Assign: task=Item1, participant=Alice". The mediator runs one for every
// change it makes to a project, the engine's own steps (SetData, RouteApproval,
// Sign, ...) included, so each can be saved and undone the same way.
class RequestCommand : public Command {
private:
    Request request_;

public:
    RequestCommand(Colleague* receiver, const Request& request, SnapshotStore* store = nullptr);

    const Request& request() const;
    // The action and what it names, e.g. "Assign: task=Item1, participant=Alice".
    std::string describe() const override;

protected:
    // The receiver's answer; it throws instead when it refuses the Request.
    Response perform() override;
};

#endif // REQUEST_COMMAND_H
