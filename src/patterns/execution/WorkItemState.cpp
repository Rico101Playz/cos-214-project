#include "patterns/execution/WorkItemState.h"

#include <stdexcept>
#include <string>

void WorkItemState::illegal(const char* operation) const {
    throw std::logic_error(std::string("Illegal transition: ") + operation +
                           "() is not allowed in state '" + name() + "'");
}

void WorkItemState::makeAvailable(LeafWorkItem&)                    { illegal("makeAvailable"); }
void WorkItemState::assign(LeafWorkItem&, const std::string&)       { illegal("assign"); }
void WorkItemState::start(LeafWorkItem&)                            { illegal("start"); }
void WorkItemState::complete(LeafWorkItem&)                         { illegal("complete"); }
void WorkItemState::approve(LeafWorkItem&)                          { illegal("approve"); }
void WorkItemState::reject(LeafWorkItem&, const std::string&)       { illegal("reject"); }
void WorkItemState::escalate(LeafWorkItem&)                         { illegal("escalate"); }
void WorkItemState::withdraw(LeafWorkItem&)                         { illegal("withdraw"); }