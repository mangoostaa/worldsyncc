#include <sdk.hpp>
#include "WorldSync.hpp"
#include "Doormodule.hpp"
#include "Cropmodule.hpp"
#include "Pathmodule.hpp"
#include "Vehiclemodule.hpp"

#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <Server/Components/NPCs/npcs.hpp>
#include <Server/Components/Databases/databases.hpp>
#include <Server/Components/TextLabels/textlabels.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>

#include <algorithm>
#include <memory>

namespace
{
worlds::WorldSyncCore* gWorld = nullptr;
IPawnComponent* gPawn = nullptr;

bool checkParams(cell* params, int count)
{
	return params && params[0] / static_cast<cell>(sizeof(cell)) >= count;
}

std::string pawnString(AMX* amx, cell address)
{
	if (!gPawn)
	{
		return {};
	}

	IPawnScript* script = gPawn->getScript(amx);
	if (!script)
	{
		return {};
	}

	cell* phys = nullptr;
	if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys)
	{
		return {};
	}

	int length = 0;
	script->StrLen(phys, &length);
	std::string value(static_cast<size_t>(length), '\0');
	if (length > 0)
	{
		script->GetString(&value[0], phys, false, static_cast<size_t>(length + 1));
	}
	return value;
}

bool setPawnString(AMX* amx, cell address, const std::string& value, size_t maxLength)
{
	if (!gPawn || maxLength == 0)
	{
		return false;
	}

	IPawnScript* script = gPawn->getScript(amx);
	if (!script)
	{
		return false;
	}

	cell* phys = nullptr;
	if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys)
	{
		return false;
	}

	return script->SetString(phys, StringView(value.data(), value.size()), false, false, maxLength) == AMX_ERR_NONE;
}

bool setPawnCell(AMX* amx, cell address, cell value)
{
	if (!gPawn)
	{
		return false;
	}

	IPawnScript* script = gPawn->getScript(amx);
	if (!script)
	{
		return false;
	}

	cell* phys = nullptr;
	if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys)
	{
		return false;
	}

	*phys = value;
	return true;
}

int fillPawnArray(AMX* amx, cell address, const std::vector<int>& values, int maxValues)
{
	if (!gPawn || maxValues <= 0)
	{
		return 0;
	}

	IPawnScript* script = gPawn->getScript(amx);
	if (!script)
	{
		return 0;
	}

	cell* phys = nullptr;
	if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys)
	{
		return 0;
	}

	const int written = std::min(static_cast<int>(values.size()), maxValues);
	for (int i = 0; i < written; ++i)
	{
		phys[i] = static_cast<cell>(values[static_cast<size_t>(i)]);
	}
	return written;
}

template <typename Invoker>
void callPawnScripts(IPawnComponent* pawn, const char* callback, Invoker invoker)
{
	if (!pawn)
	{
		return;
	}

	IPawnScript* mainScript = pawn->mainScript();
	if (mainScript)
	{
		invoker(mainScript, callback);
	}
	for (IPawnScript* script : pawn->sideScripts())
	{
		if (script)
		{
			invoker(script, callback);
		}
	}
}

cell AMX_NATIVE_CALL WS_Load(AMX*, cell*)
{
	return gWorld && gWorld->load() ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_CreateEntity(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 6))
	{
		return 0;
	}

	worlds::Vec3 position { amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]) };
	const std::string type = pawnString(amx, params[1]);
	if (type.empty())
	{
		return 0;
	}
	return gWorld->createEntity(type, position, static_cast<int>(params[5]), static_cast<int>(params[6]));
}

