#ifndef LEAFWORKITEM_H
#define LEAFWORKITEM_H

#include "core/WorkItem.h"
#include "patterns/execution/WorkItemState.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class LeafWorkItem : public WorkItem {
public:
    typedef std::pair<std::string, std::string> Signature;

    static const char* const PRIORITIES[4];
    static bool validPriority(const std::string& priority);
    static int priorityRank(const std::string& priority);

private:
    int dueHours_;
    std::unique_ptr<WorkItemState> state_;
    std::vector<std::string> approverRoles_;
    std::vector<std::string> pendingApprovers_;
    std::vector<Signature> signatures_;
    std::vector<std::string> blockedBy_;
    std::vector<std::unique_ptr<LeafWorkItem> > subtasks_;
    std::string priority_;
    bool released_;

    WorkNode* treeRoot() const;
    void releaseWaiting();

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
    void withdraw();

    // Used by the state classes
    void transitionTo(std::unique_ptr<WorkItemState> next);
    void setAssignee(const std::string& participant);
    void setReason(const std::string& reason);

    // for Memento restore
    void restore(const std::string& stateName, const std::string& assignee,
                 const std::string& reason);

    int dueHours() const;

    std::string unit() const;
    void setUnit(const std::string& unitId);

    const std::string& priority() const;
    void setPriority(const std::string& priority);

    std::string approverRole() const;
    void setApproverRole(const std::string& role);
    const std::vector<std::string>& approverRoles() const;
    void setApproverRoles(const std::vector<std::string>& roles);

    std::string pendingApprover() const;
    void setPendingApprover(const std::string& role);
    const std::vector<std::string>& pendingApprovers() const;
    void setPendingApprovers(const std::vector<std::string>& roles);
    bool takePendingSlot(const std::string& role);

    const std::vector<Signature>& signatures() const;
    void addSignature(const std::string& role, const std::string& person);
    bool hasSigned(const std::string& person) const;
    void clearApprovals();

    const std::vector<std::string>& blockedBy() const;
    void addBlocker(const std::string& itemId);
    std::vector<std::string> openBlockers() const;
    bool isBlocked() const;
    bool isWaiting() const;
    bool isReleased() const;
    void setReleased(bool released);

    LeafWorkItem& addSubtask(std::unique_ptr<LeafWorkItem> subtask);
    std::size_t childCount() const override;
    WorkNode* child(std::size_t index) const override;
    std::vector<std::string> openSubtasks() const;
    LeafWorkItem* parentItem() const;
};

#endif // LEAFWORKITEM_H
