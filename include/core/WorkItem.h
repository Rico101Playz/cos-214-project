#ifndef WORKITEM_H
#define WORKITEM_H

#include "patterns/structural/WorkNode.h"
#include <map>
#include <string>
#include <utility>

// A single unit of work, e.g. "Implement backend" or "Code review".
//
// NOTE (integration point): this is the minimal shape the assignment
// strategies and approval chain need - an id, a name, the ROLE that should do
// the work, and key/value data (e.g. risk = high). The class diagram grows this
// into WorkNode -> WorkItem -> LeafWorkItem (Composite) with a lifecycle
// (State). Whoever lands those patterns should extend this class and keep
// id(), name(), role(), setData(), hasData() and getData() working, so the
// strategies and approval handlers do not need to change.
class WorkItem : public WorkNode {
public:
    static const char* const CREATED;
    static const char* const AVAILABLE;
    static const char* const ASSIGNED;
    static const char* const IN_PROGRESS;
    static const char* const AWAITING_APPROVAL;
    static const char* const COMPLETED;
    static const char* const REJECTED;
    static const char* const CANCELLED;
    static const char* const ESCALATED;

private:
    std::string role_;
    std::string assignee_;
    std::map<std::string, std::string> data_;

protected:
    std::string status_;
    std::string reason_;
    void require(const std::string& action, const char* const allowed[], int count) const;

public:
    std::string itemId;
    std::string state;

    WorkItem(std::string id) : WorkNode(id, id), status_(CREATED), itemId(std::move(id)), state("Pending") {}
    WorkItem(const std::string& id, const std::string& name, const std::string& role);
    virtual ~WorkItem() = default;

    const std::string& role() const;
    const std::string& status() const;
    const std::string& reason() const;
    const std::string& assignee() const;
    void setAssignee(const std::string& participant);

    void setData(const std::string& key, const std::string& value);
    bool hasData(const std::string& key) const;
    std::string getData(const std::string& key, const std::string& fallback = "") const;
    void removeData(const std::string& key);
    const std::map<std::string, std::string>& data() const;

    virtual void makeAvailable();
    virtual void assign(const std::string& participant);
    virtual void start();
    virtual void complete();
    virtual void approve();
    virtual void reject(const std::string& reason);

    bool holdsLoad() const;

    bool isComplete() const override;
    double progress() const override;
    void cancel() override;
    virtual bool isReady() const;
};

#endif // WORKITEM_H