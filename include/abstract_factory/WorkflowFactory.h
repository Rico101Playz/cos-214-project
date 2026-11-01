#ifndef WORKFLOWFACTORY_H
#define WORKFLOWFACTORY_H

#include <memory>
#include <string>

#include "composite/CompositeTask.h"
#include "composite/LeafTask.h"
#include "chain_of_responsibility/ApprovalHandler.h"

// ABSTRACT FACTORY pattern - the ABSTRACT FACTORY.
//
// A workflow policy is a set of parts that only make sense together: how a
// stage runs its children, how long work may take, who gets assigned work,
// and who has to approve it. This interface makes one of each part. A concrete
// factory (one per policy) decides which concrete classes those parts are, so
// code that builds a workflow from a WorkflowFactory can never mix parts from
// different policies.
//
// Products (all returned by unique_ptr, so the caller owns them):
//   createStage()              -> CompositeTask    (Composite pattern)
//   createWorkItem()           -> LeafTask      (Composite + State)
//   createApprovalChain()      -> ApprovalHandler   (Chain of Responsibility);
//                                 the head of the chain, which owns the rest.
class WorkflowFactory {
public:
    virtual ~WorkflowFactory() = default;

    // The policy's name, e.g. "SmallTeam".
    virtual std::string name() const = 0;

    virtual std::unique_ptr<CompositeTask> createStage(
        const std::string& id, const std::string& name) const = 0;

    virtual std::unique_ptr<Task> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const = 0;


    virtual std::unique_ptr<ApprovalHandler> createApprovalChain() const = 0;
};

#endif // WORKFLOWFACTORY_H
