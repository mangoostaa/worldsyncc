#include "Vehiclemodule.hpp"

#include <iomanip>
#include <sstream>

namespace worlds
{
namespace
{
std::string floatToStr(float value)
{
	std::ostringstream stream;
	stream << std::fixed << std::setprecision(4) << value;
	return stream.str();
}

float strToFloat(const std::string& value, float fallback)
{
	if (value.empty())
	{
		return fallback;
	}
	try
	{
		return std::stof(value);
	}
	catch (...)
	{
		return fallback;
	}
}

int strToInt(const std::string& value, int fallback)
{
	if (value.empty())
	{
		return fallback;
	}
	try
	{
		return std::stoi(value);
	}
	catch (...)
	{
		return fallback;
	}
}

bool setPawnCell(AMX* amx, IPawnComponent* pawn, cell address, cell value)
{
	if (!pawn) return false;
	IPawnScript* script = pawn->getScript(amx);
	if (!script) return false;
	cell* phys = nullptr;
	if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys) return false;
	*phys = value;
	return true;
}
} // namespace

int VehicleModule::createVehicle(int model, Vec3 position, float zRotation, int colour1, int colour2, int respawnDelay, bool siren, int virtualWorld, int interior)
{
	if (model <= 0)
	{
		return 0;
	}

	const int id = core_.createEntity(VEHICLE_TYPE, position, virtualWorld, interior);
	setInt(id, VEHICLE_KEY_MODEL, model);
	setFloat(id, VEHICLE_KEY_ZROT, zRotation);
	setInt(id, VEHICLE_KEY_COLOUR1, colour1);
	setInt(id, VEHICLE_KEY_COLOUR2, colour2);
	setInt(id, VEHICLE_KEY_RESPAWN, respawnDelay);
	setBool(id, VEHICLE_KEY_SIREN, siren);
	setFloat(id, VEHICLE_KEY_HEALTH, 1000.0f);
	createRuntimeVehicle(id);
	return id;
}

bool VehicleModule::destroyVehicle(int vehicleEntityID)
{
	if (!isVehicle(vehicleEntityID))
	{
		return false;
	}
	destroyRuntimeVehicle(vehicleEntityID);
	return core_.destroyEntity(vehicleEntityID);
}

bool VehicleModule::isVehicle(int vehicleEntityID) const
{
	std::string type;
	return core_.getType(vehicleEntityID, type) && type == VEHICLE_TYPE;
}

void VehicleModule::rebuildVehicles()
{
	for (const Entity& entity : core_.entities())
	{
		if (entity.type == VEHICLE_TYPE)
		{
			createRuntimeVehicle(entity.id);
		}
	}
}

bool VehicleModule::createRuntimeVehicle(int vehicleEntityID)
{
	if (!vehicles_ || !isVehicle(vehicleEntityID))
	{
		return false;
	}
	if (getRuntimeVehicleID(vehicleEntityID) != 0)
	{
		return true;
	}

	Vec3 position;
	if (!core_.getPosition(vehicleEntityID, position))
	{
		return false;
	}

	const int model = getModel(vehicleEntityID);
	const int respawnDelay = getInt(vehicleEntityID, VEHICLE_KEY_RESPAWN, -1);
	IVehicle* vehicle = vehicles_->create(
		false,
		model,
		toVector3(position),
		getZRotation(vehicleEntityID),
		getInt(vehicleEntityID, VEHICLE_KEY_COLOUR1, -1),
		getInt(vehicleEntityID, VEHICLE_KEY_COLOUR2, -1),
		Seconds(respawnDelay),
		getBool(vehicleEntityID, VEHICLE_KEY_SIREN));
	if (!vehicle)
	{
		return false;
	}

	vehicle->setVirtualWorld(core_.getWorld(vehicleEntityID));
	vehicle->setInterior(core_.getInterior(vehicleEntityID));
	vehicle->setHealth(getHealth(vehicleEntityID));
	runtimeVehicles_[vehicleEntityID] = vehicle->getID();
	if (omp_)
	{
		omp_->printLn("[WorldSync/Vehicle] Vehicle entity=%d runtime=%d model=%d", vehicleEntityID, vehicle->getID(), model);
	}
	return true;
}

bool VehicleModule::destroyRuntimeVehicle(int vehicleEntityID)
{
	const auto it = runtimeVehicles_.find(vehicleEntityID);
	if (it == runtimeVehicles_.end())
	{
		return false;
	}
	if (vehicles_)
	{
		vehicles_->release(it->second);
	}
	runtimeVehicles_.erase(it);
	return true;
}

