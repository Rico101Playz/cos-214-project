# Testing with GoogleTest: step-by-step setup

The test suite now uses **GoogleTest** instead of our own `check()` runner. Every one of the 940 old checks was kept and turned into a GoogleTest assertion with the same message. On top of that there are fixtures (`TEST_F`) and parameterised tests (`TEST_P`), and CTest and GitHub Actions run everything.

GoogleTest is **vendored** in `third_party/googletest` (version 1.12.1, the last one that supports C++11). You do not install it. CMake builds it together with the project.

---

## 1. Install the tools (once per computer)

### Windows (MSYS2, what we already use)

1. Open the **MSYS2 UCRT64** terminal (the purple icon, not "MSYS2 MSYS").
2. Install the compiler, CMake and Ninja:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```
3. Check the versions. CMake must be **3.18 or newer**.
   ```bash
   gcc --version
   cmake --version
   ```

Do **not** install the `mingw-w64-ucrt-x86_64-gtest` package. It is a newer GoogleTest that needs C++14 or C++17, and our project is C++11. The copy in `third_party/googletest` is the one that gets used.

### Linux (Ubuntu or WSL)

```bash
sudo apt install build-essential cmake ninja-build
```

### macOS

```bash
xcode-select --install
brew install cmake ninja
```

---

## 2. Put the new files in your copy of the repo

1. Unzip `cos-214-project-web.zip` somewhere outside the repo.
2. In your repo, make a branch for the change, for example:
   ```bash
   git checkout dev
   git pull
   git checkout -b feature/googletest
   ```
3. Copy everything from the unzipped `cos-214-project` folder over your repo folder (replace the files when asked).
4. Delete the three files of the old runner if they are still there:
   ```bash
   git rm tests/main_test.cpp tests/test_utils.h tests/tests_utils.cpp
   ```
5. Check what changed:
   ```bash
   git status
   ```
   You should see `third_party/googletest/` as new, `CMakeLists.txt`, `.github/workflows/ci.yml` and every file in `tests/` as modified, `tests/policyParamTest.cpp` and `docs/TESTING.md` as new.

Committing, pushing and opening the pull request into `dev` is up to you, as usual.

---

## 3. Configure and build

Run these from the root of the repo (the folder with `CMakeLists.txt`).

### Windows (MSYS2 UCRT64 terminal)

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

If you prefer Make: `cmake -S . -B build -G "MinGW Makefiles"` and then `cmake --build build`.

### Linux and macOS

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

If you had an old `build` folder from before this change, delete it first (`rm -rf build`) so CMake starts clean.

The build makes three programs in `build/`: `TIVIDY_App`, `TIVIDY_Server` and `TIVIDY_Tests`.

---

## 4. Run the tests

### Everything

```bash
./build/TIVIDY_Tests
```

The end of the output should say:

```text
[==========] 117 tests from 23 test suites ran.
[  PASSED  ] 117 tests.
```

### Through CTest (runs every test case on its own)

```bash
ctest --test-dir build --output-on-failure
```

### Useful options

| What you want | Command |
| --- | --- |
| List every test without running them | `./build/TIVIDY_Tests --gtest_list_tests` |
| One suite | `./build/TIVIDY_Tests --gtest_filter='Proxy.*'` |
| One test | `./build/TIVIDY_Tests --gtest_filter='Facade.WorkItems'` |
| Several suites | `./build/TIVIDY_Tests --gtest_filter='Command.*:Memento.*'` |
| Everything except a suite | `./build/TIVIDY_Tests --gtest_filter='-Server.*'` |
| Only show failures | `./build/TIVIDY_Tests --gtest_brief=1` |
| Run 20 times (catches flaky tests) | `./build/TIVIDY_Tests --gtest_repeat=20` |
| Random order (catches tests that depend on each other) | `./build/TIVIDY_Tests --gtest_shuffle` |
| XML report (for the write-up or CI) | `./build/TIVIDY_Tests --gtest_output=xml:report.xml` |
| CTest, one suite | `ctest --test-dir build -R Proxy` |

### Reading a failure

A failing assertion prints the file, the line, both values and our message:

```text
tests/facadeUnitTest.cpp:107: Failure
Expected equality of these values:
  loadOf(*engine, "Alice")
    Which is: 2
  1
