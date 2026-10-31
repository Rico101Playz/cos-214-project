#ifndef APPROVECOMMAND_H
#define APPROVECOMMAND_H

#include "WorkCommand.h"
#include <string>

class WorkflowInstance;

/**
 * @class ApproveCommand
 * @brief Command that encapsulates an approval or rejection decision on a work item.
 * 
 * Implements the Command pattern to record the outcome (approved or rejected) 
 * alongside explanatory comments, enabling command execution logging and memento snapshots.
 */
class ApproveCommand : public WorkCommand {
private:
    std::string itemId;   /*!< The unique identifier of the target work item. */
    bool approved;        /*!< The approval outcome: true if approved, false if rejected. */
    std::string comment;  /*!< Explanatory comments or feedback for the decision. */

protected:
    /**
     * @brief Executes the approval command on the target workflow instance.
     * 
     * Validates that the instance and item exist, verifies the target is a leaf item,
     * and delegates the decision to the workflow instance.
     * 
     * @param[in,out] instance Pointer to the target WorkflowInstance.
     * @throws std::invalid_argument If the instance is null, the item is not found, or it is not a leaf item.
     * @throws std::logic_error If the item is in a state that does not accept the transition.
     */
    void perform(WorkflowInstance* instance) override;

public:
    /**
     * @brief Constructs an ApproveCommand.
     * 
     * @param[in] store Pointer to the SnapshotStore managing memento rollbacks.
     * @param[in] instanceId The unique ID of the target workflow instance.
     * @param[in] itemId The unique ID of the work item to approve or reject.
     * @param[in] approved True to approve the item, false to reject it.
     * @param[in] comment Reason, feedback, or notes regarding the decision.
     */
    ApproveCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId, bool approved, const std::string& comment);
                    
    /**
     * @brief Returns a descriptive string summary of the command.
     * 
     * @return std::string A summary description containing the item ID, decision, and comment.
     */
    std::string describe() const override;
};

#endif // APPROVECOMMAND_H