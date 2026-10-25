#include "patterns/creation/StateFactory.h"

#include "patterns/execution/WorkItemState.h"
#include "patterns/execution/InstanceState.h"

#include "patterns/execution/CreatedState.h"
#include "patterns/execution/AvailableState.h"
#include "patterns/execution/AssignedState.h"
#include "patterns/execution/InProgressState.h"
#include "patterns/execution/AwaitingApprovalState.h"
#include "patterns/execution/CompletedState.h"
#include "patterns/execution/RejectedState.h"
#include "patterns/execution/CancelledState.h"
#include "patterns/execution/EscalatedState.h"

#include "patterns/execution/CreatedInstance.h"
#include "patterns/execution/RunningInstance.h"
#include "patterns/execution/CompletedInstance.h"
#include "patterns/execution/CancelledInstance.h"


#include <stdexcept>

std::unique_ptr<WorkItemState> createWorkItemState(const std::string& name) {
    if (name == "Created")          return std::unique_ptr<WorkItemState>(new CreatedState());
    if (name == "Available")        return std::unique_ptr<WorkItemState>(new AvailableState());
    if (name == "Assigned")         return std::unique_ptr<WorkItemState>(new AssignedState());
    if (name == "InProgress")       return std::unique_ptr<WorkItemState>(new InProgressState());
    if (name == "AwaitingApproval") return std::unique_ptr<WorkItemState>(new AwaitingApprovalState());
    if (name == "Completed")        return std::unique_ptr<WorkItemState>(new CompletedState());
    if (name == "Rejected")         return std::unique_ptr<WorkItemState>(new RejectedState());
    if (name == "Cancelled")        return std::unique_ptr<WorkItemState>(new CancelledState());
    if (name == "Escalated")        return std::unique_ptr<WorkItemState>(new EscalatedState());
    throw std::invalid_argument("Unknown WorkItemState: " + name);
}

std::unique_ptr<InstanceState> createInstanceState(const std::string& name) {
    if (name == "Created")   return std::unique_ptr<InstanceState>(new CreatedInstance());
    if (name == "Running")   return std::unique_ptr<InstanceState>(new RunningInstance());
    if (name == "Completed") return std::unique_ptr<InstanceState>(new CompletedInstance());
    if (name == "Cancelled") return std::unique_ptr<InstanceState>(new CancelledInstance());
    throw std::invalid_argument("Unknown InstanceState: " + name);
}