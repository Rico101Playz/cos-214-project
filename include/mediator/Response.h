#ifndef RESPONSE_H
#define RESPONSE_H

#include <map>
#include <string>
#include <vector>

// MEDIATOR pattern - what the Mediator answers to a Request.
//
// ok() is false when the request was refused; code() then says why:
// "forbidden" when the actor may not do it, "invalid" for something unknown
// or malformed, "not_allowed_now" for an action the current state does not
// allow, "error" otherwise. Results come back as named values or lists, e.g.
// data("project") after StartProject.
class Response {
private:
    bool ok_;
    std::string code_;
    std::string message_;
    std::map<std::string, std::string> data_;
    std::map<std::string, std::vector<std::string> > lists_;

public:
    Response();

    void put(const std::string& key, const std::string& value);
    void putList(const std::string& key, const std::vector<std::string>& values);
    void fail(const std::string& code, const std::string& message);

    bool ok() const;
    const std::string& code() const;
    const std::string& message() const;
    const std::map<std::string, std::string>& data() const;
    const std::map<std::string, std::vector<std::string> >& lists() const;
    // The named value, or "" when there is none.
    std::string data(const std::string& key) const;
};

#endif // RESPONSE_H
