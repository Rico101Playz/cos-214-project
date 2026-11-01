#ifndef TASKSTATE_H
#define TASKSTATE_H

#include <string>

class LeafTask;

class TaskState {
protected:
    [[noreturn]] void illegal(const char* operation) const;
    
public:
    virtual ~TaskState() = default;

    virtual std::string name() const = 0;

    virtual void makeAvailable(LeafTask& item);
    virtual void assign(LeafTask& item, const std::string& participant);
    virtual void start(LeafTask& item);
    virtual void complete(LeafTask& item);
    virtual void approve(LeafTask& item);
    virtual void reject(LeafTask& item, const std::string& reason);
    virtual void escalate(LeafTask& item);
    virtual void withdraw(LeafTask& item);
};

#endif