#ifndef WORKCOMMAND_H
#define WORKCOMMAND_H

#include <string>
#include <memory>

class WorkflowInstance;
class Participant;
class InstanceMemento;
class WorkItem;

class WorkCommand {
private:
    WorkflowInstance& instance_;
    Participant* actor_;
    std::unique_ptr<InstanceMemento> before_;
    long executedAt_;

protected:
    virtual void perform() = 0;

    virtual std::string actorName() const;

    void requireAssignee(const WorkItem& item, const std::string& action) const;

public:
    WorkCommand(WorkflowInstance& instance, Participant* actor);
    virtual ~WorkCommand();

    void execute();

    void undo();

    virtual std::string describe() const = 0;

    WorkflowInstance& instance() const;

    Participant* actor() const;

    long executedAt() const;
};

#endif // WORKCOMMAND_H