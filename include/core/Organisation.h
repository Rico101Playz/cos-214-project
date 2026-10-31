#ifndef ORGANISATION_H
#define ORGANISATION_H

#include <memory>
#include <string>
#include <vector>
#include "core/Participant.h"

// The organisation that runs a workflow: it owns its participants (composition).
// Participants are looked up by name, or by the role a work item asks for.
/**
 * @class Organisation
 * @brief The organisation that runs a workflow: it owns its participants (composition).
 *
 * Participants are looked up by name, or by the role a work item asks for.
 */
class Organisation {
private:
    std::string name_;
    std::vector<std::unique_ptr<Participant> > participants_;

public:
    /**
     * @brief Constructs an Organisation with the specified name.
     * @param[in] name The name of the organisation.
     */
    explicit Organisation(const std::string& name);

    // Participants are owned uniquely, so an Organisation cannot be copied.
    /**
     * @brief Deleted copy constructor to enforce unique ownership.
     */
    Organisation(const Organisation&) = delete;
    
    /**
     * @brief Deleted assignment operator to enforce unique ownership.
     */
    Organisation& operator=(const Organisation&) = delete;

    /**
     * @brief Retrieves the name of the organisation.
     * @return const std::string& The name string.
     */
    const std::string& name() const;

    /**
     * @brief Gets the total number of participants.
     * @return std::size_t The size of the organisation.
     */
    std::size_t size() const;

    // Throws std::invalid_argument if a participant with that name already exists.
    /**
     * @brief Adds a new participant to the organisation.
     * @param[in] name The name of the new participant.
     * @param[in] role The role assigned to the new participant.
     * @return Participant& Reference to the newly created Participant.
     * @throws std::invalid_argument if a participant with that name already exists.
     */
    Participant& addParticipant(const std::string& name, const std::string& role);

    // Returns nullptr when nobody has that name.
    /**
     * @brief Finds a specific participant by their name.
     * @param[in] name The name of the participant to look up.
     * @return Participant* Pointer to the Participant if found.
     * @retval nullptr when nobody has that name.
     */
    Participant* find(const std::string& name) const;

    // Participants who hold the role AND are currently available, in the order
    // they were added. This is the "eligible for assignment" check.
    /**
     * @brief Finds all eligible participants for a given role.
     * 
     * Participants who hold the role AND are currently available, in the order
     * they were added. This is the "eligible for assignment" check.
     * 
     * @param[in] role The role a work item asks for.
     * @return std::vector<Participant*> A vector of pointers to eligible Participants.
     */
    std::vector<Participant*> eligible(const std::string& role) const;
};

#endif // ORGANISATION_H