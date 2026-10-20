#ifndef WORKITEM_H
#define WORKITEM_H

#include <map>
#include <string>
#include "patterns/structural/WorkNode.h"

// A single unit of work, e.g. "Implement backend" or "Code review".
// In the work tree it is a leaf: CompositeStage groups items, never the other
// way round.
//
// NOTE (integration point): this is the minimal shape the assignment
// strategies, approval chain and iterators need - an id, a name, the ROLE that
// should do the work, key/value data (e.g. risk = high) and a simple status.
// The class diagram grows this into WorkItem -> LeafWorkItem with a lifecycle
// driven by the State pattern. Whoever lands that should replace the status
// string below and keep id(), name(), role(), the data accessors and the
// lifecycle calls working, so the strategies, approval handlers and iterators
// do not need to change.
class WorkItem : public WorkNode {
public:
    // Status names match the states in the class diagram.
    static const char* const CREATED;
    static const char* const AVAILABLE;
    static const char* const ASSIGNED;
    static const char* const IN_PROGRESS;
    static const char* const AWAITING_APPROVAL;
    static const char* const COMPLETED;
    static const char* const REJECTED;
    static const char* const CANCELLED;

private:
    std::string role_;
    std::map<std::string, std::string> data_;
    std::string status_;
    std::string assignee_;
    std::string reason_;    // why it was rejected or cancelled

    // Throws std::logic_error naming the action when the item is not in one
    // of the allowed statuses.
    void require(const std::string& action, const char* const allowed[], int count) const;

public:
    WorkItem(const std::string& id, const std::string& name, const std::string& role);

    const std::string& role() const;

    void setData(const std::string& key, const std::string& value);
    bool hasData(const std::string& key) const;
    std::string getData(const std::string& key, const std::string& fallback = "") const;

    const std::string& status() const;
    const std::string& assignee() const;
    const std::string& reason() const;

    // Lifecycle. Each call throws std::logic_error when the move is not
    // allowed from the current status.
    void makeAvailable();                           // Created -> Available
    void assign(const std::string& participant);    // Available/Assigned/Rejected -> Assigned
    void start();                                   // Assigned -> InProgress
    void complete();                                // InProgress -> AwaitingApproval
    void approve();                                 // AwaitingApproval -> Completed
    void reject(const std::string& reason);         // AwaitingApproval -> Rejected

    bool isComplete() const override;   // Completed
    double progress() const override;   // 1.0 when complete, otherwise 0.0
    void cancel() override;             // any unfinished status -> Cancelled
    bool isReady() const override;      // Available: released and not yet taken
};

#endif // WORKITEM_H
