#ifndef ORGCOMPONENT_H
#define ORGCOMPONENT_H

#include <cstddef>
#include <string>
#include <vector>

class Participant;
class OrgUnit;

class OrgComponent {
private:
    std::string id_;
    std::string name_;
    OrgUnit* parent_;

public:
    OrgComponent(const std::string& id, const std::string& name);
    virtual ~OrgComponent() = default;

    OrgComponent(const OrgComponent&) = delete;
    OrgComponent& operator=(const OrgComponent&) = delete;

    const std::string& id() const;
    const std::string& name() const;
    OrgUnit* parent() const;
    void setParent(OrgUnit* parent);

    virtual bool isUnit() const = 0;
    virtual void collectMembers(std::vector<Participant*>& out) const = 0;
    virtual std::size_t childCount() const;
    virtual OrgComponent* child(std::size_t index) const;

    OrgComponent* find(const std::string& id);
    bool contains(const OrgComponent* other) const;
    std::string path() const;
};

#endif
