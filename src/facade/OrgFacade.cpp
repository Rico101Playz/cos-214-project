#include "facade/OrgFacade.h"
#include <memory>
#include <stdexcept>
#include "strategy/LeastLoadedAssignment.h"
#include "strategy/RoleBasedAssignment.h"
#include "strategy/RuleBasedAssignment.h"

OrgFacade::OrgFacade(const std::string& organisationName) : organisation_(organisationName) {
    assignment_.setStrategy("SmallTeam", std::unique_ptr<AssignmentStrategy>(new RoleBasedAssignment()));
    assignment_.setStrategy("BusyTeam", std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment()));
    // Everything else is shared out by load; review work on a risk=high item
    // goes to the Tech Lead. The role names match the approval handlers.
    std::unique_ptr<RuleBasedAssignment> rules(
        new RuleBasedAssignment(std::unique_ptr<AssignmentStrategy>(new LeastLoadedAssignment())));
    rules->addRule("Reviewer", "risk", "high", "TechLead");
    assignment_.setStrategy("HighRisk", std::unique_ptr<AssignmentStrategy>(rules.release()));
}

Response OrgFacade::handle(const Request& request) {
    switch (request.action()) {
        case Action::AddUnit:
            organisation_.addUnit(request.arg("parent"), request.arg("unit"), request.arg("name"));
            break;
        case Action::AddPerson:
            organisation_.addParticipant(request.arg("person"), request.arg("role"));
            organisation_.placeMember(request.arg("unit"), request.arg("person"));
            break;
        case Action::MoveMember:
            organisation_.placeMember(request.arg("unit"), request.arg("person"));
            break;
        default:
            throw std::invalid_argument(toString(request.action()) + " is not something the organisation does");
    }
    return Response();
}

Company& OrgFacade::company() { return organisation_; }

const Company& OrgFacade::company() const { return organisation_; }

Participant& OrgFacade::findParticipant(const std::string& name) const {
    Participant* participant = organisation_.find(name);
    if (participant == nullptr) {
        throw std::invalid_argument("Unknown participant: " + name);
    }
    return *participant;
}

const AssignmentStrategy& OrgFacade::strategyFor(const std::string& policy) const {
    return assignment_.strategyFor(policy);
}

AssignmentPipeline::Result OrgFacade::choose(const std::string& policy, bool teamFirst, const TaskFacts& facts,
                                             const std::string& role, const std::string& requested,
                                             const std::string& fallback) const {
    const AssignmentStrategy& strategy = assignment_.strategyFor(policy);
    if (teamFirst) return TeamFirstPipeline(organisation_, strategy).choose(facts, role, requested, fallback);
    return CompanyWidePipeline(organisation_, strategy).choose(facts, role, requested, fallback);
}

std::string OrgFacade::ownerRole() const {
    return organisation_.hasOwner() ? Company::OWNER_ROLE : std::string();
}
