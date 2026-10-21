#ifndef MEMENTO_H
#define MEMENTO_H

#include <string>
#include <vector>
#include <map>
#include <memory>

// ---------------------------------------------------------
// InstanceStatus 
// ---------------------------------------------------------
enum class InstanceStatus {
    Pending,
    Active,
    Completed,
    Cancelled
};

// ---------------------------------------------------------
// WorkItemSnapshot
// ---------------------------------------------------------
class WorkItemSnapshot {
private:
    std::string state;
    std::string assignee;
    std::string reason;

public:
    WorkItemSnapshot(std::string state, std::string assignee, std::string reason);

    const std::string& getState() const;
    const std::string& getAssignee() const;
    const std::string& getReason() const;
};

// ---------------------------------------------------------
// ApprovalSnapshot
// ---------------------------------------------------------
class ApprovalSnapshot {
private:
    std::string itemId;
    std::string pendingRole;
    bool approved;

public:
    ApprovalSnapshot(std::string itemId, std::string pendingRole, bool approved);

    const std::string& getItemId() const;
    const std::string& getPendingRole() const;
    bool getApproved() const;
};

// ---------------------------------------------------------
// InstanceMemento
// ---------------------------------------------------------
class InstanceMemento {
private:
    std::string label;
    InstanceStatus status;
    std::vector<WorkItemSnapshot> items;
    std::vector<ApprovalSnapshot> approvals;

public:
    InstanceMemento(std::string lbl, InstanceStatus stat);

    const std::string& getLabel() const;
    InstanceStatus getStatus() const;
    
    const std::vector<WorkItemSnapshot>& getItems() const;
    const std::vector<ApprovalSnapshot>& getApprovals() const;

    void addItem(WorkItemSnapshot item);
    void addApproval(ApprovalSnapshot approval);
};

// ---------------------------------------------------------
// SnapshotStore
// ---------------------------------------------------------
class SnapshotStore {
private:
    std::map<std::string, std::map<std::string, std::shared_ptr<InstanceMemento>>> snapshots;

public:
    ~SnapshotStore() = default;

    void save(const std::string& instanceId, std::shared_ptr<InstanceMemento> memento);
    InstanceMemento* get(const std::string& instanceId, const std::string& label) const;
    bool has(const std::string& instanceId, const std::string& label) const;
};

#endif // MEMENTO_H