# Native Reference

Include:

```pawn
#include <worldsync>
```

Return convention: most mutating natives return `1` on success and `0` on failure unless noted otherwise. IDs return `0` when creation or lookup fails.

## Constants

| Constant | Value | Description |
| --- | ---: | --- |
| `WS_STAT_ENTITIES` | 0 | Total loaded entities. |
| `WS_STAT_DIRTY_ENTITIES` | 1 | Entities waiting to be saved. |
| `WS_STAT_SIMULATED_ENTITIES` | 2 | Entities with simulation enabled. |
| `WS_STAT_SAVES` | 3 | Save operation count. |
| `WS_STAT_PENDING_DELETES` | 4 | Deleted entity IDs waiting to be persisted. |
| `WS_STAT_LOADS` | 5 | Load operation count. |
| `WS_STAT_LAST_LOAD_COUNT` | 6 | Entity count from the last load. |
| `WS_STAT_LAST_SAVE_COUNT` | 7 | Entity count from the last save. |
| `WS_STAT_LAST_SAVE_CHANGED` | 8 | Changed/deleted entity count from the last save. |
| `WS_LOG_ERROR` | 0 | Error logs only. |
| `WS_LOG_WARNING` | 1 | Warning and error logs. |
| `WS_LOG_INFO` | 2 | Info, warning and error logs. |
| `WS_LOG_DEBUG` | 3 | Full debug logs. |
| `WS_NPC_MOVE_NONE` | 0 | No movement type override. |
| `WS_NPC_MOVE_WALK` | 1 | Walk. |
| `WS_NPC_MOVE_JOG` | 2 | Jog. |
| `WS_NPC_MOVE_SPRINT` | 3 | Sprint. |
| `WS_NPC_MOVE_DRIVE` | 4 | Drive. |
| `WS_NPC_MOVE_AUTO` | 5 | Let WorldSync/NPC component choose. |

## Core Entity Natives

