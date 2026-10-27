#ifndef ENGINEMEMENTO_H
#define ENGINEMEMENTO_H

#include <string>

class Tividy;

class EngineMemento {
private:
    friend class Tividy;
    std::string label_;
    std::string state_;

    EngineMemento(const std::string& label, const std::string& state);

public:
    const std::string& label() const;
};

#endif
