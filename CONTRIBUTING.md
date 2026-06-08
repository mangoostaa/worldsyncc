# Contributing To WorldSync

Thanks for helping improve WorldSync. This project is intended to stay practical for open.mp servers, so contributions should keep the public Pawn API stable, documented and easy to test.

## Development Setup

WorldSync is built as an open.mp component/plugin with CMake.

On Windows PowerShell:

```powershell
cd C:\path\to\open.mp-sdk\WorldSync
cmake --build build --config Release --target WorldSync
cmake --build build --config Release --target WorldSyncTests
ctest --test-dir build -C Release --output-on-failure
```

The Windows component output is:

```text
build/Release/WorldSync.dll
```

## Release Packaging

After a successful Release build and test run:

```powershell
.\scripts\package-windows.ps1 -Version v0.1.0
```

This creates:

```text
dist/WorldSync-v0.1.0-windows-x64.zip
```

## Contribution Guidelines

- Keep generated build files out of commits.
- Add or update tests for behavior changes where possible.
- Update `include/worldsync.inc` and `docs/natives.md` when adding Pawn natives.
- Update examples when a new feature changes normal usage.
- Keep storage changes backward-compatible, or document any migration requirement clearly.
- Prefer small, focused changes over broad rewrites.

## Pull Request Checklist

Before opening a pull request:

- Build `WorldSync` in Release mode.
- Build and run `WorldSyncTests`.
- Update documentation for user-facing API changes.
- Add release notes to `CHANGELOG.md` when the change affects users.
- Confirm `git status` does not contain generated files from `build/` or `dist/`.

## Coding Notes

- C++ code should remain compatible with C++17.
- Public natives should validate invalid IDs, missing components and small output buffers defensively.
- Log messages should be concise and useful for server owners diagnosing startup, storage and autosave behavior.
