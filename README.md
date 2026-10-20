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
To build and run this project, you will need:
* **C++ Compiler:** GCC, Clang, or MSVC supporting C++11 standard.
* **CMake:** Version 3.10 or higher.
* **Build Tool:** Make.

### Build Instructions
This project uses CMake to maintain build separation and enforce standard compliance.

1. Create and enter a build directory in the root of the project:
   ```bash
   mkdir build && cd build
   ```
2. Generate build files using CMake pointing to the project root:
   ```bash
   cmake ..
   ```
3. Compile the code:
   ```bash
   make
   ```

Compiling produces two executables inside the `build/` directory:
* `TIVIDY_App`: The main interactive application.
* `TIVIDY_Tests`: The automated unit test suite.

---

### Run Instructions

> **Note:** Ensure you are executing commands from inside the `build/` directory.

#### Running the Application
To start the interactive TIVIDY workflow system:
```bash
./TIVIDY_App
```

#### Running Automated Tests
The project includes a custom automated test runner validating the core engine logic and design patterns:
```bash
./TIVIDY_Tests
```
* **Output:** Displays `[PASS]` or `[FAIL]` status per test case with a concluding summary.
* **Exit Codes:** Returns `0` if all tests pass; `1` on any failure.

---

## Repository Structure

```text
cos-214-project/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Project overview and documentation
├── include/                    # Public Header Files (.h)
│   ├── core/                   # Workflow engine core (WorkflowInstance, WorkItem)
│   ├── patterns/               # GoF Design Patterns categorized
│   │   ├── creation/           # Builder, Factory, Singleton
│   │   ├── execution/          # Command, Memento, Observer, State
│   │   └── structural/         # Adapter, Facade, Proxy, Composite, Decorator
│   └── industries/             # Industry-specific rules & extensions
├── src/                        # Implementation Files (.cpp)
│   ├── core/                   # Core engine implementation
│   ├── patterns/               # Pattern implementations
│   └── industries/             # Industry rules implementation
├── tests/                      # Automated Unit Test Suite
│   ├── main_test.cpp           # Custom test runner main entry
│   ├── test_utils.h / .cpp     # Shared testing utilities
│   ├── commandUnitTest.cpp     # Command pattern tests
│   ├── builderUnitTest.cpp     # Builder pattern tests
│   ├── compositeUnitTest.cpp   # Work tree (WorkNode, CompositeStage, WorkItem lifecycle) tests
│   ├── iteratorUnitTest.cpp    # Iterator pattern tests
│   ├── workflowInstanceUnitTest.cpp # WorkflowInstance (running a definition) tests
│   └── facadeUnitTest.cpp      # Facade (Tividy) tests
└── docs/                       # Project Documentation
    ├── diagrams/               # Activity, class, sequence, and state diagrams
    ├── research/               # Research findings and references
    └── revisions/              # Design decision records and revision history
```

---

## How the Build System Works

The `CMakeLists.txt` configuration streamlines cross-platform development:

1. **Enforce C++11:** Sets `CMAKE_CXX_STANDARD` to 11 to guarantee specification compliance.
2. **Clean Include Paths:** Adds `include/` to the compiler search path, allowing clean syntax (e.g., `#include "patterns/execution/WorkCommand.h"`) rather than complex relative paths.
3. **Automated Source Discovery:** Uses `GLOB_RECURSE` to automatically detect `.cpp` source files in `src/`. *Re-run `cmake ..` whenever a new file is added.*
4. **Isolated Test Executable:** Explicitly excludes the main application entry point (`src/main.cpp`) when building `TIVIDY_Tests` to avoid `main()` symbol collisions.

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
* User interfaces (GUI/Web UI)
* Authentication systems
* Network protocols and persistent disk databases (handled in-memory for demonstrations)
* Internal mechanisms of external systems

> **Data Separation Principle:** Definition data (structure, rules, roles) is strictly separated from instance data (live work tree, item states, history). Running workflow instances retain the specific definition version active at their creation time.

---

## Design Patterns

| Pattern | Problem Solved in TIVIDY | Key Classes |
| :--- | :--- | :--- |
| **Facade** | Unified interface over core engine subsystems | `Tividy` |
| **Composite** | Nesting stages, tasks, and sub-processes hierarchically | `WorkNode`, `CompositeStage`, `LeafWorkItem` |
| **Iterator** | Traversing the work tree in specific operational orders | `WorkIterator`, `DepthFirstIterator`, `ReadyOnlyIterator` |
| **State** | Managing task lifecycles and enforcing valid transitions | `WorkItem`, `WorkItemState`, `CreatedState`, `AvailableState`, `AssignedState`, `InProgressState`, `AwaitingApprovalState`, `CompletedState`, `RejectedState`, `CancelledState`, `EscalatedState` |
| **Decorator** | Adding dynamic task behaviors (validation, dynamic priority, auditing) | `WorkItemDecorator`, `ValidatedWork`, `PriorityWork`, `AuditedWork`, `NotifiedWork`, `SecuredWork` |
| **Strategy** | Swappable work item assignment strategies | `AssignmentStrategy`, `RoleBasedAssignment`, `LeastLoadedAssignment`, `RuleBasedAssignment` |
| **Chain of Responsibility** | Multi-level approval routing and escalation chains | `ApprovalHandler`, `AutoApproveHandler`, `ReviewerHandler`, `TechLeadHandler`, `ReleaseManagerHandler` |
| **Abstract Factory** | Instantiating consistent families of workflow elements | `WorkflowFactory`, `FactoryWorkflowFactory`, `StartupPolicyFactory`, `OnboardingFactory` |
| **Builder** | Step-by-step assembly of complex workflow definitions | `WorkflowBuilder`, `ConcreteDefinitionBuilder`, `FeatureDeliveryProcess` |
| **Command** | Encapsulating operations to enable execution logging and undo | `WorkCommand`, `AssignCommand`, `StartCommand`, `CompleteCommand`, `ApproveCommand`, `RejectCommand`, `EscalateCommand`, `InvokeCommand`, `CancelCommand` |
| **Memento** | Capturing state snapshots to support rollbacks to checkpoints | `WorkflowInstance`, `InstanceMemento`, `SnapshotStore` |
| **Observer** | Decoupling state change reactions and notifications from core logic | `EventBus`, `WorkflowObserver`, `ActivityFeed`, `Notifier`, `MetricsCollector`, `AutomationRule` |

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