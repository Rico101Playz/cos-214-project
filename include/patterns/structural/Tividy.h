#ifndef TIVIDY_H
#define TIVIDY_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "core/Organisation.h"
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "patterns/execution/CommandInvoker.h"

// FACADE pattern - the FACADE.
//
// The single entry point to the engine. Clients load definitions, start
// instances and drive work items by id here, without touching the builders,
// work tree, iterators or organisation behind it.
//
// Each work item operation updates the instance and is then recorded as the
// matching WorkCommand through the CommandInvoker, so the command history
// sees every action taken through the engine.
//
// Every call that names something unknown (definition, instance, item or
// participant) throws std::invalid_argument; a call the item's or instance's
// current state does not allow throws std::logic_error. Nothing changes when
// a call throws.
class Tividy {
private:
    Organisation organisation_;
    std::map<std::string, WorkflowDefinition> definitions_;
    std::map<std::string, std::unique_ptr<WorkflowInstance> > instances_;
    CommandInvoker invoker_;
    int nextInstance_;

    WorkflowInstance& findInstance(const std::string& instanceId) const;
    WorkItem& findItem(const std::string& instanceId, const std::string& itemId) const;
    Participant& findParticipant(const std::string& name) const;

public:
    explicit Tividy(const std::string& organisationName);

    Tividy(const Tividy&) = delete;
    Tividy& operator=(const Tividy&) = delete;

    // The people who can be assigned work.
    Organisation& organisation();

    // Builds the Feature Delivery process with the Builder pattern and stores
    // it under the given name, replacing any definition with that name.
    void loadDefinition(const std::string& name);
    // Stores a definition supplied by the caller under the given name.
    void loadDefinition(const std::string& name, const WorkflowDefinition& definition);
    bool hasDefinition(const std::string& name) const;

    // Starts a new running instance of a stored definition and returns its id.
    std::string startInstance(const std::string& definition);

    // Work item operations. assign() names a participant in the organisation
    // and adds to their load; the load is released again when the work is
    // approved, rejected or reassigned. approve() also moves the instance on
    // to its next stage once the current one is finished.
    void assign(const std::string& instanceId, const std::string& itemId,
                const std::string& participant);
    void start(const std::string& instanceId, const std::string& itemId);
    void complete(const std::string& instanceId, const std::string& itemId);
    void approve(const std::string& instanceId, const std::string& itemId);
    void reject(const std::string& instanceId, const std::string& itemId,
                const std::string& reason = "");

    // Read-only view of a running instance.
    const WorkflowInstance& instance(const std::string& instanceId) const;
    // Ids of the items in the instance that can be picked up right now.
    std::vector<std::string> readyItems(const std::string& instanceId) const;
    // Overall progress of the instance, 0.0 .. 1.0.
    double progress(const std::string& instanceId) const;
};

#endif // TIVIDY_H
