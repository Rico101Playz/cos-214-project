#ifndef SMALLTEAMFACTORY_H
#define SMALLTEAMFACTORY_H

#include "patterns/creation/WorkflowFactory.h"

// CONCRETE FACTORY - the "small team" policy.
//
// A few people who each know the whole job: stages run one step after the
// other, work is handed to the first available person with the right role,
// and approval is light (automatic, or a single reviewer).
//
//   stage      : Sequential
//   work item  : due in 48 hours
//   assignment : RoleBased
//   approvals  : Auto -> Reviewer
/**
 * @class SmallTeamFactory
 * @brief A concrete workflow factory implementing the "small team" policy.
 * 
 * Configures a workflow where stages execute sequentially, work items are 
 * due in 48 hours, assignment is strictly role-based, and the approval chain 
 * consists of an automatic approval followed by a single reviewer.
 */
class SmallTeamFactory : public WorkflowFactory {
public:
    /** 
     * @brief The default number of hours until a small team work item is due. 
     */
    static const int DUE_HOURS = 48;

    /**
     * @brief Retrieves the name of the factory policy.
     * @return std::string The factory name ("SmallTeam").
     */
    std::string name() const override;

    /**
     * @brief Creates a new composite stage configured to run in Sequential mode.
     * @param[in] id The unique identifier for the stage.
     * @param[in] name The display name of the stage.
     * @return std::unique_ptr<CompositeStage> A unique pointer to the created sequential stage.
     */
    std::unique_ptr<CompositeStage> createStage(
        const std::string& id, const std::string& name) const override;

    /**
     * @brief Creates a new work item configured with a 48-hour deadline.
     * @param[in] id The unique identifier for the work item.
     * @param[in] name The display name of the work item.
     * @param[in] role The role required to complete the work item.
     * @return std::unique_ptr<LeafWorkItem> A unique pointer to the created work item.
     */
    std::unique_ptr<LeafWorkItem> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const override;

    /**
     * @brief Creates the assignment strategy for the small team (RoleBased).
     * @return std::unique_ptr<AssignmentStrategy> A unique pointer to the created assignment strategy.
     */
    std::unique_ptr<AssignmentStrategy> createAssignmentStrategy() const override;

    /**
     * @brief Creates the approval chain (Auto -> Reviewer) for the small team.
     * @return std::unique_ptr<ApprovalHandler> A unique pointer to the head of the approval chain.
     */
    std::unique_ptr<ApprovalHandler> createApprovalChain() const override;
};

#endif // SMALLTEAMFACTORY_H
