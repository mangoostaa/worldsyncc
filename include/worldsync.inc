#if defined _worldsync_included
	#endinput
#endif
#define _worldsync_included

#define WS_STAT_ENTITIES           (0)
#define WS_STAT_DIRTY_ENTITIES     (1)
#define WS_STAT_SIMULATED_ENTITIES (2)
#define WS_STAT_SAVES              (3)
#define WS_STAT_PENDING_DELETES    (4)
#define WS_STAT_LOADS              (5)
#define WS_STAT_LAST_LOAD_COUNT    (6)
#define WS_STAT_LAST_SAVE_COUNT    (7)
#define WS_STAT_LAST_SAVE_CHANGED  (8)

#define WS_LOG_ERROR               (0)
#define WS_LOG_WARNING             (1)
#define WS_LOG_INFO                (2)
#define WS_LOG_DEBUG               (3)

#define WS_NPC_MOVE_NONE           (0)
#define WS_NPC_MOVE_WALK           (1)
#define WS_NPC_MOVE_JOG            (2)
#define WS_NPC_MOVE_SPRINT         (3)
#define WS_NPC_MOVE_DRIVE          (4)
#define WS_NPC_MOVE_AUTO           (5)

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
native WS_SetDebug(bool:enabled);
native WS_SetLogLevel(level);
native WS_GetLogLevel();
native WS_DebugSummary();

native WS_CreateDoor(model, Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz, Float:open_rx, Float:open_ry, Float:open_rz, virtualworld = 0, interior = 0);
native WS_DestroyDoor(doorid);
native WS_SetDoorOpen(doorid, bool:open);
native bool:WS_IsDoorOpen(doorid);
native WS_SetDoorLocked(doorid, bool:locked);
native bool:WS_IsDoorLocked(doorid);
native WS_GetDoorModel(doorid);
native WS_GetDoorObject(doorid);
native WS_GetDoorPos(doorid, &Float:x, &Float:y, &Float:z);
native WS_GetDoorRot(doorid, &Float:rx, &Float:ry, &Float:rz, &Float:open_rx, &Float:open_ry, &Float:open_rz);

native WS_CreateCrop(const species[], Float:x, Float:y, Float:z, Float:growthRate = 1.0, maxHarvests = 0, virtualworld = 0, interior = 0);
native WS_DestroyCrop(cropid);
native WS_GetCropGrowth(cropid);
native WS_SetCropGrowth(cropid, value);
native bool:WS_IsCropReady(cropid);
native WS_HarvestCrop(cropid);
native WS_GetCropHarvests(cropid);
native WS_GetCropSpecies(cropid, species[], size = sizeof(species));

native WS_CreatePathNode(Float:x, Float:y, Float:z, virtualworld = 0, interior = 0);
native WS_ConnectPathNodes(fromNode, toNode, bool:bidirectional = true, Float:cost = 0.0);
native WS_DisconnectPathNodes(fromNode, toNode, bool:bidirectional = true);
native WS_GetNearestPathNode(Float:x, Float:y, Float:z, virtualworld = 0, interior = 0, Float:maxDistance = 50.0);
native WS_FindPath(startNode, endNode);
native WS_DestroyPath(pathid);
native WS_GetPathLength(pathid);
native WS_GetPathNode(pathid, index);
native WS_GetPathPoint(pathid, index, &Float:x, &Float:y, &Float:z);
native WS_CreateNPCPath(pathid, Float:stopRange = 1.0);
native WS_MoveNPCByPath(npcid, pathid, moveType = WS_NPC_MOVE_AUTO, Float:speed = -1.0, bool:reverse = false);
native WS_NPCGoTo(npcid, Float:x, Float:y, Float:z, virtualworld = -1, interior = -1, Float:nodeSearchRadius = 80.0, moveType = WS_NPC_MOVE_AUTO, Float:speed = -1.0);
native WS_SetPathDebug(bool:enabled);
native WS_DebugPathRoute(pathid);
native WS_ClearPathDebug();

native WS_CreatePatrol(npcid, pathid, bool:loop = true, moveType = WS_NPC_MOVE_AUTO, Float:speed = -1.0);
native WS_StartPatrol(patrolid);
native WS_StopPatrol(patrolid);
native WS_PausePatrol(patrolid);
native WS_ResumePatrol(patrolid);
native WS_DestroyPatrol(patrolid);
native bool:WS_IsPatrolActive(patrolid);
native WS_GetPatrolRoute(patrolid);
native WS_GetPatrolNPC(patrolid);

forward OnDoorStateChange(doorid, bool:isOpen);
forward OnWorldSyncCropReady(cropid);
forward OnWorldSyncPatrolStart(patrolid, npcid, routeid);
forward OnWorldSyncPatrolPoint(patrolid, npcid, pointIndex);
forward OnWorldSyncPatrolComplete(patrolid, npcid, routeid);
