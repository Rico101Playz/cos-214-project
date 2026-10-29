#include "iterator/TaskFilter.h"
#include <vector>
#include "composite/LeafTask.h"

TaskFilter TaskFilter::ready() {
    TaskFilter filter;
    filter.readyOnly = true;
    return filter;
}

TaskFilter TaskFilter::assignedTo(const std::string& person) {
    TaskFilter filter;
    filter.assignee = person;
    filter.openOnly = true;
    return filter;
}

TaskFilter TaskFilter::awaitingApproval(const std::string& role, const std::string& exceptAssignee) {
    TaskFilter filter = awaitingAnyApprover(exceptAssignee);
    filter.approver = role;
    return filter;
}

TaskFilter TaskFilter::awaitingAnyApprover(const std::string& exceptAssignee) {
    TaskFilter filter;
    filter.status = LeafTask::AWAITING_APPROVAL;
    filter.notAssignee = exceptAssignee;
    return filter;
}

bool TaskFilter::matches(const Task& task) const {
    if (readyOnly && !task.isReady()) return false;
    if (!openOnly && status.empty() && assignee.empty() && notAssignee.empty() && approver.empty()) return true;
    const LeafTask* leaf = leafOf(&task);
    if (leaf == nullptr) return false;
    std::string now = leaf->status();
    if (openOnly && (now == LeafTask::COMPLETED || now == LeafTask::CANCELLED)) return false;
    if (!status.empty() && now != status) return false;
    if (!assignee.empty() && leaf->assignee() != assignee) return false;
    if (!notAssignee.empty() && leaf->assignee() == notAssignee) return false;
    if (approver.empty()) return true;
    const std::vector<std::string>& pending = leaf->pendingApprovers();
    for (std::size_t i = 0; i < pending.size(); ++i) {
        if (pending[i] == approver) return true;
    }
    return false;
}
