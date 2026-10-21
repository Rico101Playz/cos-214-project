#ifndef PARALLELMODE_H
#define PARALLELMODE_H

#include "StageMode.h"

class ParallelMode : public StageMode {
public:
    std::string name() const override;
    void activate(CompositeStage& stage) override;
    void childCompleted(CompositeStage& stage, WorkNode& child) override;
};

#endif