#include "patterns/execution/EngineMemento.h"

EngineMemento::EngineMemento(const std::string& label, const std::string& state)
    : label_(label), state_(state) {}

const std::string& EngineMemento::label() const { return label_; }
