#include "server/Json.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace {

const std::size_t MAX_DEPTH = 32;

class Parser {
private:
    const std::string& text_;
    std::size_t pos_;

    void fail(const std::string& what) const {
        throw JsonError(what + " at position " + std::to_string(pos_));
    }

    void skipSpace() {
        while (pos_ < text_.size() &&
               (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '\n' || text_[pos_] == '\r')) {
            ++pos_;
        }
    }

    bool consume(const char* word) {
        std::size_t len = std::string(word).size();
        if (text_.compare(pos_, len, word) == 0) {
            pos_ += len;
            return true;
        }
        return false;
    }

    static void appendUtf8(std::string& out, unsigned long cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    unsigned long hex4() {
        if (pos_ + 4 > text_.size()) fail("Short unicode escape");
        unsigned long value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = text_[pos_++];
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<unsigned long>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<unsigned long>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned long>(c - 'A' + 10);
            else fail("Bad unicode escape");
        }
        return value;
    }

    std::string parseString() {
        ++pos_;
        std::string out;
        while (true) {
            if (pos_ >= text_.size()) fail("Unterminated string");
            char c = text_[pos_++];
            if (c == '"') break;
            if (static_cast<unsigned char>(c) < 0x20) fail("Control character in string");
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos_ >= text_.size()) fail("Unterminated escape");
            char e = text_[pos_++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    unsigned long cp = hex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (!consume("\\u")) fail("Lone surrogate");
                        unsigned long low = hex4();
                        if (low < 0xDC00 || low > 0xDFFF) fail("Bad surrogate pair");
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                        fail("Lone surrogate");
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: fail("Bad escape");
            }
        }
        return out;
    }

    Json parseNumber() {
        std::size_t start = pos_;
        if (text_[pos_] == '-') ++pos_;
        if (pos_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[pos_]))) fail("Bad number");
        if (text_[pos_] == '0') {
            ++pos_;
        } else {
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            if (pos_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[pos_]))) fail("Bad number");
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            if (pos_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[pos_]))) fail("Bad number");
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) ++pos_;
        }
        return Json(std::strtod(text_.substr(start, pos_ - start).c_str(), nullptr));
    }

public:
    explicit Parser(const std::string& text) : text_(text), pos_(0) {}

    Json value(std::size_t depth) {
        if (depth > MAX_DEPTH) fail("Nesting too deep");
        skipSpace();
        if (pos_ >= text_.size()) fail("Unexpected end");
        char c = text_[pos_];
        if (c == '{') {
            ++pos_;
            Json obj = Json::object();
            skipSpace();
            if (pos_ < text_.size() && text_[pos_] == '}') { ++pos_; return obj; }
            while (true) {
                skipSpace();
                if (pos_ >= text_.size() || text_[pos_] != '"') fail("Expected key");
                std::string key = parseString();
                skipSpace();
                if (pos_ >= text_.size() || text_[pos_] != ':') fail("Expected ':'");
                ++pos_;
                obj.set(key, value(depth + 1));
                skipSpace();
                if (pos_ < text_.size() && text_[pos_] == ',') { ++pos_; continue; }
                if (pos_ < text_.size() && text_[pos_] == '}') { ++pos_; return obj; }
                fail("Expected ',' or '}'");
            }
        }
        if (c == '[') {
            ++pos_;
            Json arr = Json::array();
            skipSpace();
            if (pos_ < text_.size() && text_[pos_] == ']') { ++pos_; return arr; }
            while (true) {
                arr.push(value(depth + 1));
                skipSpace();
                if (pos_ < text_.size() && text_[pos_] == ',') { ++pos_; continue; }
                if (pos_ < text_.size() && text_[pos_] == ']') { ++pos_; return arr; }
                fail("Expected ',' or ']'");
            }
        }
        if (c == '"') return Json(parseString());
        if (consume("true")) return Json(true);
        if (consume("false")) return Json(false);
        if (consume("null")) return Json();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parseNumber();
        fail("Unexpected character");
        return Json();
    }

    void finish() {
        skipSpace();
        if (pos_ != text_.size()) fail("Trailing characters");
    }
};

}

