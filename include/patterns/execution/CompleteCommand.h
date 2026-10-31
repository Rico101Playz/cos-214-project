#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include "WorkCommand.h"
#include <string>

class WorkflowInstance;

/**
 * @class CompleteCommand
 * @brief Command that encapsulates completing an active leaf work item.
 * 
 * Part of the Command pattern; invokes the completion transition on a specified
 * LeafWorkItem within a running WorkflowInstance, enabling execution logging
 * and memento-based undo/redo support.
 */
class CompleteCommand : public WorkCommand {
private:
    std::string itemId; /*!< The unique identifier of the target work item to complete. */

protected:
    /**
     * @brief Executes the completion command on the target workflow instance.
     * 
     * Verifies that the workflow instance exists, confirms the target is a valid
     * leaf work item, and triggers its completion transition on the workflow instance.
     * 
     * @param[in,out] instance Pointer to the target WorkflowInstance.
     * @throws std::invalid_argument If instance is null, the item is not found, or it is not a leaf work item.
     * @throws std::logic_error If the work item cannot be completed in its current lifecycle state.
     */
    void perform(WorkflowInstance* instance) override;

public:
    /**
     * @brief Constructs a CompleteCommand.
     * 
     * @param[in] store Pointer to the SnapshotStore managing memento rollbacks.
     * @param[in] instanceId The unique identifier of the target workflow instance.
     * @param[in] itemId The unique identifier of the work item to complete.
     */
    explicit CompleteCommand(SnapshotStore* store, std::string instanceId, const std::string& itemId);

    /**
     * @brief Returns a descriptive string representation of the command.
     * 
     * @return std::string A summary description containing the target item ID.
     */
    std::string describe() const override;
};

#endif // COMPLETECOMMAND_H