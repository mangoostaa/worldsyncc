#include <open.mp>
#include <worldsync>

#define COLOR_GREEN  0x33DD66FF
#define COLOR_YELLOW 0xFFD447FF
#define COLOR_RED    0xFF5544FF
#define COLOR_BLUE   0x55AAFFFF
#define COLOR_WHITE  0xFFFFFFFF
#define COLOR_GREY   0xBBBBBBFF

#define DEMO_WORLD   (0)
#define DEMO_INT     (0)

new gDoor;
new gCar;
new gCrop;
new gRoute;
new gPathA;
new gPathB;
new gPathC;
new gPathD;
new gObstacle;
new gCropPickup = -1;
new gGuardNpc = INVALID_NPC_ID;
new gGuardPatrol;

forward WorldSyncDemo_Init();
forward WorldSyncDemo_StartNPC();
forward WorldSyncDemo_Tick();

main()
{
	print("[Derby/WorldSync] AMX cargado.");
}

stock DemoMsg(playerid, colour, const text[])
{
	SendClientMessage(playerid, colour, text);
	return 1;
}

stock DemoMsgAll(colour, const text[])
{
	SendClientMessageToAll(colour, text);
	return 1;
}

stock DemoFindByState(const type[], const key[], const value[])
{
	new ids[8];
	new count = WS_FindEntitiesByState(type, key, value, ids, sizeof(ids));
	return count > 0 ? ids[0] : 0;
}

stock DemoDestroyTyped(const type[], entity)
{
	if (!strcmp(type, "door"))
	{
		return WS_DestroyDoor(entity);
	}
	if (!strcmp(type, "vehicle"))
	{
		return WS_DestroyVehicle(entity);
	}
	if (!strcmp(type, "crop"))
	{
		return WS_DestroyCrop(entity);
	}
	return WS_DestroyEntity(entity);
}

stock DemoKeepOneByState(const type[], const value[])
{
	new ids[32];
	new count = WS_FindEntitiesByState(type, "demo", value, ids, sizeof(ids));
	if (count <= 0)
	{
		return 0;
	}

	new keep = ids[0];
	for (new i = 1; i < count; i++)
	{
		DemoDestroyTyped(type, ids[i]);
	}
	if (count > 1)
	{
		printf("[Derby/WorldSync] Removed %d duplicate %s demo=%s, keeping entity=%d", count - 1, type, value, keep);
	}
	return keep;
}

stock DemoCreateLabel(const text[], Float:x, Float:y, Float:z)
{
	Create3DTextLabel(text, COLOR_YELLOW, x, y, z, 35.0, DEMO_WORLD, true);
}

stock bool:DemoIsNear(Float:x, Float:y, Float:z, Float:tx, Float:ty, Float:tz, Float:radius)
{
	new Float:dx = x - tx;
	new Float:dy = y - ty;
	new Float:dz = z - tz;
	return (dx * dx + dy * dy + dz * dz) <= (radius * radius);
}

stock DemoCleanupDemoVehicles()
{
	new keep = DemoFindByState("vehicle", "demo", "inferno");
	new removed;
	new total = WS_GetEntityCount();

	for (new i = total - 1; i >= 0; i--)
	{
		new entity = WS_GetEntityIDAt(i);
		new type[24];
		if (!entity || !WS_GetEntityType(entity, type) || strcmp(type, "vehicle"))
		{
			continue;
		}
		if (WS_GetVehicleModel(entity) != 411)
		{
			continue;
		}

		new Float:x, Float:y, Float:z;
		if (!WS_GetVehiclePos(entity, x, y, z) || !DemoIsNear(x, y, z, 1528.0, -1680.0, 13.4, 8.0))
		{
			continue;
		}

		if (!keep)
		{
			keep = entity;
			WS_SetState(keep, "demo", "inferno");
			WS_SetState(keep, "name", "Persistent Inferno");
			continue;
		}

		if (entity != keep)
		{
			WS_DestroyVehicle(entity);
			removed++;
		}
	}

	if (removed > 0)
	{
		printf("[Derby/WorldSync] Removed %d duplicate demo vehicles, keeping entity=%d", removed, keep);
		WS_Save(false);
	}
	return keep;
}