cell AMX_NATIVE_CALL WS_DestroyEntity(AMX*, cell* params)
{
	return gWorld && checkParams(params, 1) && gWorld->destroyEntity(static_cast<int>(params[1])) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_EntityExists(AMX*, cell* params)
{
	return gWorld && checkParams(params, 1) && gWorld->hasEntity(static_cast<int>(params[1])) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_SetState(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 3))
	{
		return 0;
	}

	const std::string key = pawnString(amx, params[2]);
	if (key.empty())
	{
		return 0;
	}

	return gWorld->setState(static_cast<int>(params[1]), key, pawnString(amx, params[3])) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_GetState(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 4))
	{
		return 0;
	}

	std::string value;
	const std::string key = pawnString(amx, params[2]);
	if (key.empty() || params[4] <= 0 || !gWorld->getState(static_cast<int>(params[1]), key, value))
	{
		return 0;
	}

	return setPawnString(amx, params[3], value, static_cast<size_t>(params[4])) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_GetEntityType(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 3))
	{
		return 0;
	}

	std::string type;
	if (params[3] <= 0 || !gWorld->getType(static_cast<int>(params[1]), type))
	{
		return 0;
	}

	return setPawnString(amx, params[2], type, static_cast<size_t>(params[3])) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_GetEntityPos(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 4))
	{
		return 0;
	}

	worlds::Vec3 position;
	if (!gWorld->getPosition(static_cast<int>(params[1]), position))
	{
		return 0;
	}

	return setPawnCell(amx, params[2], amx_ftoc(position.x))
		&& setPawnCell(amx, params[3], amx_ftoc(position.y))
		&& setPawnCell(amx, params[4], amx_ftoc(position.z))
		? 1
		: 0;
}

cell AMX_NATIVE_CALL WS_SetEntityPos(AMX*, cell* params)
{
	if (!gWorld || !checkParams(params, 4))
	{
		return 0;
	}

	const worlds::Vec3 position { amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]) };
	return gWorld->setPosition(static_cast<int>(params[1]), position) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_GetEntityWorld(AMX*, cell* params)
{
	return gWorld && checkParams(params, 1) ? gWorld->getWorld(static_cast<int>(params[1])) : 0;
}

cell AMX_NATIVE_CALL WS_GetEntityInterior(AMX*, cell* params)
{
	return gWorld && checkParams(params, 1) ? gWorld->getInterior(static_cast<int>(params[1])) : 0;
}

cell AMX_NATIVE_CALL WS_GetEntityCount(AMX*, cell*)
{
	return gWorld ? static_cast<cell>(gWorld->entities().size()) : 0;
}

cell AMX_NATIVE_CALL WS_GetEntityIDAt(AMX*, cell* params)
{
	if (!gWorld || !checkParams(params, 1) || params[1] < 0)
	{
		return 0;
	}
	return gWorld->getEntityIDAt(static_cast<size_t>(params[1]));
}

cell AMX_NATIVE_CALL WS_GetNearestEntity(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 7))
	{
		return 0;
	}

	const worlds::Vec3 position { amx_ctof(params[1]), amx_ctof(params[2]), amx_ctof(params[3]) };
	const std::string type = pawnString(amx, params[7]);
	return gWorld->findNearestEntity(
		position,
		static_cast<int>(params[4]),
		static_cast<int>(params[5]),
		amx_ctof(params[6]),
		type);
}

cell AMX_NATIVE_CALL WS_GetEntitiesInRange(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 9))
	{
		return 0;
	}

	const int maxEntities = static_cast<int>(params[5]);
	if (maxEntities <= 0)
	{
		return 0;
	}

	const worlds::Vec3 position { amx_ctof(params[1]), amx_ctof(params[2]), amx_ctof(params[3]) };
	const std::vector<int> entities = gWorld->findEntitiesInRange(
		position,
		static_cast<int>(params[6]),
		static_cast<int>(params[7]),
		amx_ctof(params[8]),
		pawnString(amx, params[9]));

	return fillPawnArray(amx, params[4], entities, maxEntities);
}

