#include "state/StateFactory.h"

#include "state/TaskState.h"

#include "state/CreatedState.h"
#include "state/AvailableState.h"
#include "state/AssignedState.h"
#include "state/InProgressState.h"
#include "state/AwaitingApprovalState.h"
#include "state/CompletedState.h"
#include "state/RejectedState.h"
#include "state/CancelledState.h"
#include "state/EscalatedState.h"



#include <stdexcept>

std::unique_ptr<TaskState> createWorkItemState(const std::string& name) {
    if (name == "Created")          return std::unique_ptr<TaskState>(new CreatedState());
    if (name == "Available")        return std::unique_ptr<TaskState>(new AvailableState());
    if (name == "Assigned")         return std::unique_ptr<TaskState>(new AssignedState());
    if (name == "InProgress")       return std::unique_ptr<TaskState>(new InProgressState());
    if (name == "AwaitingApproval") return std::unique_ptr<TaskState>(new AwaitingApprovalState());
    if (name == "Completed")        return std::unique_ptr<TaskState>(new CompletedState());
    if (name == "Rejected")         return std::unique_ptr<TaskState>(new RejectedState());
    if (name == "Cancelled")        return std::unique_ptr<TaskState>(new CancelledState());
    if (name == "Escalated")        return std::unique_ptr<TaskState>(new EscalatedState());
    throw std::invalid_argument("Unknown TaskState: " + name);
}
