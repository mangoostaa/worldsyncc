// ============================================================================
//  WorldSync — CropModule.cpp
// ============================================================================

#include "Cropmodule.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace worlds
{

// ── Helpers de estado ─────────────────────────────────────────────────────────

static std::string floatToStr(float v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4) << v;
    return ss.str();
}

int CropModule::getInt(int id, const char* key, int fallback) const
{
    std::string val;
    if (!core_.getState(id, key, val) || val.empty()) return fallback;
    try { return std::stoi(val); } catch (...) { return fallback; }
}

void CropModule::setInt(int id, const char* key, int value)
{
    core_.setState(id, key, std::to_string(value));
}

float CropModule::getFloat(int id, const char* key, float fallback) const
{
    std::string val;
    if (!core_.getState(id, key, val) || val.empty()) return fallback;
    try { return std::stof(val); } catch (...) { return fallback; }
}

void CropModule::setFloat(int id, const char* key, float value)
{
    core_.setState(id, key, floatToStr(value));
}

std::string CropModule::getStr(int id, const char* key) const
{
    std::string val;
    core_.getState(id, key, val);
    return val;
}

void CropModule::setStr(int id, const char* key, const std::string& value)
{
    core_.setState(id, key, value);
}

// ── isCrop ────────────────────────────────────────────────────────────────────

bool CropModule::isCrop(int cropID) const
{
    std::string type;
    if (!core_.getType(cropID, type)) return false;
    return type == CROP_TYPE;
}

// ── createCrop ────────────────────────────────────────────────────────────────

int CropModule::createCrop(const std::string& species,
    Vec3 position,
    float growthRatePerSecond,
    int   maxHarvests,
    int   virtualWorld,
    int   interior)
{
    if (species.empty()) return 0;

    const int id = core_.createEntity(CROP_TYPE, position, virtualWorld, interior);

    setStr  (id, CROP_KEY_SPECIES,     species);
    setInt  (id, CROP_KEY_GROWTH,      0);
    setFloat(id, CROP_KEY_RATE,        growthRatePerSecond > 0.0f ? growthRatePerSecond : 1.0f);
    setInt  (id, CROP_KEY_MAX_HARVEST, maxHarvests);
    setInt  (id, CROP_KEY_HARVESTS,    0);
    setInt  (id, CROP_KEY_READY,       0);

    // Activa la simulación en el core para que tickCrops() lo procese
    core_.setSimulated(id, true);

    if (omp_)
    {
        omp_->printLn("[WorldSync/Crop] Cultivo creado: id=%d especie=%s rate=%.2f/s",
            id, species.c_str(), growthRatePerSecond);
    }

    return id;
}

// ── destroyCrop ───────────────────────────────────────────────────────────────

bool CropModule::destroyCrop(int cropID)
{
    if (!isCrop(cropID)) return false;
    return core_.destroyEntity(cropID);
}

// ── getGrowth / setGrowth ─────────────────────────────────────────────────────

int CropModule::getGrowth(int cropID) const
{
    if (!isCrop(cropID)) return -1;
    return getInt(cropID, CROP_KEY_GROWTH, 0);
}

bool CropModule::setGrowth(int cropID, int value)
{
    if (!isCrop(cropID)) return false;
    const int clamped = std::clamp(value, 0, 100);
    setInt(cropID, CROP_KEY_GROWTH, clamped);

    // Si se fuerza a 100 manualmente, marcar como listo y notificar
    if (clamped == 100 && getInt(cropID, CROP_KEY_READY, 0) == 0)
    {
        setInt(cropID, CROP_KEY_READY, 1);
        fireCropReady(cropID);
    }
    return true;
}

bool CropModule::isReady(int cropID) const
{
    if (!isCrop(cropID)) return false;
    return getInt(cropID, CROP_KEY_GROWTH, 0) >= 100;
}

// ── harvest ───────────────────────────────────────────────────────────────────