cell AMX_NATIVE_CALL WS_SetSimulated(AMX*, cell* params)
{
	if (!gWorld || !checkParams(params, 2))
	{
		return 0;
	}
	return gWorld->setSimulated(static_cast<int>(params[1]), params[2] != 0) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_Save(AMX*, cell* params)
{
	if (!gWorld)
	{
		return 0;
	}
	const bool dirtyOnly = !checkParams(params, 1) || params[1] != 0;
	return dirtyOnly ? gWorld->saveDirty() : gWorld->saveAll();
}

cell AMX_NATIVE_CALL WS_GetStats(AMX*, cell* params)
{
	if (!gWorld || !checkParams(params, 1))
	{
		return 0;
	}
	const worlds::Stats stats = gWorld->stats();
	switch (params[1])
	{
	case 0:
		return stats.entities;
	case 1:
		return stats.dirtyEntities;
	case 2:
		return stats.simulatedEntities;
	case 3:
		return static_cast<cell>(stats.saves);
	case 4:
		return stats.pendingDeletes;
	case 5:
		return static_cast<cell>(stats.loads);
	case 6:
		return stats.lastLoadCount;
	case 7:
		return stats.lastSaveCount;
	case 8:
		return stats.lastSaveChanged;
	default:
		return 0;
	}
}

cell AMX_NATIVE_CALL WS_SetDebug(AMX*, cell* params)
{
	if (!gWorld || !checkParams(params, 1))
	{
		return 0;
	}
	gWorld->setDebugEnabled(params[1] != 0);
	gWorld->log(worlds::LogLevelFilter::Info, "WorldSync: debug %s.", params[1] != 0 ? "activado" : "desactivado");
	return 1;
}

cell AMX_NATIVE_CALL WS_SetLogLevel(AMX*, cell* params)
{
	if (!gWorld || !checkParams(params, 1))
	{
		return 0;
	}
	int level = static_cast<int>(params[1]);
	if (level < 0) level = 0;
	if (level > 3) level = 3;
	gWorld->setLogLevel(static_cast<worlds::LogLevelFilter>(level));
	gWorld->log(worlds::LogLevelFilter::Info, "WorldSync: log level=%d.", level);
	return 1;
}

cell AMX_NATIVE_CALL WS_GetLogLevel(AMX*, cell*)
{
	return gWorld ? static_cast<cell>(gWorld->getLogLevel()) : 0;
}

cell AMX_NATIVE_CALL WS_DebugSummary(AMX*, cell*)
{
	if (!gWorld)
	{
		return 0;
	}
	const worlds::Stats stats = gWorld->stats();
	gWorld->log(worlds::LogLevelFilter::Info,
		"WorldSync summary: entities=%d dirty=%d simulated=%d pendingDeletes=%d loads=%llu saves=%llu lastLoad=%d lastSave=%d lastChanged=%d",
		stats.entities,
		stats.dirtyEntities,
		stats.simulatedEntities,
		stats.pendingDeletes,
		static_cast<unsigned long long>(stats.loads),
		static_cast<unsigned long long>(stats.saves),
		stats.lastLoadCount,
		stats.lastSaveCount,
		stats.lastSaveChanged);
	return 1;
}

const AMX_NATIVE_INFO WorldSyncNatives[] = {
	{ "WS_Load", WS_Load },
	{ "WS_CreateEntity", WS_CreateEntity },
	{ "WS_DestroyEntity", WS_DestroyEntity },
	{ "WS_EntityExists", WS_EntityExists },
	{ "WS_SetState", WS_SetState },
	{ "WS_GetState", WS_GetState },
	{ "WS_GetEntityType", WS_GetEntityType },
	{ "WS_GetEntityPos", WS_GetEntityPos },
	{ "WS_SetEntityPos", WS_SetEntityPos },
	{ "WS_GetEntityWorld", WS_GetEntityWorld },
	{ "WS_GetEntityInterior", WS_GetEntityInterior },
	{ "WS_GetEntityCount", WS_GetEntityCount },
	{ "WS_GetEntityIDAt", WS_GetEntityIDAt },
	{ "WS_GetNearestEntity", WS_GetNearestEntity },
	{ "WS_GetEntitiesInRange", WS_GetEntitiesInRange },
	{ "WS_SetSimulated", WS_SetSimulated },
	{ "WS_Save", WS_Save },
	{ "WS_GetStats", WS_GetStats },
	{ "WS_SetDebug", WS_SetDebug },
	{ "WS_SetLogLevel", WS_SetLogLevel },
	{ "WS_GetLogLevel", WS_GetLogLevel },
	{ "WS_DebugSummary", WS_DebugSummary },
};
} // namespace

