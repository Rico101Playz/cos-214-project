#ifndef PARTICIPANT_H
#define PARTICIPANT_H

#include <string>

// A person (or team member) who can be assigned work in an Organisation.
// A workflow definition only names the ROLE it needs (e.g. "Reviewer");
// the Participant is the actual person chosen for that role at run time.
/**
 * @class Participant
 * @brief A person (or team member) who can be assigned work in an Organisation.
 * 
 * A workflow definition only names the ROLE it needs (e.g. "Reviewer");
 * the Participant is the actual person chosen for that role at run time.
 */
class Participant {
private:
    std::string name_;
    std::string role_;
    int load_;          // number of items currently assigned to this participant
    bool available_;    // false when away, on leave, etc.

public:
    /**
     * @brief Constructs a Participant with the specified name and role.
     * @param[in] name The name of the participant.
     * @param[in] role The role assigned to the participant.
     */
    Participant(const std::string& name, const std::string& role);

    /**
     * @brief Retrieves the participant's name.
     * @return const std::string& The name of the participant.
     */
    const std::string& name() const;

    /**
     * @brief Retrieves the participant's role.
     * @return const std::string& The role of the participant.
     */
    const std::string& role() const;

    /**
     * @brief Gets the current workload of the participant.
     * @return int The number of items currently assigned to this participant.
     */
    int load() const;

    /**
     * @brief Checks if the participant is currently available to take on work.
     * @return true if the participant is available.
     * @return false if the participant is away, on leave, etc.
     */
    bool isAvailable() const;

    /**
     * @brief Updates the participant's availability status.
     * @param[in] available The new availability status (true for available, false for away/leave).
     */
    void setAvailable(bool available);

    // load never drops below zero
    /**
     * @brief Adjusts the participant's current workload by a given delta.
     * 
     * The load is updated by adding the delta. The load is guaranteed to never drop below zero.
     * 
     * @param[in] delta The amount to change the current load by (can be negative).
     */
    void addLoad(int delta);    // load never drops below zero
};

#endif // PARTICIPANT_H
