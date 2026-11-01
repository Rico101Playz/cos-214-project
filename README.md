# TIVIDY: Workflow Management System

> **COS 214 Project | University of Pretoria (2026)**

**TIVIDY** is a generic workflow management system implemented in modern C++11. It stores versioned workflow definitions, creates independent running instances from them, and manages each instance from start to a final outcome. Processes are supplied to TIVIDY strictly as definitions and configurations—none are hardcoded into the engine.

The primary demonstration scenario is **Software Feature Delivery**: a mid-sized software team taking a feature request from intake through implementation, review, approval, and release.

---

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

## Build & Run

### Prerequisites
* **C++ compiler** with C++11 support (GCC, Clang or MSVC).
* **CMake** 3.18 or newer, and a build tool it can drive (Make or Ninja).
* GoogleTest and civetweb ship in `third_party/`; nothing else needs installing.

### Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This produces three programs in `build/`:

| Program | What it is |
| --- | --- |
| `TIVIDY_Server` | The website: serves `web/` and talks to the browser over a WebSocket. |
| `TIVIDY_App` | The console version of the same engine. |
| `TIVIDY_Tests` | The GoogleTest suite. |

### Run
```bash
./TIVIDY_Server --ports 8080
```
It lists the saved scenarios in `scenarios/` and asks which to open; `--scenario ../scenarios/acme-software.txt`
opens one straight away. Then open `http://localhost:8080/`. `--web` and `--scenarios` point at other copies of
`web/` and `scenarios/`. `DOCKER.md` describes running everything in Docker instead.

```bash
./TIVIDY_App
```
is the console version; it takes the same `--scenarios` and `--scenario` options.

### Tests
```bash
./TIVIDY_Tests
```
or `ctest` from `build/`. `docs/TESTING.md` explains what each test file covers.

---

## Repository Structure

Code is grouped by the design pattern it belongs to; `include/<folder>/X.h` pairs with `src/<folder>/X.cpp`.

```text
cos-214-project/
├── CMakeLists.txt
├── include/ and src/
│   ├── mediator/               # Mediator, TividyMediator, Colleague, Request, Response, Action, TaskRef
│   ├── proxy/                  # SecuredMediator: the permission check in front of the mediator
│   ├── facade/                 # ProjectFacade, OrgFacade, HistoryFacade (the colleagues)
│   ├── command/                # Command, RequestCommand, CommandInvoker, CommandHistory, UndoHistory
│   ├── memento/                # Memento, EngineMemento, SnapshotStore
│   ├── observer/               # EventBus, WorkflowEvent, WorkflowObserver
│   ├── composite/              # Task, TaskNode, CompositeTask, LeafTask (the work tree)
│   ├── decorator/              # TaskDecorator, ApprovalWork, BlockedWork, ValidatedWork
│   ├── iterator/               # WorkIterator, DepthFirstIterator, FilterIterator, TaskFilter
│   ├── state/                  # TaskState and one class per task status
│   ├── strategy/               # Assignment strategies and pipelines, stage modes
│   ├── chain_of_responsibility/# Approval handlers
│   ├── abstract_factory/       # WorkflowFactory per policy, PolicyCatalog
│   ├── builder/                # Definition builder and directors
│   ├── org_composite/          # Company, OrgUnit, Participant (the organisation tree)
│   ├── core/                   # WorkflowDefinition, WorkflowInstance, Organisation
│   ├── server/                 # Website back end: sessions, request reading, views, scenarios
│   └── net/                    # The web server program (civetweb)
├── tests/                      # GoogleTest suite
├── web/                        # The website
├── scenarios/                  # Saved companies and projects
├── third_party/                # googletest, civetweb
└── docs/
```

---

## How a Request Flows

Every change, from the website, the console or a test, is a `Request` sent to the mediator.

