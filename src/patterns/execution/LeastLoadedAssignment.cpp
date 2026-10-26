#include "patterns/execution/LeastLoadedAssignment.h"
#include <vector>

Participant* LeastLoadedAssignment::assign(const WorkItem&,
                                           const std::string& role,
                                           const Organisation& organisation) const {
    std::vector<Participant*> candidates = organisation.eligible(role);

    Participant* leastLoaded = nullptr;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        // strictly less-than keeps the earliest-registered participant on a tie
        if (leastLoaded == nullptr || candidates[i]->load() < leastLoaded->load()) {
            leastLoaded = candidates[i];
        }
    }
    return leastLoaded;
}

std::string LeastLoadedAssignment::name() const {
    return "LeastLoaded";
}
