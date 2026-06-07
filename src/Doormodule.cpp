// ============================================================================
//  WorldSync — DoorModule.cpp
// ============================================================================

#include "Doormodule.hpp"

#include <sstream>
#include <iomanip>

namespace worlds
{

// ── Helpers internos ──────────────────────────────────────────────────────────

static std::string floatToStr(float v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4) << v;
    return ss.str();
}

static float strToFloat(const std::string& s, float fallback)
{
    if (s.empty()) return fallback;
    try { return std::stof(s); }
    catch (...) { return fallback; }
}

static int strToInt(const std::string& s, int fallback)
{
    if (s.empty()) return fallback;
    try { return std::stoi(s); }
    catch (...) { return fallback; }
}

// ── Helpers miembro ───────────────────────────────────────────────────────────

bool DoorModule::getBool(int id, const char* key) const
{
    std::string val;
    core_.getState(id, key, val);
    return val == "1";
}

void DoorModule::setBool(int id, const char* key, bool value)
{
    core_.setState(id, key, value ? "1" : "0");
}

float DoorModule::getFloat(int id, const char* key, float fallback) const
{
    std::string val;
    if (!core_.getState(id, key, val)) return fallback;
    return strToFloat(val, fallback);
}

void DoorModule::setFloat(int id, const char* key, float value)
{
    core_.setState(id, key, floatToStr(value));
}

int DoorModule::getInt(int id, const char* key, int fallback) const
{
    std::string val;
    if (!core_.getState(id, key, val)) return fallback;
    return strToInt(val, fallback);
}

void DoorModule::setInt(int id, const char* key, int value)
{
    core_.setState(id, key, std::to_string(value));
}

// ── isDoor ────────────────────────────────────────────────────────────────────

bool DoorModule::isDoor(int doorID) const
{
    std::string type;
    if (!core_.getType(doorID, type)) return false;
    return type == DOOR_TYPE;
}

// ── createDoor ────────────────────────────────────────────────────────────────

int DoorModule::createDoor(int model,
    Vec3 position,
    Vec3 closeRot,
    Vec3 openRot,
    int virtualWorld,
    int interior)
{
    const int id = core_.createEntity(DOOR_TYPE, position, virtualWorld, interior);

    // Estado inicial: cerrada, desbloqueada
    setBool(id, KEY_OPEN,   false);
    setBool(id, KEY_LOCKED, false);
    setInt (id, KEY_MODEL,  model);

    // Rotaciones
    setFloat(id, KEY_RX,      closeRot.x);
    setFloat(id, KEY_RY,      closeRot.y);
    setFloat(id, KEY_RZ,      closeRot.z);
    setFloat(id, KEY_OPEN_RX, openRot.x);
    setFloat(id, KEY_OPEN_RY, openRot.y);
    setFloat(id, KEY_OPEN_RZ, openRot.z);

    if (omp_)
    {
        omp_->printLn("[WorldSync/Door] Puerta creada: id=%d modelo=%d", id, model);
    }

    createObjectForDoor(id);
    return id;
}

// ── destroyDoor ───────────────────────────────────────────────────────────────

bool DoorModule::destroyDoor(int doorID)
{
    if (!isDoor(doorID)) return false;
    destroyObjectForDoor(doorID);
    return core_.destroyEntity(doorID);
}

// ── setOpen ───────────────────────────────────────────────────────────────────

bool DoorModule::setOpen(int doorID, bool open)
{
    if (!isDoor(doorID)) return false;

    // Si está bloqueada no se puede abrir (pero sí cerrar)
    if (open && isLocked(doorID)) return false;

    const bool current = isOpen(doorID);
    if (current == open) return true;  // ya estaba en ese estado

    setBool(doorID, KEY_OPEN, open);
    applyObjectState(doorID, 3.0f);

    // Notifica a Pawn
    fireCallback(doorID, open);

    return true;
}

bool DoorModule::isOpen(int doorID) const
{
    if (!isDoor(doorID)) return false;
    return getBool(doorID, KEY_OPEN);
}

// ── setLocked ─────────────────────────────────────────────────────────────────

bool DoorModule::setLocked(int doorID, bool locked)
{
    if (!isDoor(doorID)) return false;

    // Bloquear cierra la puerta si estaba abierta
    if (locked && isOpen(doorID))
    {
        setBool(doorID, KEY_OPEN, false);
        applyObjectState(doorID, 3.0f);
        fireCallback(doorID, false);
    }

    setBool(doorID, KEY_LOCKED, locked);
    return true;
}

bool DoorModule::isLocked(int doorID) const
{
    if (!isDoor(doorID)) return false;
    return getBool(doorID, KEY_LOCKED);
}

// ── Getters ───────────────────────────────────────────────────────────────────

int DoorModule::getModel(int doorID) const
{
    if (!isDoor(doorID)) return 0;
    return getInt(doorID, KEY_MODEL, 0);
}

