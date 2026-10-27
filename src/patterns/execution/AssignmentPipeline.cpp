#include "patterns/execution/AssignmentPipeline.h"
#include "patterns/execution/AssignmentStrategy.h"
#include "core/Company.h"
#include "core/Organisation.h"
#include "core/WorkItem.h"

AssignmentPipeline::AssignmentPipeline(const Company& company, const AssignmentStrategy& strategy)
    : company_(company), strategy_(strategy) {}

const Company& AssignmentPipeline::company() const { return company_; }

Participant* AssignmentPipeline::pickWithin(const WorkItem& item, const std::string& role,
                                            const std::string& scope) const {
    Organisation view(company_.name());
    std::vector<Participant*> members = company_.membersOf(scope);
    for (std::size_t i = 0; i < members.size(); ++i) {
        Participant* real = members[i];
        if (role.empty() && Company::isBossRole(real->role())) continue;
        Participant& copy = view.addParticipant(real->name(), role.empty() ? std::string() : real->role());
        copy.setAvailable(real->isAvailable());
        copy.addLoad(real->load());
    }
    Participant* chosen = strategy_.assign(item, role, view);
    return chosen == nullptr ? nullptr : company_.find(chosen->name());
}

AssignmentPipeline::Result AssignmentPipeline::choose(const WorkItem& item, const std::string& role,
                                                      const std::string& requested,
                                                      const std::string& fallbackPerson) const {
    Result result;

    if (!requested.empty()) {
        Participant* person = company_.find(requested);
        if (person != nullptr && person->isAvailable()) {
            result.person = person;
            result.via = "picked by hand";
            return result;
        }
    }

    std::vector<std::string> scopes = scopesFor(item);
    for (std::size_t i = 0; i < scopes.size() && result.person == nullptr; ++i) {
        result.person = pickWithin(item, role, scopes[i]);
        if (result.person != nullptr) {
            std::string where = scopes[i].empty() ? std::string("the whole company")
                                                  : company_.findUnit(scopes[i])->name();
            result.via = i == 0 ? where : "widened to " + where;
        }
    }
    if (result.person != nullptr) return result;

    if (!fallbackPerson.empty()) {
        Participant* person = company_.find(fallbackPerson);
        if (person != nullptr && person->isAvailable()) {
            result.person = person;
            result.via = "owner of the parent task";
            return result;
        }
    }

    if (role == Company::MANAGER_ROLE) {
        std::vector<Participant*> owners = company_.eligible(Company::OWNER_ROLE, "");
        if (!owners.empty()) {
            result.person = owners.front();
            result.via = "no manager free, went to the owner";
        }
    }
    return result;
}

TeamFirstPipeline::TeamFirstPipeline(const Company& company, const AssignmentStrategy& strategy)
    : AssignmentPipeline(company, strategy) {}

std::vector<std::string> TeamFirstPipeline::scopesFor(const WorkItem& item) const {
    std::vector<std::string> scopes;
    std::string team = item.getData("unit");
    if (!team.empty()) {
        for (OrgUnit* unit = company().findUnit(team); unit != nullptr; unit = unit->parent()) {
            scopes.push_back(unit->parent() == nullptr ? std::string() : unit->id());
        }
    } else {
        scopes.push_back("");
    }
    return scopes;
}

CompanyWidePipeline::CompanyWidePipeline(const Company& company, const AssignmentStrategy& strategy)
    : AssignmentPipeline(company, strategy) {}

std::vector<std::string> CompanyWidePipeline::scopesFor(const WorkItem&) const {
    return std::vector<std::string>(1, std::string());
}
