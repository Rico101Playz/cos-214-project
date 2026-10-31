#ifndef ENGINEMEMENTO_H
#define ENGINEMEMENTO_H

#include <string>

class TividyMediator;

class EngineMemento {
private:
    friend class TividyMediator;
    std::string label_;
    std::string state_;

    EngineMemento(const std::string& label, const std::string& state);

public:
    const std::string& label() const;
};

#endif