stock DemoSetupDoor()
{
	gDoor = DemoKeepOneByState("door", "lspd_front");
	if (!gDoor)
	{
		gDoor = WS_CreateDoor(
			1495,
			1520.0, -1675.0, 13.5,
			0.0, 0.0, 0.0,
			0.0, 0.0, 90.0,
			DEMO_WORLD, DEMO_INT
		);
		WS_SetState(gDoor, "demo", "lspd_front");
		WS_SetState(gDoor, "name", "LSPD demo door");
		WS_SetDoorLocked(gDoor, true);
	}

	DemoCreateLabel("WorldSync Door\n/wsdoor abre-cierra\n/wslock bloquea", 1520.0, -1675.0, 15.2);
	printf("[Derby/WorldSync] Door entity=%d object=%d locked=%d open=%d", gDoor, WS_GetDoorObject(gDoor), WS_IsDoorLocked(gDoor), WS_IsDoorOpen(gDoor));
	return gDoor;
}

stock DemoSetupVehicle()
{
	gCar = DemoCleanupDemoVehicles();
	if (!gCar)
	{
		gCar = WS_CreateVehicle(411, 1528.0, -1680.0, 13.4, 90.0, 1, 1, 120, false, DEMO_WORLD, DEMO_INT);
		WS_SetState(gCar, "demo", "inferno");
		WS_SetState(gCar, "name", "Persistent Inferno");
	}

	DemoCreateLabel("WorldSync Vehicle\n/wscar entrar\n/wsrepair reparar", 1528.0, -1680.0, 15.0);
	printf("[Derby/WorldSync] Vehicle entity=%d runtime=%d health=%.1f", gCar, WS_GetVehicleID(gCar), WS_GetVehicleHealth(gCar));
	return gCar;
}

stock DemoSetupCrop()
{
	gCrop = DemoKeepOneByState("crop", "corn_patch");
	if (!gCrop)
	{
		gCrop = WS_CreateCrop("corn", 1512.0, -1688.0, 13.5, 8.0, 5, DEMO_WORLD, DEMO_INT);
		WS_SetState(gCrop, "demo", "corn_patch");
		WS_SetState(gCrop, "name", "Corn patch");
	}

	gCropPickup = CreatePickup(1279, 1, 1512.0, -1688.0, 13.5, DEMO_WORLD);
	DemoCreateLabel("WorldSync Crop\n/wscrop estado\n/wsgrow +25\n/wsharvest cosechar", 1512.0, -1688.0, 15.0);
	printf("[Derby/WorldSync] Crop entity=%d growth=%d harvests=%d", gCrop, WS_GetCropGrowth(gCrop), WS_GetCropHarvests(gCrop));
	return gCrop;
}

