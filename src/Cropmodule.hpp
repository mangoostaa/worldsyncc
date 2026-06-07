#pragma once

#include "WorldSync.hpp"

#include <Server/Components/Pawn/pawn.hpp>
#include <sdk.hpp>
#include <chrono>
#include <string>

namespace worlds
{
constexpr const char* CROP_TYPE = "crop";
constexpr const char* CROP_KEY_SPECIES = "species";
constexpr const char* CROP_KEY_GROWTH = "growth";
constexpr const char* CROP_KEY_RATE = "rate";
constexpr const char* CROP_KEY_MAX_HARVEST = "max_harvests";
constexpr const char* CROP_KEY_HARVESTS = "harvests";
constexpr const char* CROP_KEY_READY = "ready";

class CropModule
{
public:
	CropModule(WorldSyncCore& core, IPawnComponent* pawn, ICore* omp)
		: core_(core)
		, pawn_(pawn)
		, omp_(omp)
	{
	}

	int createCrop(const std::string& species, Vec3 position, float growthRatePerSecond, int maxHarvests, int virtualWorld = 0, int interior = 0);
	bool destroyCrop(int cropID);
	bool isCrop(int cropID) const;

	int getGrowth(int cropID) const;
	bool setGrowth(int cropID, int value);
	bool isReady(int cropID) const;
	bool harvest(int cropID);
	int getHarvests(int cropID) const;
	bool getSpecies(int cropID, std::string& out) const;

	void tickCrops(std::chrono::milliseconds elapsed);
	void registerNatives(IPawnScript& script);
	IPawnComponent* pawnRef() const { return pawn_; }

private:
	void fireCropReady(int cropID);

	int getInt(int id, const char* key, int fallback = 0) const;
	void setInt(int id, const char* key, int value);
	float getFloat(int id, const char* key, float fallback = 0.0f) const;
	void setFloat(int id, const char* key, float value);
	std::string getStr(int id, const char* key) const;
	void setStr(int id, const char* key, const std::string& value);

	WorldSyncCore& core_;
	IPawnComponent* pawn_;
	ICore* omp_;
};
} // namespace worlds
