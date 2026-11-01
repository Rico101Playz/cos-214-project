#include "patterns/execution/AwaitingSignOffInstance.h"
#include "patterns/execution/CompletedInstance.h"
#include "patterns/execution/CancelledInstance.h"
#include "patterns/execution/RunningInstance.h"
#include "core/WorkflowInstance.h"

#include <memory>

std::string AwaitingSignOffInstance::name() const { return "AwaitingSignOff"; }

void AwaitingSignOffInstance::cancel(WorkflowInstance& instance, const std::string& reason) {
    instance.root().cancel();
    instance.setReason(reason);
    instance.setState(std::unique_ptr<InstanceState>(new CancelledInstance()));
}

void AwaitingSignOffInstance::signOff(WorkflowInstance& instance) {
    instance.setState(std::unique_ptr<InstanceState>(new CompletedInstance()));
}

void AwaitingSignOffInstance::reopen(WorkflowInstance& instance) {
    instance.setState(std::unique_ptr<InstanceState>(new RunningInstance()));
}
