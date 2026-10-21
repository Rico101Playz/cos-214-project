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
class SmallTeamFactory : public WorkflowFactory {
public:
    static const int DUE_HOURS = 48;

    std::string name() const override;

    std::unique_ptr<CompositeStage> createStage(
        const std::string& id, const std::string& name) const override;

    std::unique_ptr<LeafWorkItem> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const override;

    std::unique_ptr<AssignmentStrategy> createAssignmentStrategy() const override;

    std::unique_ptr<ApprovalHandler> createApprovalChain() const override;
};

#endif // SMALLTEAMFACTORY_H
