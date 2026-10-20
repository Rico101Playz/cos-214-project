#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <map>
#include <memory>
#include "Memento.h"
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

    void approveItem(const std::string& itemId, bool approved);
};

#endif // WORKFLOWINSTANCE_H