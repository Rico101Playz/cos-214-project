# Task 3 activity diagram traceability

Each action and decision below maps to the supplied TIVIDY class model. This is a design mapping, not a claim that the C++ system is implemented. Proposed checks and scenario policies are marked explicitly. The VPP also links actions to the referenced classes through Visual Paradigm analysis mappings.

## 01 Generic workflow instance execution

| Node | Behaviour | Class operation or design responsibility |
| --- | --- | --- |
| E01 | E01 Select definition version and organisation | Tividy.definition(name, version); Tividy.organisation(name). Validate supplied payload and configuration before creation. |
| valid | Valid request? | WorkflowDefinition.findItem(); Organisation.withRole(); payload constraints in ItemTemplate. Structural and dependency validation is a proposed precondition of startWorkflow(). |
| E02 | E02 Return validation errors | Tividy.startWorkflow(): refuse invalid configuration; no WorkflowInstance is created. Explicit validation responsibility proposed for this facade. |
| E03 | E03 Instantiate an independent work tree and pin version | WorkflowDefinition.instantiate(); WorkflowInstance.setRoot(); PolicyFactory.createWorkItem(). |
| E04 | E04 Set payload and policies; start the instance | WorkflowInstance.setPayload(); setPolicies(); start(); Tividy.startWorkflow(). |
| E05 | E05 Advance stages and evaluate dependencies | WorkflowInstance.advance(); Stage.isActivated(); WorkNode.isUnblocked(); ReadyItemIterator. |
| cancel | Cancellation requested? | A caller issues Tividy.cancel() or an AutomationRule submits CancelCommand at this event-dispatch boundary. |
| E06 | E06 Cancel open work and record the reason | Tividy.cancel(); CancelCommand.perform(); WorkflowInstance.cancel(); Stage.cancel(); WorkItem.cancel(); audit(); publish(). |
| done | Root complete? | Stage.isComplete() checks all required child work; WorkflowInstance.advance() sets the final instance status. |
| E07 | E07 Record Completed and publish completion | WorkflowInstance.advance(); audit(); publish(); EventBus.publish(); MetricsCollector.onEvent(). |
| E08 | E08 Produce final progress and audit report | Tividy.report(); WorkflowInstance.report(); auditTrail(); MetricsCollector.summary(). |
| ready | Eligible item ready? | WorkflowInstance.readyWork(); ReadyItemIterator skips blocked work. Diagram 02 returns rejected outcomes to the caller for its configured rework route. |
| E09 | E09 Fulfil selected work item | Call diagram 02 once for the selected item. CompleteCommand, DecideCommand, RejectCommand or CancelCommand changes its runtime state. |
| live | Live work or trigger can progress? | WorkflowInstance.items(); pendingApprovals(); WorkItem.isActive(); configured timers/triggers. Deadlock detection is a proposed scheduling precondition of advance(). |
| E10 | E10 Cancel stalled instance with dependency diagnosis | WorkNode.isUnblocked(); WorkflowInstance.cancel(reason); audit(); publish(). Detection of a cycle or unreachable dependency is a proposed validation/scheduling check. |
| E11 | E11 Wait for item, approval, timer or external event | WorkflowInstance.pendingApprovals(); Tividy.advanceTime(); externalEvent(). The engine resumes at event boundaries rather than busy polling. |
| recover | Permitted rollback requested? | SnapshotStore.has(); approval of recovery is a facade precondition. Restore only a valid in-memory checkpoint after any required external compensation. |
| E12 | E12 Restore checkpoint; record recovery | Tividy.rollback(); SnapshotStore.get(); WorkflowInstance.restore(); WorkItem.restoreSnapshot(); ApprovalRequest.restoreSnapshot(). |
| E13 | E13 Dispatch command, external event or elapsed time | Tividy.run(); externalEvent(); advanceTime(); WorkflowInstance.tick(); EventBus.publish(); AutomationRules.onEvent(). |

## 02 Work assignment completion and approval