int VehicleModule::getRuntimeVehicleID(int vehicleEntityID) const
{
	const auto it = runtimeVehicles_.find(vehicleEntityID);
	return it == runtimeVehicles_.end() ? 0 : it->second;
}

int VehicleModule::getModel(int vehicleEntityID) const
{
	return isVehicle(vehicleEntityID) ? getInt(vehicleEntityID, VEHICLE_KEY_MODEL, 0) : 0;
}

bool VehicleModule::getPosition(int vehicleEntityID, Vec3& out) const
{
	return isVehicle(vehicleEntityID) && core_.getPosition(vehicleEntityID, out);
}

float VehicleModule::getZRotation(int vehicleEntityID) const
{
	return isVehicle(vehicleEntityID) ? getFloat(vehicleEntityID, VEHICLE_KEY_ZROT, 0.0f) : 0.0f;
}

bool VehicleModule::setHealth(int vehicleEntityID, float health)
{
	if (!isVehicle(vehicleEntityID))
	{
		return false;
	}
	setFloat(vehicleEntityID, VEHICLE_KEY_HEALTH, health);
	if (vehicles_)
	{
		if (IVehicle* vehicle = vehicles_->get(getRuntimeVehicleID(vehicleEntityID)))
		{
			vehicle->setHealth(health);
		}
	}
	return true;
}

float VehicleModule::getHealth(int vehicleEntityID) const
{
	if (!isVehicle(vehicleEntityID))
	{
		return 0.0f;
	}
	if (vehicles_)
	{
		if (IVehicle* vehicle = vehicles_->get(getRuntimeVehicleID(vehicleEntityID)))
		{
			return vehicle->getHealth();
		}
	}
	return getFloat(vehicleEntityID, VEHICLE_KEY_HEALTH, 1000.0f);
}

bool VehicleModule::setColours(int vehicleEntityID, int colour1, int colour2)
{
	if (!isVehicle(vehicleEntityID))
	{
		return false;
	}
	setInt(vehicleEntityID, VEHICLE_KEY_COLOUR1, colour1);
	setInt(vehicleEntityID, VEHICLE_KEY_COLOUR2, colour2);
	if (vehicles_)
	{
		if (IVehicle* vehicle = vehicles_->get(getRuntimeVehicleID(vehicleEntityID)))
		{
			vehicle->setColour(colour1, colour2);
		}
	}
	return true;
}

bool VehicleModule::getColours(int vehicleEntityID, int& colour1, int& colour2) const
{
	if (!isVehicle(vehicleEntityID))
	{
		return false;
	}
	colour1 = getInt(vehicleEntityID, VEHICLE_KEY_COLOUR1, -1);
	colour2 = getInt(vehicleEntityID, VEHICLE_KEY_COLOUR2, -1);
	return true;
}

bool VehicleModule::respawnVehicle(int vehicleEntityID)
{
	if (!isVehicle(vehicleEntityID))
	{
		return false;
	}
	if (vehicles_)
	{
		if (IVehicle* vehicle = vehicles_->get(getRuntimeVehicleID(vehicleEntityID)))
		{
			vehicle->respawn();
			return true;
		}
	}
	return createRuntimeVehicle(vehicleEntityID);
}

bool VehicleModule::repairVehicle(int vehicleEntityID)
{
	if (!isVehicle(vehicleEntityID) || !vehicles_)
	{
		return false;
	}
	IVehicle* vehicle = vehicles_->get(getRuntimeVehicleID(vehicleEntityID));
	if (!vehicle)
	{
		return false;
	}
	vehicle->repair();
	setFloat(vehicleEntityID, VEHICLE_KEY_HEALTH, vehicle->getHealth());
	return true;
}

float VehicleModule::getFloat(int id, const char* key, float fallback) const
{
	std::string value;
	if (!core_.getState(id, key, value))
	{
		return fallback;
	}
	return strToFloat(value, fallback);
}

void VehicleModule::setFloat(int id, const char* key, float value)
{
	core_.setState(id, key, floatToStr(value));
}

int VehicleModule::getInt(int id, const char* key, int fallback) const
{
	std::string value;
	if (!core_.getState(id, key, value))
	{
		return fallback;
	}
	return strToInt(value, fallback);
}

void VehicleModule::setInt(int id, const char* key, int value)
{
	core_.setState(id, key, std::to_string(value));
}

bool VehicleModule::getBool(int id, const char* key) const
{
	return getInt(id, key, 0) != 0;
}

