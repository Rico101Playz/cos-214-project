#ifndef WORKITEM_H
#define WORKITEM_H

#include <map>
#include <string>

// A single unit of work, e.g. "Implement backend" or "Code review".
//
// NOTE (integration point): this is the minimal shape the assignment
// strategies and approval chain need - an id, a name, the ROLE that should do
// the work, and key/value data (e.g. risk = high). The class diagram grows this
// into WorkNode -> WorkItem -> LeafWorkItem (Composite) with a lifecycle
// (State). Whoever lands those patterns should extend this class and keep
// id(), name(), role(), setData(), hasData() and getData() working, so the
// strategies and approval handlers do not need to change.
class WorkItem {
private:
    std::string id_;
    std::string name_;
    std::string role_;
    std::map<std::string, std::string> data_;

public:
    WorkItem(const std::string& id, const std::string& name, const std::string& role);

    const std::string& id() const;
    const std::string& name() const;
    const std::string& role() const;

    void setData(const std::string& key, const std::string& value);
    bool hasData(const std::string& key) const;
    std::string getData(const std::string& key, const std::string& fallback = "") const;
};

#endif // WORKITEM_H
