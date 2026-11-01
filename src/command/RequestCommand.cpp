#include "command/RequestCommand.h"
#include <map>
#include <vector>
#include "mediator/Colleague.h"

RequestCommand::RequestCommand(Colleague* receiver, const Request& request, SnapshotStore* store)
    : Command(receiver, request.ref(), store), request_(request) {}

const Request& RequestCommand::request() const { return request_; }

std::string RequestCommand::describe() const {
    std::string out = toString(request_.action()) + ":";
    std::string sep = " ";
    const std::string& task = request_.ref().taskId;
    if (!task.empty()) {
        out += sep + "task=" + task;
        sep = ", ";
    }
    // the request's values in name order
    std::map<std::string, std::string> args;
    static const char* const keys[] = {"comment", "key", "kind", "name", "parent", "participant", "person", "priority",
                                       "reason", "role", "slot", "unit", "value", "operation"};
    for (std::size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        if (request_.has(keys[i])) args[keys[i]] = request_.arg(keys[i]);
    }
    for (std::map<std::string, std::string>::const_iterator it = args.begin(); it != args.end(); ++it) {
        out += sep + it->first + "=" + it->second;
        sep = ", ";
    }
    std::vector<std::string> pending = request_.list("pending");
    if (request_.action() == Action::RouteApproval) {
        std::string roles;
        for (std::size_t i = 0; i < pending.size(); ++i) roles += (i ? "," : "") + pending[i];
        out += sep + "pending=" + roles;
    }
    return out;
}

Response RequestCommand::perform() { return receiver_->handle(request_); }
