#include "WorkCommand.h"
#include <chrono>

WorkCommand::WorkCommand(WorkflowInstance& instance, Participant* actor): instance_(instance), actor_(actor), executedAt_(0) {}

WorkCommand::~WorkCommand() = default;

void WorkCommand::execute() {
    perform();
}

void WorkCommand::undo() {}

std::string WorkCommand::actorName() const {
    return actor_ ? "ActorName" : "System"; 
}

void WorkCommand::requireAssignee(const WorkItem& item, const std::string& action) const {}

WorkflowInstance& WorkCommand::instance() const { 
    return instance_; 
}

Participant* WorkCommand::actor() const { 
    return actor_; 
}

long WorkCommand::executedAt() const { 
    return executedAt_; 
}