# Testing & Branch Workflow

This guide explains how to spin up a new Git branch for experimentation and run
Auralis' current test suites locally.

## 1. Create and switch to a feature branch

```bash
git checkout -b your-feature-branch
```

Run this command from the repository root. It creates a new branch and
immediately checks it out so that your commits stay isolated from `main`.

## 2. Install dependencies

### Python orchestration service

```bash
cd orchestrator
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Return to the repository root after installing dependencies:

```bash
deactivate
cd ..
```

### Rust synth crate

Rust tooling uses `cargo`. If you do not already have it installed, visit
<https://rustup.rs/> for installation instructions.

## 3. Run the available tests

### Orchestrator fast checks

The FastAPI prototype is currently thin, so linting and unit tests are not yet
implemented. Once modules are added, we will integrate `pytest` and `ruff`
invocations here.

### Synth validation crate

From the repository root:

```bash
cd synth
cargo test
```

> **Note:** In restricted environments without network access, Cargo may fail to
> download crates the first time you run the tests. If that happens, cache the
> dependencies or run the tests in an environment with crate registry access.

## 4. Merge or open a pull request

When your branch is ready, push it and open a pull request:

```bash
git push --set-upstream origin your-feature-branch
```

Then create a PR using your preferred Git hosting platform or a CLI tool. Use
the summary and testing sections outlined in the repository README to keep
reports consistent.
