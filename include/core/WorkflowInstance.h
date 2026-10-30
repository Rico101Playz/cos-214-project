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

class WorkflowInstance {
public:
    static const char* const CREATED;
    static const char* const RUNNING;
    static const char* const COMPLETED;
    static const char* const CANCELLED;
    static const char* const AWAITING_SIGNOFF;

    WorkflowInstance(const std::string& id, const WorkflowDefinition& definition);
    WorkflowInstance(const std::string& id, const WorkflowDefinition& definition,
                     const WorkflowFactory& factory, const std::string& title);

    WorkflowInstance(const WorkflowInstance&) = delete;
    WorkflowInstance& operator=(const WorkflowInstance&) = delete;

    // ---- accessors ----
    const std::string& id() const;
    const std::string& title() const;
    // Created until start(), Cancelled once cancelled; when every task is
    // complete, AwaitingSignOff if the project needs sign-off and has not had
    // it, Completed otherwise; Running in between.
    std::string state() const;
    const std::string& reason() const;
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

    CompositeTask& root() const;
    Task* currentStage() const;
    Task* find(const std::string& nodeId) const;
    LeafTask* item(const std::string& itemId) const;

    // ---- lifecycle ----
    // start(), cancel(), signOff() and reopening throw std::logic_error when the
    // project's status does not allow them; advance() does nothing unless it
    // is under way.
    void start();
    void advance();
    void cancel(const std::string& reason);

    // ---- per-item operations, used by the Command classes ----
    // Each returns true iff the operation succeeded. Unknown item id or an
    // illegal state transition returns false (commands can ignore or report).
    bool assignItem  (const std::string& itemId, const std::string& participantName);
    bool startItem   (const std::string& itemId);
    bool completeItem(const std::string& itemId);
    bool approveItem (const std::string& itemId, bool approved, const std::string& comment);
    bool rejectItem  (const std::string& itemId, const std::string& reason);
    bool addWorkItem (const std::string& stageId, const std::string& itemId,
                      std::shared_ptr<LeafTask> itemTemplate);

    // ---- memento ----
    std::shared_ptr<Memento> createMemento(const std::string& label) const;
    void restoreFromMemento(Memento* memento);

    void setReason(const std::string& reason);
    // Sets the started, cancelled and signed-off flags that give this status.
    void setStatus(const std::string& state);
    void openFirstStage();
    bool stagesFinished();

    void activateStage(Task* node);
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