stock DemoSetupPath()
{
	new Float:ax = 1502.0, Float:ay = -1677.0, Float:az = 13.5;
	new Float:bx = 1502.0, Float:by = -1704.0, Float:bz = 13.5;
	new Float:cx = 1544.0, Float:cy = -1704.0, Float:cz = 13.5;
	new Float:dx = 1544.0, Float:dy = -1677.0, Float:dz = 13.5;
	new Float:ox = 1523.0, Float:oy = -1677.0, Float:oz = 13.5;

	gPathA = DemoKeepOneByState("path_node", "a");
	gPathB = DemoKeepOneByState("path_node", "b");
	gPathC = DemoKeepOneByState("path_node", "c");
	gPathD = DemoKeepOneByState("path_node", "d");
	gObstacle = DemoKeepOneByState("path_obstacle", "nav_wall");

	if (!gPathA)
	{
		gPathA = WS_CreatePathNode(ax, ay, az, DEMO_WORLD, DEMO_INT);
		WS_SetState(gPathA, "demo", "a");
	}
	if (!gPathB)
	{
		gPathB = WS_CreatePathNode(bx, by, bz, DEMO_WORLD, DEMO_INT);
		WS_SetState(gPathB, "demo", "b");
	}
	if (!gPathC)
	{
		gPathC = WS_CreatePathNode(cx, cy, cz, DEMO_WORLD, DEMO_INT);
		WS_SetState(gPathC, "demo", "c");
	}
	if (!gPathD)
	{
		gPathD = WS_CreatePathNode(dx, dy, dz, DEMO_WORLD, DEMO_INT);
		WS_SetState(gPathD, "demo", "d");
	}
	if (!gObstacle)
	{
		gObstacle = WS_CreatePathObstacle(ox, oy, oz, 24.0, 18.0, 2.0, DEMO_WORLD, DEMO_INT);
		WS_SetState(gObstacle, "demo", "nav_wall");
		WS_SetState(gObstacle, "name", "Navigation wall");
	}
	else
	{
		WS_SetEntityPos(gObstacle, ox, oy, oz);
		WS_SetStateFloat(gObstacle, "width", 24.0);
		WS_SetStateFloat(gObstacle, "depth", 18.0);
		WS_SetStateFloat(gObstacle, "margin", 2.0);
	}

	WS_SetEntityPos(gPathA, ax, ay, az);
	WS_SetEntityPos(gPathB, bx, by, bz);
	WS_SetEntityPos(gPathC, cx, cy, cz);
	WS_SetEntityPos(gPathD, dx, dy, dz);
	WS_DisconnectPathNodes(gPathA, gPathB, true);
	WS_DisconnectPathNodes(gPathB, gPathC, true);
	WS_DisconnectPathNodes(gPathC, gPathD, true);
	WS_DisconnectPathNodes(gPathD, gPathA, true);
	WS_DisconnectPathNodes(gPathA, gPathC, true);
	WS_DisconnectPathNodes(gPathA, gPathD, true);

	WS_ConnectPathNodes(gPathA, gPathB, true, 0.0);
	WS_ConnectPathNodes(gPathB, gPathC, true, 0.0);
	WS_ConnectPathNodes(gPathC, gPathD, true, 0.0);
	new directBlocked = WS_IsPathBlocked(ax, ay, az, dx, dy, dz, DEMO_WORLD, DEMO_INT) ? 1 : 0;
	new directConnected = WS_ConnectPathNodes(gPathA, gPathD, true, 0.0);
	WS_ClearPathCache();
	gRoute = WS_FindPath(gPathA, gPathD);

	CreateObject(19353, 1517.0, -1677.0, 14.2, 0.0, 0.0, 90.0, 300.0);
	CreateObject(19353, 1523.0, -1677.0, 14.2, 0.0, 0.0, 90.0, 300.0);
	CreateObject(19353, 1529.0, -1677.0, 14.2, 0.0, 0.0, 90.0, 300.0);
	DemoCreateLabel("A inicio\nNPC debe bajar", ax, ay, 15.0);
	DemoCreateLabel("B rodeo", bx, by, 15.0);
	DemoCreateLabel("C rodeo", cx, cy, 15.0);
	DemoCreateLabel("D destino", dx, dy, 15.0);
	DemoCreateLabel("Path obstacle\nbloquea A->D directo", ox, oy, 16.0);
	printf("[Derby/WorldSync] Path route=%d length=%d cache=%d obstacle=%d directBlocked=%d directConnected=%d",
		gRoute,
		WS_GetPathLength(gRoute),
		WS_GetPathCacheSize(),
		gObstacle,
		directBlocked,
		directConnected);
	return gRoute;
}

stock DemoSetupNPC()
{
	if (gGuardNpc != INVALID_NPC_ID && NPC_IsValid(gGuardNpc))
	{
		NPC_Destroy(gGuardNpc);
	}

	gGuardNpc = NPC_Create("WS_Guard");
	if (!NPC_IsValid(gGuardNpc))
	{
		printf("[Derby/WorldSync] No se pudo crear NPC WS_Guard, returned=%d. Revisa max_bots.", gGuardNpc);
		gGuardNpc = INVALID_NPC_ID;
		return 0;
	}

	NPC_SetSkin(gGuardNpc, 280);
	NPC_SetPos(gGuardNpc, 1502.0, -1677.0, 13.5);
	NPC_SetVirtualWorld(gGuardNpc, DEMO_WORLD);
	NPC_SetInterior(gGuardNpc, DEMO_INT);
	NPC_SetFacingAngle(gGuardNpc, 90.0);
	NPC_SetInvulnerable(gGuardNpc, true);
	NPC_Spawn(gGuardNpc);

	DemoCreateLabel("WorldSync NPC\n/wsnpc patrol\n/wsnpcgo destino\n/wsnpcstop stop", 1502.0, -1677.0, 16.0);
	printf("[Derby/WorldSync] NPC guard created npcid=%d route=%d", gGuardNpc, gRoute);
	return gGuardNpc;
}