class WorldSync final : public IComponent, public CoreEventHandler, public PawnEventHandler, public worlds::WorldSyncEventHandler
{
public:
	PROVIDE_UID(0x1c3f8af9876d4201);

	StringView componentName() const override { return "WorldSync"; }

	SemanticVersion componentVersion() const override
	{
		return SemanticVersion(0, 1, 0);
	}

	void onLoad(ICore* c) override
	{
		core_ = c;
		world_.setLogger(core_);
		world_.addEventHandler(this);
		gWorld = &world_;
		core_->getEventDispatcher().addEventHandler(this);
		core_->printLn("WorldSync v0.1.0 cargado.");
	}

	void onInit(IComponentList* components) override
	{
		pawn_ = components->queryComponent<IPawnComponent>();
		objects_ = components->queryComponent<IObjectsComponent>();
		npcs_ = components->queryComponent<INPCComponent>();
		databases_ = components->queryComponent<IDatabasesComponent>();
		textLabels_ = components->queryComponent<ITextLabelsComponent>();
		vehicles_ = components->queryComponent<IVehiclesComponent>();
		world_.setDatabaseComponent(databases_);
		gPawn = pawn_;
		doorModule_ = std::make_unique<worlds::DoorModule>(world_, pawn_, objects_, core_);
		cropModule_ = std::make_unique<worlds::CropModule>(world_, pawn_, core_);
		pathModule_ = std::make_unique<worlds::PathModule>(world_, pawn_, npcs_, textLabels_, core_);
		vehicleModule_ = std::make_unique<worlds::VehicleModule>(world_, pawn_, vehicles_, core_);
		if (pawn_)
		{
			pawn_->getEventDispatcher().addEventHandler(this);
		}
		else if (core_)
		{
			core_->logLn(LogLevel::Warning, "WorldSync: no se encontro el componente Pawn; las natives no se registraran.");
		}
		if (!objects_ && core_)
		{
			core_->logLn(LogLevel::Warning, "WorldSync: no se encontro el componente Objects; las puertas se guardaran pero no se veran.");
		}
		if (!npcs_ && core_)
		{
			core_->logLn(LogLevel::Warning, "WorldSync: no se encontro el componente NPCs; pathfinding funciona, pero no movera NPCs.");
		}
		if (!databases_ && core_)
		{
			core_->logLn(LogLevel::Warning, "WorldSync: no se encontro el componente Databases; usando archivo plano como fallback.");
		}
		if (!textLabels_ && core_)
		{
			core_->logLn(LogLevel::Warning, "WorldSync: no se encontro el componente TextLabels; debug visual de pathfinding desactivado.");
		}
		if (!vehicles_ && core_)
		{
			core_->logLn(LogLevel::Warning, "WorldSync: no se encontro el componente Vehicles; los vehiculos se guardaran pero no se spawnearan.");
		}
	}

	void onReady() override
	{
		if (core_)
		{
			const bool loaded = world_.load();
			if (doorModule_)
			{
				doorModule_->rebuildObjects();
			}
			if (vehicleModule_)
			{
				vehicleModule_->rebuildVehicles();
			}
			const worlds::Stats stats = world_.stats();
			core_->printLn("WorldSync listo: %d entidades cargadas, persistencia %s.", stats.entities, loaded ? "activa" : "sin archivo previo");
		}
	}

	void onTick(Microseconds elapsed, TimePoint) override
	{
		const Milliseconds delta = duration_cast<Milliseconds>(elapsed);
		world_.tick(delta);
		if (cropModule_)
		{
			cropModule_->tickCrops(delta);
		}
		if (pathModule_)
		{
			pathModule_->tickNPCSpatialGrid(delta);
		}
	}

