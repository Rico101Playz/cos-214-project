#ifndef PARALLELMODE_H
#define PARALLELMODE_H

#include "strategy/StageMode.h"

class ParallelMode : public StageMode {
public:
    std::string name() const override;
    void activate(CompositeTask& stage) override;
    void childCompleted(CompositeTask& stage, Task& child) override;
};

#endif