#include "patterns/execution/AssignmentService.h"
#include "patterns/execution/RoleBasedAssignment.h"
#include <stdexcept>
#include <utility>

AssignmentService::AssignmentService(const Organisation& organisation)
    : organisation_(organisation), strategy_(new RoleBasedAssignment()) {}

void AssignmentService::setStrategy(std::unique_ptr<AssignmentStrategy> strategy) {
    if (!strategy) {
        throw std::invalid_argument("AssignmentService: strategy must not be null");
    }
    strategy_ = std::move(strategy);
}

const AssignmentStrategy& AssignmentService::strategy() const {
    return *strategy_;
}

Participant* AssignmentService::chooseAssignee(const WorkItem& item, const std::string& role) const {
    return strategy_->assign(item, role, organisation_);
}
