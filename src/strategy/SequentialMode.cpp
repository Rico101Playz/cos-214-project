#include "strategy/SequentialMode.h"
#include "composite/CompositeTask.h"
#include <cstddef>

std::string SequentialMode::name() const { return "Sequential"; }

void SequentialMode::activate(CompositeTask& stage) {
    if (stage.childCount() > 0) {
        activateNode(*stage.child(0));
    }
}

void SequentialMode::childCompleted(CompositeTask& stage, Task& child) {
    for (std::size_t i = 0; i < stage.childCount(); ++i) {
        if (stage.child(i) == &child) {
            if (i + 1 < stage.childCount()) {
                activateNode(*stage.child(i + 1));
            }
            return;
        }
    }
}