Tividy: assign() adds to the assignee's load
[  FAILED  ] Facade.WorkItems (1 ms)
```

The program then exits with code 1, so CI fails too.

---

## 5. Run the tests from VS Code (optional)

1. Install the **CMake Tools** extension (Microsoft).
2. Open the repo folder in VS Code.
3. When CMake Tools asks for a kit, choose **GCC ... ucrt64** (on Windows) or your normal GCC or Clang.
4. Click **Build** in the status bar.
5. Open the **Testing** panel (the flask icon). All 117 tests show up, grouped by suite. Run or debug any of them with the play buttons.

If the Testing panel is empty, run **CMake: Configure** from the command palette, build once, then press refresh in the Testing panel.

---

## 6. How the tests are organised

| Suite | File | Covers |
| --- | --- | --- |
| `Builder` | `builderUnitTest.cpp` | Builder and directors |
| `Command` | `commandUnitTest.cpp` | Commands and the invoker |
| `Memento` | `mementoUnitTest.cpp` | Snapshots and undo of commands |
| `Strategy` | `strategyUnitTest.cpp` | Assignment strategies and the service |
| `ChainOfResponsibility` | `chainUnitTest.cpp` | Approval handlers |
| `AbstractFactory` | `factoryUnitTest.cpp` | Policy factories and the assembler |
| `Composite`, `CompositeTree` | `compositeUnitTest.cpp`, `test_CompositeUnit.cpp` | Work tree |
| `Iterator` | `iteratorUnitTest.cpp` | Tree iterators |
| `WorkItemState`, `InstanceState`, `StageMode` | `test_workItemState.cpp`, `test_instanceState.cpp`, `test_stageModeState.cpp` | State pattern |
| `StateCompositeIntegration` | `test_stateXcompositeIntegration.cpp` | State and Composite together |
| `WorkflowInstance` | `workflowInstanceUnitTest.cpp` | Running a definition |
| `Facade` | `facadeUnitTest.cpp` | `Tividy` |
| `Observer` | `observerUnitTest.cpp` | Event bus and observers |
| `CompanyComposite` | `orgUnitTest.cpp` | Teams inside teams, and the assignment Bridge |
| `StructuredWorkflow` | `dependencyUnitTest.cpp` | Nested projects, blocking, filtering iterators |
| `Proxy` | `proxyUnitTest.cpp` | Who may do what |
| `Server` | `serverUnitTest.cpp` | JSON, command pipeline, sessions |
| `ApprovalFlowTest` | `approvalFlowUnitTest.cpp` | Priority, sub-tasks, approvals, sign-off, undo, scenario files (uses a fixture) |
| `AllPolicies/PolicyTest` | `policyParamTest.cpp` | The same tests run once per policy (parameterised) |

There is no `main()` in `tests/`. It comes from `gtest_main`, which CMake links in.

---

## 7. Write a new test

1. Make a new file in `tests/`, for example `tests/priorityUnitTest.cpp`. CMake picks up new files on the next build, you do not have to edit `CMakeLists.txt`.
2. Start with:

```cpp
#include <gtest/gtest.h>

#include "patterns/structural/LeafWorkItem.h"

TEST(Priority, DefaultsToMedium) {
    LeafWorkItem item("t1", "Write docs", "Developer");
    EXPECT_EQ(item.priority(), "Medium") << "A new item starts at Medium";
}

TEST(Priority, RejectsUnknownLevels) {
    LeafWorkItem item("t1", "Write docs", "Developer");
    EXPECT_THROW(item.setPriority("Critical"), std::invalid_argument);
}
```

3. Build and run only your new suite:
   ```bash
   cmake --build build
   ./build/TIVIDY_Tests --gtest_filter='Priority.*'
   ```

### Which assertion to use

| Situation | Use |
| --- | --- |
| Two values must be equal | `EXPECT_EQ(actual, expected)` |
| Two values must differ | `EXPECT_NE(a, b)` |
| A condition must hold | `EXPECT_TRUE(cond)` or `EXPECT_FALSE(cond)` |
| Floating point | `EXPECT_DOUBLE_EQ(a, b)` or `EXPECT_NEAR(a, b, 0.001)` |
| Must throw a certain exception | `EXPECT_THROW(statement, std::logic_error)` |
| Must not throw | `EXPECT_NO_THROW(statement)` |
| The rest of the test makes no sense if this fails | the `ASSERT_` version, for example `ASSERT_NE(item, nullptr)` |

Add a message with `<<` so a failure explains itself: `EXPECT_EQ(a, b) << "what should have happened";`.

When comparing a `size()` with a number, write the number as unsigned (`2u`) to avoid a compiler warning: `EXPECT_EQ(list.size(), 2u)`.

### Shared setup: a fixture

When several tests need the same setup, put it in a fixture and use `TEST_F`:

```cpp
class DemoCompanyTest : public ::testing::Test {
protected:
    Tividy engine;

