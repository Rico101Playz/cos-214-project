# CI/CD pipeline

TIVIDY uses **GitHub Actions** (`.github/workflows/ci.yml`) to build the project and run all tests automatically. It is a CI pipeline only: it builds, tests and keeps the outputs, but does not deploy.

## When it runs

* On every pull request targeting `dev` or `main`.
* On every push to `dev` or `main` (in practice, a merged PR).

## What it does

One job, `build-and-test`, runs on a fresh `ubuntu-latest` machine:

1. **Check out** the code (`actions/checkout@v4`).
2. **Configure**: `cmake -S . -B build`
3. **Build**: `cmake --build build --parallel`
4. **Test**: `ctest --test-dir build --output-on-failure`
5. **Report**: write a GoogleTest XML report and upload it as the `test-results` artifact (runs even if tests fail).
6. **Keep the binary**: upload `TIVIDY_App` as the `tividy-linux` artifact (only if everything passed).

A compile error or a failing test fails the run and shows as a red check on the PR. GoogleTest is vendored in `third_party/googletest`, so nothing needs to be installed.

If you push again while an earlier run on the same branch is still going, the older run is cancelled (`concurrency` with `cancel-in-progress`).

## Branch protection

`main` and `dev` are protected:

* `build-and-test` must pass before a PR can be merged.
* Direct pushes and force pushes are blocked. All changes go through a pull request.

Code that does not build or fails a test cannot reach `dev` or `main`.

## Workflow

1. Branch from `dev` and push your branch.
2. Open a PR into `dev`.
3. Wait for `build-and-test` to go green. If it fails, open the run under the **Actions** tab, read the failing step, fix it and push again.
4. Merge once it passes.

To run the same checks locally:

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

See `docs/TESTING.md` for more on running and writing tests.