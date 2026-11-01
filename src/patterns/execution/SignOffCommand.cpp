#include "patterns/execution/SignOffCommand.h"
#include "core/WorkflowInstance.h"
#include <stdexcept>
#include <utility>

SignOffCommand::SignOffCommand(SnapshotStore* store, std::string instanceId, Decision decision,
                               const WorkflowFactory* factory)
    : WorkCommand(store, std::move(instanceId)), decision(decision), factory(factory) {}

void SignOffCommand::perform(WorkflowInstance* instance) {
    if (instance == nullptr) throw std::invalid_argument("SignOffCommand: instance is null");
    if (decision == SIGN_OFF) {
        instance->signOff();
        return;
    }
    if (factory == nullptr) throw std::invalid_argument("SignOffCommand: reopening needs the policy's factory");
    instance->reopenForFixes(*factory);
}

std::string SignOffCommand::describe() const {
    return decision == SIGN_OFF ? "SignOffCommand: sign off" : "SignOffCommand: reopen for fixes";
}
