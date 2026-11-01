#ifndef ORGMEMBER_H
#define ORGMEMBER_H

#include "patterns/structural/OrgComponent.h"

class OrgMember : public OrgComponent {
private:
    Participant* participant_;

public:
    explicit OrgMember(Participant* participant);

    bool isUnit() const override;
    void collectMembers(std::vector<Participant*>& out) const override;
    Participant* participant() const;
};

#endif
