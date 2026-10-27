#include "patterns/structural/OrgMember.h"
#include "core/Participant.h"
#include <stdexcept>

OrgMember::OrgMember(Participant* participant)
    : OrgComponent(participant ? participant->name() : "", participant ? participant->name() : ""),
      participant_(participant) {
    if (participant == nullptr) {
        throw std::invalid_argument("OrgMember needs a participant");
    }
}

bool OrgMember::isUnit() const { return false; }

void OrgMember::collectMembers(std::vector<Participant*>& out) const {
    out.push_back(participant_);
}

Participant* OrgMember::participant() const { return participant_; }
