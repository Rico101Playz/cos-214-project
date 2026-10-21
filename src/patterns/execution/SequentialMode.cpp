#include "patterns/execution/SequentialMode.h"
#include "patterns/structural/CompositeStage.h"
#include <cstddef>

std::string SequentialMode::name() const { return "Sequential"; }

void SequentialMode::activate(CompositeStage& stage) {
    if (stage.childCount() > 0) {
        activateNode(*stage.child(0));
    }
}

void SequentialMode::childCompleted(CompositeStage& stage, WorkNode& child) {
    for (std::size_t i = 0; i < stage.childCount(); ++i) {
        if (stage.child(i) == &child) {
            if (i + 1 < stage.childCount()) {
                activateNode(*stage.child(i + 1));
            }
            return;
        }
    }
}
