# TIVIDY Task 3 activity diagrams

This folder contains the three UML Activity Diagrams required by Section 7 of the COS 214 Practical 6 specification. The diagrams model generic TIVIDY behaviour and the selected Software Feature Delivery scenario. They describe the proposed system; Practical 6 does not require its implementation.

## Open the Visual Paradigm project

Open **TIVIDY_Task3.vpp** directly in Visual Paradigm 18.1 or later. The project contains editable UML actions, control flows, decisions, merges, forks, joins, activity partitions and called activities. No custom plugin is required to open or edit it.

In the Diagram Navigator, open these three activity diagrams:

1. **01 Generic workflow instance execution**: select a published definition version, create an independent instance, evaluate dependencies, fulfil eligible work, handle events and permitted recovery, and reach completion or cancellation. Invalid creation is refused before an instance exists; a stalled instance is cancelled with a recorded diagnosis.
2. **02 Work assignment completion and approval**: assign work by configured policy, validate submitted data, complete work, obtain automatic or human approval, refuse unauthorised decisions, escalate an overdue request, and return Completed, Rejected, Cancelled or not-eligible outcomes to the caller. Responsibility swimlanes distinguish workers, the engine, and approvers.
3. **03 Software feature delivery**: intake and planning; parallel backend, frontend and documentation; CI and required code approvals; parallel QA and conditional security review; sequential Tech Lead and Release Manager approvals; merge, deployment and close-out. Failure paths show rework, bounded service retries and cancellation.

The project also retains **Reference - supplied TIVIDY class diagram** from the supplied XML. It is supporting context, not a fourth Task 3 activity diagram. Activity actions have class analysis mappings; select an action and inspect its documentation for the corresponding operations. [TRACEABILITY.md](TRACEABILITY.md) provides the complete action and decision mapping.

**TIVIDY_Task3_XML.zip** contains Visual Paradigm's native XML export for interchange and recovery. Extract the complete archive and import its **project.xml** if recovery is needed; keep the accompanying data.zip and images together. The VPP file opens directly without this import. The matching PNG and SVG files in [exports](exports/) are exported from the native diagrams for review and inclusion in the combined submission PDF. Their tall layouts preserve readable detail when zoomed; avoid shrinking a whole diagram onto an A4 page with unreadable text.

## Task 3 requirement coverage

| Specification requirement | Where it appears |
| --- | --- |
| Generic instance creation through a final outcome | Diagram 01: E01-E08 and the cancellation/stall outcomes |
| Another substantial generic process | Diagram 02: assignment, validation, approval, rejection and escalation |
| Important process from the selected scenario | Diagram 03: the five Software Feature Delivery stages |
| Actions, decisions and guards | All three diagrams; alternative decision exits have explicit guards |
| Merges and loops | Diagram 01's scheduling loop; Diagram 02's assignment, validation and approval waits; Diagram 03's clarification, rework and bounded retry loops |
| Fork and join | Diagram 03: backend/frontend/documentation fork and join, and QA/security fork and join |
| Called or composite activity | Diagram 01 E09 and Diagram 03's work-fulfilment actions synchronously call the activity defined by Diagram 02 |
| Swimlanes | Diagram 02: Worker or requester, TIVIDY engine and policies, Approver and escalation role |
| Traceability to class design | Action documentation and class analysis mappings in the VPP; all important actions and decisions in TRACEABILITY.md |

Guards shown in brackets belong to outgoing control flows. A diamond with several incoming flows is a merge of alternatives, while a black join bar waits for every parallel branch. In Diagram 03, normal-risk work explicitly follows the security-not-required action and merge, so it still supplies the token required by the verification join. A called activity's final node returns its outcome to its caller; it does not terminate every other invocation of that activity.

## Design decisions and proposed responsibilities

These decisions reconcile the scenario and supplied class model. They should be carried forward when Tasks 4-7 refine the design.

