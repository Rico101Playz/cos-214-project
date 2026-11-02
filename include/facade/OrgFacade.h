#ifndef ORG_FACADE_H
#define ORG_FACADE_H

#include <string>
#include "org_composite/Company.h"
#include "strategy/AssignmentPipeline.h"
#include "strategy/AssignmentService.h"
#include "mediator/Colleague.h"

// FACADE pattern - one of the three facades behind TividyMediator. MEDIATOR pattern - a COLLEAGUE.
//
// Fronts the organisation: the company, its teams and its people, and who
// gets work: each policy's assignment strategy lives here, keyed by the
// policy's name, and choose() picks a person from the facts of a task.
// Changes to the organisation arrive as Requests (AddUnit, AddPerson, MoveMember).
// It never refers to the other facades; TividyMediator coordinates them through the public interface below.
class OrgFacade : public Colleague {
private:
    Company organisation_;
    AssignmentService assignment_;

public:
    explicit OrgFacade(const std::string& organisationName);

    OrgFacade(const OrgFacade&) = delete;
    OrgFacade& operator=(const OrgFacade&) = delete;

    // AddUnit (parent, unit, name), AddPerson (person, role, unit) and MoveMember (unit, person).
    Response handle(const Request& request) override;

    Company& company();
    const Company& company() const;
    Participant& findParticipant(const std::string& name) const;
    const AssignmentStrategy& strategyFor(const std::string& policy) const;
    // Picks someone for a task: the requested person when they are free,
    // otherwise the policy's strategy, team first when `teamFirst`, else
    // across the whole company. `fallback` is tried when nobody else fits.
    AssignmentPipeline::Result choose(const std::string& policy, bool teamFirst, const TaskFacts& facts,
                                      const std::string& role, const std::string& requested,
                                      const std::string& fallback) const;
    // The role an owner approves as, or "" when the company has no owner.
    std::string ownerRole() const;
};

#endif // ORG_FACADE_H
