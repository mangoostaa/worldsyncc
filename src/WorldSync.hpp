#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct IDatabasesComponent;
struct IDatabaseConnection;
struct ILogger;

namespace worlds
{
struct Vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct Entity
{
	int id = 0;
	std::string type;
	Vec3 position;
	int world = 0;
	int interior = 0;
	bool dirty = true;
	bool simulated = false;
	std::chrono::milliseconds simulationElapsed { 0 };
	std::unordered_map<std::string, std::string> state;
};

struct Stats
{
	int entities = 0;
	int dirtyEntities = 0;
	int simulatedEntities = 0;
	int pendingDeletes = 0;
	uint64_t loads = 0;
	uint64_t saves = 0;
	int lastLoadCount = 0;
	int lastSaveCount = 0;
	int lastSaveChanged = 0;
};

class WorldSyncEventHandler
{
public:
	virtual ~WorldSyncEventHandler() = default;
	virtual void onEntityCreated(const Entity& entity) {}
	virtual void onEntityDestroyed(int entityID, const std::string& type) {}
	virtual void onEntityStateChanged(const Entity& entity, const std::string& key, const std::string& oldValue, const std::string& newValue) {}
	virtual void onWorldLoaded(int entityCount, bool storageAvailable) {}
	virtual void onWorldSaved(int entityCount, int changedCount, bool dirtyOnly) {}
};

enum class LogLevelFilter
{
	Error = 0,
	Warning = 1,
	Info = 2,
	Debug = 3
};

class WorldSyncCore
{
public:
	void setLogger(ILogger* logger);
	void setLogLevel(LogLevelFilter level);
	LogLevelFilter getLogLevel() const { return logLevel_; }
	void setDebugEnabled(bool enabled);
	bool isDebugEnabled() const { return debugEnabled_; }
	void log(LogLevelFilter level, const char* fmt, ...) const;
	void addEventHandler(WorldSyncEventHandler* handler);
	void removeEventHandler(WorldSyncEventHandler* handler);

	void setDatabaseComponent(IDatabasesComponent* databases);
	void closeStorage();
	bool load();
	int createEntity(std::string type, Vec3 position, int world, int interior);
	bool destroyEntity(int id);
	bool hasEntity(int id) const;

	bool setState(int id, std::string key, std::string value);
	bool getState(int id, const std::string& key, std::string& value) const;
	int getStateInt(int id, const std::string& key, int fallback) const;
	float getStateFloat(int id, const std::string& key, float fallback) const;
	bool getStateBool(int id, const std::string& key, bool fallback) const;
	bool getType(int id, std::string& type) const;
	bool getPosition(int id, Vec3& position) const;
	bool setPosition(int id, Vec3 position);
	int getWorld(int id) const;
	int getInterior(int id) const;
	int getEntityIDAt(size_t index) const;
	std::vector<int> findEntitiesInRange(Vec3 position, int world, int interior, float radius, const std::string& type = {}) const;
	int findNearestEntity(Vec3 position, int world, int interior, float maxDistance, const std::string& type = {}) const;
	std::vector<int> findEntitiesByState(const std::string& type, const std::string& key, const std::string& value, size_t maxResults) const;
	int countEntitiesByState(const std::string& type, const std::string& key, const std::string& value) const;

	bool setSimulated(int id, bool enabled);
	void tick(std::chrono::milliseconds elapsed);

	int saveDirty();
	int saveAll();
	void reset();

	Stats stats() const;
	const std::vector<Entity>& entities() const { return entities_; }

private:
	struct SpatialCell
	{
		int world = 0;
		int interior = 0;
		int x = 0;
		int y = 0;

		bool operator==(const SpatialCell& other) const
		{
			return world == other.world && interior == other.interior && x == other.x && y == other.y;
		}
	};

	struct SpatialCellHash
	{
		size_t operator()(const SpatialCell& cell) const;
	};

	Entity* findEntity(int id);
	const Entity* findEntity(int id) const;
	SpatialCell spatialCellFor(const Entity& entity) const;
	SpatialCell spatialCellFor(Vec3 position, int world, int interior) const;
	void addToSpatialGrid(const Entity& entity);
	void removeFromSpatialGrid(const Entity& entity);
	void rebuildSpatialGrid();
	void fireEntityCreated(const Entity& entity);
	void fireEntityDestroyed(int entityID, const std::string& type);
	void fireEntityStateChanged(const Entity& entity, const std::string& key, const std::string& oldValue, const std::string& newValue);
	void fireWorldLoaded(bool storageAvailable);
	void fireWorldSaved(int changedCount, bool dirtyOnly);
	void simulateEntity(Entity& entity, std::chrono::milliseconds elapsed);
	int writeSnapshot(bool dirtyOnly);
	bool parseSnapshotLine(const std::string& line, Entity& entity) const;
	bool openSQLite();
	bool initSQLiteSchema();
	int sqliteSchemaVersion();
	bool setSQLiteSchemaVersion(int version);
	bool loadSQLite();
	int writeSQLite(bool dirtyOnly);
	bool execSQL(const std::string& query);

	std::vector<Entity> entities_;
	std::unordered_map<SpatialCell, std::vector<int>, SpatialCellHash> spatialGrid_;
	std::vector<WorldSyncEventHandler*> eventHandlers_;
	IDatabasesComponent* databases_ = nullptr;
	IDatabaseConnection* database_ = nullptr;
	ILogger* logger_ = nullptr;
	LogLevelFilter logLevel_ = LogLevelFilter::Info;
	bool debugEnabled_ = false;
	bool sqliteReady_ = false;
	int nextID_ = 1;
	int deletedSinceSave_ = 0;
	std::vector<int> deletedEntityIDs_;
	int lastLoadCount_ = 0;
	int lastSaveCount_ = 0;
	int lastSaveChanged_ = 0;
	uint64_t loadCount_ = 0;
	uint64_t saveCount_ = 0;
	std::chrono::milliseconds autosaveElapsed_ { 0 };
};
} // namespace worlds
