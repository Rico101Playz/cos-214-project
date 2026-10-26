#include "patterns/creation/HighRiskFactory.h"

#include "patterns/execution/SequentialMode.h"
#include "patterns/execution/LeastLoadedAssignment.h"
#include "patterns/execution/RuleBasedAssignment.h"
#include "patterns/execution/ReviewerHandler.h"
#include "patterns/execution/TechLeadHandler.h"
#include "patterns/execution/TimeoutEscalationHandler.h"
#include "patterns/execution/ReleaseManagerHandler.h"

const int HighRiskFactory::DUE_HOURS;

std::string HighRiskFactory::name() const {
    return "HighRisk";
}

std::unique_ptr<CompositeStage> HighRiskFactory::createStage(
    const std::string& id, const std::string& name) const {
    std::unique_ptr<CompositeStage> stage(new CompositeStage(id, name));
    stage->setMode(std::unique_ptr<StageMode>(new SequentialMode()));
    return stage;
}

std::unique_ptr<LeafWorkItem> HighRiskFactory::createWorkItem(
    const std::string& id, const std::string& name,
    const std::string& role) const {
    std::unique_ptr<LeafWorkItem> item(new LeafWorkItem(id, name, role, DUE_HOURS));
    item->setData("risk", "high");   // the tag the assignment rule below reads
    return item;
}

std::unique_ptr<AssignmentStrategy> HighRiskFactory::createAssignmentStrategy() const {
    // Everything else is shared out by load; review work on a risk=high item
    // goes to the Tech Lead. The role names match the approval handlers.
    std::unique_ptr<RuleBasedAssignment> rules(
        new RuleBasedAssignment(std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment())));
    rules->addRule("Reviewer", "risk", "high", "TechLead");
    return std::unique_ptr<AssignmentStrategy>(rules.release());
}

std::unique_ptr<ApprovalHandler> HighRiskFactory::createApprovalChain() const {
    // Reviewer -> Tech Lead -> Timeout escalation -> Release Manager.
    // Each handler owns the one after it, so the chain is built from the
    // front and `tail` always points at its last link.
    std::unique_ptr<ApprovalHandler> head(new ReviewerHandler());
    ApprovalHandler* tail = head.get();

    ApprovalHandler* techLead = new TechLeadHandler();
    tail->setNext(techLead);
    tail = techLead;

    ApprovalHandler* timeout = new TimeoutEscalationHandler("ReleaseManager");
    tail->setNext(timeout);
    tail = timeout;

    tail->setNext(new ReleaseManagerHandler());
    return head;
}
