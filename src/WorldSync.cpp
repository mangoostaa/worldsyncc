#include "WorldSync.hpp"

#include <Server/Components/Databases/databases.hpp>
#include <core.hpp>

#include <algorithm>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace worlds
{
namespace
{
constexpr std::chrono::milliseconds AutosaveInterval { 30000 };
constexpr const char* StoragePath = "scriptfiles/WorldSync.entities";
constexpr const char* SQLitePath = "scriptfiles/WorldSync.db";

std::string escapeField(const std::string& value)
{
	std::string output;
	output.reserve(value.size());
	for (char ch : value)
	{
		if (ch == '\\' || ch == '|' || ch == '\n' || ch == '\r')
		{
			output.push_back('\\');
		}
		output.push_back(ch);
	}
	return output;
}

int parseInt(const std::string& value, int fallback)
{
	try
	{
		return std::stoi(value);
	}
	catch (...)
	{
		return fallback;
	}
}

std::vector<std::string> splitEscaped(const std::string& line, char delimiter)
{
	std::vector<std::string> fields;
	std::string field;
	bool escaped = false;

	for (char ch : line)
	{
		if (escaped)
		{
			field.push_back(ch);
			escaped = false;
			continue;
		}

		if (ch == '\\')
		{
			escaped = true;
			continue;
		}

		if (ch == delimiter)
		{
			fields.push_back(field);
			field.clear();
			continue;
		}

		field.push_back(ch);
	}

	if (escaped)
	{
		field.push_back('\\');
	}
	fields.push_back(field);
	return fields;
}

bool parseFloat(const std::string& value, float& output)
{
	try
	{
		output = std::stof(value);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

std::string sqlEscape(const std::string& value)
{
	std::string output;
	output.reserve(value.size() + 2);
	for (char ch : value)
	{
		if (ch == '\'')
		{
			output.push_back('\'');
		}
		output.push_back(ch);
	}
	return output;
}
} // namespace

void WorldSyncCore::setLogger(ILogger* logger)
{
	logger_ = logger;
}

void WorldSyncCore::setLogLevel(LogLevelFilter level)
{
	logLevel_ = level;
}

void WorldSyncCore::setDebugEnabled(bool enabled)
{
	debugEnabled_ = enabled;
	logLevel_ = enabled ? LogLevelFilter::Debug : logLevel_;
}

void WorldSyncCore::log(LogLevelFilter level, const char* fmt, ...) const
{
	if (!logger_ || static_cast<int>(level) > static_cast<int>(logLevel_))
	{
		return;
	}

	LogLevel sdkLevel = LogLevel::Message;
	switch (level)
	{
	case LogLevelFilter::Error:
		sdkLevel = LogLevel::Error;
		break;
	case LogLevelFilter::Warning:
		sdkLevel = LogLevel::Warning;
		break;
	case LogLevelFilter::Debug:
		sdkLevel = LogLevel::Debug;
		break;
	case LogLevelFilter::Info:
	default:
		sdkLevel = LogLevel::Message;
		break;
	}

	va_list args;
	va_start(args, fmt);
	logger_->vlogLn(sdkLevel, fmt, args);
	va_end(args);
}

void WorldSyncCore::setDatabaseComponent(IDatabasesComponent* databases)
{
	if (database_ && databases_)
	{
		databases_->close(*database_);
		database_ = nullptr;
	}
	databases_ = databases;
	sqliteReady_ = false;
}

void WorldSyncCore::closeStorage()
{
	if (database_ && databases_)
	{
		databases_->close(*database_);
	}
	database_ = nullptr;
	sqliteReady_ = false;
}

bool WorldSyncCore::load()
{
	if (openSQLite() && loadSQLite())
	{
		log(LogLevelFilter::Info, "WorldSync: cargadas %d entidades desde SQLite.", lastLoadCount_);
		return true;
	}

	std::ifstream file(StoragePath);
	if (!file.is_open())
	{
		lastLoadCount_ = 0;
		log(LogLevelFilter::Warning, "WorldSync: no hay almacenamiento previo para cargar.");
		return false;
	}

	entities_.clear();
	nextID_ = 1;
	deletedSinceSave_ = 0;
	deletedEntityIDs_.clear();

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty())
		{
			continue;
		}

		Entity entity;
		if (!parseSnapshotLine(line, entity))
		{
			continue;
		}

		entity.dirty = false;
		nextID_ = std::max(nextID_, entity.id + 1);
		entities_.push_back(std::move(entity));
	}

	++loadCount_;
	lastLoadCount_ = static_cast<int>(entities_.size());
	log(LogLevelFilter::Info, "WorldSync: cargadas %d entidades desde archivo plano.", lastLoadCount_);
	return true;
}

int WorldSyncCore::createEntity(std::string type, Vec3 position, int world, int interior)
{
	Entity entity;
	entity.id = nextID_++;
	entity.type = std::move(type);
	entity.position = position;
	entity.world = world;
	entity.interior = interior;
	entities_.push_back(std::move(entity));
	log(LogLevelFilter::Debug, "WorldSync: entidad creada id=%d type=%s.", entities_.back().id, entities_.back().type.c_str());
	return entities_.back().id;
}

bool WorldSyncCore::destroyEntity(int id)
{
	const auto it = std::remove_if(entities_.begin(), entities_.end(), [id](const Entity& entity) {
		return entity.id == id;
	});
	if (it == entities_.end())
	{
		return false;
	}
	entities_.erase(it, entities_.end());
	++deletedSinceSave_;
	deletedEntityIDs_.push_back(id);
	log(LogLevelFilter::Debug, "WorldSync: entidad destruida id=%d.", id);
	return true;
}

bool WorldSyncCore::hasEntity(int id) const
{
	return findEntity(id) != nullptr;
}

bool WorldSyncCore::setState(int id, std::string key, std::string value)
{
	Entity* entity = findEntity(id);
	if (!entity)
	{
		return false;
	}
	entity->state[std::move(key)] = std::move(value);
	entity->dirty = true;
	log(LogLevelFilter::Debug, "WorldSync: state actualizado entity=%d.", id);
	return true;
}

bool WorldSyncCore::getState(int id, const std::string& key, std::string& value) const
{
	const Entity* entity = findEntity(id);
	if (!entity)
	{
		return false;
	}
	const auto it = entity->state.find(key);
	if (it == entity->state.end())
	{
		return false;
	}
	value = it->second;
	return true;
}

bool WorldSyncCore::getType(int id, std::string& type) const
{
	const Entity* entity = findEntity(id);
	if (!entity)
	{
		return false;
	}
	type = entity->type;
	return true;
}

bool WorldSyncCore::getPosition(int id, Vec3& position) const
{
	const Entity* entity = findEntity(id);
	if (!entity)
	{
		return false;
	}
	position = entity->position;
	return true;
}

int WorldSyncCore::getWorld(int id) const
{
	const Entity* entity = findEntity(id);
	return entity ? entity->world : 0;
}

int WorldSyncCore::getInterior(int id) const
{
	const Entity* entity = findEntity(id);
	return entity ? entity->interior : 0;
}

int WorldSyncCore::getEntityIDAt(size_t index) const
{
	if (index >= entities_.size())
	{
		return 0;
	}
	return entities_[index].id;
}

bool WorldSyncCore::setSimulated(int id, bool enabled)
{
	Entity* entity = findEntity(id);
	if (!entity)
	{
		return false;
	}
	entity->simulated = enabled;
	entity->dirty = true;
	return true;
}

void WorldSyncCore::tick(std::chrono::milliseconds elapsed)
{
	for (Entity& entity : entities_)
	{
		if (entity.simulated)
		{
			simulateEntity(entity, elapsed);
		}
	}

	autosaveElapsed_ += elapsed;
	if (autosaveElapsed_ >= AutosaveInterval)
	{
		autosaveElapsed_ = std::chrono::milliseconds { 0 };
		saveDirty();
	}
}

int WorldSyncCore::saveDirty()
{
	if (openSQLite())
	{
		const int changed = writeSQLite(true);
		log(LogLevelFilter::Debug, "WorldSync: saveDirty SQLite changed=%d.", changed);
		return changed;
	}
	const int changed = writeSnapshot(true);
	log(LogLevelFilter::Debug, "WorldSync: saveDirty file changed=%d.", changed);
	return changed;
}

int WorldSyncCore::saveAll()
{
	if (openSQLite())
	{
		const int changed = writeSQLite(false);
		log(LogLevelFilter::Debug, "WorldSync: saveAll SQLite changed=%d.", changed);
		return changed;
	}
	const int changed = writeSnapshot(false);
	log(LogLevelFilter::Debug, "WorldSync: saveAll file changed=%d.", changed);
	return changed;
}

void WorldSyncCore::reset()
{
	entities_.clear();
	nextID_ = 1;
	deletedSinceSave_ = 0;
	deletedEntityIDs_.clear();
	loadCount_ = 0;
	saveCount_ = 0;
	autosaveElapsed_ = std::chrono::milliseconds { 0 };
}

Stats WorldSyncCore::stats() const
{
	Stats result;
	result.entities = static_cast<int>(entities_.size());
	result.pendingDeletes = deletedSinceSave_;
	result.loads = loadCount_;
	result.saves = saveCount_;
	result.lastLoadCount = lastLoadCount_;
	result.lastSaveCount = lastSaveCount_;
	result.lastSaveChanged = lastSaveChanged_;
	for (const Entity& entity : entities_)
	{
		if (entity.dirty)
		{
			++result.dirtyEntities;
		}
		if (entity.simulated)
		{
			++result.simulatedEntities;
		}
	}
	return result;
}

Entity* WorldSyncCore::findEntity(int id)
{
	for (Entity& entity : entities_)
	{
		if (entity.id == id)
		{
			return &entity;
		}
	}
	return nullptr;
}

const Entity* WorldSyncCore::findEntity(int id) const
{
	for (const Entity& entity : entities_)
	{
		if (entity.id == id)
		{
			return &entity;
		}
	}
	return nullptr;
}

void WorldSyncCore::simulateEntity(Entity& entity, std::chrono::milliseconds elapsed)
{
	entity.simulationElapsed += elapsed;
}

int WorldSyncCore::writeSnapshot(bool dirtyOnly)
{
	std::filesystem::create_directories("scriptfiles");

	std::ofstream file(StoragePath, std::ios::trunc);
	if (!file.is_open())
	{
		return 0;
	}

	int changed = deletedSinceSave_;
	for (Entity& entity : entities_)
	{
		const bool wasDirty = entity.dirty;

		file << entity.id << '|'
			 << escapeField(entity.type) << '|'
			 << entity.position.x << '|'
			 << entity.position.y << '|'
			 << entity.position.z << '|'
			 << entity.world << '|'
			 << entity.interior << '|'
			 << (entity.simulated ? 1 : 0);

		for (const auto& item : entity.state)
		{
			file << '|' << escapeField(item.first) << '=' << escapeField(item.second);
		}
		file << '\n';

		entity.dirty = false;
		if (!dirtyOnly || wasDirty)
		{
			++changed;
		}
	}

	deletedSinceSave_ = 0;
	deletedEntityIDs_.clear();
	++saveCount_;
	lastSaveCount_ = static_cast<int>(entities_.size());
	lastSaveChanged_ = changed;
	return changed;
}

bool WorldSyncCore::parseSnapshotLine(const std::string& line, Entity& entity) const
{
	const std::vector<std::string> fields = splitEscaped(line, '|');
	if (fields.size() < 8)
	{
		return false;
	}

	entity.id = parseInt(fields[0], 0);
	entity.type = fields[1];
	if (entity.id <= 0 || entity.type.empty())
	{
		return false;
	}
	if (!parseFloat(fields[2], entity.position.x) || !parseFloat(fields[3], entity.position.y) || !parseFloat(fields[4], entity.position.z))
	{
		return false;
	}

	entity.world = parseInt(fields[5], 0);
	entity.interior = parseInt(fields[6], 0);
	entity.simulated = parseInt(fields[7], 0) != 0;

	for (size_t i = 8; i < fields.size(); ++i)
	{
		const size_t equals = fields[i].find('=');
		if (equals == std::string::npos)
		{
			continue;
		}
		const std::string key = fields[i].substr(0, equals);
		if (key.empty())
		{
			continue;
		}
		entity.state[key] = fields[i].substr(equals + 1);
	}

	return true;
}

bool WorldSyncCore::openSQLite()
{
	if (sqliteReady_ && database_)
	{
		return true;
	}
	if (!databases_)
	{
		return false;
	}

	std::filesystem::create_directories("scriptfiles");
	database_ = databases_->open(SQLitePath);
	if (!database_)
	{
		return false;
	}

	sqliteReady_ = initSQLiteSchema();
	if (!sqliteReady_)
	{
		databases_->close(*database_);
		database_ = nullptr;
	}
	return sqliteReady_;
}

bool WorldSyncCore::initSQLiteSchema()
{
	if (!execSQL("CREATE TABLE IF NOT EXISTS worlds_meta ("
			"key TEXT PRIMARY KEY,"
			"value TEXT NOT NULL"
			")"))
	{
		return false;
	}

	const int version = sqliteSchemaVersion();
	if (version > 1)
	{
		log(LogLevelFilter::Warning, "WorldSync: DB schema_version=%d es mas nuevo que este plugin.", version);
	}

	const bool ok = execSQL("CREATE TABLE IF NOT EXISTS worlds_entities ("
		"id INTEGER PRIMARY KEY,"
		"type TEXT NOT NULL,"
		"x REAL NOT NULL,"
		"y REAL NOT NULL,"
		"z REAL NOT NULL,"
		"world INTEGER NOT NULL,"
		"interior INTEGER NOT NULL,"
		"simulated INTEGER NOT NULL DEFAULT 0"
		")")
		&& execSQL("CREATE TABLE IF NOT EXISTS worlds_state ("
			"entity_id INTEGER NOT NULL,"
			"key TEXT NOT NULL,"
			"value TEXT NOT NULL,"
			"PRIMARY KEY(entity_id, key),"
			"FOREIGN KEY(entity_id) REFERENCES worlds_entities(id) ON DELETE CASCADE"
			")")
		&& execSQL("CREATE INDEX IF NOT EXISTS idx_worlds_entities_type ON worlds_entities(type)");

	if (!ok)
	{
		return false;
	}

	if (version <= 0)
	{
		return setSQLiteSchemaVersion(1);
	}
	return true;
}

int WorldSyncCore::sqliteSchemaVersion()
{
	if (!database_ || !databases_)
	{
		return 0;
	}

	IDatabaseResultSet* result = database_->executeQuery("SELECT value FROM worlds_meta WHERE key='schema_version'");
	if (!result)
	{
		return 0;
	}

	int version = 0;
	if (result->selectNextRow())
	{
		const StringView value = result->getFieldString(0);
		version = parseInt(std::string(value.data(), value.size()), 0);
	}
	databases_->freeResultSet(*result);
	return version;
}

bool WorldSyncCore::setSQLiteSchemaVersion(int version)
{
	std::ostringstream sql;
	sql << "INSERT OR REPLACE INTO worlds_meta(key,value) VALUES('schema_version','" << version << "')";
	return execSQL(sql.str());
}

bool WorldSyncCore::loadSQLite()
{
	if (!database_ || !databases_)
	{
		return false;
	}

	IDatabaseResultSet* result = database_->executeQuery("SELECT id, type, x, y, z, world, interior, simulated FROM worlds_entities ORDER BY id");
	if (!result)
	{
		return false;
	}

	entities_.clear();
	nextID_ = 1;
	deletedSinceSave_ = 0;
	deletedEntityIDs_.clear();

	while (result->selectNextRow())
	{
		Entity entity;
		entity.id = static_cast<int>(result->getFieldIntByName("id"));
		const StringView type = result->getFieldStringByName("type");
		entity.type.assign(type.data(), type.size());
		entity.position.x = static_cast<float>(result->getFieldFloatByName("x"));
		entity.position.y = static_cast<float>(result->getFieldFloatByName("y"));
		entity.position.z = static_cast<float>(result->getFieldFloatByName("z"));
		entity.world = static_cast<int>(result->getFieldIntByName("world"));
		entity.interior = static_cast<int>(result->getFieldIntByName("interior"));
		entity.simulated = result->getFieldIntByName("simulated") != 0;
		entity.dirty = false;
		nextID_ = std::max(nextID_, entity.id + 1);
		entities_.push_back(std::move(entity));
	}
	databases_->freeResultSet(*result);

	result = database_->executeQuery("SELECT entity_id, key, value FROM worlds_state ORDER BY entity_id");
	if (!result)
	{
		++loadCount_;
		return true;
	}

	while (result->selectNextRow())
	{
		const int entityID = static_cast<int>(result->getFieldIntByName("entity_id"));
		Entity* entity = findEntity(entityID);
		if (!entity)
		{
			continue;
		}
		const StringView key = result->getFieldStringByName("key");
		const StringView value = result->getFieldStringByName("value");
		entity->state[std::string(key.data(), key.size())] = std::string(value.data(), value.size());
	}
	databases_->freeResultSet(*result);

	++loadCount_;
	lastLoadCount_ = static_cast<int>(entities_.size());
	return true;
}

int WorldSyncCore::writeSQLite(bool dirtyOnly)
{
	if (!database_)
	{
		return 0;
	}

	int changed = deletedSinceSave_;
	for (const Entity& entity : entities_)
	{
		if (!dirtyOnly || entity.dirty)
		{
			++changed;
		}
	}

	if (!execSQL("BEGIN IMMEDIATE TRANSACTION"))
	{
		return 0;
	}

	bool ok = true;
	for (int deletedID : deletedEntityIDs_)
	{
		std::ostringstream deleteStateSQL;
		deleteStateSQL << "DELETE FROM worlds_state WHERE entity_id=" << deletedID;
		std::ostringstream deleteEntitySQL;
		deleteEntitySQL << "DELETE FROM worlds_entities WHERE id=" << deletedID;
		ok = execSQL(deleteStateSQL.str()) && execSQL(deleteEntitySQL.str());
		if (!ok)
		{
			break;
		}
	}

	for (const Entity& entity : entities_)
	{
		if (dirtyOnly && !entity.dirty)
		{
			continue;
		}
		if (!ok)
		{
			break;
		}

		std::ostringstream entitySQL;
		entitySQL << "INSERT OR REPLACE INTO worlds_entities(id,type,x,y,z,world,interior,simulated) VALUES("
				  << entity.id << ",'"
				  << sqlEscape(entity.type) << "',"
				  << entity.position.x << ','
				  << entity.position.y << ','
				  << entity.position.z << ','
				  << entity.world << ','
				  << entity.interior << ','
				  << (entity.simulated ? 1 : 0)
				  << ')';
		ok = execSQL(entitySQL.str());

		if (ok)
		{
			std::ostringstream clearStateSQL;
			clearStateSQL << "DELETE FROM worlds_state WHERE entity_id=" << entity.id;
			ok = execSQL(clearStateSQL.str());
		}

		for (const auto& item : entity.state)
		{
			if (!ok)
			{
				break;
			}
			std::ostringstream stateSQL;
			stateSQL << "INSERT INTO worlds_state(entity_id,key,value) VALUES("
					 << entity.id << ",'"
					 << sqlEscape(item.first) << "','"
					 << sqlEscape(item.second) << "')";
			ok = execSQL(stateSQL.str());
		}
	}

	if (ok)
	{
		ok = execSQL("COMMIT");
	}
	else
	{
		execSQL("ROLLBACK");
		return 0;
	}

	for (Entity& entity : entities_)
	{
		entity.dirty = false;
	}
	deletedSinceSave_ = 0;
	deletedEntityIDs_.clear();
	++saveCount_;
	lastSaveCount_ = static_cast<int>(entities_.size());
	lastSaveChanged_ = changed;
	return changed;
}

bool WorldSyncCore::execSQL(const std::string& query)
{
	if (!database_ || !databases_)
	{
		return false;
	}

	IDatabaseResultSet* result = database_->executeQuery(StringView(query.data(), query.size()));
	if (result)
	{
		databases_->freeResultSet(*result);
	}
	return true;
}
} // namespace worlds
