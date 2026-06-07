# Installation

This guide assumes WorldSync is inside an open.mp SDK checkout, matching the current CMake layout:

```text
open.mp-sdk/
  WorldSync/
    CMakeLists.txt
```

## Runtime Requirements

Required:

- open.mp server.
- Pawn component, if you want to use the Pawn natives.

Optional:

- Databases component, recommended for SQLite persistence.
- Objects component, required for visible WorldSync doors.
- NPCs component, required for NPC route movement and patrols.
- TextLabels component, required for path debug labels.

WorldSync still loads if optional components are missing, but the related feature is disabled or degraded.

## Install On Windows

1. Build the plugin:

```powershell
cd C:\path\to\open.mp-sdk\WorldSync
$p = $env:Path
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $p, 'Process')
cmake --build build --config Release --target WorldSync
```

2. Copy the output plugin to your server components/plugins folder according to your open.mp server layout:

```text
build/Release/WorldSync.dll
```

3. Copy the Pawn include:

```text
include/worldsync.inc
```

to your Pawn compiler include directory.

4. Include it in your gamemode or filterscript:

```pawn
#include <worldsync>
```

5. Enable the component/plugin in your open.mp server configuration using the same loading method used by your server for native components.

## Install On Linux

Linux builds should be made from a Linux environment with a C++17 compiler and CMake.

```bash
cd /path/to/open.mp-sdk/WorldSync
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target WorldSync
```

The expected output is a shared object, usually:

```text
build/libWorldSync.so
```

The exact path can vary by generator and SDK layout. Copy the `.so` to your server components/plugins folder and copy `include/worldsync.inc` to your Pawn include directory.

## Verify Installation

Add this to a test gamemode:

```pawn
#include <worldsync>

public OnGameModeInit()
{
	WS_SetLogLevel(WS_LOG_DEBUG);
	WS_DebugSummary();
	printf("WorldSync entities: %d", WS_GetEntityCount());
	return 1;
}
```

On server start, the console should show WorldSync loading and reporting how many entities were loaded.

## Common Issues

### MSBuild Path/PATH Error On Windows

If MSBuild fails with a duplicate `Path` / `PATH` environment variable error, run this in the same PowerShell before building:

```powershell
$p = $env:Path
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $p, 'Process')
```

### No SQLite File

If `scriptfiles/WorldSync.db` is not created, the Databases component is probably not loaded. WorldSync will fall back to `scriptfiles/WorldSync.entities`.

### Doors Save But Are Invisible

The Objects component is missing. WorldSync can persist the door entity, but cannot create the open.mp object.

### Patrols Return 0

The NPC component is missing, the route ID is invalid, or the NPC ID does not exist.
