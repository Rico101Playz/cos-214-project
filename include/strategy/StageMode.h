#ifndef STAGEMODE_H
#define STAGEMODE_H

#include <string>

class CompositeTask;
class Task;

class StageMode {
protected:
//begin with this node
    void activateNode(Task& node) const;

public:
    virtual ~StageMode() = default;

    virtual std::string name() const = 0;
    virtual void activate(CompositeTask& stage) = 0;
    virtual void childCompleted(CompositeTask& stage, Task& child) = 0;
};

#endif