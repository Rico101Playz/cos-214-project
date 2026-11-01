#ifndef TIVIDY_H
#define TIVIDY_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "core/Company.h"
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "patterns/creation/WorkflowFactory.h"
#include "patterns/execution/AssignmentService.h"
#include "patterns/execution/CommandInvoker.h"
#include "patterns/execution/EngineMemento.h"
#include "patterns/execution/UndoHistory.h"
#include "patterns/execution/Memento.h"
#include "patterns/execution/EventBus.h"
#include "patterns/structural/TividyService.h"

class LeafWorkItem;

struct ItemRef {
    std::string instanceId;
    std::string itemId;

    ItemRef(const std::string& instance, const std::string& item)
        : instanceId(instance), itemId(item) {}
};

// FACADE pattern - the FACADE.
//
// The single entry point to the engine. Clients load definitions, start
// instances and drive work items by id here, without touching the builders,
// work tree, iterators or organisation behind it.
//
//
// Every call that names something unknown (definition, instance, item or
// participant) throws std::invalid_argument; a call the item's or instance's
// current state does not allow throws std::logic_error. Nothing changes when
// a call throws.
class Tividy : public TividyService {
public:
    class ActorScope {
    private:
        Tividy& engine_;
        std::string previous_;

    public:
        ActorScope(Tividy& engine, const std::string& actor);
        ~ActorScope();
    };

private:
    struct Runtime {
        std::unique_ptr<WorkflowFactory> factory;
        std::unique_ptr<AssignmentService> assignment;
        std::unique_ptr<ApprovalHandler> approvals;
        bool structured;
    };

    class Step {
    private:
        Tividy& engine_;
        bool outer_;
        bool committed_;
        std::string before_;
        std::string label_;

    public:
        Step(Tividy& engine, const std::string& label);
        ~Step();
        void commit();
    };

    Company organisation_;
    std::map<std::string, WorkflowDefinition> definitions_;
    std::map<std::string, std::unique_ptr<WorkflowInstance> > instances_;
    std::map<std::string, std::unique_ptr<Runtime> > runtimes_;
    std::vector<std::string> instanceOrder_;
    CommandInvoker invoker_;
    EventBus bus_;
    EventBus historyBus_;
    std::unique_ptr<UndoHistory> ownHistory_;
    UndoHistory* history_;
    int nextInstance_;
    int nextTask_;
    int depth_;
    std::string actor_;
    mutable std::vector<WorkflowEvent> pendingEvents_;


    void record(const std::string& instanceId, WorkCommand* command);

    WorkflowInstance& findInstance(const std::string& instanceId) const;
    WorkItem& findItem(const std::string& instanceId, const std::string& itemId) const;
    LeafWorkItem& findLeaf(const std::string& instanceId, const std::string& itemId) const;
    Participant& findParticipant(const std::string& name) const;
    Runtime& runtime(const std::string& instanceId) const;
    std::unique_ptr<Runtime> makeRuntime(const std::string& policy, bool structured,
                                         const Organisation& people);
    void publish(const std::string& type, const std::string& instanceId,
                 const std::string& itemId = "", const std::string& detail = "") const;
    void recomputeLoads();
    void routeApproval(const std::string& instanceId, const std::string& itemId);
    void finishApproval(const std::string& instanceId, const std::string& itemId);
    void forceApprove(const std::string& instanceId, const std::string& itemId);
    void assignPending(const std::string& instanceId);
    void assignThroughPipeline(const std::string& instanceId, const std::string& itemId,
                               const std::string& requested, bool mustSucceed);
    std::string nextTaskId(const WorkflowInstance& instance);
    bool dependsOn(const LeafWorkItem& from, const LeafWorkItem& target) const;
    std::string launch(const WorkflowDefinition& definition, const std::string& title,
                       const std::string& policy, SignOffRule signOff);
    std::string label(const std::string& verb, const std::string& instanceId,
                      const std::string& itemId = "") const;

public:
    explicit Tividy(const std::string& organisationName);

    Tividy(const Tividy&) = delete;
    Tividy& operator=(const Tividy&) = delete;

    // The people who can be assigned work.
    Company& organisation();
    const Company& organisation() const;

