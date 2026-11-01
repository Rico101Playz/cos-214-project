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

class WorkflowFactory;
class LeafWorkItem;

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
    const std::string& state() const;
    const std::string& reason() const;
    const WorkflowDefinition& definition() const;
    const std::string& name() const;
    std::size_t currentStageIndex() const;
    bool adoptItem(const std::string& stageId, std::unique_ptr<LeafWorkItem> item);
    bool isActive(const WorkNode* node) const;
    std::size_t topLevelIndex(const WorkNode* node) const;

    bool requiresSignOff() const;
    void setRequiresSignOff(bool required);
    void signOff();
    CompositeStage& reopenForFixes(const WorkflowFactory& factory);

    void rebuild(const WorkflowDefinition& definition, const std::string& name,
                 std::unique_ptr<CompositeStage> root, const std::string& state,
                 std::size_t stageIndex, const std::string& reason, bool signOffRequired);

    CompositeStage& root() const;
    WorkNode* currentStage() const;
    WorkNode* find(const std::string& nodeId) const;
    WorkItem* item(const std::string& itemId) const;

    // ---- lifecycle (delegated to instanceState_) ----
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
                      std::shared_ptr<WorkItem> itemTemplate);

    // ---- memento ----
    std::shared_ptr<InstanceMemento> createMemento(const std::string& label) const;
    void restoreFromMemento(InstanceMemento* memento);

    // ---- used by InstanceState subclasses ----
    void setState(std::unique_ptr<InstanceState> next);
    void setReason(const std::string& reason);
    void openFirstStage();
    bool stagesFinished();

    void activateStage(WorkNode* node);
    void release(WorkNode* stage);

private:
    std::string id_;
    WorkflowDefinition definition_;
    std::unique_ptr<CompositeStage> root_;
    std::unique_ptr<InstanceState> instanceState_;
    std::string state_;
    std::string reason_;
    std::size_t currentStage_;
    std::string name_;
    bool requiresSignOff_;
};

#endif // WORKFLOWINSTANCE_H