stock DemoStartNPCPatrolCore()
{
	if (gGuardNpc == INVALID_NPC_ID || !NPC_IsValid(gGuardNpc))
	{
		return 0;
	}
	if (!gRoute)
	{
		return 0;
	}

	if (gGuardPatrol && WS_IsPatrolActive(gGuardPatrol))
	{
		WS_StopPatrol(gGuardPatrol);
	}
	if (gGuardPatrol)
	{
		WS_DestroyPatrol(gGuardPatrol);
		gGuardPatrol = 0;
	}

	NPC_SetPos(gGuardNpc, 1502.0, -1677.0, 13.5);
	NPC_SetVirtualWorld(gGuardNpc, DEMO_WORLD);
	NPC_SetInterior(gGuardNpc, DEMO_INT);
	gGuardPatrol = WS_CreatePatrol(gGuardNpc, gRoute, true, WS_NPC_MOVE_WALK, -1.0);
	if (!gGuardPatrol)
	{
		printf("[Derby/WorldSync] NPC patrol create failed npc=%d route=%d", gGuardNpc, gRoute);
		return 0;
	}

	new started = WS_StartPatrol(gGuardPatrol);
	printf("[Derby/WorldSync] NPC patrol start result=%d patrol=%d npc=%d route=%d active=%d",
		started,
		gGuardPatrol,
		gGuardNpc,
		gRoute,
		WS_IsPatrolActive(gGuardPatrol) ? 1 : 0);
	return started;
}

stock DemoStartNPCPatrol(playerid)
{
	if (!DemoStartNPCPatrolCore())
	{
		DemoMsg(playerid, COLOR_RED, "No se pudo iniciar patrol. Revisa NPC component, route y max_bots.");
		return 0;
	}
	DemoMsg(playerid, COLOR_GREEN, "NPC patrol iniciado por nodos WorldSync.");
	return 1;
}

stock DemoShowHelp(playerid)
{
	DemoMsg(playerid, COLOR_YELLOW, "WorldSync demo: /wsdemo /wsstats /wsdoor /wslock /wscar /wsrepair");
	DemoMsg(playerid, COLOR_YELLOW, "WorldSync demo: /wscrop /wsgrow /wsharvest /wsnear /wspath /wsdebug /wssave");
	DemoMsg(playerid, COLOR_YELLOW, "WorldSync NPC: /wsnpc /wsnpcgo /wsnpcstop /wsnpcnear");
	DemoMsg(playerid, COLOR_GREY, "Tip: reinicia el server y vas a ver que las entidades vuelven desde SQLite.");
	return 1;
}

stock DemoTeleport(playerid)
{
	SetPlayerInterior(playerid, DEMO_INT);
	SetPlayerVirtualWorld(playerid, DEMO_WORLD);
	SetPlayerPos(playerid, 1515.0, -1685.0, 13.6);
	SetPlayerFacingAngle(playerid, 45.0);
	SetPlayerCheckpoint(playerid, 1520.0, -1675.0, 13.5, 3.0);
	GameTextForPlayer(playerid, "WorldSync Demo", 2500, 3);
	return 1;
}

stock DemoPrintStats(playerid)
{
	new line[144];
	format(line, sizeof(line), "Entities=%d dirty=%d simulated=%d saves=%d loads=%d lastLoad=%d",
		WS_GetEntityCount(),
		WS_GetStats(WS_STAT_DIRTY_ENTITIES),
		WS_GetStats(WS_STAT_SIMULATED_ENTITIES),
		WS_GetStats(WS_STAT_SAVES),
		WS_GetStats(WS_STAT_LOADS),
		WS_GetStats(WS_STAT_LAST_LOAD_COUNT));
	DemoMsg(playerid, COLOR_BLUE, line);
	return 1;
}

public OnGameModeInit()
{
	SetGameModeText("WorldSync Derby Demo");
	AddPlayerClass(0, 1515.0, -1685.0, 13.6, 45.0);

	WS_SetLogLevel(WS_LOG_INFO);
	WS_DebugSummary();

	print("====================================");
	print("   WorldSync - Interactive Demo");
	print("====================================");

	SetTimer("WorldSyncDemo_Init", 1000, false);
	SetTimer("WorldSyncDemo_Tick", 7000, true);
	return 1;
}

public WorldSyncDemo_Init()
{
	DemoSetupDoor();
	DemoSetupVehicle();
	DemoSetupCrop();
	DemoSetupPath();
	DemoSetupNPC();
	SetTimer("WorldSyncDemo_StartNPC", 2500, false);
	WS_Save(true);
	WS_DebugSummary();
	print("[Derby/WorldSync] Demo lista. Usa /wshelp dentro del server.");
	return 1;
}

