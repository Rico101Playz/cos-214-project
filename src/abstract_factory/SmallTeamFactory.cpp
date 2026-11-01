#include "abstract_factory/SmallTeamFactory.h"
#include "decorator/ValidatedWork.h"
#include "chain_of_responsibility/RoleApprovalHandler.h"

#include "strategy/SequentialMode.h"
#include "chain_of_responsibility/AutoApproveHandler.h"

const int SmallTeamFactory::DUE_HOURS;

std::string SmallTeamFactory::name() const {
    return "SmallTeam";
}

std::unique_ptr<CompositeTask> SmallTeamFactory::createStage(
    const std::string& id, const std::string& name) const {
    std::unique_ptr<CompositeTask> stage(new CompositeTask(id, name));
    stage->setMode(std::unique_ptr<StageMode>(new SequentialMode()));
    return stage;
}

std::unique_ptr<Task> SmallTeamFactory::createWorkItem(
    const std::string& id, const std::string& name,
    const std::string& role) const {
    return decorateProjectTask(std::unique_ptr<Task>(new LeafTask(id, name, role, DUE_HOURS)));
}

std::unique_ptr<ApprovalHandler> SmallTeamFactory::createApprovalChain() const {
    // Auto -> Reviewer. Each handler owns the one after it.
    std::unique_ptr<ApprovalHandler> head(new AutoApproveHandler());
    head->setNext(new RoleApprovalHandler("Reviewer"));
    return head;
}
