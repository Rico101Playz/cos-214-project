#include "patterns/creation/SmallTeamFactory.h"

#include "patterns/execution/SequentialMode.h"
#include "patterns/execution/RoleBasedAssignment.h"
#include "patterns/execution/AutoApproveHandler.h"
#include "patterns/execution/ReviewerHandler.h"

const int SmallTeamFactory::DUE_HOURS;

std::string SmallTeamFactory::name() const {
    return "SmallTeam";
}

std::unique_ptr<CompositeStage> SmallTeamFactory::createStage(
    const std::string& id, const std::string& name) const {
    std::unique_ptr<CompositeStage> stage(new CompositeStage(id, name));
    stage->setMode(std::unique_ptr<StageMode>(new SequentialMode()));
    return stage;
}

std::unique_ptr<LeafWorkItem> SmallTeamFactory::createWorkItem(
    const std::string& id, const std::string& name,
    const std::string& role) const {
    return std::unique_ptr<LeafWorkItem>(new LeafWorkItem(id, name, role, DUE_HOURS));
}

std::unique_ptr<AssignmentStrategy> SmallTeamFactory::createAssignmentStrategy() const {
    return std::unique_ptr<AssignmentStrategy>(new RoleBasedAssignment());
}

std::unique_ptr<ApprovalHandler> SmallTeamFactory::createApprovalChain() const {
    // Auto -> Reviewer. Each handler owns the one after it.
    std::unique_ptr<ApprovalHandler> head(new AutoApproveHandler());
    head->setNext(new ReviewerHandler());
    return head;
}