public WorldSyncDemo_StartNPC()
{
	DemoStartNPCPatrolCore();
	return 1;
}

public WorldSyncDemo_Tick()
{
	if (gCrop && WS_EntityExists(gCrop))
	{
		printf("[Derby/WorldSync] Crop %d growth=%d ready=%d harvests=%d", gCrop, WS_GetCropGrowth(gCrop), WS_IsCropReady(gCrop), WS_GetCropHarvests(gCrop));
	}
	if (gGuardNpc != INVALID_NPC_ID && NPC_IsValid(gGuardNpc))
	{
		new Float:x, Float:y, Float:z;
		NPC_GetPos(gGuardNpc, x, y, z);
		new patrolActive = (gGuardPatrol != 0 && WS_IsPatrolActive(gGuardPatrol)) ? 1 : 0;
		printf("[Derby/WorldSync] NPC %d pos=%.2f %.2f %.2f nearest=%d patrol=%d active=%d",
			gGuardNpc,
			x, y, z,
			WS_GetNearestNPC(x, y, z, DEMO_WORLD, DEMO_INT, 80.0),
			gGuardPatrol,
			patrolActive);
	}
	return 1;
}

public OnPlayerConnect(playerid)
{
	DemoMsg(playerid, COLOR_GREEN, "WorldSync demo cargada.");
	DemoShowHelp(playerid);
	return 1;
}

public OnPlayerSpawn(playerid)
{
	DemoTeleport(playerid);
	return 1;
}

public OnPlayerEnterCheckpoint(playerid)
{
	DemoMsg(playerid, COLOR_YELLOW, "Checkpoint de puerta: usa /wsdoor o /wslock.");
	return 1;
}