| Native | Description | Parameters | Example |
| --- | --- | --- | --- |
| `WS_Load()` | Reloads persisted entities from storage. WorldSync also loads automatically on component ready. | None. | `WS_Load();` |
| `WS_CreateEntity(const type[], Float:x, Float:y, Float:z, world = 0, interior = 0)` | Creates a generic persistent entity. | `type`: string type. `x/y/z`: position. `world/interior`: scope. | `new id = WS_CreateEntity("stash", 10.0, 20.0, 5.0);` |
| `WS_DestroyEntity(entityid)` | Deletes an entity and queues the deletion for persistence. | `entityid`: WorldSync entity ID. | `WS_DestroyEntity(id);` |
| `bool:WS_EntityExists(entityid)` | Checks whether an entity exists. | `entityid`: WorldSync entity ID. | `if (WS_EntityExists(id)) { }` |
| `WS_SetState(entityid, const key[], const value[])` | Sets a string state value. If the value did not change, the entity is not marked dirty. | `key`: state key. `value`: state value. | `WS_SetState(id, "owner", "42");` |
| `WS_GetState(entityid, const key[], value[], size = sizeof(value))` | Reads a string state value into `value`. | `value`: output buffer. | `new v[32]; WS_GetState(id, "owner", v);` |
| `WS_SetStateInt(entityid, const key[], value)` | Stores an integer state value. | Entity, key and value. | `WS_SetStateInt(id, "owner", 42);` |
| `WS_GetStateInt(entityid, const key[], fallback = 0)` | Reads an integer state value with fallback. | Entity, key and fallback. | `new owner = WS_GetStateInt(id, "owner", -1);` |
| `WS_SetStateFloat(entityid, const key[], Float:value)` | Stores a float state value. | Entity, key and value. | `WS_SetStateFloat(id, "tax", 12.5);` |
| `Float:WS_GetStateFloat(entityid, const key[], Float:fallback = 0.0)` | Reads a float state value with fallback. | Entity, key and fallback. | `new Float:tax = WS_GetStateFloat(id, "tax");` |
| `WS_SetStateBool(entityid, const key[], bool:value)` | Stores a boolean state value as `1` or `0`. | Entity, key and value. | `WS_SetStateBool(id, "locked", true);` |
| `bool:WS_GetStateBool(entityid, const key[], bool:fallback = false)` | Reads a boolean state value with fallback. | Entity, key and fallback. | `if (WS_GetStateBool(id, "locked")) { }` |
| `WS_GetEntityType(entityid, type[], size = sizeof(type))` | Reads the entity type. | `type`: output buffer. | `new t[32]; WS_GetEntityType(id, t);` |
| `WS_GetEntityPos(entityid, &Float:x, &Float:y, &Float:z)` | Reads the entity position. | `x/y/z`: output references. | `WS_GetEntityPos(id, x, y, z);` |
| `WS_SetEntityPos(entityid, Float:x, Float:y, Float:z)` | Updates entity position and reindexes it in the spatial grid. | Entity ID and new position. | `WS_SetEntityPos(id, x, y, z);` |
| `WS_GetEntityWorld(entityid)` | Gets the entity virtual world. | `entityid`: WorldSync entity ID. | `new vw = WS_GetEntityWorld(id);` |
| `WS_GetEntityInterior(entityid)` | Gets the entity interior. | `entityid`: WorldSync entity ID. | `new intid = WS_GetEntityInterior(id);` |
| `WS_GetEntityCount()` | Returns loaded entity count. | None. | `printf("%d", WS_GetEntityCount());` |
| `WS_GetEntityIDAt(index)` | Returns the entity ID at an index in the loaded entity list. | `index`: zero-based index. | `new id = WS_GetEntityIDAt(i);` |
| `WS_GetNearestEntity(Float:x, Float:y, Float:z, virtualworld = 0, interior = 0, Float:maxDistance = 50.0, const type[] = "")` | Uses the spatial grid to find the closest entity. If `type` is empty, any type matches. | Position, scope, max distance, optional type filter. | `new crop = WS_GetNearestEntity(x, y, z, 0, 0, 25.0, "crop");` |
| `WS_GetEntitiesInRange(Float:x, Float:y, Float:z, entityids[], maxEntities = sizeof(entityids), virtualworld = 0, interior = 0, Float:radius = 50.0, const type[] = "")` | Fills `entityids` with nearby entity IDs and returns the number written. | Output array, max output count, scope, radius, optional type. | `new ids[16]; new n = WS_GetEntitiesInRange(x, y, z, ids, sizeof(ids), 0, 0, 20.0);` |
| `WS_FindEntitiesByState(const type[], const key[], const value[], entityids[], maxEntities = sizeof(entityids))` | Finds loaded entities by optional type plus exact state key/value. Empty `type` matches any type. | Type filter, key, value and output array. | `new ids[16]; new n = WS_FindEntitiesByState("vehicle", "owner", "42", ids);` |
| `WS_CountEntitiesByState(const type[], const key[], const value[])` | Counts loaded entities by optional type plus exact state key/value. | Type filter, key and value. | `new ready = WS_CountEntitiesByState("crop", "ready", "1");` |
| `WS_SetSimulated(entityid, bool:enabled)` | Enables generic simulation ticking for an entity. | `enabled`: true/false. | `WS_SetSimulated(id, true);` |
| `WS_Save(bool:dirtyOnly = true)` | Saves dirty entities or all entities. Returns changed count. | `dirtyOnly`: true for incremental save. | `WS_Save();` |
| `WS_GetStats(stat)` | Reads a stat by constant. | `stat`: `WS_STAT_*`. | `WS_GetStats(WS_STAT_DIRTY_ENTITIES);` |
| `WS_SetDebug(bool:enabled)` | Enables debug mode and debug logs. | `enabled`: true/false. | `WS_SetDebug(true);` |
| `WS_SetLogLevel(level)` | Sets log verbosity. | `level`: `WS_LOG_*`. | `WS_SetLogLevel(WS_LOG_DEBUG);` |
| `WS_GetLogLevel()` | Returns current log level. | None. | `new level = WS_GetLogLevel();` |
| `WS_DebugSummary()` | Prints entity and persistence stats to server log. | None. | `WS_DebugSummary();` |

## Door Natives

