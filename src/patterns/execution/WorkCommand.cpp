#include "patterns/execution/WorkCommand.h"
#include "patterns/execution/Memento.h"
#include "core/WorkflowInstance.h" // Needed to call createSnapshot/restore
#include <utility>

WorkCommand::WorkCommand(SnapshotStore* st, std::string instId)
    : store(st), instanceId(std::move(instId)) {}

void WorkCommand::execute(WorkflowInstance* instance) {
    if (!instance) return;

    // 1. Create the snapshot BEFORE doing the work (UML: before_)
    before = instance->createSnapshot(describe());
    
    // 2. Execute the actual command logic
    perform(instance);
    
    // 3. Save the snapshot to the historical store
    if (store && before) {
        store->save(instanceId, before);
    }
}

void WorkCommand::undo(WorkflowInstance* instance) {
    if (!instance) return;

    // Restore the state using the memento we are holding
    if (before) {
        instance->restoreFromSnapshot(before.get());
    }
}