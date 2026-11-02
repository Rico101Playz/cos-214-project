#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <cstddef>
#include <memory>

#include "core/WorkflowDefinition.h"
#include "composite/LeafTask.h"
#include "memento/Memento.h"
#include "composite/CompositeTask.h"

class WorkflowFactory;
class LeafTask;

/**
 * @class WorkflowInstance
 * @brief Represents a running instance of a workflow definition.
 *
 * Manages the execution state, the hierarchical tree of work items,
 * and handles lifecycle and item operations.
 */
class WorkflowInstance {
public:
    static const char* const CREATED;   /*!< String constant representing the Created state. */
    static const char* const RUNNING;   /*!< String constant representing the Running state. */
    static const char* const COMPLETED; /*!< String constant representing the Completed state. */
    static const char* const CANCELLED; /*!< String constant representing the Cancelled state. */
    static const char* const AWAITING_SIGNOFF;

    /**
     * @brief Constructs a new Workflow Instance from a given definition.
     * @param[in] id The unique identifier for this workflow instance.
     * @param[in] definition The WorkflowDefinition blueprint to build this instance from.
     */
    WorkflowInstance(const std::string& id, const WorkflowDefinition& definition);
    WorkflowInstance(const std::string& id, const WorkflowDefinition& definition,
                     const WorkflowFactory& factory, const std::string& title);

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
    // Created until start(), Cancelled once cancelled; when every task is
    // complete, AwaitingSignOff if the project needs sign-off and has not had
    // it, Completed otherwise; Running in between.
    /**
     * @brief Gets the current overall state of the workflow instance.
     * @return std::string The current state.
     */
    std::string state() const;

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
    const std::string& name() const;
    std::size_t currentStageIndex() const;
    bool adoptItem(const std::string& stageId, std::unique_ptr<Task> item);
    bool isActive(const Task* node) const;
    std::size_t topLevelIndex(const Task* node) const;

    bool requiresSignOff() const;
    void setRequiresSignOff(bool required);
    void signOff();
    CompositeTask& reopenForFixes(const WorkflowFactory& factory);

    void rebuild(const WorkflowDefinition& definition, const std::string& name,
                 std::unique_ptr<CompositeTask> root, const std::string& state,
                 std::size_t stageIndex, const std::string& reason, bool signOffRequired);

    /**
     * @brief Gets the root composite stage containing all work nodes.
     * @return CompositeTask& Reference to the root stage.
     */
    CompositeTask& root() const;

    /**
     * @brief Gets the currently active stage of the workflow.
     * @return Task* Pointer to the current stage node, or nullptr if not running.
     */
    Task* currentStage() const;

    /**
     * @brief Finds a specific node (stage or item) within the workflow tree by its ID.
     * @param[in] nodeId The ID of the node to find.
     * @return Task* Pointer to the found node, or nullptr if not found.
     */
    Task* find(const std::string& nodeId) const;

    /**
     * @brief Finds a specific work item within the workflow tree by its ID.
     * @param[in] itemId The ID of the work item to find.
     * @return LeafTask* Pointer to the found work item, or nullptr if not found.
     */
    LeafTask* item(const std::string& itemId) const;

    // ---- lifecycle ----
    // start(), cancel(), signOff() and reopening throw std::logic_error when the
    // project's status does not allow them; advance() does nothing unless it
    // is under way.

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
                      std::shared_ptr<LeafTask> itemTemplate);

    // ---- memento ----

    /**
     * @brief Creates a snapshot memento of the current workflow instance state.
     * @param[in] label A descriptive label for the checkpoint/snapshot.
     * @return std::shared_ptr<Memento> A shared pointer to the generated memento.
     */
    std::shared_ptr<Memento> createMemento(const std::string& label) const;

    /**
     * @brief Restores the workflow instance state from a given memento.
     * @param[in] memento Pointer to the memento to restore from.
     */
    void restoreFromMemento(Memento* memento);

    /**
     * @brief Sets the reason string for the current state.
     * @param[in] reason The reason string.
     */
    void setReason(const std::string& reason);
    // Sets the started, cancelled and signed-off flags that give this status.
    void setStatus(const std::string& state);

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
    void activateStage(Task* node);

    /**
     * @brief Releases a stage, making its created work items available for assignment.
     * @param[in] stage Pointer to the stage to release.
     */
    void release(Task* stage);

private:
    std::string id_;
    WorkflowDefinition definition_;
    std::unique_ptr<CompositeTask> root_;
    bool started_;
    bool cancelled_;
    bool signedOff_;
    std::string reason_;
    std::size_t currentStage_;
    std::string name_;
    bool requiresSignOff_;
};

#endif // WORKFLOWINSTANCE_H