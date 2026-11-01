#include "strategy/StageMode.h"
#include "composite/CompositeTask.h"
#include "composite/LeafTask.h"

void StageMode::activateNode(Task& node) const {
    CompositeTask* stage = stageOf(&node);
    if (stage != nullptr) {
        stage->activate();
        return;
    }
    LeafTask* work = leafOf(&node);
    if (work != nullptr && work->status() == LeafTask::CREATED) {
        work->makeAvailable();
    }
}