Json::Json() : type_(NUL), bool_(false), number_(0) {}
Json::Json(bool value) : type_(BOOLEAN), bool_(value), number_(0) {}
Json::Json(int value) : type_(NUMBER), bool_(false), number_(value) {}
Json::Json(std::size_t value) : type_(NUMBER), bool_(false), number_(static_cast<double>(value)) {}
Json::Json(double value) : type_(NUMBER), bool_(false), number_(value) {}
Json::Json(const char* value) : type_(STRING), bool_(false), number_(0), string_(value ? value : "") {}
Json::Json(const std::string& value) : type_(STRING), bool_(false), number_(0), string_(value) {}

Json Json::array() {
    Json j;
    j.type_ = ARRAY;
    return j;
}

Json Json::object() {
    Json j;
    j.type_ = OBJECT;
    return j;
}

Json Json::parse(const std::string& text) {
    Parser parser(text);
    Json result = parser.value(0);
    parser.finish();
    return result;
}

const Json& Json::null() {
    static const Json instance;
    return instance;
}

Json::Type Json::type() const { return type_; }
bool Json::isNull() const { return type_ == NUL; }
bool Json::isString() const { return type_ == STRING; }
bool Json::isArray() const { return type_ == ARRAY; }
bool Json::isObject() const { return type_ == OBJECT; }

bool Json::asBool(bool fallback) const { return type_ == BOOLEAN ? bool_ : fallback; }

double Json::asNumber(double fallback) const { return type_ == NUMBER ? number_ : fallback; }

const std::string& Json::asString() const {
    static const std::string empty;
    return type_ == STRING ? string_ : empty;
}

std::size_t Json::size() const {
    if (type_ == ARRAY) return array_.size();
    if (type_ == OBJECT) return object_.size();
    return 0;
}

const Json& Json::at(std::size_t index) const {
    return (type_ == ARRAY && index < array_.size()) ? array_[index] : null();
}

Json& Json::push(const Json& value) {
    if (type_ != ARRAY) throw JsonError("push on a non-array");
    array_.push_back(value);
    return *this;
}

bool Json::has(const std::string& key) const {
    for (std::size_t i = 0; i < object_.size(); ++i) {
        if (object_[i].first == key) return true;
    }
    return false;
}

const Json& Json::get(const std::string& key) const {
    for (std::size_t i = 0; i < object_.size(); ++i) {
        if (object_[i].first == key) return object_[i].second;
    }
    return null();
}

Json& Json::set(const std::string& key, const Json& value) {
    if (type_ != OBJECT) throw JsonError("set on a non-object");
    for (std::size_t i = 0; i < object_.size(); ++i) {
        if (object_[i].first == key) {
            object_[i].second = value;
            return *this;
        }
    }
    object_.push_back(std::make_pair(key, value));
    return *this;
}

std::string Json::str(const std::string& key, const std::string& fallback) const {
    const Json& value = get(key);
    return value.isString() ? value.asString() : fallback;
}

std::string Json::quote(const std::string& text) {
    std::string out = "\"";
    for (std::size_t i = 0; i < text.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '<': out += "\\u003c"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out + "\"";
}

void Json::dumpTo(std::string& out) const {
    switch (type_) {
        case NUL: out += "null"; break;
        case BOOLEAN: out += bool_ ? "true" : "false"; break;
        case NUMBER: {
            if (!std::isfinite(number_)) {
                out += "null";
            } else if (number_ == std::floor(number_) && std::fabs(number_) < 1e15) {
                out += std::to_string(static_cast<long long>(number_));
            } else {
                char buf[32];
                std::snprintf(buf, sizeof buf, "%.17g", number_);
                out += buf;
            }
            break;
        }
        case STRING: out += quote(string_); break;
        case ARRAY:
            out += '[';
            for (std::size_t i = 0; i < array_.size(); ++i) {
                if (i) out += ',';
                array_[i].dumpTo(out);
            }
            out += ']';
            break;
        case OBJECT:
            out += '{';
            for (std::size_t i = 0; i < object_.size(); ++i) {
                if (i) out += ',';
                out += quote(object_[i].first);
                out += ':';
                object_[i].second.dumpTo(out);
            }
            out += '}';
            break;
    }
}

std::string Json::dump() const {
    std::string out;
    dumpTo(out);
    return out;
}
