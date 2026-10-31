#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <cstddef>
#include <memory>

#include "core/WorkflowDefinition.h"
#include "core/WorkItem.h"
#include "patterns/execution/InstanceState.h"
#include "patterns/execution/Memento.h"
#include "patterns/structural/CompositeStage.h"

/**
 * @class WorkflowInstance
 * @brief Represents a running instance of a workflow definition.
 * 
 * Manages the execution state, the hierarchical tree of work items, 
 * and handles lifecycle and item operations delegated to polymorphic state objects.
 */
class WorkflowInstance {
public:
    static const char* const CREATED;   /*!< String constant representing the Created state. */
    static const char* const RUNNING;   /*!< String constant representing the Running state. */
    static const char* const COMPLETED; /*!< String constant representing the Completed state. */
    static const char* const CANCELLED; /*!< String constant representing the Cancelled state. */

    /**
     * @brief Constructs a new Workflow Instance from a given definition.
     * @param[in] id The unique identifier for this workflow instance.
     * @param[in] definition The WorkflowDefinition blueprint to build this instance from.
     */
    WorkflowInstance(const std::string& id, const WorkflowDefinition& definition);

    /**
     * @brief Deleted copy constructor to prevent copying of workflow instances.
     */
    WorkflowInstance(const WorkflowInstance&) = delete;
    
    /**
     * @brief Deleted assignment operator to prevent copying of workflow instances.
     */
    WorkflowInstance& operator=(const WorkflowInstance&) = delete;

    // ---- accessors ----
    
    /**
     * @brief Gets the ID of the workflow instance.
     * @return const std::string& The instance ID.
     */
    const std::string& id() const;
    
    /**
     * @brief Gets the title of the workflow instance (typically the definition name).
     * @return const std::string& The title string.
     */
    const std::string& title() const;
    
    /**
     * @brief Gets the current overall state of the workflow instance.
     * @return const std::string& The current state.
     */
    const std::string& state() const;
    
    /**
     * @brief Gets the reason for the current state (e.g., cancellation explanation).
     * @return const std::string& The reason string.
     */
    const std::string& reason() const;
    
    /**
     * @brief Gets the blueprint definition used to create this instance.
     * @return const WorkflowDefinition& The workflow definition.
     */
    const WorkflowDefinition& definition() const;

    /**
     * @brief Gets the root composite stage containing all work nodes.
     * @return CompositeStage& Reference to the root stage.
     */
    CompositeStage& root() const;
    
    /**
     * @brief Gets the currently active stage of the workflow.
     * @return WorkNode* Pointer to the current stage node, or nullptr if not running.
     */
    WorkNode* currentStage() const;
    
    /**
     * @brief Finds a specific node (stage or item) within the workflow tree by its ID.
     * @param[in] nodeId The ID of the node to find.
     * @return WorkNode* Pointer to the found node, or nullptr if not found.
     */
    WorkNode* find(const std::string& nodeId) const;
    
    /**
     * @brief Finds a specific work item within the workflow tree by its ID.
     * @param[in] itemId The ID of the work item to find.
     * @return WorkItem* Pointer to the found work item, or nullptr if not found.
     */
    WorkItem* item(const std::string& itemId) const;

    // ---- lifecycle (delegated to instanceState_) ----
    
    /**
     * @brief Starts the workflow instance execution.
     */
    void start();
    
    /**
     * @brief Advances the workflow instance to the next logical stage or step.
     */
    void advance();
    
    /**
     * @brief Cancels the workflow instance.
     * @param[in] reason The explanation for why the workflow is being cancelled.
     */
    void cancel(const std::string& reason);

    // ---- per-item operations, used by the Command classes ----
    // Each returns true iff the operation succeeded. Unknown item id or an
    // illegal state transition returns false (commands can ignore or report).
    
    /**
     * @brief Assigns a specific work item to a participant.
     * @param[in] itemId The ID of the work item.
     * @param[in] participantName The name of the participant to assign.
     * @return true if the assignment was successful.
     * @return false if the item was not found or the state transition was illegal.
     */
    bool assignItem  (const std::string& itemId, const std::string& participantName);
    
    /**
     * @brief Starts execution of a specific work item.
     * @param[in] itemId The ID of the work item.
     * @return true if the item was successfully started.
     * @return false if the item was not found or the state transition was illegal.
     */
    bool startItem   (const std::string& itemId);
    
    /**
     * @brief Marks a specific work item as complete.
     * @param[in] itemId The ID of the work item.
     * @return true if the item was successfully completed.
     * @return false if the item was not found or the state transition was illegal.
     */
    bool completeItem(const std::string& itemId);
    
    /**
     * @brief Approves or rejects a specific work item.
     * @param[in] itemId The ID of the work item.
     * @param[in] approved true to approve, false to reject.
     * @param[in] comment The comment or reason accompanying the decision.
     * @return true if the action was successful.
     * @return false if the item was not found or the state transition was illegal.
     */
    bool approveItem (const std::string& itemId, bool approved, const std::string& comment);
    
    /**
     * @brief Rejects a specific work item.
     * @param[in] itemId The ID of the work item.
     * @param[in] reason The explanation for the rejection.
     * @return true if the rejection was successful.
     * @return false if the item was not found or the state transition was illegal.
     */
    bool rejectItem  (const std::string& itemId, const std::string& reason);
    
    /**
     * @brief Adds a new dynamically generated work item to a specific stage.
     * @param[in] stageId The ID of the parent stage to add the item to.
     * @param[in] itemId The unique ID for the new work item.
     * @param[in] itemTemplate The template containing metadata for the new work item.
     * @return true if the item was successfully added.
     * @return false if the stage is invalid, the ID is a duplicate, or the addition fails.
     */
    bool addWorkItem (const std::string& stageId, const std::string& itemId,
                      std::shared_ptr<WorkItem> itemTemplate);

    // ---- memento ----
    
    /**
     * @brief Creates a snapshot memento of the current workflow instance state.
     * @param[in] label A descriptive label for the checkpoint/snapshot.
     * @return std::shared_ptr<InstanceMemento> A shared pointer to the generated memento.
     */
    std::shared_ptr<InstanceMemento> createMemento(const std::string& label) const;
    
    /**
     * @brief Restores the workflow instance state from a given memento.
     * @param[in] memento Pointer to the memento to restore from.
     */
    void restoreFromMemento(InstanceMemento* memento);

    // ---- used by InstanceState subclasses ----
    
    /**
     * @brief Sets the next overall state of the workflow instance.
     * @param[in] next A unique pointer to the next InstanceState object.
     */
    void setState(std::unique_ptr<InstanceState> next);
    
    /**
     * @brief Sets the reason string for the current state.
     * @param[in] reason The reason string.
     */
    void setReason(const std::string& reason);
    
    /**
     * @brief Opens and activates the first stage of the workflow.
     */
    void openFirstStage();
    
    /**
     * @brief Checks if all stages in the workflow have finished processing.
     * @return true if all stages are complete.
     * @return false otherwise.
     */
    bool stagesFinished();

    /**
     * @brief Activates a specific stage or node.
     * @param[in] node Pointer to the node to activate.
     */
    void activateStage(WorkNode* node);
    
    /**
     * @brief Releases a stage, making its created work items available for assignment.
     * @param[in] stage Pointer to the stage to release.
     */
    void release(WorkNode* stage);

private:
    std::string id_;
    WorkflowDefinition definition_;
    std::unique_ptr<CompositeStage> root_;
    std::unique_ptr<InstanceState> instanceState_;
    std::string state_;
    std::string reason_;
    std::size_t currentStage_;
};

#endif // WORKFLOWINSTANCE_H