    DemoCompanyTest() : engine("Acme") {}

    void SetUp() override {
        DemoSeed::company(engine);
        DemoSeed::templates(engine);
    }
};

TEST_F(DemoCompanyTest, OwnerExists) {
    EXPECT_TRUE(engine.organisation().hasOwner());
}
```

Every `TEST_F` gets a fresh fixture, so tests never share state. See `ApprovalFlowTest` in `approvalFlowUnitTest.cpp`.

### Same test, several inputs: a parameterised test

```cpp
class PolicyTest : public ::testing::TestWithParam<std::string> {};

TEST_P(PolicyTest, CatalogKnowsIt) {
    EXPECT_TRUE(PolicyCatalog::exists(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(AllPolicies, PolicyTest,
                         ::testing::Values("SmallTeam", "BusyTeam", "HighRisk"));
```

See `tests/policyParamTest.cpp`.

### Naming rules

* Suite and test names are written without underscores (GoogleTest reserves them): `TEST(Proxy, WorkersCannotUndo)`.
* One suite per pattern or feature, one test per behaviour.

---

## 8. What changed from the old `check()` runner

| Old | New |
| --- | --- |
| `void test_Something() { ... }` plus a call in `run_x_tests()` | `TEST(Suite, Something) { ... }` (no list to keep up to date) |
| `check(a == b, "msg");` | `EXPECT_EQ(a, b) << "msg";` |
| `check(a != b, "msg");` | `EXPECT_NE(a, b) << "msg";` |
| `check(!x, "msg");` | `EXPECT_FALSE(x) << "msg";` |
| `check(x && y, "msg");` | `EXPECT_TRUE(x && y) << "msg";` |
| `check(throws<std::invalid_argument>([&] { f(); }), "msg");` | `EXPECT_THROW(([&] { f(); })(), std::invalid_argument) << "msg";` |
| `check(throwsLogicError([&] { f(); }), "msg");` | `EXPECT_THROW(([&] { f(); })(), std::logic_error) << "msg";` |
| `tests/main_test.cpp`, `test_utils.h`, `tests_utils.cpp` | Removed. `main()` comes from `gtest_main` |
| `[PASS]` and `[FAIL]` lines | GoogleTest output, CTest, XML reports |

The conversion was done by a script and checked by hand: same 940 assertions with the same messages, plus 12 new parameterised test cases.

---

## 9. What GitHub Actions does now

`.github/workflows/ci.yml` builds the project, runs every test through `ctest --output-on-failure`, and uploads `test-results.xml` (the GoogleTest XML report) as an artifact. It does this even when tests fail, so you can open the report and see which ones. A failing test makes the run red.

---

## 10. Troubleshooting

| Problem | Fix |
| --- | --- |
| `gtest/gtest.h: No such file or directory` | `third_party/googletest` is missing. Copy it from the zip, then run `cmake -S . -B build` again. |
| `CMake 3.18 or higher is required` | Update CMake (`pacman -S mingw-w64-ucrt-x86_64-cmake` in UCRT64). |
| Errors mentioning C++14 or C++17 inside GoogleTest | A different GoogleTest is being picked up (for example the MSYS2 `gtest` package through `find_package`). Our `CMakeLists.txt` only uses `third_party/googletest`. Delete `build` and configure again. |
| `multiple definition of main` | A test file has its own `main()`. Remove it, `gtest_main` provides one. |
| `ctest` says "No tests were found" | Build first (`cmake --build build`), then run `ctest --test-dir build`. |
| New test file is not compiled | Run `cmake --build build` again. If it still does not show, run `cmake -S . -B build` once. |
| The `.exe` is blocked or deleted on Windows | Windows Defender sometimes quarantines freshly built programs. Add the `build` folder to its exclusions. |
| A test passes alone but fails with the rest | It depends on another test. Run with `--gtest_shuffle` to find it, and give it its own setup (a fixture). |
