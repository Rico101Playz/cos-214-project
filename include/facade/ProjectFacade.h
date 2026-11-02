#ifndef PROJECT_FACADE_H
#define PROJECT_FACADE_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "abstract_factory/WorkflowFactory.h"
#include "chain_of_responsibility/ApprovalHandler.h"
#include "composite/LeafTask.h"
#include "mediator/Colleague.h"

// One item of one project, as returned by the work queries.
struct ItemRef {
    std::string instanceId;
    std::string itemId;

    ItemRef(const std::string& instance, const std::string& item)
        : instanceId(instance), itemId(item) {}
};

// FACADE pattern - one of the three facades behind TividyMediator. MEDIATOR pattern - a COLLEAGUE.
//
// Fronts the workflow definitions and the running projects: their task trees, runtimes (policy factory,
// approval chain) and every lookup and query on them. Every change to a project arrives as a Request
// (handle): AddTask/AddSubtask build the task with the project's factory and its decorators, the rest
// go to the project or to the outermost decorator of the task they name. A change that moves work on or
// off someone's desk is reported to the mediator with "loads" set, so it can update the organisation.
// It never refers to the other facades; TividyMediator coordinates them through the public interface below.
class ProjectFacade : public Colleague {
public:
    struct Runtime {
        std::unique_ptr<WorkflowFactory> factory;
        std::string policy;
        std::unique_ptr<ApprovalHandler> approvals;
        bool structured;
    };

private:
    std::map<std::string, WorkflowDefinition> definitions_;
    std::map<std::string, std::unique_ptr<WorkflowInstance> > instances_;
    std::map<std::string, std::unique_ptr<Runtime> > runtimes_;
    std::vector<std::string> instanceOrder_;
    int nextInstance_;
    int nextTask_;

public:
    ProjectFacade() : nextInstance_(1), nextTask_(1) {}

    ProjectFacade(const ProjectFacade&) = delete;
    ProjectFacade& operator=(const ProjectFacade&) = delete;

    // Performs a change to the project the Request names. AddTask/AddSubtask take the
    // task's fields (parent, name, role, unit, approvers, priority, kind, blockedBy) and
    // answer with data("item"), the new task's id.
    Response handle(const Request& request) override;
    // The named project's state, for undoing one command.
    std::shared_ptr<Memento> createMemento(const TaskRef& scope, const std::string& label) const override;
    void restore(const TaskRef& scope, Memento& memento) override;

    WorkflowInstance& findInstance(const std::string& instanceId) const;
    LeafTask& findItem(const std::string& instanceId, const std::string& itemId) const;
    LeafTask& findLeaf(const std::string& instanceId, const std::string& itemId) const;
    Runtime& runtime(const std::string& instanceId) const;
    void loadDefinition(const std::string& name);
    void loadDefinition(const std::string& name, const WorkflowDefinition& definition);
    bool hasDefinition(const std::string& name) const;
    std::vector<std::string> definitionNames() const;
    const WorkflowDefinition& definition(const std::string& name) const;
    bool dependsOn(const LeafTask& from, const LeafTask& target) const;
    std::string nextTaskId(const WorkflowInstance& instance);
    const WorkflowInstance& instance(const std::string& instanceId) const;
    const std::vector<std::string>& instanceIds() const;
    std::string policyOf(const std::string& instanceId) const;
    std::vector<ItemRef> workFor(const std::string& participant) const;
    std::vector<std::string> readyItems(const std::string& instanceId) const;
    double progress(const std::string& instanceId) const;

    const std::map<std::string, WorkflowDefinition>& definitions() const;
    void setDefinitions(const std::map<std::string, WorkflowDefinition>& definitions);
    // The id the next project gets ("instance-<n>").
    std::string nextInstanceId() const;
    int nextInstanceNumber() const;
    int nextTaskNumber() const;
    // Takes in a newly started project (filed under its id) and moves the project counter on.
    void addInstance(std::unique_ptr<WorkflowInstance> instance, std::unique_ptr<Runtime> runtime);
    // Hands back an existing project so a restore can rebuild it; null when there is none.
    std::unique_ptr<WorkflowInstance> takeInstance(const std::string& instanceId);
    // Replaces every project and both counters at once (a restore).
    void replaceInstances(std::map<std::string, std::unique_ptr<WorkflowInstance> >& instances,
                          std::map<std::string, std::unique_ptr<Runtime> >& runtimes,
                          const std::vector<std::string>& order, int nextInstance, int nextTask);
};

#endif // PROJECT_FACADE_H
