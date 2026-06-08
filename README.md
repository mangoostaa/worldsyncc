# WorldSync

WorldSync is an open.mp component/plugin for persistent world systems. It gives Pawn scripts a reusable entity layer with persistent state, spatial queries, doors, crops, pathfinding and NPC patrol helpers.

The goal is to let a gamemode build systems like houses, farms, guarded zones, resources or businesses without rewriting storage and world-state plumbing every time.

## Features

- Persistent entities with `id`, `type`, position, virtual world, interior and key/value state.
- Dirty tracking and autosave from the open.mp tick loop.
- SQLite storage at `scriptfiles/WorldSync.db` when the Databases component is loaded.
- File fallback at `scriptfiles/WorldSync.entities` when Databases is unavailable.
- Stable entity IDs across server restarts.
- Spatial grid queries for nearby entities and nearest-entity lookup.
- General Pawn callbacks for entity creation, destruction, state changes, load and save.
- Door module backed by real open.mp objects.
- Vehicle module backed by real open.mp vehicles.
- Crop module with growth, harvests and ready callbacks.
- Path node storage, A* routes, route cache and optional visual debug labels.
- NPC route movement and patrol helpers when the NPC component is loaded.
- Pawn include at `include/worldsync.inc`.

## Quick Example

```pawn
#include <worldsync>

public OnGameModeInit()
{
	WS_SetLogLevel(WS_LOG_INFO);

	new door = WS_CreateDoor(
		1495,
		1520.0, -1675.0, 13.5,
		0.0, 0.0, 0.0,
		0.0, 0.0, 90.0
	);
	WS_SetDoorLocked(door, true);

	new car = WS_CreateVehicle(411, 1528.0, -1680.0, 13.4, 90.0, 1, 1);
	printf("Created persistent vehicle entity %d runtime %d", car, WS_GetVehicleID(car));

	new crop = WS_CreateCrop("corn", 1500.0, -1200.0, 20.0, 0.5, 3);
	printf("Created crop %d, total WorldSync entities: %d", crop, WS_GetEntityCount());

	new nearest = WS_GetNearestEntity(1500.0, -1200.0, 20.0, 0, 0, 30.0, "crop");
	printf("Nearest crop: %d", nearest);
	return 1;
}
```

## Documentation

- [Installation](docs/installation.md)
- [Complete native reference](docs/natives.md)
- [15 minute houses, doors and guards guide](docs/quickstart-houses-doors-guards.md)
- [Example gamemode / filterscript](docs/examples.md)
- [Release and precompiled build guide](docs/releases.md)
- [Contributing](CONTRIBUTING.md)
- [Roadmap](ROADMAP.md)

## Example

The main Pawn example is in [examples/worldsync_demo.pwn](examples/worldsync_demo.pwn). It creates:

- A basic house entrance with a persistent door.
- A crop field with harvest callbacks.
- Path nodes around a guarded zone.
- A reusable patrol route for an existing NPC.

## Build

Windows PowerShell:

```powershell
cd C:\path\to\open.mp-sdk\WorldSync
$p = $env:Path
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $p, 'Process')
cmake --build build --config Release --target WorldSync
```

Tests:

```powershell
cmake --build build --config Release --target WorldSyncTests
ctest --test-dir build -C Release --output-on-failure
```

The Windows plugin output is:

```text
build/Release/WorldSync.dll
```

## Storage

WorldSync tries SQLite first. If the open.mp Databases component is loaded, data is stored at:

```text
scriptfiles/WorldSync.db
```

If Databases is not loaded, WorldSync uses:

```text
scriptfiles/WorldSync.entities
```

## Status

This repository is ready for early adoption testing. The public Pawn API is usable, but future releases may add schema migrations and more typed modules for vehicles, businesses, production chains and richer NPC routines.

## License

WorldSync is released under the [MIT License](LICENSE).
