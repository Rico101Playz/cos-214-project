#include "mediator/Action.h"

std::string toString(Action action) {
    switch (action) {
        case Action::StartProject: return "StartProject";
        case Action::CancelProject: return "CancelProject";
        case Action::SignOff: return "SignOff";
        case Action::FlagIssue: return "FlagIssue";
        case Action::AddTask: return "AddTask";
        case Action::AddSubtask: return "AddSubtask";
        case Action::SetPriority: return "SetPriority";
        case Action::Assign: return "Assign";
        case Action::AutoAssign: return "AutoAssign";
        case Action::Start: return "Start";
        case Action::Complete: return "Complete";
        case Action::Withdraw: return "Withdraw";
        case Action::Approve: return "Approve";
        case Action::Reject: return "Reject";
        case Action::Escalate: return "Escalate";
        case Action::AddUnit: return "AddUnit";
        case Action::AddPerson: return "AddPerson";
        case Action::MoveMember: return "MoveMember";
        case Action::Undo: return "Undo";
        case Action::Redo: return "Redo";
        case Action::SetData: return "SetData";
        case Action::Invoke: return "Invoke";
        case Action::RouteApproval: return "RouteApproval";
        case Action::Sign: return "Sign";
        case Action::ReopenForFixes: return "ReopenForFixes";
    }
    return "Unknown";
}