bool CropModule::harvest(int cropID)
{
    if (!isCrop(cropID))    return false;
    if (!isReady(cropID))   return false;

    const int harvests    = getInt(cropID, CROP_KEY_HARVESTS,    0);
    const int maxHarvests = getInt(cropID, CROP_KEY_MAX_HARVEST, 0);
    const int newHarvests = harvests + 1;

    setInt(cropID, CROP_KEY_HARVESTS, newHarvests);

    // ¿Agotó las cosechas?
    if (maxHarvests > 0 && newHarvests >= maxHarvests)
    {
        // Destruye el cultivo definitivamente
        core_.destroyEntity(cropID);
        if (omp_)
            omp_->printLn("[WorldSync/Crop] Cultivo %d agotado y destruido.", cropID);
        return true;
    }

    // Resetea para volver a crecer
    setInt(cropID, CROP_KEY_GROWTH, 0);
    setInt(cropID, CROP_KEY_READY,  0);

    if (omp_)
        omp_->printLn("[WorldSync/Crop] Cultivo %d cosechado (x%d).", cropID, newHarvests);

    return true;
}

int CropModule::getHarvests(int cropID) const
{
    if (!isCrop(cropID)) return 0;
    return getInt(cropID, CROP_KEY_HARVESTS, 0);
}

bool CropModule::getSpecies(int cropID, std::string& out) const
{
    if (!isCrop(cropID)) return false;
    out = getStr(cropID, CROP_KEY_SPECIES);
    return true;
}

// ── tickCrops ─────────────────────────────────────────────────────────────────
// Se llama cada frame desde WorldSync::onTick con el tiempo transcurrido.
// Solo itera entidades simulated==true de tipo "crop".
// Acumula tiempo fraccional para no perder precisión sub-segundo.

void CropModule::tickCrops(std::chrono::milliseconds elapsed)
{
    const float elapsedSec = static_cast<float>(elapsed.count()) / 1000.0f;

    for (const Entity& e : core_.entities())
    {
        if (!e.simulated) continue;

        std::string type;
        core_.getType(e.id, type);
        if (type != CROP_TYPE) continue;

        // Si ya está listo no hace falta calcular más
        const int currentGrowth = getInt(e.id, CROP_KEY_GROWTH, 0);
        if (currentGrowth >= 100) continue;

        // Acumula tiempo fraccional en el estado para no perder sub-segundos
        // Usamos un campo auxiliar "acc" (acumulador de tiempo en segundos)
        float acc = getFloat(e.id, "acc", 0.0f);
        acc += elapsedSec;

        const float rate    = getFloat(e.id, CROP_KEY_RATE, 1.0f);
        const int   gained  = static_cast<int>(acc * rate);  // puntos enteros ganados

        if (gained > 0)
        {
            acc -= static_cast<float>(gained) / rate;       // descuenta solo lo usado
            const int newGrowth = std::min(100, currentGrowth + gained);
            setInt(e.id, CROP_KEY_GROWTH, newGrowth);

            // Si llegó a 100 y aún no se notificó este ciclo de crecimiento
            if (newGrowth >= 100 && getInt(e.id, CROP_KEY_READY, 0) == 0)
            {
                setInt(e.id, CROP_KEY_READY, 1);
                fireCropReady(e.id);
            }
        }

        // Siempre actualiza el acumulador (puede ser 0 si no ganó nada)
        setFloat(e.id, "acc", acc);
    }
}

// ── Callback Pawn ─────────────────────────────────────────────────────────────

void CropModule::fireCropReady(int cropID)
{
    if (!pawn_) return;

    if (omp_)
        omp_->printLn("[WorldSync/Crop] Cultivo %d listo — disparando OnWorldSyncCropReady.", cropID);

    auto callScript = [cropID](IPawnScript* script) {
        if (!script) return;
        script->Call("OnWorldSyncCropReady", DefaultReturnValue_True, cropID);
    };

    callScript(pawn_->mainScript());
    for (IPawnScript* script : pawn_->sideScripts())
    {
        callScript(script);
    }
}

// ── Natives Pawn ─────────────────────────────────────────────────────────────

static CropModule* s_cropModule = nullptr;

// Helper: escribe string en referencia Pawn
static bool amxSetStr(AMX* amx, IPawnComponent* pawn,
    cell address, const std::string& value, size_t maxLen)
{
    if (!pawn || maxLen == 0) return false;
    IPawnScript* script = pawn->getScript(amx);
    if (!script) return false;
    cell* phys = nullptr;
    if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys) return false;
    return script->SetString(phys,
        StringView(value.data(), value.size()),
        false, false, maxLen) == AMX_ERR_NONE;
}

