#pragma once

#include "WorldSync.hpp"

#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <sdk.hpp>
#include <string>
#include <unordered_map>

namespace worlds
{
constexpr const char* VEHICLE_TYPE = "vehicle";
constexpr const char* VEHICLE_KEY_MODEL = "model";
constexpr const char* VEHICLE_KEY_ZROT = "zrot";
constexpr const char* VEHICLE_KEY_COLOUR1 = "colour1";
constexpr const char* VEHICLE_KEY_COLOUR2 = "colour2";
constexpr const char* VEHICLE_KEY_RESPAWN = "respawn";
constexpr const char* VEHICLE_KEY_SIREN = "siren";
constexpr const char* VEHICLE_KEY_HEALTH = "health";

class VehicleModule
{
public:
	VehicleModule(WorldSyncCore& core, IPawnComponent* pawn, IVehiclesComponent* vehicles, ICore* omp)
		: core_(core)
		, pawn_(pawn)
		, vehicles_(vehicles)
		, omp_(omp)
	{
	}

	int createVehicle(int model, Vec3 position, float zRotation, int colour1, int colour2, int respawnDelay, bool siren, int virtualWorld = 0, int interior = 0);
	bool destroyVehicle(int vehicleEntityID);
	bool isVehicle(int vehicleEntityID) const;

	void rebuildVehicles();
	bool createRuntimeVehicle(int vehicleEntityID);
	bool destroyRuntimeVehicle(int vehicleEntityID);
	int getRuntimeVehicleID(int vehicleEntityID) const;

	int getModel(int vehicleEntityID) const;
	bool getPosition(int vehicleEntityID, Vec3& out) const;
	float getZRotation(int vehicleEntityID) const;
	bool setHealth(int vehicleEntityID, float health);
	float getHealth(int vehicleEntityID) const;
	bool setColours(int vehicleEntityID, int colour1, int colour2);
	bool getColours(int vehicleEntityID, int& colour1, int& colour2) const;
	bool respawnVehicle(int vehicleEntityID);
	bool repairVehicle(int vehicleEntityID);

	void registerNatives(IPawnScript& script);
	IPawnComponent* pawnRef() const { return pawn_; }

private:
	float getFloat(int id, const char* key, float fallback = 0.0f) const;
	void setFloat(int id, const char* key, float value);
	int getInt(int id, const char* key, int fallback = 0) const;
	void setInt(int id, const char* key, int value);
	bool getBool(int id, const char* key) const;
	void setBool(int id, const char* key, bool value);
	Vector3 toVector3(Vec3 value) const;

	WorldSyncCore& core_;
	IPawnComponent* pawn_;
	IVehiclesComponent* vehicles_;
	ICore* omp_;
	std::unordered_map<int, int> runtimeVehicles_;
};
} // namespace worlds
