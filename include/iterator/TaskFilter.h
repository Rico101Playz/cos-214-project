#ifndef TASK_FILTER_H
#define TASK_FILTER_H

#include <string>

class Task;

// What a FilterIterator stops on. Every field that is set must match; an
// empty filter matches every node.
struct TaskFilter {
    bool readyOnly = false;      // only nodes that can be picked up now (Task::isReady)
    bool openOnly = false;       // only tasks that are not Completed or Cancelled
    std::string status;          // only tasks in this status, e.g. "AwaitingApproval"
    std::string assignee;        // only tasks assigned to this person
    std::string notAssignee;     // never tasks assigned to this person
    std::string approver;        // only tasks waiting for this role's approval

    // What can the team work on right now?
    static TaskFilter ready();
    // The unfinished work assigned to a person.
    static TaskFilter assignedTo(const std::string& person);
    // Work waiting for a role's approval, never a person's own work.
    static TaskFilter awaitingApproval(const std::string& role, const std::string& exceptAssignee = "");
    // Everything waiting for approval, never a person's own work.
    static TaskFilter awaitingAnyApprover(const std::string& exceptAssignee = "");

    bool matches(const Task& task) const;
};

#endif // TASK_FILTER_H
