#ifndef REQUEST_H
#define REQUEST_H

#include <map>
#include <string>
#include <vector>
#include "mediator/Action.h"
#include "mediator/TaskRef.h"

// MEDIATOR pattern - the message every client sends to the Mediator.
//
// One Request type for every interaction: who asks, what they ask for, which
// task it is about, and extra values by name (e.g. "participant", "reason")
// or lists by name (e.g. "approvers", "blockedBy").
class Request {
private:
    std::string actor_;
    Action action_;
    TaskRef ref_;
    std::map<std::string, std::string> args_;
    std::map<std::string, std::vector<std::string> > lists_;

public:
    Request(Action action, const TaskRef& ref = TaskRef(), const std::string& actor = "");

    // Add a named value or list and return the request, so calls can be
    // chained: Request(Action::Assign, ref).with("participant", "Alice")
    Request& with(const std::string& key, const std::string& value);
    Request& withList(const std::string& key, const std::vector<std::string>& values);

    const std::string& actor() const;
    Action action() const;
    const TaskRef& ref() const;
    bool has(const std::string& key) const;
    // The named value or list, empty when it was not given.
    std::string arg(const std::string& key) const;
    std::vector<std::string> list(const std::string& key) const;
};

#endif // REQUEST_H
