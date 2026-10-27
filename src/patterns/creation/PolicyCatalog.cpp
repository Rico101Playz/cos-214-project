#include "patterns/creation/PolicyCatalog.h"
#include "patterns/creation/SmallTeamFactory.h"
#include "patterns/creation/BusyTeamFactory.h"
#include "patterns/creation/HighRiskFactory.h"
#include <stdexcept>

std::unique_ptr<WorkflowFactory> PolicyCatalog::create(const std::string& policy) {
    if (policy.empty() || policy == "SmallTeam") {
        return std::unique_ptr<WorkflowFactory>(new SmallTeamFactory());
    }
    if (policy == "BusyTeam") {
        return std::unique_ptr<WorkflowFactory>(new BusyTeamFactory());
    }
    if (policy == "HighRisk") {
        return std::unique_ptr<WorkflowFactory>(new HighRiskFactory());
    }
    throw std::invalid_argument("Unknown policy: " + policy);
}

std::vector<std::string> PolicyCatalog::names() {
    std::vector<std::string> list;
    list.push_back("SmallTeam");
    list.push_back("BusyTeam");
    list.push_back("HighRisk");
    return list;
}

bool PolicyCatalog::exists(const std::string& policy) {
    std::vector<std::string> list = names();
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i] == policy) return true;
    }
    return false;
}
