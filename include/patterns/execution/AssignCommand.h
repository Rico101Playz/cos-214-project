#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include "WorkCommand.h"
#include <string>

class WorkflowInstance;

/**
 * @class AssignCommand
 * @brief Command that encapsulates assigning a work item to a participant.
 * 
 * Implements the Command pattern to assign a leaf work item to a specific participant
 * within a running workflow instance, enabling execution logging and memento snapshots.
 */
class AssignCommand : public WorkCommand {
private:
    std::string itemId;          /*!< The unique identifier of the target work item. */
    std::string participantName; /*!< The name or identifier of the participant receiving the assignment. */

protected:
    /**
     * @brief Executes the assignment command on the target workflow instance.
     * 
     * Validates that the instance exists, verifies the target is a leaf item,
     * and delegates the assignment to the workflow instance.
     * 
     * @param[in,out] instance Pointer to the target WorkflowInstance.
     * @throws std::invalid_argument If the instance is null, the item is unknown, or it is not a leaf item.
     * @throws std::logic_error If the item cannot be assigned to the specified participant.
     */
    void perform(WorkflowInstance* instance) override;

public:
    /**
     * @brief Constructs an AssignCommand.
     * 
     * @param[in] store Pointer to the SnapshotStore managing memento rollbacks.
     * @param[in] instanceId The unique ID of the target workflow instance.
     * @param[in] itemId The unique ID of the work item to assign.
     * @param[in] participantName The name of the participant being assigned the work.
     */
    AssignCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, const std::string& participantName);

    /**
     * @brief Returns a descriptive string summary of the command.
     * 
     * @return std::string A summary description containing the item ID and participant name.
     */
    std::string describe() const override;
};

#endif // ASSIGNCOMMAND_H