    // Builds the Feature Delivery process with the Builder pattern and stores
    // it under the given name, replacing any definition with that name.
    void loadDefinition(const std::string& name);
    // Stores a definition supplied by the caller under the given name.
    void loadDefinition(const std::string& name, const WorkflowDefinition& definition);
    bool hasDefinition(const std::string& name) const;
    std::vector<std::string> definitionNames() const;
    const WorkflowDefinition& definition(const std::string& name) const;

    // Starts a new running instance of a stored definition and returns its id.
    std::string startInstance(const std::string& definition);

    std::string startProject(const std::string& definition, const std::string& title,
                             const std::string& policy,
                             SignOffRule signOff = SignOffRule::FromTemplate) override;
    std::string addTask(const std::string& instanceId, const TaskSpec& spec) override;
    std::string addSubtask(const std::string& instanceId, const std::string& parentItemId,
                           const TaskSpec& spec) override;

    // Work item operations. assign() names a participant in the organisation
    // and adds to their load; the load is released again when the work is
    // approved, rejected or reassigned. approve() also moves the instance on
    // to its next stage once the current one is finished.
    void assign(const std::string& instanceId, const std::string& itemId,
                const std::string& participant) override;
    void autoAssign(const std::string& instanceId, const std::string& itemId) override;
    void start(const std::string& instanceId, const std::string& itemId) override;
    void complete(const std::string& instanceId, const std::string& itemId) override;
    void withdraw(const std::string& instanceId, const std::string& itemId) override;
    void approve(const std::string& instanceId, const std::string& itemId) override;
    void approveAs(const std::string& instanceId, const std::string& itemId,
                   const std::string& person);
    void reject(const std::string& instanceId, const std::string& itemId,
                const std::string& reason = "") override;
    void escalate(const std::string& instanceId, const std::string& itemId) override;
    void setPriority(const std::string& instanceId, const std::string& itemId,
                     const std::string& priority) override;

    // Cancels all unfinished work in the instance and releases the load of
    // everyone still working on it.
    void cancel(const std::string& instanceId, const std::string& reason = "") override;

    void signOff(const std::string& instanceId) override;
    std::vector<std::string> flagIssue(const std::string& instanceId, const IssueSpec& spec) override;

    void undo() override;
    void redo() override;
    void undoFor(const std::string& actor, bool anyone);
    void redoFor(const std::string& actor, bool anyone);
    bool canUndo(const std::string& actor = "", bool anyone = true) const;
    bool canRedo(const std::string& actor = "", bool anyone = true) const;
    std::vector<std::string> undoLabels(std::size_t max = 10) const;
    std::vector<std::string> redoLabels(std::size_t max = 10) const;
    UndoHistory* history() const;
    void useHistory(UndoHistory* history);

    std::shared_ptr<const EngineMemento> createMemento(const std::string& label) const;
    void restoreMemento(const EngineMemento& memento);

    void addUnit(const std::string& parentId, const std::string& id,
                 const std::string& name) override;
    void addPerson(const std::string& name, const std::string& role,
                   const std::string& unitId) override;
    void moveMember(const std::string& unitId, const std::string& name) override;

    std::string snapshot() const;
    void restore(const std::string& text);
    void clearHistory();

    // Read-only view of a running instance.
    const WorkflowInstance& instance(const std::string& instanceId) const;
    std::vector<std::string> instanceIds() const;
    std::string policyOf(const std::string& instanceId) const;
    // Ids of the items in the instance that can be picked up right now.
    std::vector<std::string> readyItems(const std::string& instanceId) const;
    // The share of the instance's work items that are complete, 0.0 .. 1.0.
    double progress(const std::string& instanceId) const;

    std::vector<ItemRef> workFor(const std::string& participant) const;
    std::vector<ItemRef> approvalsFor(const std::string& participant) const;
    std::vector<std::string> signOffsFor(const std::string& participant) const;
    bool canApprove(const std::string& participant, const std::string& instanceId,
                    const std::string& itemId) const;
    bool canSignOff(const std::string& participant, const std::string& instanceId) const;

    // OBSERVER pattern: attach observers here to hear about everything the
    // engine does.
    EventBus& bus();
    EventBus& historyBus();
};

#endif // TIVIDY_H
