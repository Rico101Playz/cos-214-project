#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <map>
#include <memory>
#include "patterns/execution/Memento.h"
#include "WorkItem.h"

class WorkflowInstance {
private:
    std::string id;
    InstanceStatus status;
    std::map<std::string, std::shared_ptr<WorkItem>> items;

public:
    WorkflowInstance(std::string id, InstanceStatus initialStatus);

    std::string getId() const;
    InstanceStatus getStatus() const;
    
    void addItem(std::shared_ptr<WorkItem> item);
    std::shared_ptr<WorkItem> getItem(const std::string& itemId) const;

    std::shared_ptr<InstanceMemento> createMemento(const std::string& label) const;
    void restoreFromMemento(InstanceMemento* memento);
    
    void approveItem(const std::string& itemId, bool approved, const std::string& comment);
    
    void startItem(const std::string& itemId);
    
    void completeItem(const std::string& itemId);
    
    void assignItem(const std::string& itemId, const std::string& participantName);
    
    void addWorkItem(const std::string& stageId, const std::string& itemId, std::shared_ptr<WorkItem> itemTemplate);
    
    void rejectItem(const std::string& itemId, const std::string& reason);
};

#endif // WORKFLOWINSTANCE_H