#ifndef BUSYTEAMFACTORY_H
#define BUSYTEAMFACTORY_H

#include "abstract_factory/WorkflowFactory.h"

// CONCRETE FACTORY - the "busy team" policy.
//
// Lots of work and not enough hands: stages let their children run side by
// side, work goes to whoever has the lightest load, and changes need a
// reviewer and then the tech lead.
//
//   stage      : Parallel
//   work item  : due in 24 hours
//   assignment : LeastLoaded
//   approvals  : Reviewer -> Tech Lead
/**
 * @class BusyTeamFactory
 * @brief A concrete workflow factory implementing the "busy team" policy.
 *
 * Configures a workflow where stages run in parallel, tasks are assigned
 * to the least loaded participant, items are due in 24 hours, and the
 * approval chain routes from a Reviewer to a Tech Lead.
 */
class BusyTeamFactory : public WorkflowFactory {
public:
    /**
     * @brief The default number of hours until a work item is due.
     */
    static const int DUE_HOURS = 24;

    /**
     * @brief Retrieves the name of the factory policy.
     * @return std::string The factory name ("BusyTeam").
     */
    std::string name() const override;

    /**
     * @brief Creates a new composite stage configured to run in Parallel mode.
     * @param[in] id The unique identifier for the stage.
     * @param[in] name The display name of the stage.
     * @return std::unique_ptr<CompositeTask> A unique pointer to the created stage.
     */
    std::unique_ptr<CompositeTask> createStage(
        const std::string& id, const std::string& name) const override;

    /**
     * @brief Creates a new work item configured with the factory's default due hours.
     * @param[in] id The unique identifier for the work item.
     * @param[in] name The display name of the work item.
     * @param[in] role The role required to complete the work item.
     * @return std::unique_ptr<Task> A unique pointer to the created work item.
     */
    std::unique_ptr<Task> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const override;


    /**
     * @brief Creates the approval chain (Reviewer -> Tech Lead) for the busy team.
     * @return std::unique_ptr<ApprovalHandler> A unique pointer to the head of the approval chain.
     */
    std::unique_ptr<ApprovalHandler> createApprovalChain() const override;
};

#endif // BUSYTEAMFACTORY_H