bool DoorModule::getPosition(int doorID, Vec3& out) const
{
    if (!isDoor(doorID)) return false;
    return core_.getPosition(doorID, out);
}

bool DoorModule::getCloseRot(int doorID, Vec3& out) const
{
    if (!isDoor(doorID)) return false;
    out.x = getFloat(doorID, KEY_RX);
    out.y = getFloat(doorID, KEY_RY);
    out.z = getFloat(doorID, KEY_RZ);
    return true;
}

bool DoorModule::getOpenRot(int doorID, Vec3& out) const
{
    if (!isDoor(doorID)) return false;
    out.x = getFloat(doorID, KEY_OPEN_RX);
    out.y = getFloat(doorID, KEY_OPEN_RY);
    out.z = getFloat(doorID, KEY_OPEN_RZ);
    return true;
}

Vector3 DoorModule::toVector3(Vec3 value) const
{
	return Vector3(value.x, value.y, value.z);
}

void DoorModule::rebuildObjects()
{
	for (const Entity& entity : core_.entities())
	{
		if (entity.type == DOOR_TYPE)
		{
			createObjectForDoor(entity.id);
		}
	}
}

bool DoorModule::createObjectForDoor(int doorID)
{
	if (!objects_ || !isDoor(doorID))
	{
		return false;
	}
	if (doorObjects_.find(doorID) != doorObjects_.end())
	{
		return applyObjectState(doorID);
	}

	Vec3 position;
	Vec3 rotation;
	if (!getPosition(doorID, position))
	{
		return false;
	}
	if (isOpen(doorID))
	{
		getOpenRot(doorID, rotation);
	}
	else
	{
		getCloseRot(doorID, rotation);
	}

	IObject* object = objects_->create(getModel(doorID), toVector3(position), toVector3(rotation), 200.0f);
	if (!object)
	{
		return false;
	}

	doorObjects_[doorID] = object->getID();
	return true;
}

bool DoorModule::destroyObjectForDoor(int doorID)
{
	if (!objects_)
	{
		doorObjects_.erase(doorID);
		return false;
	}

	const auto it = doorObjects_.find(doorID);
	if (it == doorObjects_.end())
	{
		return false;
	}

	objects_->release(it->second);
	doorObjects_.erase(it);
	return true;
}

bool DoorModule::applyObjectState(int doorID, float speed)
{
	if (!objects_ || !isDoor(doorID))
	{
		return false;
	}

	auto it = doorObjects_.find(doorID);
	if (it == doorObjects_.end())
	{
		return createObjectForDoor(doorID);
	}

	IObject* object = objects_->get(it->second);
	if (!object)
	{
		doorObjects_.erase(it);
		return createObjectForDoor(doorID);
	}

	Vec3 position;
	Vec3 rotation;
	if (!getPosition(doorID, position))
	{
		return false;
	}
	if (isOpen(doorID))
	{
		getOpenRot(doorID, rotation);
	}
	else
	{
		getCloseRot(doorID, rotation);
	}

	ObjectMoveData move;
	move.targetPos = toVector3(position);
	move.targetRot = toVector3(rotation);
	move.speed = speed > 0.0f ? speed : 1000.0f;
	object->move(move);
	return true;
}

int DoorModule::getObjectID(int doorID) const
{
	const auto it = doorObjects_.find(doorID);
	return it == doorObjects_.end() ? -1 : it->second;
}

// ── Callback hacia Pawn ───────────────────────────────────────────────────────

void DoorModule::fireCallback(int doorID, bool isOpen)
{
    if (!pawn_) return;

    auto callScript = [doorID, isOpen](IPawnScript* script) {
        if (!script) return;
        script->Call("OnDoorStateChange", DefaultReturnValue_True, doorID, isOpen ? 1 : 0);
    };

    callScript(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts())
    {
        callScript(script);
    }
}

// ── Natives Pawn ─────────────────────────────────────────────────────────────
//
//  Usamos un puntero estático al módulo para los wrappers C de las natives.
//  Se asigna en registerNatives().

static DoorModule* s_doorModule = nullptr;

// Lectura de float desde parámetro AMX
static float amxParamFloat(cell* params, int index)
{
    return amx_ctof(params[index]);
}

// Escritura de float en referencia AMX
static bool amxSetFloat(AMX* amx, IPawnComponent* pawn, cell address, float value)
{
    if (!pawn) return false;
    IPawnScript* script = pawn->getScript(amx);
    if (!script) return false;
    cell* phys = nullptr;
    if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys) return false;
    *phys = amx_ftoc(value);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  WS_CreateDoor(model, Float:x, Float:y, Float:z,
//                Float:rx,  Float:ry,  Float:rz,
//                Float:orx, Float:ory, Float:orz,
//                vw=0, interior=0)
static cell AMX_NATIVE_CALL n_WS_CreateDoor(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 12) return 0;

    const int    model    = static_cast<int>(params[1]);
    const Vec3   pos      = { amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]) };
    const Vec3   closeRot = { amx_ctof(params[5]), amx_ctof(params[6]), amx_ctof(params[7]) };
    const Vec3   openRot  = { amx_ctof(params[8]), amx_ctof(params[9]), amx_ctof(params[10]) };
    const int    vw       = static_cast<int>(params[11]);
    const int    interior = static_cast<int>(params[12]);

    return static_cast<cell>(s_doorModule->createDoor(model, pos, closeRot, openRot, vw, interior));
}

