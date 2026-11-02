#ifndef ASSIGNMENTPIPELINE_H
#define ASSIGNMENTPIPELINE_H

#include <string>
#include <vector>
#include "strategy/AssignmentStrategy.h"

class Company;
class Organisation;
class Participant;

class AssignmentPipeline {
private:
    const Company& company_;
    const AssignmentStrategy& strategy_;

    Participant* pickWithin(const TaskFacts& facts, const std::string& role, const std::string& scope) const;

protected:
    const Company& company() const;
    virtual std::vector<std::string> scopesFor(const TaskFacts& facts) const = 0;

public:
    struct Result {
        Participant* person;
        std::string via;
        Result() : person(nullptr) {}
    };

    AssignmentPipeline(const Company& company, const AssignmentStrategy& strategy);
    virtual ~AssignmentPipeline() = default;

    Result choose(const TaskFacts& facts, const std::string& role, const std::string& requested = "",
                  const std::string& fallbackPerson = "") const;
};

class TeamFirstPipeline : public AssignmentPipeline {
protected:
    std::vector<std::string> scopesFor(const TaskFacts& facts) const override;

public:
    TeamFirstPipeline(const Company& company, const AssignmentStrategy& strategy);
};

class CompanyWidePipeline : public AssignmentPipeline {
protected:
    std::vector<std::string> scopesFor(const TaskFacts& facts) const override;

public:
    CompanyWidePipeline(const Company& company, const AssignmentStrategy& strategy);
};

#endif
