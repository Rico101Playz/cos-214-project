#ifndef MEDIATOR_H
#define MEDIATOR_H

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "mediator/Request.h"
#include "mediator/Response.h"

class Colleague;
class EngineMemento;

struct TaskSpec {
    std::string name;
    std::string stageId;
    std::string unit;
    std::string role;
    std::string approverRole;
    std::vector<std::string> approverRoles;
    std::vector<std::string> blockedBy;
    std::string priority;
    std::string assignee;
    bool autoAssign;

    TaskSpec() : autoAssign(true) {}

    std::vector<std::string> approvers() const {
        std::vector<std::string> all = approverRoles;
        if (!approverRole.empty()) all.insert(all.begin(), approverRole);
        return all;
    }
};

enum class SignOffRule { FromTemplate, Required, NotRequired };

struct IssueSpec {
    std::string name;
    std::vector<std::string> units;
    std::string priority;
};

// Thrown when the actor may not do what they asked; send() answers it with
// the code "forbidden".
class AccessDenied : public std::runtime_error {
public:
    explicit AccessDenied(const std::string& message) : std::runtime_error(message) {}
};

// MEDIATOR pattern - the MEDIATOR interface.
//
// Clients send every change as a Request and get a Response; send() routes the
// Request's Action to the matching operation below and turns a refusal into a
// Response code instead of an exception. TividyMediator implements the
// operations; SecuredMediator (Proxy) implements them by checking permission
// first, so a Request sent to the proxy is checked before it reaches the
// mediator.
class Mediator {
public:
    virtual ~Mediator() = default;

    Response send(const Request& request);

    // A colleague reports what one of its Requests changed; the mediator
    // coordinates the other colleagues. Does nothing by default.
    virtual void notify(Colleague* sender, const Response& change);

    // MEMENTO pattern - the whole engine's state, for undo and redo.
    virtual std::shared_ptr<const EngineMemento> createMemento(const std::string& label) const = 0;
    virtual void restoreMemento(const EngineMemento& memento) = 0;

    virtual std::string startProject(const std::string& definition, const std::string& title,
                                     const std::string& policy,
                                     SignOffRule signOff = SignOffRule::FromTemplate) = 0;
    virtual std::string addTask(const std::string& instanceId, const TaskSpec& spec) = 0;
    virtual std::string addSubtask(const std::string& instanceId, const std::string& parentItemId,
                                   const TaskSpec& spec) = 0;

    virtual void assign(const std::string& instanceId, const std::string& itemId,
                        const std::string& participant) = 0;
    virtual void autoAssign(const std::string& instanceId, const std::string& itemId) = 0;
    virtual void start(const std::string& instanceId, const std::string& itemId) = 0;
    virtual void complete(const std::string& instanceId, const std::string& itemId) = 0;
    virtual void withdraw(const std::string& instanceId, const std::string& itemId) = 0;
    virtual void approve(const std::string& instanceId, const std::string& itemId) = 0;
    virtual void reject(const std::string& instanceId, const std::string& itemId,
                        const std::string& reason = "") = 0;
    virtual void escalate(const std::string& instanceId, const std::string& itemId) = 0;
    virtual void setPriority(const std::string& instanceId, const std::string& itemId,
                             const std::string& priority) = 0;
    virtual void cancel(const std::string& instanceId, const std::string& reason = "") = 0;
    virtual void signOff(const std::string& instanceId) = 0;
    virtual std::vector<std::string> flagIssue(const std::string& instanceId, const IssueSpec& spec) = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual void addUnit(const std::string& parentId, const std::string& id,
                         const std::string& name) = 0;
    virtual void addPerson(const std::string& name, const std::string& role,
                           const std::string& unitId) = 0;
    virtual void moveMember(const std::string& unitId, const std::string& name) = 0;
};

#endif // MEDIATOR_H
