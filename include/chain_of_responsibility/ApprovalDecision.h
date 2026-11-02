#ifndef APPROVAL_DECISION_H
#define APPROVAL_DECISION_H

#include <string>

/**
 * @class ApprovalDecision
 * @brief Represents a record of a single approval decision made within an approval chain.
 * 
 * Stores audit details regarding an approval action, including the identity and role 
 * of the decider, the outcome (approved or rejected), and any accompanying feedback or comments.
 */
class ApprovalDecision {
public:
    std::string participant; /*!< The name or identifier of the participant or handler making the decision. */
    std::string role;        /*!< The role of the decider (e.g., "Reviewer", "TechLead", "ReleaseManager"). */
    bool approved;           /*!< The decision outcome: true if approved, false if rejected. */
    std::string comment;     /*!< Explanatory comments or reasoning provided for the decision. */

    /**
     * @brief Constructs an ApprovalDecision record with specified or default values.
     * 
     * @param[in] p The participant name or system identifier making the decision.
     * @param[in] r The role associated with the decision maker.
     * @param[in] a The decision outcome flag (true for approved, false for rejected).
     * @param[in] c Any comment, reason, or notes describing the decision.
     */
    ApprovalDecision(std::string p = "", std::string r = "", bool a = false, std::string c = "")
        : participant(p), role(r), approved(a), comment(c) {}
};

#endif // APPROVAL_DECISION_H