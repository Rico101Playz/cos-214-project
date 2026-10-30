#include "abstract_factory/BusyTeamFactory.h"
#include "decorator/ValidatedWork.h"
#include "chain_of_responsibility/RoleApprovalHandler.h"

#include "strategy/ParallelMode.h"

const int BusyTeamFactory::DUE_HOURS;

std::string BusyTeamFactory::name() const {
    return "BusyTeam";
}

std::unique_ptr<CompositeTask> BusyTeamFactory::createStage(
    const std::string& id, const std::string& name) const {
    std::unique_ptr<CompositeTask> stage(new CompositeTask(id, name));
    stage->setMode(std::unique_ptr<StageMode>(new ParallelMode()));
    return stage;
}

std::unique_ptr<Task> BusyTeamFactory::createWorkItem(
    const std::string& id, const std::string& name,
    const std::string& role) const {
    return decorateProjectTask(std::unique_ptr<Task>(new LeafTask(id, name, role, DUE_HOURS)));
}

std::unique_ptr<ApprovalHandler> BusyTeamFactory::createApprovalChain() const {
    // Reviewer -> Tech Lead. Each handler owns the one after it.
    std::unique_ptr<ApprovalHandler> head(new RoleApprovalHandler("Reviewer"));
    head->setNext(new RoleApprovalHandler("TechLead"));
    return head;
}
