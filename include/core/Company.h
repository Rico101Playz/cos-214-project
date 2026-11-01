#ifndef COMPANY_H
#define COMPANY_H

#include <memory>
#include <string>
#include <vector>
#include "core/Organisation.h"
#include "patterns/structural/OrgUnit.h"

class Company {
private:
    std::unique_ptr<Organisation> people_;
    std::unique_ptr<OrgUnit> structure_;
    std::vector<std::string> order_;

public:
    static const char* const MANAGER_ROLE;
    static const char* const OWNER_ROLE;

    explicit Company(const std::string& name);

    Company(const Company&) = delete;
    Company& operator=(const Company&) = delete;

    void swap(Company& other);
    void reset(const std::string& name);

    const std::string& name() const;
    std::size_t size() const;

    Organisation& people();
    const Organisation& people() const;

    Participant& addParticipant(const std::string& name, const std::string& role);
    Participant* find(const std::string& name) const;
    std::vector<Participant*> participants() const;

    bool isOwner(const std::string& participantName) const;
    bool hasOwner() const;
    bool isManager(const std::string& participantName) const;
    static bool isBossRole(const std::string& role);

    OrgUnit& structure() const;
    OrgUnit& addUnit(const std::string& parentId, const std::string& id, const std::string& name);
    void placeMember(const std::string& unitId, const std::string& participantName);
    OrgUnit* findUnit(const std::string& id) const;
    OrgUnit* unitOf(const std::string& participantName) const;
    bool inUnit(const std::string& participantName, const std::string& unitId) const;

    std::vector<Participant*> eligible(const std::string& role) const;
    std::vector<Participant*> membersOf(const std::string& unitId) const;
    std::vector<Participant*> eligible(const std::string& role, const std::string& unitId) const;
};

#endif
