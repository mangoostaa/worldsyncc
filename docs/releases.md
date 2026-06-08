# Releases And Precompiled Builds

Early adopters should receive both source and precompiled artifacts:

- `WorldSync.dll` for Windows.
- `WorldSync.so` for Linux.
- `worldsync.inc`.
- Documentation snapshot.
- Changelog.

## Recommended Release Layout

```text
WorldSync-v0.1.0-windows-x64.zip
  components/WorldSync.dll
  pawno/include/worldsync.inc
  docs/
  examples/

WorldSync-v0.1.0-linux-x64.tar.gz
  components/WorldSync.so
  pawno/include/worldsync.inc
  docs/
  examples/
```

## Windows Build

```powershell
cd C:\path\to\open.mp-sdk\WorldSync
$p = $env:Path
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $p, 'Process')
cmake --build build --config Release --target WorldSync
cmake --build build --config Release --target WorldSyncTests
ctest --test-dir build -C Release --output-on-failure
```

Artifact:

```text
build/Release/WorldSync.dll
```

## Windows Package

After the Windows build and tests pass, create the release archive:

```powershell
.\scripts\package-windows.ps1 -Version v0.1.0
```

The generated archive is:

```text
dist/WorldSync-v0.1.0-windows-x64.zip
```

## Linux Build

```bash
cd /path/to/open.mp-sdk/WorldSync
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target WorldSync
cmake --build build --target WorldSyncTests
ctest --test-dir build --output-on-failure
```

Artifact path depends on the generator, but it is usually:

```text
build/libWorldSync.so
```

## Release Checklist

- Build Windows release.
- Build Linux release.
- Run tests on both platforms.
- Start a test open.mp server with Pawn, Objects, Databases, NPCs and TextLabels components.
- Verify `WS_DebugSummary()` logs correctly.
- Verify door object creation.
- Verify crop ready callback.
- Verify path debug labels if TextLabels is loaded.
- Verify NPC patrol if NPCs is loaded.
- Package `.dll`, `.so`, `worldsync.inc`, docs and examples.
- Include `CHANGELOG.md` in release notes.
- Create Git tag, for example `v0.1.0`.
- Publish archives in GitHub Releases.

## Current Local Build Status

On this Windows workspace, `WorldSync.dll` and `WorldSyncTests.exe` build successfully. Linux `.so` should be produced from Linux or CI, not from this Windows-only local build.
