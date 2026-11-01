#ifndef LEAFTASK_H
#define LEAFTASK_H

#include "composite/TaskNode.h"
#include "state/TaskState.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class LeafTask : public TaskNode {
public:
    // The names status() returns.
    static const char* const CREATED;
    static const char* const AVAILABLE;
    static const char* const ASSIGNED;
    static const char* const IN_PROGRESS;
    static const char* const AWAITING_APPROVAL;
    static const char* const COMPLETED;
    static const char* const REJECTED;
    static const char* const CANCELLED;
    static const char* const ESCALATED;
    typedef std::pair<std::string, std::string> Signature;

    static const char* const PRIORITIES[4];
    static bool validPriority(const std::string& priority);
    static int priorityRank(const std::string& priority);

private:
    Task* outermost_;    // the decorator a parent sees, or this task when it is not decorated
    std::string role_;
    std::string assignee_;
    std::string reason_;
    std::map<std::string, std::string> data_;
    int dueHours_;
    std::unique_ptr<TaskState> state_;
    std::string priority_;
    bool released_;

    Task* treeRoot() const;
    void releaseWaiting();

public:
    // A task named after its id, with no role.
    explicit LeafTask(const std::string& id);
    LeafTask(const std::string& id, const std::string& name,
             const std::string& role, int dueHours = 0);

    const std::string& role() const;
    // The current TaskState's name; there is no second copy of the status.
    std::string status() const;
    const std::string& reason() const;
    const std::string& assignee() const;
    void setData(const std::string& key, const std::string& value);
    bool hasData(const std::string& key) const;
    std::string getData(const std::string& key, const std::string& fallback = "") const;
    void removeData(const std::string& key);
    const std::map<std::string, std::string>& data() const;
    // True while the assignee is working on it: assigned, started, submitted or escalated.
    bool holdsLoad() const;
    bool isComplete() const override;
    double progress() const override;
    bool isReady() const override;

    LeafTask* leaf() override;
    // Performs a Request aimed at this task: Assign, Start, Complete, Approve,
    // Reject, Escalate, Withdraw, SetPriority, CancelProject (this task),
    // SetData, Invoke, RouteApproval or Sign. The task's state throws
    // std::logic_error when it does not allow the action.
    void handle(const Request& request) override;
    // Set by a TaskDecorator wrapping this task.
    void setOutermost(Task* outermost);
    Task& outermost() const;

    // Lifecycle (overrides of LeafTask)
    void makeAvailable();
    void assign(const std::string& participant);
    void start();
    void complete();
    void approve();
    void reject(const std::string& reason);
    void cancel() override;
    void escalate();
    void withdraw();

    // Used by the state classes
    void transitionTo(std::unique_ptr<TaskState> next);
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
    const std::vector<std::string>& approverRoles() const;

    std::string pendingApprover() const;
    const std::vector<std::string>& pendingApprovers() const;

    const std::vector<Signature>& signatures() const;
    bool hasSigned(const std::string& person) const;
    // The approval decorator wrapping this task (every task in a project has
    // one); nullptr for a bare task. The read-only approval queries above ask it.
    ApprovalWork* approvals() const;

    // The blocker decorator wrapping this task (every task in a project has
    // one); nullptr for a bare task. The read-only blocker queries below ask it.
    BlockedWork* blocking() const;
    // The completion-check decorator wrapping this task, or nullptr.
    ValidatedWork* validation() const;
    const std::vector<std::string>& blockedBy() const;
    std::vector<std::string> openBlockers() const;
    bool isBlocked() const;
    bool isWaiting() const;
    bool isReleased() const;
    void setReleased(bool released);

    std::vector<std::string> openSubtasks() const;
    LeafTask* parentItem() const;
};

#endif // LEAFTASK_H