| Native | Description | Parameters | Example |
| --- | --- | --- | --- |
| `WS_CreateDoor(model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, Float:open_rx, Float:open_ry, Float:open_rz, virtualworld = 0, interior = 0)` | Creates a persistent door and an open.mp object if Objects is loaded. | Model, position, closed rotation, open rotation, scope. | `new d = WS_CreateDoor(1495, x, y, z, 0.0, 0.0, 0.0, 0.0, 0.0, 90.0);` |
| `WS_DestroyDoor(doorid)` | Destroys a door entity and its object. | `doorid`: WorldSync door entity ID. | `WS_DestroyDoor(d);` |
| `WS_SetDoorOpen(doorid, bool:open)` | Opens or closes the door by rotating the object. | `open`: true/false. | `WS_SetDoorOpen(d, true);` |
| `bool:WS_IsDoorOpen(doorid)` | Returns door open state. | `doorid`: door ID. | `if (WS_IsDoorOpen(d)) { }` |
| `WS_SetDoorLocked(doorid, bool:locked)` | Sets door lock state. | `locked`: true/false. | `WS_SetDoorLocked(d, true);` |
| `bool:WS_IsDoorLocked(doorid)` | Returns lock state. | `doorid`: door ID. | `if (WS_IsDoorLocked(d)) { }` |
| `WS_GetDoorModel(doorid)` | Returns object model ID. | `doorid`: door ID. | `new model = WS_GetDoorModel(d);` |
| `WS_GetDoorObject(doorid)` | Returns backing open.mp object ID. | `doorid`: door ID. | `new objectid = WS_GetDoorObject(d);` |
| `WS_GetDoorPos(doorid, &Float:x, &Float:y, &Float:z)` | Reads door position. | Output references. | `WS_GetDoorPos(d, x, y, z);` |
| `WS_GetDoorRot(doorid, &Float:rx, &Float:ry, &Float:rz, &Float:open_rx, &Float:open_ry, &Float:open_rz)` | Reads closed and open rotations. | Output references. | `WS_GetDoorRot(d, rx, ry, rz, orx, ory, orz);` |

## Crop Natives

| Native | Description | Parameters | Example |
| --- | --- | --- | --- |
| `WS_CreateCrop(const species[], Float:x, Float:y, Float:z, Float:growthRate = 1.0, maxHarvests = 0, virtualworld = 0, interior = 0)` | Creates a persistent crop. Growth reaches 100 over time. | Species, position, growth rate, max harvests, scope. | `new c = WS_CreateCrop("corn", x, y, z, 0.5, 3);` |
| `WS_DestroyCrop(cropid)` | Destroys a crop entity. | `cropid`: crop entity ID. | `WS_DestroyCrop(c);` |
| `WS_GetCropGrowth(cropid)` | Returns growth from `0` to `100`. | `cropid`: crop ID. | `new g = WS_GetCropGrowth(c);` |
| `WS_SetCropGrowth(cropid, value)` | Sets growth. Values are clamped by the module. | `value`: growth value. | `WS_SetCropGrowth(c, 100);` |
| `bool:WS_IsCropReady(cropid)` | Returns true when growth is 100. | `cropid`: crop ID. | `if (WS_IsCropReady(c)) { }` |
| `WS_HarvestCrop(cropid)` | Harvests a ready crop. Growth resets or crop is destroyed when max harvests is reached. | `cropid`: crop ID. | `WS_HarvestCrop(c);` |
| `WS_GetCropHarvests(cropid)` | Returns current harvest count. | `cropid`: crop ID. | `new h = WS_GetCropHarvests(c);` |
| `WS_GetCropSpecies(cropid, species[], size = sizeof(species))` | Reads crop species. | Output buffer. | `new s[24]; WS_GetCropSpecies(c, s);` |

## Vehicle Natives

