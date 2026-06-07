# WorldSync

WorldSync is an open.mp component/plugin that provides reusable infrastructure for persistent worlds: entity state, dirty tracking, periodic saves, simulation ticks and Pawn natives.

This repository currently contains the first framework slice:

- In-memory world entities with type, position, world/interior and key/value state.
- Dirty tracking for changed entities.
- Periodic autosave from the open.mp tick loop.
- SQLite entity loading/saving at `scriptfiles/WorldSync.db` when the open.mp Databases component is loaded.
- SQLite schema versioning through `worlds_meta.schema_version`.
- Incremental SQLite saves for dirty entities and deleted IDs.
- File fallback at `scriptfiles/WorldSync.entities` when Databases is unavailable.
- Stable entity IDs across server restarts.
- Pawn natives exposed through `include/worldsync.inc`.
- A first reusable simulation rule: entities of type `crop` grow from `growth=0` to `growth=100`.

## Pawn Usage

```pawn
#include <worldsync>

public OnGameModeInit()
{
	for (new i = 0, count = WS_GetEntityCount(); i < count; i++)
	{
		new entityid = WS_GetEntityIDAt(i);
		new type[32];
		WS_GetEntityType(entityid, type);
		printf("Loaded WorldSync entity %d (%s)", entityid, type);
	}

	new crop = WS_CreateEntity("crop", 1500.0, -1200.0, 20.0);
	WS_SetState(crop, "growth", "0");
	WS_SetSimulated(crop, true);

	printf("WorldSync entities: %d", WS_GetStats(WS_STAT_ENTITIES));
	return 1;
}
```

## Natives

```pawn
native WS_Load();
native WS_CreateEntity(const type[], Float:x, Float:y, Float:z, world = 0, interior = 0);
native WS_DestroyEntity(entityid);
native bool:WS_EntityExists(entityid);
native WS_SetState(entityid, const key[], const value[]);
native WS_GetState(entityid, const key[], value[], size = sizeof(value));
native WS_GetEntityType(entityid, type[], size = sizeof(type));
native WS_GetEntityPos(entityid, &Float:x, &Float:y, &Float:z);
native WS_GetEntityWorld(entityid);
native WS_GetEntityInterior(entityid);
native WS_GetEntityCount();
native WS_GetEntityIDAt(index);
native WS_SetSimulated(entityid, bool:enabled);
native WS_Save(bool:dirtyOnly = true);
native WS_GetStats(stat);
```

## Door Module

```pawn
new door = WS_CreateDoor(1495, 1520.0, -1675.0, 13.5, 0.0, 0.0, 0.0, 0.0, 0.0, 90.0);
new objectid = WS_GetDoorObject(door);
WS_SetDoorLocked(door, false);
WS_SetDoorOpen(door, true);

public OnDoorStateChange(doorid, bool:isOpen)
{
	printf("Door %d changed: %d", doorid, isOpen);
	return 1;
}
```

Doors are backed by real open.mp objects. Loaded door entities are rebuilt automatically when WorldSync starts, and `WS_SetDoorOpen` rotates the object between the closed and open rotations.

## Crop Module

```pawn
new crop = WS_CreateCrop("corn", 1500.0, -1200.0, 20.0, 0.5, 3);

public OnWorldSyncCropReady(cropid)
{
	printf("Crop %d is ready", cropid);
	return 1;
}
```

## Pathfinding / NPC AI

WorldSync stores path nodes as persistent entities and uses A* to build routes between connected nodes.

```pawn
new a = WS_CreatePathNode(1500.0, -1200.0, 20.0);
new b = WS_CreatePathNode(1520.0, -1200.0, 20.0);
new c = WS_CreatePathNode(1520.0, -1180.0, 20.0);

WS_ConnectPathNodes(a, b);
WS_ConnectPathNodes(b, c);

new route = WS_FindPath(a, c);
for (new i = 0, len = WS_GetPathLength(route); i < len; i++)
{
	new Float:x, Float:y, Float:z;
	WS_GetPathPoint(route, i, x, y, z);
	printf("Route point %d: %.2f %.2f %.2f", i, x, y, z);
}
```

If the open.mp NPC component is loaded, a route can be converted to a native NPC path or assigned directly:

```pawn
WS_MoveNPCByPath(npcid, route, WS_NPC_MOVE_WALK);
```

For simple movement, `WS_NPCGoTo` finds the closest node to the NPC and destination, calculates a route, and starts moving:

```pawn
new route = WS_NPCGoTo(npcid, 1540.0, -1180.0, 20.0, -1, -1, 80.0, WS_NPC_MOVE_WALK);
```

Patrols wrap a route into reusable NPC behavior:

```pawn
new patrol = WS_CreatePatrol(npcid, route, true, WS_NPC_MOVE_WALK);
WS_StartPatrol(patrol);

public OnWorldSyncPatrolPoint(patrolid, npcid, pointIndex)
{
	printf("Patrol %d reached point %d", patrolid, pointIndex);
	return 1;
}

public OnWorldSyncPatrolComplete(patrolid, npcid, routeid)
{
	printf("Patrol %d completed route %d", patrolid, routeid);
	return 1;
}
```

## Tests

```powershell
cmake --build build --config Release --target WorldSyncTests
ctest --test-dir build -C Release --output-on-failure
```

## Logging / Debugging

```pawn
WS_SetLogLevel(WS_LOG_DEBUG);
WS_SetDebug(true);
WS_DebugSummary();

printf("Dirty entities: %d", WS_GetStats(WS_STAT_DIRTY_ENTITIES));
printf("Last saved entities: %d", WS_GetStats(WS_STAT_LAST_SAVE_COUNT));
printf("Last changed entities: %d", WS_GetStats(WS_STAT_LAST_SAVE_CHANGED));
```

Log levels:

```pawn
WS_LOG_ERROR
WS_LOG_WARNING
WS_LOG_INFO
WS_LOG_DEBUG
```

## Roadmap

- Add migration steps for future schema versions.
- Add typed components for vehicles, businesses, resources and production chains.
- Add higher-level NPC routines such as workers and player following.
- Add Pawn callbacks for simulation events such as crop ready, stock changed or production completed.
- Add tests for the core simulation and persistence layer.
