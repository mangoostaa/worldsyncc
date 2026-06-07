#pragma once

#include "WorldSync.hpp"

#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <sdk.hpp>
#include <unordered_map>

namespace worlds
{
constexpr const char* DOOR_TYPE = "door";
constexpr const char* KEY_OPEN = "open";
constexpr const char* KEY_LOCKED = "locked";
constexpr const char* KEY_MODEL = "model";
constexpr const char* KEY_RX = "rx";
constexpr const char* KEY_RY = "ry";
constexpr const char* KEY_RZ = "rz";
constexpr const char* KEY_OPEN_RX = "open_rx";
constexpr const char* KEY_OPEN_RY = "open_ry";
constexpr const char* KEY_OPEN_RZ = "open_rz";

class DoorModule
{
public:
	DoorModule(WorldSyncCore& core, IPawnComponent* pawn, IObjectsComponent* objects, ICore* omp)
		: core_(core)
		, pawn_(pawn)
		, objects_(objects)
		, omp_(omp)
	{
	}

	int createDoor(int model, Vec3 position, Vec3 closeRot, Vec3 openRot, int virtualWorld = 0, int interior = 0);
	bool destroyDoor(int doorID);

	bool setOpen(int doorID, bool open);
	bool isOpen(int doorID) const;
	bool setLocked(int doorID, bool locked);
	bool isLocked(int doorID) const;

	int getModel(int doorID) const;
	bool getPosition(int doorID, Vec3& out) const;
	bool getCloseRot(int doorID, Vec3& out) const;
	bool getOpenRot(int doorID, Vec3& out) const;
	bool isDoor(int doorID) const;

	void rebuildObjects();
	bool createObjectForDoor(int doorID);
	bool destroyObjectForDoor(int doorID);
	bool applyObjectState(int doorID, float speed = 0.0f);
	int getObjectID(int doorID) const;

	void registerNatives(IPawnScript& script);
	IPawnComponent* pawnRef() const { return pawn_; }

private:
	void fireCallback(int doorID, bool isOpen);

	bool getBool(int id, const char* key) const;
	void setBool(int id, const char* key, bool value);
	float getFloat(int id, const char* key, float fallback = 0.0f) const;
	void setFloat(int id, const char* key, float value);
	int getInt(int id, const char* key, int fallback = 0) const;
	void setInt(int id, const char* key, int value);
	Vector3 toVector3(Vec3 value) const;

	WorldSyncCore& core_;
	IPawnComponent* pawn_;
	IObjectsComponent* objects_;
	ICore* omp_;
	std::unordered_map<int, int> doorObjects_;
};
} // namespace worlds