void VehicleModule::setBool(int id, const char* key, bool value)
{
	setInt(id, key, value ? 1 : 0);
}

Vector3 VehicleModule::toVector3(Vec3 value) const
{
	return Vector3(value.x, value.y, value.z);
}

static VehicleModule* s_vehicleModule = nullptr;

static cell AMX_NATIVE_CALL n_WS_CreateVehicle(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 11) return 0;
	const Vec3 position { amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]) };
	return s_vehicleModule->createVehicle(
		static_cast<int>(params[1]),
		position,
		amx_ctof(params[5]),
		static_cast<int>(params[6]),
		static_cast<int>(params[7]),
		static_cast<int>(params[8]),
		params[9] != 0,
		static_cast<int>(params[10]),
		static_cast<int>(params[11]));
}

static cell AMX_NATIVE_CALL n_WS_DestroyVehicle(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_vehicleModule->destroyVehicle(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_GetVehicleID(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_vehicleModule->getRuntimeVehicleID(static_cast<int>(params[1]));
}

static cell AMX_NATIVE_CALL n_WS_GetVehicleModel(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_vehicleModule->getModel(static_cast<int>(params[1]));
}

static cell AMX_NATIVE_CALL n_WS_GetVehiclePos(AMX* amx, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 4) return 0;
	Vec3 position;
	if (!s_vehicleModule->getPosition(static_cast<int>(params[1]), position)) return 0;
	return setPawnCell(amx, s_vehicleModule->pawnRef(), params[2], amx_ftoc(position.x))
		&& setPawnCell(amx, s_vehicleModule->pawnRef(), params[3], amx_ftoc(position.y))
		&& setPawnCell(amx, s_vehicleModule->pawnRef(), params[4], amx_ftoc(position.z))
		? 1
		: 0;
}

static cell AMX_NATIVE_CALL n_WS_GetVehicleZRot(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return amx_ftoc(s_vehicleModule->getZRotation(static_cast<int>(params[1])));
}

static cell AMX_NATIVE_CALL n_WS_SetVehicleHealth(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 2) return 0;
	return s_vehicleModule->setHealth(static_cast<int>(params[1]), amx_ctof(params[2])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_GetVehicleHealth(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return amx_ftoc(s_vehicleModule->getHealth(static_cast<int>(params[1])));
}

static cell AMX_NATIVE_CALL n_WS_SetVehicleColours(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 3) return 0;
	return s_vehicleModule->setColours(static_cast<int>(params[1]), static_cast<int>(params[2]), static_cast<int>(params[3])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_GetVehicleColours(AMX* amx, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 3) return 0;
	int colour1 = -1;
	int colour2 = -1;
	if (!s_vehicleModule->getColours(static_cast<int>(params[1]), colour1, colour2)) return 0;
	return setPawnCell(amx, s_vehicleModule->pawnRef(), params[2], colour1)
		&& setPawnCell(amx, s_vehicleModule->pawnRef(), params[3], colour2)
		? 1
		: 0;
}

static cell AMX_NATIVE_CALL n_WS_RespawnVehicle(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_vehicleModule->respawnVehicle(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_RepairVehicle(AMX*, cell* params)
{
	if (!s_vehicleModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_vehicleModule->repairVehicle(static_cast<int>(params[1])) ? 1 : 0;
}

static const AMX_NATIVE_INFO VehicleNatives[] = {
	{ "WS_CreateVehicle", n_WS_CreateVehicle },
	{ "WS_DestroyVehicle", n_WS_DestroyVehicle },
	{ "WS_GetVehicleID", n_WS_GetVehicleID },
	{ "WS_GetVehicleModel", n_WS_GetVehicleModel },
	{ "WS_GetVehiclePos", n_WS_GetVehiclePos },
	{ "WS_GetVehicleZRot", n_WS_GetVehicleZRot },
	{ "WS_SetVehicleHealth", n_WS_SetVehicleHealth },
	{ "WS_GetVehicleHealth", n_WS_GetVehicleHealth },
	{ "WS_SetVehicleColours", n_WS_SetVehicleColours },
	{ "WS_GetVehicleColours", n_WS_GetVehicleColours },
	{ "WS_RespawnVehicle", n_WS_RespawnVehicle },
	{ "WS_RepairVehicle", n_WS_RepairVehicle },
};

void VehicleModule::registerNatives(IPawnScript& script)
{
	s_vehicleModule = this;
	script.Register(VehicleNatives, static_cast<int>(sizeof(VehicleNatives) / sizeof(VehicleNatives[0])));
}
} // namespace worlds
