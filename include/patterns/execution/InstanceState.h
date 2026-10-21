#ifndef INSTANCESTATE_H
#define INSTANCESTATE_H

#include <string>

class WorkflowInstance;

class InstanceState {
protected:
  [[noreturn]] void illegal(const char* operation) const;
public:
    virtual ~InstanceState() = default;

    virtual std::string name() const = 0;

    virtual void start(WorkflowInstance& instance);
    virtual void advance(WorkflowInstance& instance);
    virtual void cancel(WorkflowInstance& instance, const std::string& reason);
};

#endif