#include "patterns/creation/BusyTeamFactory.h"

#include "patterns/execution/ParallelMode.h"
#include "patterns/execution/LeastLoadedAssignment.h"
#include "patterns/execution/ReviewerHandler.h"
#include "patterns/execution/TechLeadHandler.h"

const int BusyTeamFactory::DUE_HOURS;

std::string BusyTeamFactory::name() const {
    return "BusyTeam";
}

std::unique_ptr<CompositeStage> BusyTeamFactory::createStage(
    const std::string& id, const std::string& name) const {
    std::unique_ptr<CompositeStage> stage(new CompositeStage(id, name));
    stage->setMode(std::unique_ptr<StageMode>(new ParallelMode()));
    return stage;
}

std::unique_ptr<LeafWorkItem> BusyTeamFactory::createWorkItem(
    const std::string& id, const std::string& name,
    const std::string& role) const {
    return std::unique_ptr<LeafWorkItem>(new LeafWorkItem(id, name, role, DUE_HOURS));
}

std::unique_ptr<AssignmentStrategy> BusyTeamFactory::createAssignmentStrategy() const {
    return std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment());
}

std::unique_ptr<ApprovalHandler> BusyTeamFactory::createApprovalChain() const {
    // Reviewer -> Tech Lead. Each handler owns the one after it.
    std::unique_ptr<ApprovalHandler> head(new ReviewerHandler());
    head->setNext(new TechLeadHandler());
    return head;
}
