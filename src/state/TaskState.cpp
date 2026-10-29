#include "state/TaskState.h"

#include <stdexcept>
#include <string>

void TaskState::illegal(const char* operation) const {
    throw std::logic_error(std::string("Illegal transition: ") + operation +
                           "() is not allowed in state '" + name() + "'");
}

void TaskState::makeAvailable(LeafTask&)                    { illegal("makeAvailable"); }
void TaskState::assign(LeafTask&, const std::string&)       { illegal("assign"); }
void TaskState::start(LeafTask&)                            { illegal("start"); }
void TaskState::complete(LeafTask&)                         { illegal("complete"); }
void TaskState::approve(LeafTask&)                          { illegal("approve"); }
void TaskState::reject(LeafTask&, const std::string&)       { illegal("reject"); }
void TaskState::escalate(LeafTask&)                         { illegal("escalate"); }
void TaskState::withdraw(LeafTask&)                         { illegal("withdraw"); }