| Node | Behaviour | Class operation or design responsibility |
| --- | --- | --- |
| A01 | A01 Check current state and dependencies | WorkItem.stateName(); WorkNode.isUnblocked(); WorkItemState rejects invalid operations. |
| eligible | Eligible for assignment? | AvailableState or RejectedState accepts assignment only after dependencies are satisfied. |
| A02 | A02 Refuse operation and return not eligible | WorkItemState.invalid(); preserve the current state and report why the command was refused. |
| A03 | A03 Choose eligible assignee using configured strategy | WorkflowInstance.chooseAssignee(); AssignmentStrategy.choose(); Organisation.withRole(); Participant.load(). |
| candidate | Assignee available? | AssignmentStrategy.choose() returns a Participant or no matching candidate. |
| A04 | A04 Wait for role availability or cancellation | Organisation.addParticipant(); Tividy.advanceTime(); cancel(). Availability is an input event in the in-memory demonstration. |
| A05 | A05 Assign item and notify the participant | Tividy.assign(); AssignCommand.perform(); WorkItem.assign(); Participant.addLoad(); NotificationService.onEvent(). |
| A06 | A06 Start assigned item | Tividy.start(); StartCommand.perform(); AssignedState.start(); Task.markStarted(). |
| A07 | A07 Perform work or rework outside the engine | Participant performs scenario work; TIVIDY coordinates it. WorkItem remains InProgress. |
| A08 | A08 Submit data and request completion | Tividy.setData(); SetDataCommand.perform(); Tividy.complete(). |
| data | Required data valid? | ValidatedWork.missingKeys(); ItemTemplate.needs(); ValidatedWork.complete(). |
| A09 | A09 Explain missing data; leave item InProgress | ValidatedWork.complete(); missingKeys(); refuse completion without transitioning to Completed. |
| A10 | A10 Execute completion and record transition | CompleteCommand.perform(); WorkItem.complete(); InProgressState.complete(); AuditedWork.complete(). |
| approval | Approval required? | WorkItem.requiresApproval(); configured approval policy. |
| A11 | A11 Open request with configured approval chain | WorkflowInstance.openApproval(); approvalFor(); PolicyFactory.createApprovalChain(); AwaitingApprovalState. |
| A12 | A12 Evaluate auto approval and required role decisions | ApprovalHandler.handle(); AutoApproveHandler.handle(); RoleApprovalHandler.handle(); DeclaredApproverHandler.requiredRole(). |
| resolved | Request resolved? | ApprovalRequest.isResolved(); decisionFor(role); pendingRole(). |
| approved | Approved? | ApprovalRequest.resolve(approved, comment); final recorded approval outcome. |
| A13 | A13 Reject item; record review feedback | Tividy.reject(); RejectCommand.perform(); WorkItem.reject(); RejectedState. |
| A14 | A14 Notify required role; await decision or clock event | ApprovalRequest.waitFor(); pendingRole(); NotificationService.onEvent(). A response or elapsed-time input resumes processing. |
| signal | Received input? | One input event is handled at a time: approved/declined decision, timer or cancellation. |
| A15 | A15 Advance request waiting time from clock event | Tividy.advanceTime(); WorkflowInstance.tick(); ApprovalRequest.addWait(); waiting(). |
| deadline | Deadline passed and not yet escalated? | ApprovalRequest.waiting(); WorkItem.dueHours(); ApprovalRequest.escalated(). |
| A16 | A16 Escalate to configured role and publish notification | TimeoutEscalationHandler.handle(); ApprovalRequest.escalateTo(); WorkItem.escalate(); NotificationService.onEvent(). For approval requests the pending role changes; completion still requires a decision. |
| authorised | Actor may decide? | ApprovalRequest.canDecide(participant); Participant.role(); reject unauthorised input. |
| A17 | A17 Refuse unauthorised decision | ApprovalRequest.canDecide() is a precondition of Tividy.decide(); no decision is recorded. |
| A18 | A18 Record approve or decline and reevaluate the chain | Tividy.decide(); DecideCommand.perform(); ApprovalRequest.record(); ApprovalHandler.handle(). |
| A19 | A19 Mark item Completed; release workload and publish | AwaitingApprovalState.complete() after approval, or InProgressState.complete() without approval; Task.markCompleted(); syncLoad(); WorkflowInstance.publish(). |
| A20 | A20 Advance dependencies and return item outcome | WorkflowInstance.advance(); Stage.isComplete(); caller resumes diagram 01 or 03. |
| A21 | A21 Return Rejected outcome to caller for process routing | WorkItem.stateName() returns Rejected. The caller chooses a fresh scenario rework iteration, reassigns this item using RejectedState.assign(), or explicitly cancels the instance. |
| A22 | A22 Cancel pending item and approval wait | Tividy.cancel(); WorkItem.cancel(); WorkflowInstance.pendingApprovals() must exclude final/cancelled items. Suppress late decisions after cancellation. |

