#ifndef TEXTRECORD_H
#define TEXTRECORD_H

#include <string>
#include <vector>

class TextRecord {
public:
    static std::string escape(const std::string& field);
    static std::string unescape(const std::string& field);
    static std::string line(const std::vector<std::string>& fields);
    static std::vector<std::string> split(const std::string& line);
};

#endif
