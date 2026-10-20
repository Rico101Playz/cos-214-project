#ifndef WORKCOMMAND_H
#define WORKCOMMAND_H

#include <string>
#include <memory>

class InstanceMemento;
class SnapshotStore;
class WorkflowInstance;

class WorkCommand {
protected:
    std::shared_ptr<InstanceMemento> before; 
    
    SnapshotStore* store;
    std::string instanceId;

public:
    WorkCommand(SnapshotStore* st, std::string instId);
    virtual ~WorkCommand() = default;

    void execute(WorkflowInstance* instance);
    
    void undo(WorkflowInstance* instance);
    
    virtual std::string describe() const = 0;

protected:
    virtual void perform(WorkflowInstance* instance) = 0;
};

#endif // WORKCOMMAND_H