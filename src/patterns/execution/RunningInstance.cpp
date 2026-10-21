#include "patterns/execution/RunningInstance.h"
#include "patterns/execution/CompletedInstance.h"
#include "patterns/execution/CancelledInstance.h"
#include "core/WorkflowInstance.h"

#include <memory>

std::string RunningInstance::name() const { return "Running"; }

void RunningInstance::advance(WorkflowInstance& instance) {
    if (instance.stagesFinished()) {
        instance.setState(std::unique_ptr<InstanceState>(new CompletedInstance()));
    }
}

void RunningInstance::cancel(WorkflowInstance& instance, const std::string& reason) {
    instance.root().cancel();
    instance.setReason(reason);
    instance.setState(std::unique_ptr<InstanceState>(new CancelledInstance()));
}