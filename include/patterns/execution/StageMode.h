#ifndef STAGEMODE_H
#define STAGEMODE_H

#include <string>

class CompositeStage;
class WorkNode;

class StageMode {
protected:
//begin with this node
    void activateNode(WorkNode& node) const;

public:
    virtual ~StageMode() = default;

    virtual std::string name() const = 0;
    virtual void activate(CompositeStage& stage) = 0;
    virtual void childCompleted(CompositeStage& stage, WorkNode& child) = 0;
};

#endif