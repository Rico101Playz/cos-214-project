#ifndef HIGHRISKFACTORY_H
#define HIGHRISKFACTORY_H

#include "abstract_factory/WorkflowFactory.h"

// CONCRETE FACTORY - the "high risk release" policy.
//
// Changes that can hurt if they go wrong: stages run one step at a time, work
// is short-deadline and tagged risk=high, and the assignment strategy reads
// that tag to send review work to the Tech Lead instead of a plain Reviewer.
// Nothing is approved automatically, and a request that times out is
// escalated to the Release Manager.
//
//   stage      : Sequential
//   work item  : due in 8 hours, data risk=high
//   assignment : RuleBased (Reviewer + risk=high -> TechLead), else LeastLoaded
//   approvals  : Reviewer -> Tech Lead -> Timeout escalation -> Release Manager
//
// The tag and the rule are two halves of one policy, which is why they are
// made by the same factory: the items it makes always carry the data that
// its strategy looks for.
/**
 * @class HighRiskFactory
 * @brief A concrete workflow factory implementing the "high risk release" policy.
 *
 * Configures a strict, tightly-controlled workflow where stages execute sequentially,
 * tasks are due in 8 hours and tagged as high risk, assignment dynamically routes
 * risk-flagged reviews to a Tech Lead, and the approval chain escalates timeouts
 * directly to a Release Manager.
 */
class HighRiskFactory : public WorkflowFactory {
public:
    /**
     * @brief The default number of hours until a high-risk work item is due.
     */
    static const int DUE_HOURS = 8;

    /**
     * @brief Retrieves the name of the factory policy.
     * @return std::string The factory name ("HighRisk").
     */
    std::string name() const override;

    /**
     * @brief Creates a new composite stage configured to run in Sequential mode.
     * @param[in] id The unique identifier for the stage.
     * @param[in] name The display name of the stage.
     * @return std::unique_ptr<CompositeTask> A unique pointer to the created sequential stage.
     */
    std::unique_ptr<CompositeTask> createStage(
        const std::string& id, const std::string& name) const override;

    /**
     * @brief Creates a new work item with an 8-hour deadline and tags it with high risk.
     * @param[in] id The unique identifier for the work item.
     * @param[in] name The display name of the work item.
     * @param[in] role The role required to complete the work item.
     * @return std::unique_ptr<Task> A unique pointer to the created work item.
     */
    std::unique_ptr<Task> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const override;


    /**
     * @brief Creates the stringent approval chain for high-risk releases.
     *
     * The chain routes sequentially: Reviewer -> Tech Lead -> Timeout Escalation -> Release Manager.
     *
     * @return std::unique_ptr<ApprovalHandler> A unique pointer to the head of the approval chain.
     */
    std::unique_ptr<ApprovalHandler> createApprovalChain() const override;
};

#endif // HIGHRISKFACTORY_H
