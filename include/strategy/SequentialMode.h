#ifndef SEQUENTIALMODE_H
#define SEQUENTIALMODE_H

#include "strategy/StageMode.h"

class SequentialMode : public StageMode {
public:
    std::string name() const override;
    void activate(CompositeTask& stage) override;
    void childCompleted(CompositeTask& stage, Task& child) override;
};

#endif