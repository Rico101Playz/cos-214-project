#include "strategy/AssignmentService.h"
#include "strategy/RoleBasedAssignment.h"
#include <stdexcept>
#include <utility>

AssignmentService::AssignmentService() : organisation_(nullptr), strategy_(new RoleBasedAssignment()) {}

AssignmentService::AssignmentService(const Organisation& organisation)
    : organisation_(&organisation), strategy_(new RoleBasedAssignment()) {}

void AssignmentService::setStrategy(std::unique_ptr<AssignmentStrategy> strategy) {
    if (!strategy) {
        throw std::invalid_argument("AssignmentService: strategy must not be null");
    }
    strategy_ = std::move(strategy);
}

const AssignmentStrategy& AssignmentService::strategy() const {
    return *strategy_;
}

void AssignmentService::setStrategy(const std::string& policy, std::unique_ptr<AssignmentStrategy> strategy) {
    if (!strategy) {
        throw std::invalid_argument("AssignmentService: strategy must not be null");
    }
    policies_[policy] = std::move(strategy);
}

const AssignmentStrategy& AssignmentService::strategyFor(const std::string& policy) const {
    std::map<std::string, std::unique_ptr<AssignmentStrategy> >::const_iterator it = policies_.find(policy);
    return it == policies_.end() ? *strategy_ : *it->second;
}

Participant* AssignmentService::chooseAssignee(const TaskFacts& facts, const std::string& role) const {
    if (organisation_ == nullptr) {
        throw std::logic_error("AssignmentService: no organisation to choose from");
    }
    return strategy_->assign(facts, role, *organisation_);
}