| Native | Description | Parameters | Example |
| --- | --- | --- | --- |
| `WS_CreateVehicle(model, Float:x, Float:y, Float:z, Float:zRot = 0.0, colour1 = -1, colour2 = -1, respawnDelay = -1, bool:siren = false, virtualworld = 0, interior = 0)` | Creates a persistent vehicle entity and a real open.mp vehicle if the Vehicles component is loaded. | Model, position, Z rotation, colours, respawn delay, siren, scope. | `new v = WS_CreateVehicle(411, x, y, z, 90.0, 1, 1);` |
| `WS_DestroyVehicle(vehicleEntityID)` | Destroys the persistent vehicle and releases its runtime vehicle. | `vehicleEntityID`: WorldSync vehicle entity ID. | `WS_DestroyVehicle(v);` |
| `WS_GetVehicleID(vehicleEntityID)` | Returns the backing open.mp vehicle ID, or `0` if not spawned. | `vehicleEntityID`: WorldSync vehicle entity ID. | `new sampveh = WS_GetVehicleID(v);` |
| `WS_GetVehicleModel(vehicleEntityID)` | Returns the model ID stored for the vehicle. | `vehicleEntityID`: WorldSync vehicle entity ID. | `new model = WS_GetVehicleModel(v);` |
| `WS_GetVehiclePos(vehicleEntityID, &Float:x, &Float:y, &Float:z)` | Reads the persisted spawn position. | Output references. | `WS_GetVehiclePos(v, x, y, z);` |
| `Float:WS_GetVehicleZRot(vehicleEntityID)` | Returns persisted spawn Z rotation. | `vehicleEntityID`: WorldSync vehicle entity ID. | `new Float:rot = WS_GetVehicleZRot(v);` |
| `WS_SetVehicleHealth(vehicleEntityID, Float:health)` | Persists health and applies it to the runtime vehicle if spawned. | Health value. | `WS_SetVehicleHealth(v, 850.0);` |
| `Float:WS_GetVehicleHealth(vehicleEntityID)` | Returns runtime health when available, otherwise persisted health. | `vehicleEntityID`: WorldSync vehicle entity ID. | `new Float:hp = WS_GetVehicleHealth(v);` |
| `WS_SetVehicleColours(vehicleEntityID, colour1, colour2)` | Persists and applies vehicle colours. | Two GTA colour IDs. | `WS_SetVehicleColours(v, 3, 6);` |
| `WS_GetVehicleColours(vehicleEntityID, &colour1, &colour2)` | Reads persisted colours. | Output references. | `WS_GetVehicleColours(v, c1, c2);` |
| `WS_RespawnVehicle(vehicleEntityID)` | Respawns the runtime vehicle or creates it if missing. | `vehicleEntityID`: WorldSync vehicle entity ID. | `WS_RespawnVehicle(v);` |
| `WS_RepairVehicle(vehicleEntityID)` | Repairs the runtime vehicle and updates persisted health. | `vehicleEntityID`: WorldSync vehicle entity ID. | `WS_RepairVehicle(v);` |

## Pathfinding And NPC Natives

