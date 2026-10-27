#ifndef JSON_H
#define JSON_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class JsonError : public std::runtime_error {
public:
    explicit JsonError(const std::string& message) : std::runtime_error(message) {}
};

class Json {
public:
    enum Type { NUL, BOOLEAN, NUMBER, STRING, ARRAY, OBJECT };

private:
    Type type_;
    bool bool_;
    double number_;
    std::string string_;
    std::vector<Json> array_;
    std::vector<std::pair<std::string, Json> > object_;

    static const Json& null();
    void dumpTo(std::string& out) const;

public:
    Json();
    Json(bool value);
    Json(int value);
    Json(std::size_t value);
    Json(double value);
    Json(const char* value);
    Json(const std::string& value);

    static Json array();
    static Json object();
    static Json parse(const std::string& text);
    static std::string quote(const std::string& text);

    Type type() const;
    bool isNull() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    const std::string& asString() const;

    std::size_t size() const;
    const Json& at(std::size_t index) const;
    Json& push(const Json& value);

    bool has(const std::string& key) const;
    const Json& get(const std::string& key) const;
    Json& set(const std::string& key, const Json& value);
    std::string str(const std::string& key, const std::string& fallback = "") const;

    std::string dump() const;
};

#endif
