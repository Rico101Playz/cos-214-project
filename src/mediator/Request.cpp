#include "mediator/Request.h"

Request::Request(Action action, const TaskRef& ref, const std::string& actor)
    : actor_(actor), action_(action), ref_(ref) {}

Request& Request::with(const std::string& key, const std::string& value) {
    args_[key] = value;
    return *this;
}

Request& Request::withList(const std::string& key, const std::vector<std::string>& values) {
    lists_[key] = values;
    return *this;
}

const std::string& Request::actor() const { return actor_; }

Action Request::action() const { return action_; }

const TaskRef& Request::ref() const { return ref_; }

bool Request::has(const std::string& key) const { return args_.count(key) != 0; }

std::string Request::arg(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = args_.find(key);
    return it == args_.end() ? "" : it->second;
}

std::vector<std::string> Request::list(const std::string& key) const {
    std::map<std::string, std::vector<std::string> >::const_iterator it = lists_.find(key);
    return it == lists_.end() ? std::vector<std::string>() : it->second;
}