1. **Website:** `SessionHub` hands the browser's JSON to `RequestReader`, which turns it into a `Request` (action, project and task, actor, arguments).
2. **Proxy:** the session's `SecuredMediator` checks the actor may do it, then passes it on. `Mediator::send` turns any refusal into a `Response` code (`forbidden`, `invalid`, `not_allowed_now`).
3. **Mediator:** `TividyMediator` coordinates. It checks what spans facades (for example, that a unit exists in the organisation), opens an undo step (a whole-engine `EngineMemento`), and runs each change as a `RequestCommand` through its `CommandInvoker`.
4. **Colleague:** the command's receiver is `ProjectFacade`. Its `handle(request)` either builds a new task with the project's factory, or acts on the project, or hands the `Request` to the outermost decorator of the named task: `BlockedWork` → `ValidatedWork` → `ApprovalWork` → `LeafTask`, whose `TaskState` decides whether the action is allowed.
5. **Back to the mediator:** `ProjectFacade` reports the change with `notify`. When work moved on or off someone's desk, the mediator updates the people's loads in `OrgFacade`. When work needs a person, it asks `OrgFacade::choose` (the policy's assignment strategy) and sends `ProjectFacade` an `Assign`.
6. **Events:** the mediator publishes `WorkflowEvent`s on the `EventBus`; the website's observers turn them into the activity feed and refresh every open page.

Organisation changes go to `OrgFacade::handle`, and undo and redo go to `HistoryFacade::handle`, the same way. The three facades never refer to each other. Between them they share only the mediator's protocol classes: `Colleague`, `Request`, `Response`, `Action`, `TaskRef` and `Memento`.

---

## How the Build System Works

* **C++11** is enforced (`CMAKE_CXX_STANDARD 11`, no extensions).
* `include/` is on the include path, so headers are included by folder: `#include "decorator/ApprovalWork.h"`.
* **Engine** (`tividy_engine`): every `src/**/*.cpp` except `src/server/`, `src/net/` and `src/main.cpp`, found with `GLOB_RECURSE ... CONFIGURE_DEPENDS`, so new files are picked up on the next build.
* **Service** (`tividy_service`): `src/server/`, on top of the engine. `TIVIDY_App`, `TIVIDY_Server` and `TIVIDY_Tests` all link it.
* **Tests:** every `tests/*.cpp`, linked with GoogleTest's `gtest_main` and registered with CTest.

---

## System Scope

### In Scope
* Workflow definitions & versioning
* Execution instances & live tracking
* Stages, work items, and sub-processes
* Work item state lifecycles
* Assignment & rule-based routing
* Approval chains & escalation handling
* Event bus & notification mechanisms
* Rollbacks & history via snapshot mementos
* Basic engine monitoring and metrics reporting

### Out of Scope
* Business domain work execution
* Authentication systems (people choose who they are on the website; permissions are enforced, identity is not)
* Persistent databases (scenarios are saved as text files)
* Internal mechanisms of external systems

> **Data Separation Principle:** Definition data (structure, rules, roles) is strictly separated from instance data (live work tree, item states, history). Running workflow instances retain the specific definition version active at their creation time.

---

## Design Patterns

| Pattern | Problem Solved in TIVIDY | Key Classes |
| :--- | :--- | :--- |
| **Mediator** | One place coordinates projects, people and history, so none of them know each other | `Mediator`, `TividyMediator`, `Colleague` |
| **Facade** | A simple front over each part of the engine | `ProjectFacade`, `OrgFacade`, `HistoryFacade` |
| **Proxy** | Checking permissions before a request reaches the engine | `SecuredMediator` |
| **Command** | Every change is an object that can be run, recorded and undone | `Command`, `RequestCommand`, `CommandInvoker`, `CommandHistory` |
| **Memento** | Snapshots for undo, redo and saving scenarios | `Memento`, `EngineMemento`, `SnapshotStore`, `UndoHistory` |
| **Observer** | Telling the website and the history what happened | `EventBus`, `WorkflowEvent`, `WorkflowObserver` |
| **Composite** | Projects, stages, tasks and sub-tasks as one tree; the organisation as another | `Task`, `TaskNode`, `CompositeTask`, `LeafTask`; `OrgComponent`, `OrgUnit`, `OrgMember` |
| **Decorator** | Approval, blockers and required fields wrap a task and see each request first | `TaskDecorator`, `ApprovalWork`, `BlockedWork`, `ValidatedWork` |
| **Iterator** | Walking the work tree, optionally filtered | `WorkIterator`, `DepthFirstIterator`, `FilterIterator`, `TaskFilter` |
| **State** | Task lifecycles and which actions each status allows | `TaskState`, `CreatedState`, `AvailableState`, `AssignedState`, `InProgressState`, `AwaitingApprovalState`, `CompletedState`, `RejectedState`, `CancelledState`, `EscalatedState` |
| **Strategy** | Swappable assignment rules per policy, and how a stage releases its work | `AssignmentStrategy`, `RoleBasedAssignment`, `LeastLoadedAssignment`, `RuleBasedAssignment`, `AssignmentService`; `StageMode`, `ParallelMode`, `SequentialMode` |
| **Chain of Responsibility** | Routing an approval to the right role | `ApprovalHandler`, `OwnerHandler`, `PolicyChainHandler`, `RoleApprovalHandler`, `AutoApproveHandler`, `TimeoutEscalationHandler`, `ManagerFallbackHandler` |
| **Abstract Factory** | Consistent stages, tasks and approval chains per policy | `WorkflowFactory`, `SmallTeamFactory`, `BusyTeamFactory`, `HighRiskFactory`, `PolicyCatalog` |
| **Builder** | Step-by-step assembly of workflow definitions | `WorkflowBuilder`, `ConcreteDefinitionBuilder`, `FeatureDeliveryDirector`, `OutlineDirector` |

---

## Contributing

Development follows a strict branching workflow:

1. **Main Branch Safety:** Direct commits to `main` are prohibited. All work is performed on `dev` and merged into `main` only upon stable release milestones.
2. **Pulling Latest Code:** Always pull the latest changes from `dev` before creating a new branch:
   ```bash
   git checkout dev && git pull origin dev
   ```
3. **Branch Naming Conventions:** Create task-focused feature branches off `dev`:
   * `feature/<short-description>`
   * `docs/<short-description>`
   * `fix/<short-description>`
4. **Commits & Pull Requests:** Make atomic commits with descriptive commit messages, and submit a Pull Request (PR) targeting the `dev` branch.
5. **Documentation Updates:** Log significant architectural adjustments under `docs/revisions/` alongside relevant diagram updates.

---

## References

* Atlassian. (2019). *Using automation for round-robin assignee which is different than reporter*. Atlassian Community.
* Atlassian. (2020). *What are permission schemes in Jira?* Atlassian Support.
* Atlassian. (2022). *Turn your board into a template*. Atlassian Support.
* Atlassian. (2024). *Introduction to Jira workflows*. Atlassian.
* Atlassian. (2025). *Restrict workflows based on user groups*. Atlassian Support.

*Full citations including URLs and access metadata are documented in [`docs/research/workflow-research.md`](docs/research/workflow-research.md).*