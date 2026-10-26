#ifndef BUSYTEAMFACTORY_H
#define BUSYTEAMFACTORY_H

#include "patterns/creation/WorkflowFactory.h"

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
class BusyTeamFactory : public WorkflowFactory {
public:
    static const int DUE_HOURS = 24;

    std::string name() const override;

    std::unique_ptr<CompositeStage> createStage(
        const std::string& id, const std::string& name) const override;

    std::unique_ptr<LeafWorkItem> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const override;

    std::unique_ptr<AssignmentStrategy> createAssignmentStrategy() const override;

    std::unique_ptr<ApprovalHandler> createApprovalChain() const override;
};

#endif // BUSYTEAMFACTORY_H
