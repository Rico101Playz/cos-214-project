#ifndef ACTION_H
#define ACTION_H

#include <string>

// What a Request asks the engine to do. It is a value, not state: a task's
// lifecycle lives in the TaskState classes (State pattern).
enum class Action {
    StartProject,
    CancelProject,
    SignOff,
    FlagIssue,
    AddTask,
    AddSubtask,
    SetPriority,
    Assign,
    AutoAssign,
    Start,
    Complete,
    Withdraw,
    Approve,
    Reject,
    Escalate,
    AddUnit,
    AddPerson,
    MoveMember,
    Undo,
    Redo,
    // Steps the engine records inside the actions above; clients cannot send them.
    SetData,
    Invoke,
    RouteApproval,
    Sign,
    ReopenForFixes
};

// The action's name, e.g. "Assign", for messages and logs.
std::string toString(Action action);

#endif // ACTION_H