	void onAmxLoad(IPawnScript& script) override
	{
		script.Register(WorldSyncNatives, static_cast<int>(sizeof(WorldSyncNatives) / sizeof(WorldSyncNatives[0])));
		if (doorModule_)
		{
			doorModule_->registerNatives(script);
		}
		if (cropModule_)
		{
			cropModule_->registerNatives(script);
		}
		if (pathModule_)
		{
			pathModule_->registerNatives(script);
		}
		if (vehicleModule_)
		{
			vehicleModule_->registerNatives(script);
		}
	}

	void onAmxUnload(IPawnScript&) override
	{
	}

	void onFree(IComponent*) override
	{
		world_.removeEventHandler(this);
		if (pawn_)
		{
			pawn_->getEventDispatcher().removeEventHandler(this);
		}
		if (core_)
		{
			core_->getEventDispatcher().removeEventHandler(this);
		}
		world_.saveDirty();
		world_.closeStorage();
	}

	void free() override
	{
		delete this;
	}

	void reset() override
	{
		world_.reset();
	}

	void onEntityCreated(const worlds::Entity& entity) override
	{
		const StringView type(entity.type.data(), entity.type.size());
		callPawnScripts(pawn_, "OnWorldSyncEntityCreated", [entityID = entity.id, type](IPawnScript* script, const char* callback) {
			script->Call(callback, DefaultReturnValue_True, entityID, type);
		});
	}

	void onEntityDestroyed(int entityID, const std::string& typeValue) override
	{
		const StringView type(typeValue.data(), typeValue.size());
		callPawnScripts(pawn_, "OnWorldSyncEntityDestroyed", [entityID, type](IPawnScript* script, const char* callback) {
			script->Call(callback, DefaultReturnValue_True, entityID, type);
		});
	}

	void onEntityStateChanged(const worlds::Entity& entity, const std::string& keyValue, const std::string& oldValue, const std::string& newValue) override
	{
		const StringView key(keyValue.data(), keyValue.size());
		const StringView oldState(oldValue.data(), oldValue.size());
		const StringView newState(newValue.data(), newValue.size());
		callPawnScripts(pawn_, "OnWorldSyncEntityStateChange", [entityID = entity.id, key, oldState, newState](IPawnScript* script, const char* callback) {
			script->Call(callback, DefaultReturnValue_True, entityID, key, oldState, newState);
		});
	}

	void onWorldLoaded(int entityCount, bool storageAvailable) override
	{
		callPawnScripts(pawn_, "OnWorldSyncLoaded", [entityCount, storageAvailable](IPawnScript* script, const char* callback) {
			script->Call(callback, DefaultReturnValue_True, entityCount, storageAvailable ? 1 : 0);
		});
	}

	void onWorldSaved(int entityCount, int changedCount, bool dirtyOnly) override
	{
		callPawnScripts(pawn_, "OnWorldSyncSaved", [entityCount, changedCount, dirtyOnly](IPawnScript* script, const char* callback) {
			script->Call(callback, DefaultReturnValue_True, entityCount, changedCount, dirtyOnly ? 1 : 0);
		});
	}

private:
	ICore* core_ = nullptr;
	IPawnComponent* pawn_ = nullptr;
	IObjectsComponent* objects_ = nullptr;
	INPCComponent* npcs_ = nullptr;
	IDatabasesComponent* databases_ = nullptr;
	ITextLabelsComponent* textLabels_ = nullptr;
	IVehiclesComponent* vehicles_ = nullptr;
	worlds::WorldSyncCore world_;
	std::unique_ptr<worlds::DoorModule> doorModule_;
	std::unique_ptr<worlds::CropModule> cropModule_;
	std::unique_ptr<worlds::PathModule> pathModule_;
	std::unique_ptr<worlds::VehicleModule> vehicleModule_;
};

COMPONENT_ENTRY_POINT()
{
	return new WorldSync();
}