public OnPlayerPickUpPickup(playerid, pickupid)
{
	if (pickupid == gCropPickup)
	{
		new line[96];
		format(line, sizeof(line), "Corn patch: growth=%d ready=%d harvests=%d", WS_GetCropGrowth(gCrop), WS_IsCropReady(gCrop), WS_GetCropHarvests(gCrop));
		DemoMsg(playerid, COLOR_BLUE, line);
	}
	return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
	if (!strcmp(cmdtext, "/wshelp", true) || !strcmp(cmdtext, "/comandos", true))
	{
		return DemoShowHelp(playerid);
	}

	if (!strcmp(cmdtext, "/wsdemo", true))
	{
		DemoTeleport(playerid);
		DemoMsg(playerid, COLOR_GREEN, "Teletransportado a la zona WorldSync.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsstats", true))
	{
		return DemoPrintStats(playerid);
	}

	if (!strcmp(cmdtext, "/wsdoor", true))
	{
		if (!gDoor || !WS_EntityExists(gDoor))
		{
			DemoMsg(playerid, COLOR_RED, "La puerta no existe.");
			return 1;
		}
		if (WS_IsDoorLocked(gDoor))
		{
			DemoMsg(playerid, COLOR_RED, "La puerta esta bloqueada. Usa /wslock.");
			return 1;
		}
		new bool:open = !WS_IsDoorOpen(gDoor);
		WS_SetDoorOpen(gDoor, open);
		DemoMsg(playerid, COLOR_GREEN, open ? "Puerta abierta y estado persistido." : "Puerta cerrada y estado persistido.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wslock", true))
	{
		if (!gDoor || !WS_EntityExists(gDoor))
		{
			DemoMsg(playerid, COLOR_RED, "La puerta no existe.");
			return 1;
		}
		new bool:locked = !WS_IsDoorLocked(gDoor);
		WS_SetDoorLocked(gDoor, locked);
		DemoMsg(playerid, COLOR_YELLOW, locked ? "Puerta bloqueada." : "Puerta desbloqueada.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wscar", true))
	{
		new runtime = WS_GetVehicleID(gCar);
		if (!runtime)
		{
			DemoMsg(playerid, COLOR_RED, "Vehiculo runtime no disponible.");
			return 1;
		}
		PutPlayerInVehicle(playerid, runtime, 0);
		DemoMsg(playerid, COLOR_GREEN, "Subiste al vehiculo persistente.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsrepair", true))
	{
		WS_RepairVehicle(gCar);
		WS_SetVehicleColours(gCar, random(126), random(126));
		DemoMsg(playerid, COLOR_GREEN, "Vehiculo reparado y colores guardados.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wscrop", true))
	{
		new line[128], species[24];
		WS_GetCropSpecies(gCrop, species);
		format(line, sizeof(line), "Crop %d species=%s growth=%d ready=%d harvests=%d",
			gCrop, species, WS_GetCropGrowth(gCrop), WS_IsCropReady(gCrop), WS_GetCropHarvests(gCrop));
		DemoMsg(playerid, COLOR_BLUE, line);
		return 1;
	}

	if (!strcmp(cmdtext, "/wsgrow", true))
	{
		new growth = WS_GetCropGrowth(gCrop) + 25;
		if (growth > 100)
		{
			growth = 100;
		}
		WS_SetCropGrowth(gCrop, growth);
		DemoMsg(playerid, COLOR_GREEN, "Crecimiento del cultivo aumentado.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsharvest", true))
	{
		if (!WS_IsCropReady(gCrop))
		{
			DemoMsg(playerid, COLOR_RED, "El cultivo todavia no esta listo. Usa /wsgrow o espera.");
			return 1;
		}
		WS_HarvestCrop(gCrop);
		DemoMsg(playerid, COLOR_GREEN, "Cultivo cosechado. WorldSync actualizo harvests/growth.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsnear", true))
	{
		new Float:x, Float:y, Float:z;
		new ids[16], type[24], line[144];
		GetPlayerPos(playerid, x, y, z);
		new nearest = WS_GetNearestEntity(x, y, z, DEMO_WORLD, DEMO_INT, 80.0, "");
		new count = WS_GetEntitiesInRange(x, y, z, ids, sizeof(ids), DEMO_WORLD, DEMO_INT, 80.0, "");
		WS_GetEntityType(nearest, type);
		format(line, sizeof(line), "Cerca tuyo: %d entidades. Nearest=%d type=%s", count, nearest, type);
		DemoMsg(playerid, COLOR_BLUE, line);
		return 1;
	}

	if (!strcmp(cmdtext, "/wspath", true))
	{
		new line[128];
		WS_SetPathDebug(true);
		WS_DebugPathRoute(gRoute);
		format(line, sizeof(line), "Ruta=%d len=%d obstaculo=%d A-D bloqueado=%d",
			gRoute,
			WS_GetPathLength(gRoute),
			gObstacle,
			WS_IsPathBlocked(1502.0, -1677.0, 13.5, 1544.0, -1677.0, 13.5, DEMO_WORLD, DEMO_INT) ? 1 : 0);
		DemoMsg(playerid, COLOR_GREEN, line);
		DemoMsg(playerid, COLOR_GREEN, "Debug de path activo. Mira labels/ruta; /wsdebug lo apaga.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsnpc", true))
	{
		return DemoStartNPCPatrol(playerid);
	}

	if (!strcmp(cmdtext, "/wsnpcstop", true))
	{
		if (gGuardPatrol)
		{
			WS_StopPatrol(gGuardPatrol);
			WS_DestroyPatrol(gGuardPatrol);
			gGuardPatrol = 0;
		}
		if (gGuardNpc != INVALID_NPC_ID && NPC_IsValid(gGuardNpc))
		{
			NPC_StopMove(gGuardNpc);
		}
		DemoMsg(playerid, COLOR_YELLOW, "NPC patrol detenido.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsnpcgo", true))
	{
		if (gGuardNpc == INVALID_NPC_ID || !NPC_IsValid(gGuardNpc))
		{
			DemoMsg(playerid, COLOR_RED, "NPC no disponible.");
			return 1;
		}
		WS_NPCGoTo(gGuardNpc, 1544.0, -1677.0, 13.5, DEMO_WORLD, DEMO_INT, 80.0, WS_NPC_MOVE_JOG, 0.65);
		DemoMsg(playerid, COLOR_GREEN, "NPC enviado al nodo D con WS_NPCGoTo.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wsnpcnear", true))
	{
		new Float:x, Float:y, Float:z, ids[8], line[128];
		GetPlayerPos(playerid, x, y, z);
		new nearest = WS_GetNearestNPC(x, y, z, DEMO_WORLD, DEMO_INT, 80.0);
		new count = WS_GetNPCsInRange(x, y, z, ids, sizeof(ids), DEMO_WORLD, DEMO_INT, 80.0);
		new canSee = (gGuardNpc != INVALID_NPC_ID && NPC_IsValid(gGuardNpc) && WS_IsPlayerInNPCSight(gGuardNpc, playerid, 80.0, 140.0, false)) ? 1 : 0;
		format(line, sizeof(line), "NPCs cerca=%d nearest=%d guard=%d sight=%d", count, nearest, gGuardNpc,
			canSee);
		DemoMsg(playerid, COLOR_BLUE, line);
		return 1;
	}

	if (!strcmp(cmdtext, "/wsdebug", true))
	{
		WS_SetPathDebug(false);
		WS_ClearPathDebug();
		DemoMsg(playerid, COLOR_YELLOW, "Debug visual de paths desactivado.");
		return 1;
	}

	if (!strcmp(cmdtext, "/wssave", true))
	{
		new changed = WS_Save(false);
		new line[80];
		format(line, sizeof(line), "Guardado completo: %d cambios.", changed);
		DemoMsg(playerid, COLOR_GREEN, line);
		return 1;
	}

	if (!strcmp(cmdtext, "/wsentity", true))
	{
		new Float:x, Float:y, Float:z, line[128];
		GetPlayerPos(playerid, x, y, z);
		new entity = WS_CreateEntity("marker", x, y, z, DEMO_WORLD, DEMO_INT);
		WS_SetState(entity, "demo", "player_marker");
		WS_SetStateInt(entity, "owner", playerid);
		WS_SetSimulated(entity, true);
		format(line, sizeof(line), "Entidad generica creada: id=%d type=marker simulated=1", entity);
		DemoMsg(playerid, COLOR_GREEN, line);
		return 1;
	}

	return 0;
}