//  WS_DestroyDoor(doorID)
static cell AMX_NATIVE_CALL n_WS_DestroyDoor(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 1) return 0;
    return s_doorModule->destroyDoor(static_cast<int>(params[1])) ? 1 : 0;
}

//  WS_SetDoorOpen(doorID, bool:open)
static cell AMX_NATIVE_CALL n_WS_SetDoorOpen(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 2) return 0;
    return s_doorModule->setOpen(static_cast<int>(params[1]), params[2] != 0) ? 1 : 0;
}

//  WS_IsDoorOpen(doorID)
static cell AMX_NATIVE_CALL n_WS_IsDoorOpen(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 1) return 0;
    return s_doorModule->isOpen(static_cast<int>(params[1])) ? 1 : 0;
}

//  WS_SetDoorLocked(doorID, bool:locked)
static cell AMX_NATIVE_CALL n_WS_SetDoorLocked(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 2) return 0;
    return s_doorModule->setLocked(static_cast<int>(params[1]), params[2] != 0) ? 1 : 0;
}

//  WS_IsDoorLocked(doorID)
static cell AMX_NATIVE_CALL n_WS_IsDoorLocked(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 1) return 0;
    return s_doorModule->isLocked(static_cast<int>(params[1])) ? 1 : 0;
}

//  WS_GetDoorModel(doorID)
static cell AMX_NATIVE_CALL n_WS_GetDoorModel(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 1) return 0;
    return static_cast<cell>(s_doorModule->getModel(static_cast<int>(params[1])));
}

//  WS_GetDoorObject(doorID)
static cell AMX_NATIVE_CALL n_WS_GetDoorObject(AMX*, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 1) return -1;
    return static_cast<cell>(s_doorModule->getObjectID(static_cast<int>(params[1])));
}

//  WS_GetDoorPos(doorID, &Float:x, &Float:y, &Float:z)
static cell AMX_NATIVE_CALL n_WS_GetDoorPos(AMX* amx, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 4) return 0;
    Vec3 pos;
    if (!s_doorModule->getPosition(static_cast<int>(params[1]), pos)) return 0;
    auto* pawn = s_doorModule->pawnRef();
    amxSetFloat(amx, pawn, params[2], pos.x);
    amxSetFloat(amx, pawn, params[3], pos.y);
    amxSetFloat(amx, pawn, params[4], pos.z);
    return 1;
}

//  WS_GetDoorRot(doorID, &Float:rx, &Float:ry, &Float:rz,
//                         &Float:orx, &Float:ory, &Float:orz)
static cell AMX_NATIVE_CALL n_WS_GetDoorRot(AMX* amx, cell* params)
{
    if (!s_doorModule || params[0] / (cell)sizeof(cell) < 7) return 0;
    Vec3 closeRot, openRot;
    const int id = static_cast<int>(params[1]);
    if (!s_doorModule->getCloseRot(id, closeRot)) return 0;
    if (!s_doorModule->getOpenRot(id, openRot))   return 0;
    auto* pawn = s_doorModule->pawnRef();
    amxSetFloat(amx, pawn, params[2], closeRot.x);
    amxSetFloat(amx, pawn, params[3], closeRot.y);
    amxSetFloat(amx, pawn, params[4], closeRot.z);
    amxSetFloat(amx, pawn, params[5], openRot.x);
    amxSetFloat(amx, pawn, params[6], openRot.y);
    amxSetFloat(amx, pawn, params[7], openRot.z);
    return 1;
}

static const AMX_NATIVE_INFO DoorNatives[] = {
    { "WS_CreateDoor",   n_WS_CreateDoor   },
    { "WS_DestroyDoor",  n_WS_DestroyDoor  },
    { "WS_SetDoorOpen",  n_WS_SetDoorOpen  },
    { "WS_IsDoorOpen",   n_WS_IsDoorOpen   },
    { "WS_SetDoorLocked",n_WS_SetDoorLocked},
    { "WS_IsDoorLocked", n_WS_IsDoorLocked },
    { "WS_GetDoorModel", n_WS_GetDoorModel },
    { "WS_GetDoorObject",n_WS_GetDoorObject},
    { "WS_GetDoorPos",   n_WS_GetDoorPos   },
    { "WS_GetDoorRot",   n_WS_GetDoorRot   },
};

void DoorModule::registerNatives(IPawnScript& script)
{
    s_doorModule = this;
    script.Register(DoorNatives, static_cast<int>(sizeof(DoorNatives) / sizeof(DoorNatives[0])));
}

// Expone pawn_ para los helpers de natives (acceso package-internal)
} // namespace worlds
