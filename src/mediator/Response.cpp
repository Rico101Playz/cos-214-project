#include "mediator/Response.h"

Response::Response() : ok_(true) {}

void Response::put(const std::string& key, const std::string& value) { data_[key] = value; }

void Response::putList(const std::string& key, const std::vector<std::string>& values) { lists_[key] = values; }

void Response::fail(const std::string& code, const std::string& message) {
    ok_ = false;
    code_ = code;
    message_ = message;
}

bool Response::ok() const { return ok_; }

const std::string& Response::code() const { return code_; }

const std::string& Response::message() const { return message_; }

const std::map<std::string, std::string>& Response::data() const { return data_; }

const std::map<std::string, std::vector<std::string> >& Response::lists() const { return lists_; }

std::string Response::data(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = data_.find(key);
    return it == data_.end() ? "" : it->second;
}