public OnDoorStateChange(doorid, bool:isOpen)
{
	new line[96];
	format(line, sizeof(line), "[WorldSync] Door %d open=%d", doorid, isOpen);
	DemoMsgAll(COLOR_YELLOW, line);
	return 1;
}

public OnWorldSyncEntityCreated(entityid, const type[])
{
	printf("[Derby/WorldSync] Entity created id=%d type=%s", entityid, type);
	return 1;
}

public OnWorldSyncEntityDestroyed(entityid, const type[])
{
	printf("[Derby/WorldSync] Entity destroyed id=%d type=%s", entityid, type);
	return 1;
}

public OnWorldSyncEntityStateChange(entityid, const key[], const oldValue[], const newValue[])
{
	if (!strcmp(key, "growth") && strcmp(newValue, "100"))
	{
		return 1;
	}

	if (!strcmp(key, "open") || !strcmp(key, "locked") || !strcmp(key, "growth") || !strcmp(key, "harvests"))
	{
		printf("[Derby/WorldSync] State entity=%d %s: %s -> %s", entityid, key, oldValue, newValue);
	}
	return 1;
}

public OnWorldSyncLoaded(entityCount, bool:storageAvailable)
{
	printf("[Derby/WorldSync] Loaded entities=%d storage=%d", entityCount, storageAvailable);
	return 1;
}

public OnWorldSyncSaved(entityCount, changedCount, bool:dirtyOnly)
{
	printf("[Derby/WorldSync] Saved entities=%d changed=%d dirtyOnly=%d", entityCount, changedCount, dirtyOnly);
	return 1;
}

public OnWorldSyncCropReady(cropid)
{
	new line[96];
	format(line, sizeof(line), "[WorldSync] Crop %d listo para cosechar. Usa /wsharvest.", cropid);
	DemoMsgAll(COLOR_GREEN, line);
	return 1;
}

public OnWorldSyncPatrolStart(patrolid, npcid, routeid)
{
	new line[96];
	format(line, sizeof(line), "[WorldSync] Patrol start patrol=%d npc=%d route=%d", patrolid, npcid, routeid);
	DemoMsgAll(COLOR_GREEN, line);
	return 1;
}

public OnWorldSyncPatrolPoint(patrolid, npcid, pointIndex)
{
	printf("[Derby/WorldSync] Patrol point patrol=%d npc=%d point=%d", patrolid, npcid, pointIndex);
	return 1;
}

public OnWorldSyncPatrolComplete(patrolid, npcid, routeid)
{
	new line[96];
	format(line, sizeof(line), "[WorldSync] Patrol complete patrol=%d npc=%d route=%d", patrolid, npcid, routeid);
	DemoMsgAll(COLOR_YELLOW, line);
	return 1;
}
