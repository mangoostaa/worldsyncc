#include <sdk.hpp>
#include "WorldSync.hpp"
#include "Doormodule.hpp"
#include "Cropmodule.hpp"
#include "Pathmodule.hpp"

#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <Server/Components/NPCs/npcs.hpp>
#include <Server/Components/Databases/databases.hpp>
#include <Server/Components/TextLabels/textlabels.hpp>

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
	return gWorld->createEntity(pawnString(amx, params[1]), position, static_cast<int>(params[5]), static_cast<int>(params[6]));
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

	return gWorld->setState(static_cast<int>(params[1]), pawnString(amx, params[2]), pawnString(amx, params[3])) ? 1 : 0;
}

cell AMX_NATIVE_CALL WS_GetState(AMX* amx, cell* params)
{
	if (!gWorld || !checkParams(params, 4))
	{
		return 0;
	}

	std::string value;
	if (!gWorld->getState(static_cast<int>(params[1]), pawnString(amx, params[2]), value))
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
	if (!gWorld->getType(static_cast<int>(params[1]), type))
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
	{ "WS_GetEntityWorld", WS_GetEntityWorld },
	{ "WS_GetEntityInterior", WS_GetEntityInterior },
	{ "WS_GetEntityCount", WS_GetEntityCount },
	{ "WS_GetEntityIDAt", WS_GetEntityIDAt },
	{ "WS_SetSimulated", WS_SetSimulated },
	{ "WS_Save", WS_Save },
	{ "WS_GetStats", WS_GetStats },
	{ "WS_SetDebug", WS_SetDebug },
	{ "WS_SetLogLevel", WS_SetLogLevel },
	{ "WS_GetLogLevel", WS_GetLogLevel },
	{ "WS_DebugSummary", WS_DebugSummary },
};
} // namespace

class WorldSync final : public IComponent, public CoreEventHandler, public PawnEventHandler
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
		world_.setDatabaseComponent(databases_);
		gPawn = pawn_;
		doorModule_ = std::make_unique<worlds::DoorModule>(world_, pawn_, objects_, core_);
		cropModule_ = std::make_unique<worlds::CropModule>(world_, pawn_, core_);
		pathModule_ = std::make_unique<worlds::PathModule>(world_, pawn_, npcs_, textLabels_, core_);
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
	}

	void onAmxUnload(IPawnScript&) override
	{
	}

	void onFree(IComponent*) override
	{
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

private:
	ICore* core_ = nullptr;
	IPawnComponent* pawn_ = nullptr;
	IObjectsComponent* objects_ = nullptr;
	INPCComponent* npcs_ = nullptr;
	IDatabasesComponent* databases_ = nullptr;
	ITextLabelsComponent* textLabels_ = nullptr;
	worlds::WorldSyncCore world_;
	std::unique_ptr<worlds::DoorModule> doorModule_;
	std::unique_ptr<worlds::CropModule> cropModule_;
	std::unique_ptr<worlds::PathModule> pathModule_;
};

COMPONENT_ENTRY_POINT()
{
	return new WorldSync();
}
