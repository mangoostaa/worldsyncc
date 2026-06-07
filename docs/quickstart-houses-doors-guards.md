# 15 Minute Guide: Houses, Doors And Guards

This guide builds a small persistent house zone:

- One locked house door.
- One crop field near the house.
- Four path nodes around the property.
- One guard patrol route.

The NPC must already exist in the NPC component. WorldSync controls movement and patrols, but does not create NPC accounts for you.

## Minute 0-2: Include WorldSync

```pawn
#include <worldsync>

new gHouseDoor;
new gHouseCrop;
new gGuardRoute;
new gGuardPatrol;
```

## Minute 2-5: Create The Door

```pawn
stock SetupHouseDoor()
{
	gHouseDoor = WS_CreateDoor(
		1495,
		1520.0, -1675.0, 13.5,
		0.0, 0.0, 0.0,
		0.0, 0.0, 90.0
	);

	WS_SetDoorLocked(gHouseDoor, true);
	WS_SetDoorOpen(gHouseDoor, false);
	return gHouseDoor;
}
```

Use the door state from commands, pickups or key systems:

```pawn
stock ToggleHouseDoor()
{
	if (WS_IsDoorLocked(gHouseDoor))
	{
		return 0;
	}

	WS_SetDoorOpen(gHouseDoor, !WS_IsDoorOpen(gHouseDoor));
	return 1;
}
```

## Minute 5-7: Add Crops

```pawn
stock SetupHouseFarm()
{
	gHouseCrop = WS_CreateCrop("corn", 1510.0, -1688.0, 13.5, 0.35, 5);
	return gHouseCrop;
}

public OnWorldSyncCropReady(cropid)
{
	if (cropid == gHouseCrop)
	{
		printf("House crop is ready to harvest.");
	}
	return 1;
}
```

## Minute 7-11: Add Path Nodes

```pawn
stock SetupGuardRoute()
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
	return gGuardRoute;
}
```

For debugging:

```pawn
WS_SetPathDebug(true);
WS_DebugPathRoute(gGuardRoute);
```

## Minute 11-14: Attach A Guard NPC

Call this after your server creates or loads the NPC:

```pawn
stock StartHouseGuard(npcid)
{
	if (!gGuardRoute)
	{
		return 0;
	}

	gGuardPatrol = WS_CreatePatrol(npcid, gGuardRoute, true, WS_NPC_MOVE_WALK);
	if (!gGuardPatrol)
	{
		printf("Could not create guard patrol. Check NPC component and NPC id.");
		return 0;
	}

	WS_StartPatrol(gGuardPatrol);
	return 1;
}
```

## Minute 14-15: Wire It Together

```pawn
public OnGameModeInit()
{
	WS_SetLogLevel(WS_LOG_INFO);
	SetupHouseDoor();
	SetupHouseFarm();
	SetupGuardRoute();
	WS_Save(false);
	return 1;
}
```

## Spatial Lookup Example

Use the spatial grid to find nearby WorldSync entities:

```pawn
stock PrintNearbyCrops(Float:x, Float:y, Float:z)
{
	new ids[16];
	new count = WS_GetEntitiesInRange(x, y, z, ids, sizeof(ids), 0, 0, 30.0, "crop");

	for (new i = 0; i < count; i++)
	{
		printf("Nearby crop entity: %d", ids[i]);
	}
}
```

## Persistence Notes

The examples above create new entities each time `OnGameModeInit` runs. For production systems, store stable IDs in your gamemode config or detect existing entities by type/state on startup.

For early adoption testing, this is acceptable because it demonstrates the full API quickly.
