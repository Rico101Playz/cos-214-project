#include "strategy/RuleBasedAssignment.h"
#include "strategy/RoleBasedAssignment.h"
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

Participant* RuleBasedAssignment::assign(const TaskFacts& facts,
                                         const std::string& role,
                                         const Organisation& organisation) const {
    for (std::size_t i = 0; i < rules_.size(); ++i) {
        const Rule& rule = rules_[i];
        TaskFacts::const_iterator fact = facts.find(rule.dataKey);
        if (rule.fromRole == role && fact != facts.end() && fact->second == rule.dataValue) {
            return fallback_->assign(facts, rule.toRole, organisation);
        }
    }
    return fallback_->assign(facts, role, organisation);
}

std::string RuleBasedAssignment::name() const {
    return "RuleBased(" + fallback_->name() + ")";
}
