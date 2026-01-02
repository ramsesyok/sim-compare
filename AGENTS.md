# Repository Guidelines

## Project Structure & Module Organization
- Root docs: `README.md` (project overview) and `simulation_outline.md` (simulation spec).
- JSON schemas live in `schemas/` (e.g., `schemas/scenario.schema.json`). Keep schema updates in sync with the outline.
- Implementation directories are planned per architecture/language:
  - Rust: `aos_rs`, `soa_rs`, `hecs_rs`, `bevy_rs`
  - Go: `aos_go`, `soa_go`, `donburi_go`, `ark_go`
- Put source under each language’s conventions (e.g., `src/` for Rust, package folders for Go). Keep shared assets alongside each implementation to avoid cross-language coupling.

## Build, Test, and Development Commands
- No build scripts are checked in yet. Use language defaults within each implementation directory when they exist:
  - Rust (Cargo): `cargo build`, `cargo run -- --scenario <path>`
  - Go (modules): `go build ./...`, `go run . --scenario <path>`
- CLI arguments must be consistent across implementations:
  - `--scenario`, `--timeline-log`, `--event-log`

## Coding Style & Naming Conventions
- Comments must be in Japanese and intentionally verbose for beginners.
- Explain the simulation architecture (AoS/SoA/ECS) at the start of each main processing entry point.
- Prefer clear, descriptive names that mirror domain terms (e.g., `DetectionEvent`, `TimelineLog`).
- Keep indentation consistent with the language standard (Rust: 4 spaces via rustfmt; Go: tabs via gofmt once introduced).

## Testing Guidelines
- No test framework is set up yet. When tests are added, place them with language conventions (Rust `tests/`, Go `_test.go`).
- Target schema validation and deterministic simulation steps first.

## Commit & Pull Request Guidelines
- Git history is not available in this workspace. Use conventional, readable commits (e.g., `feat: add aos_rs skeleton`).
- PRs should include: summary of architecture choice, CLI compatibility confirmation, and sample input/output paths.

## Security & Configuration Tips
- Treat scenario and log paths as untrusted input; validate against the `schemas/` definitions.
