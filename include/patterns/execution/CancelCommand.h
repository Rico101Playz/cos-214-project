#ifndef CANCELCOMMAND_H
#define CANCELCOMMAND_H

#include "WorkCommand.h"
#include <string>

class WorkflowInstance;

/**
 * @class CancelCommand
 * @brief Command that encapsulates the cancellation of an entire workflow instance or a specific work item.
 * 
 * Part of the Command pattern; supports cancelling an entire workflow instance when the target
 * node matches the instance ID, or cancelling an individual LeafWorkItem within the running workflow.
 */
class CancelCommand : public WorkCommand {
private:
    std::string nodeId; /*!< The identifier of the node (workflow instance or work item) to cancel. */
    std::string reason; /*!< The explanatory reason or justification for cancellation. */

protected:
    /**
     * @brief Executes the cancellation on the specified workflow instance.
     * 
     * If the target nodeId matches the instance ID, the entire workflow instance is cancelled.
     * Otherwise, locates the specific work item, verifies that it is a cancellable leaf node,
     * checks that it is not already in a terminal state, and executes cancellation.
     * 
     * @param[in,out] instance Pointer to the target WorkflowInstance.
     * @throws std::invalid_argument If instance is null, the target item does not exist, or the node is not a leaf item.
     * @throws std::logic_error If the instance cannot be cancelled or the work item is already completed or cancelled.
     */
    void perform(WorkflowInstance* instance) override;

public:
    /**
     * @brief Constructs a CancelCommand.
     * 
     * @param[in] store Pointer to the SnapshotStore managing memento rollbacks.
     * @param[in] instanceId The unique ID of the target workflow instance.
     * @param[in] nodeId The ID of the node (workflow instance ID or leaf work item ID) to cancel.
     * @param[in] reason Explanatory note or reason for cancelling the node.
     */
    CancelCommand(SnapshotStore* store, std::string instanceId, const std::string& nodeId, const std::string& reason);

    /**
     * @brief Returns a descriptive string summary of the command.
     * 
     * @return std::string A summary description containing the target node ID and cancellation reason.
     */
    std::string describe() const override;
};

#endif // CANCELCOMMAND_H