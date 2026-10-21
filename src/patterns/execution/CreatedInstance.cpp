#include "patterns/execution/CreatedInstance.h"
#include "patterns/execution/RunningInstance.h"
#include "patterns/execution/CancelledInstance.h"
#include "core/WorkflowInstance.h"

#include <memory>

std::string CreatedInstance::name() const { return "Created"; }

void CreatedInstance::start(WorkflowInstance& instance) {
    instance.setState(std::unique_ptr<InstanceState>(new RunningInstance()));
    instance.openFirstStage();
    instance.advance();
}

void CreatedInstance::cancel(WorkflowInstance& instance, const std::string& reason) {
    instance.root().cancel();
    instance.setReason(reason);
    instance.setState(std::unique_ptr<InstanceState>(new CancelledInstance()));
}