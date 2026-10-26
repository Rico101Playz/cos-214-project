#ifndef RULEBASEDASSIGNMENT_H
#define RULEBASEDASSIGNMENT_H

#include <memory>
#include <string>
#include <vector>
#include "patterns/execution/AssignmentStrategy.h"

// Routes work by the item's data, then lets a fallback strategy pick the person.
//
// A rule reads: "when the role asked for is <fromRole> and the item's
// <dataKey> equals <dataValue>, ask for <toRole> instead".
// Example (Feature Delivery): Reviewer + risk=high -> Tech Lead.
//
// Rules are checked in the order they were added and the first match wins. If
// no rule matches, the original role is used. If the rerouted role has nobody
// available the result is nullptr - a high-risk item must never silently fall
// back to the ordinary role.
//
// The fallback is owned by this object (composition) and decides WHO among the
// eligible people is chosen, so rules and picking policy combine freely.
class RuleBasedAssignment : public AssignmentStrategy {
private:
    struct Rule {
        std::string fromRole;
        std::string dataKey;
        std::string dataValue;
        std::string toRole;
    };

    std::vector<Rule> rules_;
    std::unique_ptr<AssignmentStrategy> fallback_;

public:
    // A null fallback means "use RoleBasedAssignment".
    explicit RuleBasedAssignment(std::unique_ptr<AssignmentStrategy> fallback);

    void addRule(const std::string& fromRole,
                 const std::string& dataKey,
                 const std::string& dataValue,
                 const std::string& toRole);

    Participant* assign(const WorkItem& item,
                        const std::string& role,
                        const Organisation& organisation) const override;

    std::string name() const override;
};

#endif // RULEBASEDASSIGNMENT_H