| Question | Decision and reason | Class design consequence |
| --- | --- | --- |
| The scenario describes review and QA as parallel but also says QA depends on code review | Follow the explicit dependency: finish all required code approvals before starting QA. QA and security review may run in parallel. | Configure WorkNode dependencies and Stage execution modes accordingly. |
| Frontend depends on an agreed backend API contract | Agree the contract before the implementation fork. Backend and frontend implementation can then proceed independently. | Represent contract completion as a dependency, rather than allowing frontend work to start with an unresolved prerequisite. |
| A high-risk feature requires two Reviewer approvals | Use two distinct review work items, each with its own ApprovalRequest. Select a different Reviewer for the second item. Disable low-risk auto-approval for these gates. | ApprovalRequest.decisionFor(role) alone does not count two people in one role. Excluding the first reviewer is a proposed scenario constraint on RuleBasedAssignment; the supplied interface does not expose an exclusion parameter. |
| No Security Reviewer role is listed | The Tech Lead performs the conditional security review as a separate work item. | This is a proposed scenario role assignment, not a new engine class. |
| Rework occurs after some original items are already Completed | Create a fresh iteration with fresh item identifiers and approval gates; never reopen CompletedState directly. Retain earlier results in the audit trail. | Stage.remove(), Tividy.addWork(), AddWorkCommand and dependency construction can replace superseded active-tree nodes. A safe operation that replaces an iteration and reconnects downstream dependencies is a proposed orchestration responsibility; addWork() alone does not establish all of this behaviour. |
| Rejected approval must route back to the process | Diagram 02 returns Rejected; the caller selects rework or cancellation. | RejectedState.assign()/start() can support item rework; scenario-wide rework uses the fresh iteration above. |
| A request times out | Escalate once to its configured escalation role, notify that role, and continue waiting for an authorised decision or cancellation. | Use TimeoutEscalationHandler, ApprovalRequest.escalateTo(), waiting()/escalated(), and NotificationService. Escalation does not automatically approve work. |
| Workflow work is incomplete but nothing is ready | Wait only if live work or a configured trigger can still progress; otherwise cancel with a diagnosis. | Definition validation, dependency-cycle detection and this scheduler liveness check are proposed preconditions/responsibilities of the Tividy facade and WorkflowInstance.advance(). The supplied model has no dedicated validator or deadlock-detector class. |
| Definitions are edited while instances run | Each instance keeps its starting definition version and its own runtime work tree. | WorkflowDefinition.instantiate() creates independent work; changes to a definition do not mutate existing instances. |
| A checkpoint is restored after an external action | Restore only engine state after any required business compensation; do not treat it as undoing a Git merge or deployment. | SnapshotStore and WorkflowInstance.restore() cover in-memory runtime state. External compensation is separate scenario work. |
| A Git service outage or merge conflict occurs | Retry transient outages up to a configured limit. Resolve conflicts in a fresh implementation/build/review cycle; cancelled retries retain a reason. | GitHostingConnector reports the simulated outcome through Tividy.externalEvent(); AutomationRules evaluates configured retry policy. Changed code requires fresh approvals. |
| Deployment is outside the supplied Git connector | Release Manager performs simulated deployment and reports the result. Failure ends the instance Cancelled with a reason. | Participant, deployment WorkItem and externalEvent() suffice for this scope. No deploy operation is claimed on GitHostingConnector. |
| Cancellation arrives during a parallel stage or approval wait | Instance cancellation cancels all open work and prevents late decisions or releases from advancing it. | WorkflowInstance.cancel(), Stage.cancel() and state guards apply to every child. Diagram 01 checks cancellation on dispatch; the scenario assumes those generic checks between its actions. Pending approval enumeration must ignore cancelled/final items. |

The generic engine coordinates business work; participants and simulated external systems perform that work. Feature names, risk gates and release policies belong to FeatureDeliveryProcess, the definition, runtime payload and configured rules.

## Sources

- Local specification: **COS214_Practical6_TIVIDY (2).pdf**, Section 7, with Sections 4 and 6 establishing the generic engine and scenario boundaries.
- Team research and selected scenario: [COS 214 TIVIDY Workflow Management System](https://docs.google.com/document/d/13nnrzcGJ9g7ikiHVaY9U0yIP8Tpx56XB8VCrPTZr0zE/edit?usp=sharing), Task 1 and Task 2, read 5 October 2026. The scenario, participants, dependencies and failure cases come from that document.
- Supplied class design: **CD_Tividy_VisualParadigm.xml**, project TIVIDY, class diagram CD_Tividy. The imported class models provide the operation names used in traceability.
- Native authoring interface: [Visual Paradigm activity diagram Open API](https://knowhow.visual-paradigm.com/openapi/activity-diagram/) and [command-line plugin execution](https://www.visual-paradigm.com/support/documents/vpuserguide/124/255/84449_runningplug-.html).

## Validation

The authoring data checks every flow endpoint, outgoing guards on decisions, alternative merges, initial/final nodes, and fork/join connectivity. Each ordinary action and decision has one incoming control flow; alternatives combine through explicit merges. Visual Paradigm created the VPP and the image/XML exports. A separate native reopen confirmed three activity diagrams, 125 activity nodes, 149 control flows with connected endpoints, 61 saved guards, 10 resolved called-activity references, and three named swimlane headers. Visual inspection of the native PNG exports checked text, connectors and swimlane placement. These checks validate the design artifact, not a C++ implementation.
