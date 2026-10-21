#ifndef APPROVAL_DECISION_H
#define APPROVAL_DECISION_H

#include <string>

class ApprovalDecision {
public:
    std::string participant;
    std::string role;
    bool approved;
    std::string comment;

    ApprovalDecision(std::string p = "", std::string r = "", bool a = false, std::string c = "")
        : participant(p), role(r), approved(a), comment(c) {}
};

#endif