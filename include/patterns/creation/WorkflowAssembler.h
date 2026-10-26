#ifndef WORKFLOWASSEMBLER_H
#define WORKFLOWASSEMBLER_H

#include <memory>

#include <string>

#include "core/Organisation.h"
#include "core/WorkflowDefinition.h"
#include "patterns/creation/WorkflowFactory.h"
#include "patterns/execution/AssignmentService.h"

// The CLIENT of the Abstract Factory.
//
// Everything here works only through WorkflowFactory: it never names
// SmallTeamFactory, BusyTeamFactory or HighRiskFactory, or any of the
// concrete stage, strategy or handler classes they make. Hand it a different
// factory and the whole workflow follows that policy, with no change here.
class WorkflowAssembler {
public:
    // Builds the Feature Delivery work tree:
    //
    //   Feature Delivery
    //   |- Develop : Implement feature (Developer), Write tests (QATester)
    //   |- Review  : Code review (Reviewer)
    //   `- Release : Deploy (ReleaseManager)
    //
    // Every stage and every item comes from `factory`.
    static std::unique_ptr<CompositeStage> buildFeatureTree(const WorkflowFactory& factory);

    // Builds a work tree from a definition made by the Builder pattern:
    // a root named after the definition, one stage per definition stage, and
    // one work item per definition item. As in WorkflowInstance, the items go
    // into the first stage (or straight under the root when there are no
    // stages). The definition carries no roles, so every item gets
    // `itemRole`. Every stage and item comes from `factory`.
    static std::unique_ptr<CompositeStage> buildFromDefinition(
        const WorkflowFactory& factory, const WorkflowDefinition& definition,
        const std::string& itemRole);

    // An AssignmentService (over `organisation`) already set to the policy's
    // assignment strategy.
    static std::unique_ptr<AssignmentService> buildAssignmentService(
        const WorkflowFactory& factory, const Organisation& organisation);
};

#endif // WORKFLOWASSEMBLER_H
