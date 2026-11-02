#include "abstract_factory/HighRiskFactory.h"
#include "decorator/ValidatedWork.h"
#include "chain_of_responsibility/RoleApprovalHandler.h"

#include "strategy/SequentialMode.h"
#include "chain_of_responsibility/TimeoutEscalationHandler.h"

const int HighRiskFactory::DUE_HOURS;

std::string HighRiskFactory::name() const {
    return "HighRisk";
}

std::unique_ptr<CompositeTask> HighRiskFactory::createStage(
    const std::string& id, const std::string& name) const {
    std::unique_ptr<CompositeTask> stage(new CompositeTask(id, name));
    stage->setMode(std::unique_ptr<StageMode>(new SequentialMode()));
    return stage;
}

std::unique_ptr<Task> HighRiskFactory::createWorkItem(
    const std::string& id, const std::string& name,
    const std::string& role) const {
    std::unique_ptr<LeafTask> item(new LeafTask(id, name, role, DUE_HOURS));
    item->setData("risk", "high");   // the tag the assignment rule below reads
    return decorateProjectTask(std::unique_ptr<Task>(item.release()));
}

std::unique_ptr<ApprovalHandler> HighRiskFactory::createApprovalChain() const {
    // Reviewer -> Tech Lead -> Timeout escalation -> Release Manager.
    // Each handler owns the one after it, so the chain is built from the
    // front and `tail` always points at its last link.
    std::unique_ptr<ApprovalHandler> head(new RoleApprovalHandler("Reviewer"));
    ApprovalHandler* tail = head.get();

    ApprovalHandler* techLead = new RoleApprovalHandler("TechLead");
    tail->setNext(techLead);
    tail = techLead;

    ApprovalHandler* timeout = new TimeoutEscalationHandler("ReleaseManager");
    tail->setNext(timeout);
    tail = timeout;

    tail->setNext(new RoleApprovalHandler("ReleaseManager"));
    return head;
}
