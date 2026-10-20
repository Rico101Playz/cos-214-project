#include "patterns/execution/RoleBasedAssignment.h"
#include <vector>

Participant* RoleBasedAssignment::assign(const WorkItem&,
                                         const std::string& role,
                                         const Organisation& organisation) const {
    std::vector<Participant*> candidates = organisation.eligible(role);
    if (candidates.empty()) {
        return nullptr;
    }
    return candidates.front();
}

std::string RoleBasedAssignment::name() const {
    return "RoleBased";
}
