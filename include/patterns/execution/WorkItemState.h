#ifndef WORKITEMSTATE_H
#define WORKITEMSTATE_H

#include <string>

class LeafWorkItem;

class WorkItemState {
protected:
    [[noreturn]] void illegal(const char* operation) const;
    
public:
    virtual ~WorkItemState() = default;

    virtual std::string name() const = 0;

    virtual void makeAvailable(LeafWorkItem& item);
    virtual void assign(LeafWorkItem& item, const std::string& participant);
    virtual void start(LeafWorkItem& item);
    virtual void complete(LeafWorkItem& item);
    virtual void approve(LeafWorkItem& item);
    virtual void reject(LeafWorkItem& item, const std::string& reason);
    virtual void escalate(LeafWorkItem& item);
    virtual void withdraw(LeafWorkItem& item);
};

#endif