#include "patterns/execution/InstanceState.h"

#include <stdexcept>
#include <string>

void InstanceState::illegal(const char* operation) const {
    throw std::logic_error(std::string("Illegal instance operation: ") + operation +
                           "() is not allowed in state '" + name() + "'");
}

void InstanceState::start(WorkflowInstance&)                      { illegal("start"); }
void InstanceState::cancel(WorkflowInstance&, const std::string&) { illegal("cancel"); }


void InstanceState::advance(WorkflowInstance&) {}