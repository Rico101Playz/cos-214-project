#include "patterns/execution/RuleBasedAssignment.h"
#include "patterns/execution/RoleBasedAssignment.h"
#include <utility>

RuleBasedAssignment::RuleBasedAssignment(std::unique_ptr<AssignmentStrategy> fallback)
    : fallback_(std::move(fallback)) {
    if (!fallback_) {
        fallback_.reset(new RoleBasedAssignment());
    }
}

void RuleBasedAssignment::addRule(const std::string& fromRole,
                                  const std::string& dataKey,
                                  const std::string& dataValue,
                                  const std::string& toRole) {
    Rule rule;
    rule.fromRole = fromRole;
    rule.dataKey = dataKey;
    rule.dataValue = dataValue;
    rule.toRole = toRole;
    rules_.push_back(rule);
}

Participant* RuleBasedAssignment::assign(const WorkItem& item,
                                         const std::string& role,
                                         const Organisation& organisation) const {
    for (std::size_t i = 0; i < rules_.size(); ++i) {
        const Rule& rule = rules_[i];
        if (rule.fromRole == role &&
            item.hasData(rule.dataKey) &&
            item.getData(rule.dataKey) == rule.dataValue) {
            return fallback_->assign(item, rule.toRole, organisation);
        }
    }
    return fallback_->assign(item, role, organisation);
}

std::string RuleBasedAssignment::name() const {
    return "RuleBased(" + fallback_->name() + ")";
}
