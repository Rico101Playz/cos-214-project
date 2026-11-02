#include "mediator/Mediator.h"

namespace {

SignOffRule signOffRule(const std::string& rule) {
    if (rule == "required") return SignOffRule::Required;
    if (rule == "skip") return SignOffRule::NotRequired;
    if (!rule.empty() && rule != "template") throw std::invalid_argument("signOff must be template, required or skip");
    return SignOffRule::FromTemplate;
}

TaskSpec taskSpec(const Request& request) {
    TaskSpec spec;
    spec.name = request.arg("name");
    spec.stageId = request.arg("stage");
    spec.unit = request.arg("unit");
    spec.role = request.arg("role");
    spec.approverRole = request.arg("approver");
    spec.approverRoles = request.list("approvers");
    spec.priority = request.arg("priority");
    spec.assignee = request.arg("assignee");
    spec.autoAssign = request.arg("autoAssign") != "false";
    spec.blockedBy = request.list("blockedBy");
    return spec;
}

IssueSpec issueSpec(const Request& request) {
    IssueSpec spec;
    spec.name = request.arg("name");
    spec.units = request.list("units");
    spec.priority = request.arg("priority");
    return spec;
}

}

Response Mediator::send(const Request& request) {
    Response response;
    const std::string& project = request.ref().projectId;
    const std::string& task = request.ref().taskId;
    try {
        switch (request.action()) {
            case Action::StartProject:
                response.put("project", startProject(request.arg("definition"), request.arg("title"), request.arg("policy"),
                                                     signOffRule(request.arg("signOff"))));
                break;
            case Action::CancelProject: cancel(project, request.arg("reason")); break;
            case Action::SignOff: signOff(project); break;
            case Action::FlagIssue: response.putList("items", flagIssue(project, issueSpec(request))); break;
            case Action::AddTask: response.put("item", addTask(project, taskSpec(request))); break;
            case Action::AddSubtask: response.put("item", addSubtask(project, task, taskSpec(request))); break;
            case Action::SetPriority: setPriority(project, task, request.arg("priority")); break;
            case Action::Assign: assign(project, task, request.arg("participant")); break;
            case Action::AutoAssign: autoAssign(project, task); break;
            case Action::Start: start(project, task); break;
            case Action::Complete: complete(project, task); break;
            case Action::Withdraw: withdraw(project, task); break;
            case Action::Approve: approve(project, task); break;
            case Action::Reject: reject(project, task, request.arg("reason")); break;
            case Action::Escalate: escalate(project, task); break;
            case Action::AddUnit: addUnit(request.arg("parent"), request.arg("id"), request.arg("name")); break;
            case Action::AddPerson: addPerson(request.arg("name"), request.arg("role"), request.arg("unit")); break;
            case Action::MoveMember: moveMember(request.arg("unit"), request.arg("name")); break;
            case Action::Undo: undo(); break;
            case Action::Redo: redo(); break;
            case Action::SetData:
            case Action::Invoke:
            case Action::RouteApproval:
            case Action::Sign:
            case Action::ReopenForFixes:
                throw std::invalid_argument(toString(request.action()) + " is a step inside the engine, not a request");
        }
    } catch (const AccessDenied& e) {
        response.fail("forbidden", e.what());
    } catch (const std::invalid_argument& e) {
        response.fail("invalid", e.what());
    } catch (const std::logic_error& e) {
        response.fail("not_allowed_now", e.what());
    } catch (const std::exception& e) {
        response.fail("error", e.what());
    }
    return response;
}

void Mediator::notify(Colleague*, const Response&) {}
