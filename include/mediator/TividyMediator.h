#ifndef TIVIDY_MEDIATOR_H
#define TIVIDY_MEDIATOR_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "org_composite/Company.h"
#include "core/WorkflowDefinition.h"
#include "core/WorkflowInstance.h"
#include "abstract_factory/WorkflowFactory.h"
#include "strategy/AssignmentService.h"
#include "command/CommandInvoker.h"
#include "memento/EngineMemento.h"
#include "command/UndoHistory.h"
#include "memento/Memento.h"
#include "observer/EventBus.h"
#include "mediator/Mediator.h"
#include "facade/HistoryFacade.h"
#include "facade/OrgFacade.h"
#include "facade/ProjectFacade.h"

class LeafTask;


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
class TividyMediator : public Mediator {
public:
    class ActorScope {
    private:
        TividyMediator& engine_;
        std::string previous_;

    public:
        ActorScope(TividyMediator& engine, const std::string& actor);
        ~ActorScope();
    };

private:
    typedef ProjectFacade::Runtime Runtime;

    class Step {
    private:
        TividyMediator& engine_;
        bool outer_;
        bool committed_;
        std::string before_;
        std::string label_;

    public:
        Step(TividyMediator& engine, const std::string& label);
        ~Step();
        void commit();
    };

    ProjectFacade projectFacade_;
    OrgFacade orgFacade_;
    HistoryFacade historyFacade_;
    CommandInvoker invoker_;        // runs every change as a command; HistoryFacade keeps the CommandHistory
    EventBus bus_;
    int depth_;
    std::string actor_;
    mutable std::vector<WorkflowEvent> pendingEvents_;


    // Runs the Request as a RequestCommand bound to ProjectFacade and returns its answer.
    Response record(const Request& request);

    // ownerRole: the role the owner approves as, or "" when the company has none.
    std::unique_ptr<Runtime> makeRuntime(const std::string& policy, bool structured,
                                         const std::string& ownerRole);
    void publish(const std::string& type, const std::string& instanceId,
                 const std::string& itemId = "", const std::string& detail = "") const;
    void recomputeLoads();
    void routeApproval(const std::string& instanceId, const std::string& itemId);
    void finishApproval(const std::string& instanceId, const std::string& itemId);
    void forceApprove(const std::string& instanceId, const std::string& itemId);
    void assignPending(const std::string& instanceId);
    void assignThroughPipeline(const std::string& instanceId, const std::string& itemId,
                               const std::string& requested, bool mustSucceed);
    std::string launch(const WorkflowDefinition& definition, const std::string& title,
                       const std::string& policy, SignOffRule signOff);
    std::string label(const std::string& verb, const std::string& instanceId,
                      const std::string& itemId = "") const;

public:
    explicit TividyMediator(const std::string& organisationName);

    TividyMediator(const TividyMediator&) = delete;
    TividyMediator& operator=(const TividyMediator&) = delete;

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

    std::shared_ptr<const EngineMemento> createMemento(const std::string& label) const override;
    void restoreMemento(const EngineMemento& memento) override;
    // ProjectFacade reports a change; when it moved work on or off someone's
    // desk ("loads"), the people's loads are worked out again.
    void notify(Colleague* sender, const Response& change) override;

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

#endif // TIVIDY_MEDIATOR_H
