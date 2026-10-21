#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <cstddef>
#include <map>
#include <memory>
#include "patterns/execution/Memento.h"
#include "core/WorkflowDefinition.h"
#include "core/WorkItem.h"
#include "patterns/structural/CompositeStage.h"
#include "patterns/execution/InstanceState.h"
#include "patterns/execution/CreatedInstance.h"

class WorkflowInstance {

public:
    static const char* const CREATED;
    static const char* const RUNNING;
    static const char* const COMPLETED;
    static const char* const CANCELLED;

private:
    std::string legacyId;
    InstanceStatus status;
    std::map<std::string, std::shared_ptr<WorkItem>> items;

    std::string id_;
    WorkflowDefinition definition_;
    std::unique_ptr<CompositeStage> root_;
    std::string state_;
    std::string reason_; 
    std::size_t currentStage_;  

    std::unique_ptr<InstanceState> instanceState_{new CreatedInstance()};
    void activateStage(WorkNode* node);

    void release(WorkNode* stage);
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

    WorkflowInstance(const std::string& id, const WorkflowDefinition& definition);

    WorkflowInstance(const WorkflowInstance&) = delete;
    WorkflowInstance& operator=(const WorkflowInstance&) = delete;

    const std::string& id() const;
    const std::string& title() const;       // the definition's name
    const std::string& state() const;
    const std::string& reason() const;
    const WorkflowDefinition& definition() const;

    // The root of the work tree, for iterating over it.
    CompositeStage& root() const;
    // The stage whose work is currently released; nullptr when none is.
    WorkNode* currentStage() const;

    // Created -> Running, releasing the first stage.
    // Throws std::logic_error when the instance is not Created.
    void start();

    // Moves past every finished stage, releasing the next one. When the last
    // stage is finished the instance becomes Completed. Does nothing unless
    // the instance is Running.
    void advance();

    // Cancels all unfinished work. Throws std::logic_error when the instance
    // is already Completed or Cancelled.
    void cancel(const std::string& reason);

    // Any node in the tree by id; nullptr when not found.
    WorkNode* find(const std::string& nodeId) const;
    // A work item by id; nullptr when the id is unknown or names a stage.
    WorkItem* item(const std::string& itemId) const;

    void setState(std::unique_ptr<InstanceState> next);
    void setReason(const std::string& reason);
    void openFirstStage();
    bool stagesFinished();
};

#endif