// Helper: lee string desde parámetro Pawn
static std::string amxGetStr(AMX* amx, IPawnComponent* pawn, cell address)
{
    if (!pawn) return {};
    IPawnScript* script = pawn->getScript(amx);
    if (!script) return {};
    cell* phys = nullptr;
    if (script->GetAddr(address, &phys) != AMX_ERR_NONE || !phys) return {};
    int length = 0;
    script->StrLen(phys, &length);
    std::string out(static_cast<size_t>(length), '\0');
    if (length > 0)
        script->GetString(&out[0], phys, false, static_cast<size_t>(length + 1));
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  WS_CreateCrop(const species[], Float:x, Float:y, Float:z,
//                Float:growthRate=1.0, maxHarvests=0, vw=0, interior=0)
static cell AMX_NATIVE_CALL n_WS_CreateCrop(AMX* amx, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 8) return 0;

    const std::string species = amxGetStr(amx, s_cropModule->pawnRef(), params[1]);
    if (species.empty()) return 0;
    const Vec3 pos = { amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]) };
    const float rate       = amx_ctof(params[5]);
    const int   maxHarv    = static_cast<int>(params[6]);
    const int   vw         = static_cast<int>(params[7]);
    const int   interior   = static_cast<int>(params[8]);

    return static_cast<cell>(s_cropModule->createCrop(species, pos, rate, maxHarv, vw, interior));
}

//  WS_DestroyCrop(cropID)
static cell AMX_NATIVE_CALL n_WS_DestroyCrop(AMX*, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 1) return 0;
    return s_cropModule->destroyCrop(static_cast<int>(params[1])) ? 1 : 0;
}

//  WS_GetCropGrowth(cropID)  → 0-100
static cell AMX_NATIVE_CALL n_WS_GetCropGrowth(AMX*, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 1) return 0;
    return static_cast<cell>(s_cropModule->getGrowth(static_cast<int>(params[1])));
}

//  WS_SetCropGrowth(cropID, value)
static cell AMX_NATIVE_CALL n_WS_SetCropGrowth(AMX*, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 2) return 0;
    return s_cropModule->setGrowth(
        static_cast<int>(params[1]),
        static_cast<int>(params[2])) ? 1 : 0;
}

//  WS_IsCropReady(cropID)
static cell AMX_NATIVE_CALL n_WS_IsCropReady(AMX*, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 1) return 0;
    return s_cropModule->isReady(static_cast<int>(params[1])) ? 1 : 0;
}

//  WS_HarvestCrop(cropID)
static cell AMX_NATIVE_CALL n_WS_HarvestCrop(AMX*, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 1) return 0;
    return s_cropModule->harvest(static_cast<int>(params[1])) ? 1 : 0;
}

//  WS_GetCropHarvests(cropID)
static cell AMX_NATIVE_CALL n_WS_GetCropHarvests(AMX*, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 1) return 0;
    return static_cast<cell>(s_cropModule->getHarvests(static_cast<int>(params[1])));
}

//  WS_GetCropSpecies(cropID, output[], maxlen)
static cell AMX_NATIVE_CALL n_WS_GetCropSpecies(AMX* amx, cell* params)
{
    if (!s_cropModule || !params || params[0] / (cell)sizeof(cell) < 3 || params[3] <= 0) return 0;
    std::string species;
    if (!s_cropModule->getSpecies(static_cast<int>(params[1]), species)) return 0;
    return amxSetStr(amx, s_cropModule->pawnRef(), params[2], species,
        static_cast<size_t>(params[3])) ? 1 : 0;
}

static const AMX_NATIVE_INFO CropNatives[] = {
    { "WS_CreateCrop",      n_WS_CreateCrop      },
    { "WS_DestroyCrop",     n_WS_DestroyCrop     },
    { "WS_GetCropGrowth",   n_WS_GetCropGrowth   },
    { "WS_SetCropGrowth",   n_WS_SetCropGrowth   },
    { "WS_IsCropReady",     n_WS_IsCropReady     },
    { "WS_HarvestCrop",     n_WS_HarvestCrop     },
    { "WS_GetCropHarvests", n_WS_GetCropHarvests },
    { "WS_GetCropSpecies",  n_WS_GetCropSpecies  },
};

void CropModule::registerNatives(IPawnScript& script)
{
    s_cropModule = this;
    script.Register(CropNatives, static_cast<int>(sizeof(CropNatives) / sizeof(CropNatives[0])));
}

} // namespace worlds