| Native | Description | Parameters | Example |
| --- | --- | --- | --- |
| `WS_CreatePathNode(Float:x, Float:y, Float:z, virtualworld = 0, interior = 0)` | Creates a persistent path node. | Position and scope. | `new a = WS_CreatePathNode(x, y, z);` |
| `WS_CreatePathObstacle(Float:x, Float:y, Float:z, Float:width, Float:depth, Float:margin = 1.0, virtualworld = 0, interior = 0)` | Creates a persistent 2D navigation blocker used by path edges. | Center, box size, safety margin and scope. | `new wall = WS_CreatePathObstacle(x, y, z, 10.0, 4.0);` |
| `WS_DestroyPathObstacle(obstacleid)` | Removes a navigation obstacle and clears route cache. | Obstacle entity ID. | `WS_DestroyPathObstacle(wall);` |
| `bool:WS_IsPathBlocked(Float:fromX, Float:fromY, Float:fromZ, Float:toX, Float:toY, Float:toZ, virtualworld = 0, interior = 0)` | Checks if a straight segment crosses any path obstacle in that scope. | Segment endpoints and scope. | `if (WS_IsPathBlocked(ax, ay, az, bx, by, bz)) { }` |
| `WS_ConnectPathNodes(fromNode, toNode, bool:bidirectional = true, Float:cost = 0.0)` | Connects two nodes. Cost defaults to distance. | Node IDs, direction flag, optional cost. | `WS_ConnectPathNodes(a, b);` |
| `WS_DisconnectPathNodes(fromNode, toNode, bool:bidirectional = true)` | Removes an edge. | Node IDs, direction flag. | `WS_DisconnectPathNodes(a, b);` |
| `WS_GetNearestPathNode(Float:x, Float:y, Float:z, virtualworld = 0, interior = 0, Float:maxDistance = 50.0)` | Finds the closest path node using the spatial grid. | Position, scope, radius. | `new n = WS_GetNearestPathNode(x, y, z);` |
| `WS_FindPath(startNode, endNode)` | Builds or reuses an A* route. | Start and end node IDs. | `new route = WS_FindPath(a, c);` |
| `WS_DestroyPath(pathid)` | Destroys a stored route instance. | `pathid`: route ID. | `WS_DestroyPath(route);` |
| `WS_ClearPathCache()` | Clears cached A* results. | None. | `WS_ClearPathCache();` |
| `WS_GetPathCacheSize()` | Returns cached route count. | None. | `printf("%d", WS_GetPathCacheSize());` |
| `WS_GetPathLength(pathid)` | Returns number of nodes in route. | `pathid`: route ID. | `new len = WS_GetPathLength(route);` |
| `WS_GetPathNode(pathid, index)` | Returns route node ID at index. | Route ID and zero-based index. | `new node = WS_GetPathNode(route, i);` |
| `WS_GetPathPoint(pathid, index, &Float:x, &Float:y, &Float:z)` | Reads route point position. | Route ID, index, outputs. | `WS_GetPathPoint(route, i, x, y, z);` |
| `WS_CreateNPCPath(pathid, Float:stopRange = 1.0)` | Converts a route to an NPC component path. | Route ID and stop range. | `new np = WS_CreateNPCPath(route);` |
| `WS_MoveNPCByPath(npcid, pathid, moveType = WS_NPC_MOVE_AUTO, Float:speed = -1.0, bool:reverse = false)` | Moves an NPC along a WorldSync route. | NPC ID, route ID, movement options. | `WS_MoveNPCByPath(npcid, route, WS_NPC_MOVE_WALK);` |
| `WS_NPCGoTo(npcid, Float:x, Float:y, Float:z, virtualworld = -1, interior = -1, Float:nodeSearchRadius = 80.0, moveType = WS_NPC_MOVE_AUTO, Float:speed = -1.0)` | Finds nearest start/end nodes, creates route and starts movement. | NPC, destination, optional scope/radius/move settings. | `WS_NPCGoTo(npcid, x, y, z);` |
| `WS_GetNearestNPC(Float:x, Float:y, Float:z, virtualworld = 0, interior = 0, Float:maxDistance = 50.0)` | Uses the runtime NPC spatial grid to find the closest NPC. | Position, scope and max distance. | `new npc = WS_GetNearestNPC(x, y, z, 0, 0, 30.0);` |
| `WS_GetNPCsInRange(Float:x, Float:y, Float:z, npcids[], maxNPCs = sizeof(npcids), virtualworld = 0, interior = 0, Float:radius = 50.0)` | Fills `npcids` with nearby NPC IDs from the runtime NPC spatial grid. | Output array, max output count, scope and radius. | `new ids[8]; new n = WS_GetNPCsInRange(x, y, z, ids, sizeof(ids));` |
| `WS_GetNearestPlayerToNPC(npcid, Float:maxDistance = 50.0, bool:includeBots = false)` | Finds the nearest player to an NPC in the same virtual world and interior. | NPC ID, max distance, bot inclusion flag. | `new playerid = WS_GetNearestPlayerToNPC(npcid, 40.0);` |
| `bool:WS_IsPlayerInNPCSight(npcid, playerid, Float:maxDistance = 50.0, Float:fovDegrees = 120.0, bool:includeBots = false)` | Checks distance, scope and horizontal FOV from an NPC to a player. | NPC ID, player ID, distance, FOV and bot inclusion flag. | `if (WS_IsPlayerInNPCSight(npcid, playerid)) { }` |
| `WS_SetPathDebug(bool:enabled)` | Shows or hides debug labels for path graph. | `enabled`: true/false. | `WS_SetPathDebug(true);` |
| `WS_DebugPathRoute(pathid)` | Shows debug labels for one route. | `pathid`: route ID. | `WS_DebugPathRoute(route);` |
| `WS_ClearPathDebug()` | Removes path debug labels. | None. | `WS_ClearPathDebug();` |
| `WS_CreatePatrol(npcid, pathid, bool:loop = true, moveType = WS_NPC_MOVE_AUTO, Float:speed = -1.0)` | Creates reusable NPC patrol behavior. | NPC ID, route ID, loop flag, movement options. | `new p = WS_CreatePatrol(npcid, route, true);` |
| `WS_StartPatrol(patrolid)` | Starts a patrol. | `patrolid`: patrol ID. | `WS_StartPatrol(p);` |
| `WS_StopPatrol(patrolid)` | Stops a patrol and destroys active NPC path. | `patrolid`: patrol ID. | `WS_StopPatrol(p);` |
| `WS_PausePatrol(patrolid)` | Pauses active patrol movement. | `patrolid`: patrol ID. | `WS_PausePatrol(p);` |
| `WS_ResumePatrol(patrolid)` | Resumes active patrol movement. | `patrolid`: patrol ID. | `WS_ResumePatrol(p);` |
| `WS_DestroyPatrol(patrolid)` | Stops and deletes patrol state. | `patrolid`: patrol ID. | `WS_DestroyPatrol(p);` |
| `bool:WS_IsPatrolActive(patrolid)` | Returns active state. | `patrolid`: patrol ID. | `if (WS_IsPatrolActive(p)) { }` |
| `WS_GetPatrolRoute(patrolid)` | Returns patrol route ID. | `patrolid`: patrol ID. | `new r = WS_GetPatrolRoute(p);` |
| `WS_GetPatrolNPC(patrolid)` | Returns patrol NPC ID. | `patrolid`: patrol ID. | `new npc = WS_GetPatrolNPC(p);` |

