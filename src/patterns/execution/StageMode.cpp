#include "patterns/execution/StageMode.h"
#include "patterns/structural/CompositeStage.h"
#include "core/WorkItem.h"

void StageMode::activateNode(WorkNode& node) const {
    CompositeStage* stage = dynamic_cast<CompositeStage*>(&node);
    if (stage != nullptr) {
        stage->activate();
        return;
    }
    WorkItem* work = dynamic_cast<WorkItem*>(&node);
    if (work != nullptr && work->status() == WorkItem::CREATED) {
        work->makeAvailable();
    }
}