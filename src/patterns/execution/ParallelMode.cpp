#include "patterns/execution/ParallelMode.h"
#include "patterns/structural/CompositeStage.h"
#include <cstddef>

std::string ParallelMode::name() const { return "Parallel"; }

void ParallelMode::activate(CompositeStage& stage) {
    for (std::size_t i = 0; i < stage.childCount(); ++i) {
        activateNode(*stage.child(i));
    }
}

void ParallelMode::childCompleted(CompositeStage&, WorkNode&) {}