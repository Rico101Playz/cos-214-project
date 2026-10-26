#include "patterns/execution/WorkCommand.h"
#include "core/WorkflowInstance.h"

WorkCommand::WorkCommand(SnapshotStore* st, std::string instId)
    : store(st), instanceId(std::move(instId)) {}

void WorkCommand::execute(WorkflowInstance* instance) {
    if (!instance) return;
    before = instance->createMemento(describe()); 
    perform(instance);                            
    if (store && before) store->save(instanceId, before); 
}

void WorkCommand::undo(WorkflowInstance* instance) {
    if (instance && before) {
        instance->restoreFromMemento(before.get());
    }
}