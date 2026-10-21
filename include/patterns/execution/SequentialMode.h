#ifndef SEQUENTIALMODE_H
#define SEQUENTIALMODE_H

#include "StageMode.h"

class SequentialMode : public StageMode {
public:
    std::string name() const override;
    void activate(CompositeStage& stage) override;
    void childCompleted(CompositeStage& stage, WorkNode& child) override;
};

#endif