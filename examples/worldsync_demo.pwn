#include <a_samp>
#include <worldsync>

new gHouseDoor;
new gHouseCrop;
new gGuardRoute;
new gGuardPatrol;

stock WorldSyncDemo_CreateHouseDoor()
{
	gHouseDoor = WS_CreateDoor(
		1495,
		1520.0, -1675.0, 13.5,
		0.0, 0.0, 0.0,
		0.0, 0.0, 90.0
	);

	if (!gHouseDoor)
	{
		printf("[WorldSyncDemo] Could not create house door.");
		return 0;
	}

	WS_SetDoorLocked(gHouseDoor, true);
	WS_SetDoorOpen(gHouseDoor, false);
	printf("[WorldSyncDemo] Door entity: %d object: %d", gHouseDoor, WS_GetDoorObject(gHouseDoor));
	return gHouseDoor;
}

stock WorldSyncDemo_CreateFarm()
{
	gHouseCrop = WS_CreateCrop("corn", 1510.0, -1688.0, 13.5, 0.35, 5);
	printf("[WorldSyncDemo] Crop entity: %d", gHouseCrop);
	return gHouseCrop;
}

stock WorldSyncDemo_CreateGuardRoute()
{
	new a = WS_CreatePathNode(1505.0, -1690.0, 13.5);
	new b = WS_CreatePathNode(1535.0, -1690.0, 13.5);
	new c = WS_CreatePathNode(1535.0, -1660.0, 13.5);
	new d = WS_CreatePathNode(1505.0, -1660.0, 13.5);

	WS_ConnectPathNodes(a, b);
	WS_ConnectPathNodes(b, c);
	WS_ConnectPathNodes(c, d);
	WS_ConnectPathNodes(d, a);

	gGuardRoute = WS_FindPath(a, c);
	printf("[WorldSyncDemo] Guard route: %d length: %d", gGuardRoute, WS_GetPathLength(gGuardRoute));
	return gGuardRoute;
}

stock WorldSyncDemo_StartGuardPatrol(npcid)
{
	if (!gGuardRoute)
	{
		printf("[WorldSyncDemo] Guard route is not ready.");
		return 0;
	}

	gGuardPatrol = WS_CreatePatrol(npcid, gGuardRoute, true, WS_NPC_MOVE_WALK);
	if (!gGuardPatrol)
	{
		printf("[WorldSyncDemo] Could not create patrol for NPC %d.", npcid);
		return 0;
	}

	WS_StartPatrol(gGuardPatrol);
	printf("[WorldSyncDemo] Patrol %d started for NPC %d.", gGuardPatrol, npcid);
	return 1;
}

stock WorldSyncDemo_PrintNearbyCrops(Float:x, Float:y, Float:z)
{
	new ids[16];
	new count = WS_GetEntitiesInRange(x, y, z, ids, sizeof(ids), 0, 0, 40.0, "crop");

	printf("[WorldSyncDemo] Nearby crops: %d", count);
	for (new i = 0; i < count; i++)
	{
		new species[24];
		WS_GetCropSpecies(ids[i], species);
		printf("[WorldSyncDemo] crop entity=%d species=%s growth=%d", ids[i], species, WS_GetCropGrowth(ids[i]));
	}
	return count;
}

public OnGameModeInit()
{
	WS_SetLogLevel(WS_LOG_INFO);
	WS_DebugSummary();

	WorldSyncDemo_CreateHouseDoor();
	WorldSyncDemo_CreateFarm();
	WorldSyncDemo_CreateGuardRoute();

	WorldSyncDemo_PrintNearbyCrops(1510.0, -1688.0, 13.5);
	WS_Save(false);
	return 1;
}

public OnDoorStateChange(doorid, bool:isOpen)
{
	printf("[WorldSyncDemo] Door %d open=%d", doorid, isOpen);
	return 1;
}

public OnWorldSyncEntityCreated(entityid, const type[])
{
	printf("[WorldSyncDemo] Entity created: id=%d type=%s", entityid, type);
	return 1;
}

public OnWorldSyncEntityDestroyed(entityid, const type[])
{
	printf("[WorldSyncDemo] Entity destroyed: id=%d type=%s", entityid, type);
	return 1;
}

public OnWorldSyncEntityStateChange(entityid, const key[], const oldValue[], const newValue[])
{
	if (!strcmp(key, "growth") || !strcmp(key, "open") || !strcmp(key, "locked"))
	{
		printf("[WorldSyncDemo] Entity %d state %s: %s -> %s", entityid, key, oldValue, newValue);
	}
	return 1;
}

public OnWorldSyncLoaded(entityCount, bool:storageAvailable)
{
	printf("[WorldSyncDemo] Loaded entities=%d storage=%d", entityCount, storageAvailable);
	return 1;
}

public OnWorldSyncSaved(entityCount, changedCount, bool:dirtyOnly)
{
	printf("[WorldSyncDemo] Saved entities=%d changed=%d dirtyOnly=%d", entityCount, changedCount, dirtyOnly);
	return 1;
}

public OnWorldSyncCropReady(cropid)
{
	new species[24];
	WS_GetCropSpecies(cropid, species);
	printf("[WorldSyncDemo] Crop ready: %d species=%s", cropid, species);
	return 1;
}

public OnWorldSyncPatrolStart(patrolid, npcid, routeid)
{
	printf("[WorldSyncDemo] Patrol started: patrol=%d npc=%d route=%d", patrolid, npcid, routeid);
	return 1;
}

public OnWorldSyncPatrolPoint(patrolid, npcid, pointIndex)
{
	printf("[WorldSyncDemo] Patrol point: patrol=%d npc=%d point=%d", patrolid, npcid, pointIndex);
	return 1;
}

public OnWorldSyncPatrolComplete(patrolid, npcid, routeid)
{
	printf("[WorldSyncDemo] Patrol complete: patrol=%d npc=%d route=%d", patrolid, npcid, routeid);
	return 1;
}