## Callbacks

```pawn
forward OnDoorStateChange(doorid, bool:isOpen);
forward OnWorldSyncEntityCreated(entityid, const type[]);
forward OnWorldSyncEntityDestroyed(entityid, const type[]);
forward OnWorldSyncEntityStateChange(entityid, const key[], const oldValue[], const newValue[]);
forward OnWorldSyncLoaded(entityCount, bool:storageAvailable);
forward OnWorldSyncSaved(entityCount, changedCount, bool:dirtyOnly);
forward OnWorldSyncCropReady(cropid);
forward OnWorldSyncPatrolStart(patrolid, npcid, routeid);
forward OnWorldSyncPatrolPoint(patrolid, npcid, pointIndex);
forward OnWorldSyncPatrolComplete(patrolid, npcid, routeid);
```

| Callback | Description |
| --- | --- |
| `OnDoorStateChange(doorid, bool:isOpen)` | Called when a WorldSync door opens or closes. |
| `OnWorldSyncEntityCreated(entityid, const type[])` | Called after a new entity is created by any module. |
| `OnWorldSyncEntityDestroyed(entityid, const type[])` | Called after an entity is destroyed. The entity no longer exists, so the type is passed directly. |
| `OnWorldSyncEntityStateChange(entityid, const key[], const oldValue[], const newValue[])` | Called after `WS_SetState` or a module state update. Can be frequent for simulated systems. |
| `OnWorldSyncLoaded(entityCount, bool:storageAvailable)` | Called after a load attempt. `storageAvailable` is false when no previous storage exists. |
| `OnWorldSyncSaved(entityCount, changedCount, bool:dirtyOnly)` | Called after a successful save. |
| `OnWorldSyncCropReady(cropid)` | Called when a crop reaches 100 growth for the current cycle. |
| `OnWorldSyncPatrolStart(patrolid, npcid, routeid)` | Called when a patrol starts moving. |
| `OnWorldSyncPatrolPoint(patrolid, npcid, pointIndex)` | Called when a patrol reaches a route point. |
| `OnWorldSyncPatrolComplete(patrolid, npcid, routeid)` | Called when a patrol route completes. |

Example:

```pawn
public OnWorldSyncEntityStateChange(entityid, const key[], const oldValue[], const newValue[])
{
	printf("Entity %d state changed: %s %s -> %s", entityid, key, oldValue, newValue);
	return 1;
}

public OnWorldSyncCropReady(cropid)
{
	new species[24];
	WS_GetCropSpecies(cropid, species);
	printf("Crop %d is ready (%s)", cropid, species);
	return 1;
}
```
