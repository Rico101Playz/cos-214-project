#ifndef ORGUNIT_H
#define ORGUNIT_H

#include <memory>
#include <string>
#include <vector>
#include "patterns/structural/OrgComponent.h"

class OrgUnit : public OrgComponent {
private:
    std::vector<std::unique_ptr<OrgComponent> > children_;

public:
    OrgUnit(const std::string& id, const std::string& name);

    bool isUnit() const override;
    void collectMembers(std::vector<Participant*>& out) const override;
    std::size_t childCount() const override;
    OrgComponent* child(std::size_t index) const override;

    OrgComponent& add(std::unique_ptr<OrgComponent> child);
    std::unique_ptr<OrgComponent> remove(const std::string& id);
    std::vector<OrgUnit*> subUnits() const;
};

#endif