## 03 Software feature delivery

| Node | Behaviour | Class operation or design responsibility |
| --- | --- | --- |
| F01 | F01 Receive approved feature request and create instance | FeatureDeliveryProcess.construct(WorkflowBuilder&); DefinitionBuilder.result(); Tividy.startWorkflow("Feature Delivery", ...); WorkflowDefinition.instantiate(). |
| F02 | F02 Product Owner records criteria, estimate, risk and priority | Planning WorkItem; Tividy.setData(); SetDataCommand; payload fields risk and acceptance criteria. |
| criteria | Criteria sufficiently clear? | ValidatedWork.missingKeys(); Product Owner validates business criteria; TIVIDY validates required fields. |
| F03 | F03 Clarify requirements with the requester | Participant performs clarification; Tividy.setData(); planning item remains InProgress. |
| F04 | F04 Agree and record API contract before implementation | WorkItem stores apiContract via setData(); ItemTemplate.after() makes frontend dependent on contract completion. |
| F05 | F05 Save implementation checkpoint | Tividy.checkpoint(); WorkflowInstance.createMemento(); SnapshotStore.save(). |
| F06 | F06 Implement backend and unit tests | Developer fulfils backend WorkItem using diagram 02; business coding and tests occur outside TIVIDY. |
| F07 | F07 Implement frontend and unit tests | Developer fulfils frontend WorkItem after agreed API contract; WorkNode.addDependency(); diagram 02. |
| F08 | F08 Write feature and release documentation | Documentation WorkItem is fulfilled via diagram 02; required by release dependencies. |
| F09 | F09 Open pull request; wait for CI result event | GitHostingConnector.createBranch(); openPullRequest(); Tividy.externalEvent(); WorkflowEvent.value(). Simulated CI emits build result. |
| ci | Build and tests pass? | WorkflowEvent.value("buildStatus"); AutomationRule.when evaluates the configured CI result. |
| F10 | F10 Record build defects and reopen implementation work | AutomationRules.onEvent(); Tividy.addWork() creates a rework iteration; Tividy.reject() on the active verification item. A fresh PR/CI check follows rework. |
| F11 | F11 Obtain first code approval from a Reviewer | Review WorkItem with ItemTemplate.approvedBy("Reviewer"); GitHostingConnector.requestReview(); diagram 02 handles decision and timeout. |
| review1 | First review approved? | Completed review WorkItem and approved ApprovalRequest; rejected outcome requires rework. |
| F12 | F12 Record review feedback and request changes | Tividy.reject(); RejectCommand; Tividy.addWork() creates the configured rework iteration. |
| risk1 | High-risk feature? | Configured RuleBasedAssignment / AutomationRule evaluates instance risk payload. Risk is fixed for the current review cycle. |
| F13 | F13 Obtain independent second Reviewer approval | A second distinct review WorkItem uses approvedBy("Reviewer"); Organisation supplies a different Reviewer participant. Assignment must exclude the first reviewer as a scenario policy precondition. |
| review2 | Second review approved? | Second review WorkItem must be Completed with an approved request; first approval alone is insufficient for high risk. |
| F14 | F14 Capture second-review feedback and schedule rework | Tividy.reject(); addWork(); preserve both review outcomes in audit history. |
| F15 | F15 QA verifies acceptance criteria and regression tests | QA Tester fulfils QA WorkItem via diagram 02; dependency is implementation plus required code approvals. |
| risk2 | Security review required? | High-risk guard uses the same scenario risk payload as the second-review guard. |
| F16 | F16 Tech Lead performs security review | Conditional security-review WorkItem assigned to Tech Lead; Tividy.addWork(); ItemTemplate.approvedBy("Tech Lead"); diagram 02. |
| F17 | F17 Record security review not required for normal risk | Configured scenario branch records notRequired in payload/audit. No high-risk security task is created. |
| checks | QA and required security checks pass? | QA result payload and conditional security WorkItem result; fork branches both reach the join with recorded pass/fail results. A failed check does not abandon its token. |
| F18 | F18 Record defects; add rework and repeat build and reviews | Tividy.addWork(); AddWorkCommand; configured AutomationRule supplies a new iteration with dependencies rather than resetting a CompletedState item. |
| F19 | F19 Obtain Tech Lead release approval | Tech Lead approval WorkItem after passed QA and completed documentation; ItemTemplate.approvedBy("Tech Lead"); diagram 02. |
| lead | Tech Lead approves? | ApprovalRequest result for Tech Lead gate; a cancelled called activity also takes the unsuccessful path. |
| F20 | F20 Obtain Release Manager approval and release schedule | Separate sequential Release Manager WorkItem; ItemTemplate.approvedBy("Release Manager"); dueHours(); diagram 02. |
| release | Release Manager approves? | ApprovalRequest result for Release Manager gate; both release gates must succeed. |
| F21 | F21 Record release rejection and Product Owner disposition | Tividy.reject(); payload stores whether Product Owner requests rework or cancellation; audit preserves reason. |
| disposition | Rework authorised? | Configured process decision from Product Owner; rejected release may loop or end Cancelled. |
| F22 | F22 Save approved-release checkpoint | Tividy.checkpoint(); WorkflowInstance.createMemento(); SnapshotStore.save(). Checkpoint covers engine state only. |
| F23 | F23 Merge approved pull request through Git connector | GitHostingConnector.mergePullRequest(); outcome reported through Tividy.externalEvent(). External operations are simulated. |
| merged | Merge succeeded? | Simulated connector result or external event; failure distinguishes service outage from merge conflict. |
| transient | Transient service failure? | Connector outcome classifies unavailable service versus conflict; retry policy is scenario configuration. |
| F24 | F24 Wait for service recovery and retry within retry limit | Tividy.advanceTime(); externalEvent(); AutomationRule with configurable bounded retry count. Exhaustion routes to cancellation. |
| retry | Retry still permitted? | Scenario payload retryCount and configured retryLimit; prevents an endless outage retry loop. |
| F25 | F25 Resolve merge conflict; require fresh build and reviews | Developer performs conflict resolution externally; Tividy.addWork() creates rework iteration; old approvals cannot authorise changed code. |
| F26 | F26 Release Manager deploys approved feature | Release Manager performs simulated deployment outside TIVIDY; deployment WorkItem and Tividy.externalEvent() record outcome. No deployment connector is present in supplied class model. |
| deployed | Deployment succeeds? | Recorded deployment result event; unsuccessful instance is Cancelled with a reason per the chosen scenario. |
| F27 | F27 Notify requester and team; confirm released documentation | NotificationService.onEvent(); send(); documentation WorkItem already completed; requester receives result. |
| F28 | F28 Close request Completed; retain audit and cycle-time report | WorkflowInstance.advance(); Stage.isComplete(); EventBus.publish(); MetricsCollector.cycleTime(); Tividy.report(). |
| F29 | F29 Cancel open feature work; record unsuccessful outcome | Tividy.cancel(instanceId, rootId, reason,...); WorkflowInstance.cancel(); Stage.cancel(); audit(); publish(); NotificationService.onEvent(). External compensation, if needed, is separate business work. |
