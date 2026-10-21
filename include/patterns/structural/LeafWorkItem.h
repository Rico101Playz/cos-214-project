#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H

#include "core/WorkItem.h"
#include "patterns/execution/WorkItemState.h"

#include <memory>
#include <string>

class LeafWorkItem : public WorkItem {
private:
    int dueHours_;
    std::unique_ptr<WorkItemState> state_;

    static std::unique_ptr<WorkItemState> stateFromName(const std::string& name);

public:
    LeafWorkItem(const std::string& id, const std::string& name,
                 const std::string& role, int dueHours = 0);

    // Lifecycle (overrides of WorkItem)
    void makeAvailable() override;
    void assign(const std::string& participant) override;
    void start() override;
    void complete() override;
    void approve() override;
    void reject(const std::string& reason) override;
    void cancel();
    void escalate();

    // Used by the state classes
    void transitionTo(std::unique_ptr<WorkItemState> next);
    void setAssignee(const std::string& participant);
    void setReason(const std::string& reason);

    // for Memento restore
    void restore(const std::string& stateName, const std::string& assignee,
                 const std::string& reason);

    int dueHours() const;
};

#endif // LEAFWORKITEM_H