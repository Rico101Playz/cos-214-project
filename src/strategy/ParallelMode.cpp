#include "strategy/ParallelMode.h"
#include "composite/CompositeTask.h"
#include <cstddef>

std::string ParallelMode::name() const { return "Parallel"; }

void ParallelMode::activate(CompositeTask& stage) {
    for (std::size_t i = 0; i < stage.childCount(); ++i) {
        activateNode(*stage.child(i));
    }
}

void ParallelMode::childCompleted(CompositeTask&, Task&) {}