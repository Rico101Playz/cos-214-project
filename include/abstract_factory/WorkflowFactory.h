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
/**
 * @class WorkflowFactory
 * @brief Abstract factory interface for creating workflow policy components.
 *
 * Defines the standard operations required to instantiate consistent families
 * of workflow elements (stages, work items, and handlers) so that
 * parts from different policies are never mixed.
 */
class WorkflowFactory {
public:
    /**
     * @brief Default virtual destructor.
     */
    virtual ~WorkflowFactory() = default;

    // The policy's name, e.g. "SmallTeam".
    /**
     * @brief Retrieves the name of the workflow policy.
     * @return std::string The policy's name (e.g., "SmallTeam").
     */
    virtual std::string name() const = 0;

    /**
     * @brief Creates a new composite stage for the workflow.
     * @param[in] id The unique identifier for the stage.
     * @param[in] name The display name of the stage.
     * @return std::unique_ptr<CompositeTask> A unique pointer to the created stage.
     */
    virtual std::unique_ptr<CompositeTask> createStage(
        const std::string& id, const std::string& name) const = 0;

    /**
     * @brief Creates a new leaf work item for the workflow.
     * @param[in] id The unique identifier for the work item.
     * @param[in] name The display name of the work item.
     * @param[in] role The role required to complete the work item.
     * @return std::unique_ptr<Task> A unique pointer to the created work item.
     */
    virtual std::unique_ptr<Task> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const = 0;


    /**
     * @brief Creates the approval chain associated with this workflow policy.
     * @return std::unique_ptr<ApprovalHandler> A unique pointer to the head of the approval chain.
     */
    virtual std::unique_ptr<ApprovalHandler> createApprovalChain() const = 0;
};

#endif // WORKFLOWFACTORY_H
