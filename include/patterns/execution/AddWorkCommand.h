#ifndef ADDWORKCOMMAND_H
#define ADDWORKCOMMAND_H

#include "WorkCommand.h"
#include "core/WorkItem.h"
#include <string>

/**
 * @class AddWorkCommand
 * @brief A command that encapsulates adding a new work item to a workflow stage.
 * 
 * Part of the Command pattern; enables execution, snapshot logging, and undo/redo 
 * capabilities when inserting a new work item into a running workflow instance.
 */
class AddWorkCommand : public WorkCommand {
private:
    std::string stageId;                        /*!< The target stage identifier where the item is added. */
    std::string itemId;                         /*!< The unique identifier of the work item to add. */
    std::shared_ptr<WorkItem> itemTemplate;     /*!< The template providing metadata for the new work item. */

protected:
    /**
     * @brief Executes the command on the given workflow instance.
     * 
     * Adds the work item template to the specified stage in the workflow instance.
     * 
     * @param[in,out] instance Pointer to the target WorkflowInstance.
     * @throws std::invalid_argument if the instance or item template is null.
     * @throws std::logic_error if the workflow instance fails to add the work item.
     */
    void perform(WorkflowInstance* instance) override;

public:
    /**
     * @brief Constructs an AddWorkCommand.
     * 
     * @param[in] store Pointer to the SnapshotStore managing memento rollbacks.
     * @param[in] instanceId The unique ID of the target workflow instance.
     * @param[in] stageId The ID of the stage where the new work item will be placed.
     * @param[in] itemTemplate Shared pointer to the WorkItem template to be added.
     */
    explicit AddWorkCommand(SnapshotStore* store, std::string instanceId, const std::string& stageId, std::shared_ptr<WorkItem> itemTemplate);

    /**
     * @brief Returns a descriptive string representation of the command.
     * 
     * @return std::string A summary description containing the target stage ID.
     */
    std::string describe() const override;
};

#endif // ADDWORKCOMMAND_H