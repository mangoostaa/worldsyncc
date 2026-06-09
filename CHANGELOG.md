# Changelog

## v0.3.0

- Added typed state natives for integers, floats and booleans.
- Added loaded-entity queries by state key/value.
- Avoid marking entities dirty when setting state to the existing value.
- Added SQLite state key/value index and schema version bump.
- Cached path node edge decoding in PathModule to reduce A* overhead.
- Added runtime NPC spatial queries and NPC perception helpers.
- Added `WS_SetEntityPos` with spatial grid reindexing.

## v0.1.0

Initial early adoption release.

- Added persistent WorldSync entities with stable IDs and state storage.
- Added SQLite storage with file fallback.
- Added spatial entity lookup helpers.
- Added door, vehicle, crop, pathfinding and NPC patrol helpers.
- Added Pawn include, documentation and example gamemode.
