# Examples

The main example is:

```text
examples/worldsync_demo.pwn
```

It demonstrates:

- WorldSync startup logging.
- Persistent house door.
- Crop field and crop-ready callback.
- Spatial grid queries.
- Path node route creation.
- NPC patrol setup for an existing NPC ID.

## How To Use

1. Copy `include/worldsync.inc` to your Pawn include directory.
2. Copy or compile `examples/worldsync_demo.pwn` as a gamemode or adapt the functions into a filterscript.
3. Load WorldSync in the server.
4. Start the server.
5. If you have an NPC, call:

```pawn
WorldSyncDemo_StartGuardPatrol(npcid);
```

from the place where your NPC is created or becomes available.

## Why The Example Does Not Create The NPC

WorldSync consumes NPC IDs from the open.mp NPC component. Actual NPC creation can differ by server setup, NPC scripts and component version. The example exposes `WorldSyncDemo_StartGuardPatrol(npcid)` so it can work with your existing NPC creation flow.

## Production Pattern

For production gamemodes, avoid creating duplicate world content every restart. Recommended approaches:

- Store created entity IDs in your own config/database.
- Use `WS_GetEntityCount`, `WS_GetEntityIDAt`, `WS_GetEntityType` and `WS_GetState` to find existing bootstrap entities.
- Mark bootstrap entities with state keys like `system=house_demo` or `zone=market_01`.
