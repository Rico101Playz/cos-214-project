#include "core/TextRecord.h"

std::string TextRecord::escape(const std::string& field) {
    std::string out;
    for (std::size_t i = 0; i < field.size(); ++i) {
        char c = field[i];
        if (c == '\\') out += "\\\\";
        else if (c == '\t') out += "\\t";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out += c;
    }
    return out;
}

std::string TextRecord::unescape(const std::string& field) {
    std::string out;
    for (std::size_t i = 0; i < field.size(); ++i) {
        char c = field[i];
        if (c == '\\' && i + 1 < field.size()) {
            char n = field[++i];
            if (n == 't') out += '\t';
            else if (n == 'n') out += '\n';
            else if (n == 'r') out += '\r';
            else out += n;
        } else {
            out += c;
        }
    }
    return out;
}

std::string TextRecord::line(const std::vector<std::string>& fields) {
    std::string out;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i) out += '\t';
        out += escape(fields[i]);
    }
    return out + "\n";
}

std::vector<std::string> TextRecord::split(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            current += c;
            current += line[++i];
        } else if (c == '\t') {
            fields.push_back(unescape(current));
            current.clear();
        } else if (c != '\r') {
            current += c;
        }
    }
    fields.push_back(unescape(current));
    return fields;
}
