#ifndef ORGANISATION_H
#define ORGANISATION_H

#include <memory>
#include <string>
#include <vector>
#include "core/Participant.h"

// The organisation that runs a workflow: it owns its participants (composition).
// Participants are looked up by name, or by the role a work item asks for.
class Organisation {
private:
    std::string name_;
    std::vector<std::unique_ptr<Participant> > participants_;

public:
    explicit Organisation(const std::string& name);

    // Participants are owned uniquely, so an Organisation cannot be copied.
    Organisation(const Organisation&) = delete;
    Organisation& operator=(const Organisation&) = delete;

    const std::string& name() const;
    std::size_t size() const;

    // Throws std::invalid_argument if a participant with that name already exists.
    Participant& addParticipant(const std::string& name, const std::string& role);

    // Returns nullptr when nobody has that name.
    Participant* find(const std::string& name) const;

    // Participants who hold the role AND are currently available, in the order
    // they were added. This is the "eligible for assignment" check.
    std::vector<Participant*> eligible(const std::string& role) const;
};

#endif // ORGANISATION_H
