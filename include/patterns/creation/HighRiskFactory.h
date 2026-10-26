#ifndef HIGHRISKFACTORY_H
#define HIGHRISKFACTORY_H

#include "patterns/creation/WorkflowFactory.h"

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
class HighRiskFactory : public WorkflowFactory {
public:
    static const int DUE_HOURS = 8;

    std::string name() const override;

    std::unique_ptr<CompositeStage> createStage(
        const std::string& id, const std::string& name) const override;

    std::unique_ptr<LeafWorkItem> createWorkItem(
        const std::string& id, const std::string& name,
        const std::string& role) const override;

    std::unique_ptr<AssignmentStrategy> createAssignmentStrategy() const override;

    std::unique_ptr<ApprovalHandler> createApprovalChain() const override;
};

#endif // HIGHRISKFACTORY_H
