#ifndef SECUREDTIVIDY_H
#define SECUREDTIVIDY_H

#include <stdexcept>
#include <string>
#include <vector>
#include "patterns/structural/TividyService.h"

class Tividy;
class LeafWorkItem;

class AccessDenied : public std::runtime_error {
public:
    explicit AccessDenied(const std::string& message) : std::runtime_error(message) {}
};

class SecuredTividy : public TividyService {
private:
    Tividy& real_;
    std::string actor_;

    LeafWorkItem* leaf(const std::string& instanceId, const std::string& itemId) const;
    void requireManager(const std::string& action) const;
    void require(bool allowed, const std::string& action) const;

public:
    SecuredTividy(Tividy& real, const std::string& actor);

    const std::string& actor() const;
    bool isManager() const;
    bool isOwner() const;

    bool mayClaim(const std::string& instanceId, const std::string& itemId) const;
    bool mayWork(const std::string& instanceId, const std::string& itemId) const;
    bool mayDecide(const std::string& instanceId, const std::string& itemId) const;
    bool mayEscalate(const std::string& instanceId, const std::string& itemId) const;
    bool mayAddSubtask(const std::string& instanceId, const std::string& itemId) const;
    bool mayWithdraw(const std::string& instanceId, const std::string& itemId) const;
    bool maySignOff(const std::string& instanceId) const;
    bool mayUndo() const;
    bool mayRedo() const;

    std::string startProject(const std::string& definition, const std::string& title,
                             const std::string& policy,
                             SignOffRule signOff = SignOffRule::FromTemplate) override;
    std::string addTask(const std::string& instanceId, const TaskSpec& spec) override;
    std::string addSubtask(const std::string& instanceId, const std::string& parentItemId,
                           const TaskSpec& spec) override;
    void assign(const std::string& instanceId, const std::string& itemId,
                const std::string& participant) override;
    void autoAssign(const std::string& instanceId, const std::string& itemId) override;
    void start(const std::string& instanceId, const std::string& itemId) override;
    void complete(const std::string& instanceId, const std::string& itemId) override;
    void withdraw(const std::string& instanceId, const std::string& itemId) override;
    void approve(const std::string& instanceId, const std::string& itemId) override;
    void reject(const std::string& instanceId, const std::string& itemId,
                const std::string& reason = "") override;
    void escalate(const std::string& instanceId, const std::string& itemId) override;
    void setPriority(const std::string& instanceId, const std::string& itemId,
                     const std::string& priority) override;
    void cancel(const std::string& instanceId, const std::string& reason = "") override;
    void signOff(const std::string& instanceId) override;
    std::vector<std::string> flagIssue(const std::string& instanceId, const IssueSpec& spec) override;
    void undo() override;
    void redo() override;
    void addUnit(const std::string& parentId, const std::string& id,
                 const std::string& name) override;
    void addPerson(const std::string& name, const std::string& role,
                   const std::string& unitId) override;
    void moveMember(const std::string& unitId, const std::string& name) override;
};

#endif
