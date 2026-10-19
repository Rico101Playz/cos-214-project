# TIVIDY: Workflow Management System

COS 214 Project, University of Pretoria, 2026

TIVIDY is a generic workflow management system written in C++11. It stores versioned workflow definitions, creates independent running instances from them, and manages each instance from start to a final outcome. Processes are supplied to TIVIDY as definitions and configuration; none are built into the engine.

The demonstration scenario is **Software Feature Delivery**: a mid-sized software team taking a feature request from intake through implementation, review, approval and release.

## Team

| Name | Student Number |
| --- | --- |
| Kendra Maree | u25070429 |
| Riti Botha | u25062362 |
| Chitambala Sikazwe | u25173384 |
| Rico Geerkens | u25015304 |
| Gary Mull | u25071247 |
| Heinrich Muller | u25050029 |
| Graeme Geerkens | u24566285 |

**Group leader:** Heinrich Muller

## Repository Structure

```
cos-214-project/
├── README.md
└── docs/
    ├── diagrams/     Activity, class, sequence and state diagrams
    ├── research/     Research findings and references
    └── revisions/    Design decisions and revision history
```

## System Scope

**In scope:** workflow definitions and versioning, running instances, stages and work items, sub-processes, work item lifecycles, assignment and routing, approvals and escalation, events, external integrations, history and rollback, monitoring and reporting.

**Out of scope:** the business work itself, user interface design, real authentication, networking and persistent storage (simulated or kept in memory for the demo), and the internals of external systems.

Definition data (structure, required roles, rules, version) is kept separate from instance data (live work tree, item states, assigned participants, payload values, history). Running instances keep the definition version they started with.

## Design Patterns

| Pattern | Problem it solves in TIVIDY | Key classes |
| --- | --- | --- |
| Facade | Single entry point over all subsystems | `Tividy` |
| Composite | Nested stages, work items and sub-processes | `WorkNode`, `CompositeStage`, `LeafWorkItem` |
| Iterator | Traversing the work tree in different orders | `WorkIterator`, `DepthFirstIterator`, `ReadyOnlyIterator` |
| State | Work item lifecycle and valid operations per state | `WorkItem`, `WorkItemState`, `CreatedState`, `AvailableState`, `AssignedState`, `InProgressState`, `AwaitingApprovalState`, `CompletedState`, `RejectedState`, `CancelledState`, `EscalatedState` |
| Decorator | Adding validation, priority, auditing, notification or security to a task | `WorkItemDecorator`, `ValidatedWork`, `PriorityWork`, `AuditedWork`, `NotifiedWork`, `SecuredWork` |
| Strategy | Swappable assignment rules | `AssignmentStrategy`, `RoleBasedAssignment`, `LeastLoadedAssignment`, `RuleBasedAssignment` |
| Chain of Responsibility | Multi-level approval and escalation | `ApprovalHandler`, `AutoApproveHandler`, `ReviewerHandler`, `TechLeadHandler`, `ReleaseManagerHandler` |
| Abstract Factory | Consistent families of workflow parts per process type | `WorkflowFactory`, `FactoryWorkflowFactory`, `StartupPolicyFactory`, `OnboardingFactory` |
| Builder | Step-by-step construction of complex definitions | `WorkflowBuilder`, `ConcreteDefinitionBuilder`, `FeatureDeliveryProcess` |
| Command | Recording actions and supporting undo | `WorkCommand`, `AssignCommand`, `StartCommand`, `CompleteCommand`, `ApproveCommand`, `RejectCommand`, `EscalateCommand`, `InvokeCommand`, `CancelCommand` |
| Memento | Rolling an instance back to a checkpoint | `WorkflowInstance`, `InstanceMemento`, `SnapshotStore` |
| Observer | Reacting to workflow events without coupling the core | `EventBus`, `WorkflowObserver`, `ActivityFeed`, `Notifier`, `MetricsCollector`, `AutomationRule` |

## Contributing

All work is done on the `dev` branch. `dev` is merged into `main` only once it is stable and ready for submission or demo. Nobody commits directly to `main`.

1. Pull the latest `dev` before starting work.
2. Create a branch off `dev` per task: `feature/<short-name>`, `docs/<short-name>` or `fix/<short-name>`.
3. Commit small, meaningful changes with clear messages (e.g. `docs: add approval activity diagram`).
4. Open a pull request into `dev`.
5. Record any significant design change in [`docs/revisions/`](docs/revisions/) along with the updated diagrams.

## References

- Atlassian (2019). *Using automation for round-robin assignee which is different than reporter.* Atlassian Community.
- Atlassian (2020). *What are permission schemes in Jira?* Atlassian Support.
- Atlassian (2022). *Turn your board into a template.* Atlassian Support.
- Atlassian (2024). *Introduction to Jira workflows.* Atlassian.
- Atlassian (2025). *Restrict workflows based on user groups.* Atlassian Support.

Full references with URLs and access dates are in [`docs/research/workflow-research.md`](docs/research/workflow-research.md).