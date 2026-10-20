#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H

#include "core/WorkItem.h"
#include "patterns/execution/WorkItemState.h"

#include <memory>
#include <string>

class Participant;

// implemented only so far to work with the state pattern
// specifically: WorkItemState
class LeafWorkItem : public WorkItem {
private:
    int dueHours_;
    Participant* assignee_;      // not owned
    std::string reason_;         // last rejection reason
    std::unique_ptr<WorkItemState> state_;

public:
    LeafWorkItem(const std::string& id, const std::string& name,
                 const std::string& role, int dueHours = 0);

    void makeAvailable();
    void assign(Participant& participant);
    void start();
    void complete();
    void reject(const std::string& reason);
    void escalate();

    void cancel(); //reachable from any state

    bool isComplete() const;
    double progress() const;

    // Used by the state classes
    void transitionTo(std::unique_ptr<WorkItemState> next);
    void setAssignee(Participant& participant);
    void setReason(const std::string& reason);

    int dueHours() const;
    Participant* assignee() const;
    const std::string& reason() const;
    std::string stateName() const;
};

#endif // LEAFWORKITEM_H