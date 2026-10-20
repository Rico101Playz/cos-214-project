# Workflow Management Research

Research for COS 214 Practical 6 (Task 1). We studied two existing workflow products to find the responsibilities that repeat across workflow systems and the parts that are expected to vary.

| Product | Use | Technology |
| --- | --- | --- |
| [Trello](https://trello.com/) | To-dos and personal workflow automation | JavaScript, Node.js, Socket.io |
| [Jira](https://www.atlassian.com/software/jira) | Tracking and planning, single and team workflows | OSWorkflow, XML |

## Structure Research

### What is a workflow and what is a workflow management system?

A workflow is a structured sequence of actions, and a workflow management system is the engine that runs these steps automatically. Jira and Trello act as engines that systematically track and move work from creation to completion.

### What is the difference between a workflow definition and a running workflow instance?

A workflow definition is a saved, reusable blueprint, such as a published Jira workflow or a Trello board template. A running instance is an active, executing version of that template where the actual tasks are progressing.

### Which information belongs to a workflow definition and which belongs to a running instance?

The definition holds fixed structural rules, such as the required sequence of statuses and allowed transitions. The instance holds temporary, active data, such as a specific Jira issue's current assignee, active status and payload.

### How are activities or work items represented?

Work items are represented as discrete objects (Jira issues, Trello cards) that move through different phases of completion.

- **State:** tasks transition between pending, busy and completed states, so the item's behaviour changes with its current status.
- **Adapter:** if tasks are imported from another workflow system (for example a Trello card into Jira), an adapter translates the external data into the system's standard format.

### How do real workflow systems model nested activities or sub-processes?

Complex work is broken down into nested pieces, such as Jira sub-tasks inside a larger task.

- **Composite:** lets the engine treat single tasks and nested sub-tasks identically when tracking completion, without knowing their internal structure.

### How are participants, roles and responsibilities represented?

Systems assign abstract project permissions and roles (such as Admin or Member) rather than hard-coding specific people to tasks.

- **Strategy:** organisations decide how to assign work when it arrives. One might assign based on hours worked this week, another on a completely different routing rule.

### How are changes to workflow definitions handled while work is already in progress?

Systems use strict versioning by saving live edits as drafts. Active instances finish on the older published version they started on, so running Jira issues or Trello cards do not break or skip steps mid-execution.

## Runtime Research

### How are work items assigned and routed?

- **Trello:** a card moves between lists and keeps a reference to its members and creator. People can be auto-assigned by rules (random or in turn). This is a "who must handle me" approach based on role.
- **Jira:** a new work item is resolved in this order: component lead, project lead, unassigned, project default. Automation adds if/else, lookup-table or round-robin picking. This is a "how to pick" approach, treating selection as a fixed strategy.

**Design option:** fixed strategies handle work items, or the work item describes its own assignment through strategies.

### How are approvals, rejection and escalation handled?

Approvals and rejections are decisions made by people while the workflow is running, and the decision picks the next path.

- **Trello:** no built-in approvals. Uses labels, lists, custom fields or Power-Ups to name approvers.
- **Jira:** specifies who (and how many) may make the decision, with one path for approve and one for decline. Conditions can limit who may make a transition. Escalation is separate and timer-based (for example an SLA about to breach).

**Design option:** model approval and rejection as a state with two exits, and escalation as a timer rule.

### Which decisions are normally configurable rather than hard-coded into the workflow engine?

In both Trello and Jira, behavioural data and actions are configurable: permissions, notifications, updates and rules, meaning all movement of a request inside the system. The core structure (item, state, move) stays fixed.

**Design option:** hard-code the skeletal structure, which generates configurable processes.

### How are dependencies, events and failures handled?

- **Trello:** no built-in dependencies (uses checklists, links and automation rules). Webhooks monitor changes, and an automation log records errors.
- **Jira:** link types such as "blocks" hold dependencies. Rule runs end in set statuses (for example failure), with loop detection.

**Design option:** built-in dependencies, a loop guard, and error handling with retries and error logs.

### How do workflow systems interact with external systems?

Both use a REST API as a middleman (outside systems call in), webhooks (events go out), and add-ons (Power-Ups or apps) for extras.

**Design option:** a single connection point for external systems, with each integration behind its own connector.

### How is workflow history or audit information recorded?

- **Trello:** timestamped card activity, a board activity feed and an automation error log.
- **Jira:** a history record logging each change (old and new value) with user information. Admin changes go in a separate audit log.

**Design option:** an action log with timestamps and the user responsible, kept separate from the work items.

### How are running workflows monitored and reported on?

- **Trello:** a dashboard showing card counts per list, label, member and due-date status.
- **Jira:** built-in reports and dashboards (burndown, velocity, cumulative flow).

**Design option:** generate reports from recorded activity, and keep the choice of report separate from the work model.

### What makes a workflow engine reusable across different processes?

Both keep the engine separate from the process setup. Jira reuses workflows and settings (schemes) across projects. Trello has board and card templates, shared automations and Power-Ups.

**Design option:** the engine only knows items, states and transitions; each process is a saved definition, with plug-in points for custom rules.

## References

Atlassian (2019). *Using automation for round-robin assignee which is different than reporter.* [online] Atlassian Community. Available at: https://community.atlassian.com/forums/discussion/1202858/using-automation-for-round-robin-assignee-which-is-different-than-the-reporter [Accessed 5 Oct. 2026].

Atlassian (2020). *What are permission schemes in Jira?* [online] Atlassian.com. Available at: https://support.atlassian.com/jira-cloud-administration/docs/what-are-permission-schemes-in-jira/ [Accessed 3 Oct. 2026].

Atlassian (2022). *Turn your board into a template.* [online] Atlassian.com. Available at: https://support.atlassian.com/trello/docs/creating-template-boards/ [Accessed 3 Oct. 2026].

Atlassian (2024). *Introduction to Jira workflows.* [online] Atlassian. Available at: https://www.atlassian.com/software/jira/guides/workflows/overview [Accessed 3 Oct. 2026].

Atlassian (2025). *Restrict workflows based on user groups.* [online] Atlassian.com. Available at: https://support.atlassian.com/jira-cloud-administration/docs/restrict-workflows-based-on-user-groups/ [Accessed 3 Oct. 2026].