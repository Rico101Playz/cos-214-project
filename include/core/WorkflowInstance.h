#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <cstddef>
#include <memory>
#include <string>
#include "core/WorkflowDefinition.h"
#include "core/WorkItem.h"
#include "patterns/structural/CompositeStage.h"

// One running copy of a workflow definition.
//
// The instance keeps its own copy of the definition it was started from, so
// later changes to the stored definition never affect work already running.
// From that copy it builds a work tree: a root stage named after the workflow,
// one CompositeStage per definition stage, and a WorkItem per item template.
// Stages run in order: starting the instance releases the first stage's
// items, and advance() releases the next stage once the current one is done.
//
// NOTE (integration point): this is the minimal shape the Facade needs. The
// class diagram also gives the instance mementos, events and an InstanceState;
// whoever lands Memento, Observer and State should extend this class and keep
// the methods below working. WorkflowDefinition does not yet say which stage
// an item belongs to, so every item is placed in the first stage for now.
class WorkflowInstance {
public:
    // State names match the InstanceState classes in the class diagram.
    static const char* const CREATED;
    static const char* const RUNNING;
    static const char* const COMPLETED;
    static const char* const CANCELLED;

private:
    std::string id_;
    WorkflowDefinition definition_;
    std::unique_ptr<CompositeStage> root_;
    std::string state_;
    std::string reason_;            // why it was cancelled
    std::size_t currentStage_;      // index into root_'s children

    // Makes every Created item in the stage Available.
    void release(WorkNode* stage);

public:
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
};

#endif // WORKFLOWINSTANCE_H
