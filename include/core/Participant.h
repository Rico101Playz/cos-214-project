#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>

// A person (or team member) who can be assigned work in an Organisation.
// A workflow definition only names the ROLE it needs (e.g. "Reviewer");
// the Participant is the actual person chosen for that role at run time.
class Participant {
private:
    std::string name_;
    std::string role_;
    int load_;          // number of items currently assigned to this participant
    bool available_;    // false when away, on leave, etc.

public:
    Participant(const std::string& name, const std::string& role);

    const std::string& name() const;
    const std::string& role() const;
    int load() const;
    bool isAvailable() const;

    void setAvailable(bool available);
    void addLoad(int delta);    // load never drops below zero
};

#endif // PARTICIPANT_H
