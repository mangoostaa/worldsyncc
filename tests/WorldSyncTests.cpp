#include "Cropmodule.hpp"
#include "Pathmodule.hpp"
#include "Vehiclemodule.hpp"
#include "WorldSync.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
struct EventRecorder final : worlds::WorldSyncEventHandler
{
	int created = 0;
	int destroyed = 0;
	int stateChanged = 0;
	int loaded = 0;
	int saved = 0;
	int lastEntity = 0;
	std::string lastType;
	std::string lastKey;
	std::string lastOldValue;
	std::string lastNewValue;

	void onEntityCreated(const worlds::Entity& entity) override
	{
		++created;
		lastEntity = entity.id;
		lastType = entity.type;
	}

	void onEntityDestroyed(int entityID, const std::string& type) override
	{
		++destroyed;
		lastEntity = entityID;
		lastType = type;
	}

	void onEntityStateChanged(const worlds::Entity& entity, const std::string& key, const std::string& oldValue, const std::string& newValue) override
	{
		++stateChanged;
		lastEntity = entity.id;
		lastKey = key;
		lastOldValue = oldValue;
		lastNewValue = newValue;
	}

	void onWorldLoaded(int, bool) override
	{
		++loaded;
	}

	void onWorldSaved(int, int, bool) override
	{
		++saved;
	}
};

void cleanStorage()
{
	std::filesystem::remove("scriptfiles/WorldSync.entities");
	std::filesystem::remove("scriptfiles/WorldSync.db");
}

void assertNear(float actual, float expected)
{
	assert(std::fabs(actual - expected) < 0.001f);
}

void testCoreEntityLifecycle()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	assert(core.createEntity("", worlds::Vec3 {}, 0, 0) == 0);

	const int id = core.createEntity("door", worlds::Vec3 { 1.0f, 2.0f, 3.0f }, 4, 5);
	assert(id == 1);
	assert(core.hasEntity(id));
	std::string value;
	assert(!core.setState(id, "", "bad"));
	assert(!core.getState(id, "", value));
	assert(core.setState(id, "locked", "1"));

	assert(core.getState(id, "locked", value));
	assert(value == "1");

	worlds::Vec3 position;
	assert(core.getPosition(id, position));
	assertNear(position.x, 1.0f);
	assertNear(position.y, 2.0f);
	assertNear(position.z, 3.0f);
	assert(core.getWorld(id) == 4);
	assert(core.getInterior(id) == 5);

	worlds::Stats stats = core.stats();
	assert(stats.entities == 1);
	assert(stats.dirtyEntities == 1);

	assert(core.saveAll() == 1);
	stats = core.stats();
	assert(stats.dirtyEntities == 0);
	assert(stats.saves == 1);
	assert(stats.lastSaveCount == 1);
	assert(stats.lastSaveChanged == 1);
}

void testFallbackPersistenceRoundTrip()
{
	cleanStorage();

	{
		worlds::WorldSyncCore core;
		const int crop = core.createEntity("crop", worlds::Vec3 { 10.0f, 20.0f, 30.0f }, 1, 2);
		assert(core.setState(crop, "species", "corn"));
		assert(core.setState(crop, "note", "a|b\\c"));
		assert(core.setSimulated(crop, true));
		assert(core.saveAll() == 1);
	}

	worlds::WorldSyncCore loaded;
	assert(loaded.load());
	assert(loaded.stats().entities == 1);
	assert(loaded.stats().lastLoadCount == 1);
	assert(loaded.getEntityIDAt(0) == 1);

	std::string type;
	assert(loaded.getType(1, type));
	assert(type == "crop");

	std::string species;
	assert(loaded.getState(1, "species", species));
	assert(species == "corn");

	std::string note;
	assert(loaded.getState(1, "note", note));
	assert(note == "a|b\\c");

	assert(loaded.destroyEntity(1));
	assert(loaded.stats().pendingDeletes == 1);
	assert(loaded.saveAll() == 1);

	worlds::WorldSyncCore empty;
	assert(empty.load());
	assert(empty.stats().entities == 0);
}

void testSpatialGridQueries()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	const int door = core.createEntity("door", worlds::Vec3 { 0.0f, 0.0f, 0.0f }, 0, 0);
	const int crop = core.createEntity("crop", worlds::Vec3 { 10.0f, 0.0f, 0.0f }, 0, 0);
	const int farCrop = core.createEntity("crop", worlds::Vec3 { 200.0f, 0.0f, 0.0f }, 0, 0);
	const int otherWorld = core.createEntity("crop", worlds::Vec3 { 5.0f, 0.0f, 0.0f }, 1, 0);

	assert(core.findNearestEntity(worlds::Vec3 { 8.0f, 0.0f, 0.0f }, 0, 0, 50.0f) == crop);
	assert(core.findNearestEntity(worlds::Vec3 { 8.0f, 0.0f, 0.0f }, 0, 0, 50.0f, "door") == door);
	assert(core.findNearestEntity(worlds::Vec3 { 8.0f, 0.0f, 0.0f }, 1, 0, 50.0f, "crop") == otherWorld);

	std::vector<int> nearby = core.findEntitiesInRange(worlds::Vec3 { 0.0f, 0.0f, 0.0f }, 0, 0, 15.0f);
	assert(nearby.size() == 2);
	assert(std::find(nearby.begin(), nearby.end(), door) != nearby.end());
	assert(std::find(nearby.begin(), nearby.end(), crop) != nearby.end());
	assert(std::find(nearby.begin(), nearby.end(), farCrop) == nearby.end());
	assert(std::find(nearby.begin(), nearby.end(), otherWorld) == nearby.end());

	assert(core.destroyEntity(crop));
	assert(core.findNearestEntity(worlds::Vec3 { 8.0f, 0.0f, 0.0f }, 0, 0, 50.0f, "crop") == 0);
}

void testCoreEventCallbacks()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	EventRecorder events;
	core.addEventHandler(&events);

	const int entity = core.createEntity("stash", worlds::Vec3 { 1.0f, 2.0f, 3.0f }, 0, 0);
	assert(events.created == 1);
	assert(events.lastEntity == entity);
	assert(events.lastType == "stash");

	assert(core.setState(entity, "owner", "10"));
	assert(events.stateChanged == 1);
	assert(events.lastKey == "owner");
	assert(events.lastOldValue.empty());
	assert(events.lastNewValue == "10");

	assert(core.setState(entity, "owner", "11"));
	assert(events.stateChanged == 2);
	assert(events.lastOldValue == "10");
	assert(events.lastNewValue == "11");

	assert(core.saveAll() == 1);
	assert(events.saved == 1);

	assert(core.destroyEntity(entity));
	assert(events.destroyed == 1);
	assert(events.lastEntity == entity);
	assert(events.lastType == "stash");

	core.removeEventHandler(&events);
	core.createEntity("silent", worlds::Vec3 {}, 0, 0);
	assert(events.created == 1);
}

void testCropModuleGrowthAndHarvest()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	worlds::CropModule crops(core, nullptr, nullptr);
	assert(crops.createCrop("", worlds::Vec3 {}, 1.0f, 1) == 0);

	const int crop = crops.createCrop("corn", worlds::Vec3 { 0.0f, 0.0f, 0.0f }, 10.0f, 2);

	assert(crops.isCrop(crop));
	assert(crops.getGrowth(crop) == 0);

	crops.tickCrops(std::chrono::milliseconds(5500));
	assert(crops.getGrowth(crop) == 55);
	assert(!crops.isReady(crop));

	assert(crops.setGrowth(crop, 100));
	assert(crops.isReady(crop));
	assert(crops.harvest(crop));
	assert(crops.getGrowth(crop) == 0);
	assert(crops.getHarvests(crop) == 1);

	assert(crops.setGrowth(crop, 100));
	assert(crops.harvest(crop));
	assert(!core.hasEntity(crop));
}

void testVehicleModulePersistenceFallback()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	worlds::VehicleModule vehicles(core, nullptr, nullptr, nullptr);
	assert(vehicles.createVehicle(0, worlds::Vec3 {}, 0.0f, 1, 2, -1, false) == 0);

	const int vehicle = vehicles.createVehicle(411, worlds::Vec3 { 1.0f, 2.0f, 3.0f }, 90.0f, 1, 2, -1, false);

	assert(vehicle == 1);
	assert(core.hasEntity(vehicle));
	assert(vehicles.isVehicle(vehicle));
	assert(vehicles.getRuntimeVehicleID(vehicle) == 0);
	assert(vehicles.getModel(vehicle) == 411);
	assertNear(vehicles.getZRotation(vehicle), 90.0f);

	int colour1 = -1;
	int colour2 = -1;
	assert(vehicles.getColours(vehicle, colour1, colour2));
	assert(colour1 == 1);
	assert(colour2 == 2);

	assert(vehicles.setHealth(vehicle, 850.0f));
	assertNear(vehicles.getHealth(vehicle), 850.0f);
	assert(vehicles.destroyVehicle(vehicle));
	assert(!core.hasEntity(vehicle));
}

void testPathModuleAStar()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	worlds::PathModule paths(core, nullptr, nullptr, nullptr, nullptr);

	const int a = paths.createNode(worlds::Vec3 { 0.0f, 0.0f, 0.0f });
	const int b = paths.createNode(worlds::Vec3 { 10.0f, 0.0f, 0.0f });
	const int c = paths.createNode(worlds::Vec3 { 20.0f, 0.0f, 0.0f });
	const int d = paths.createNode(worlds::Vec3 { 0.0f, 100.0f, 0.0f });

	assert(paths.connectNodes(a, b, true));
	assert(paths.connectNodes(b, c, true));
	assert(paths.connectNodes(a, d, true, 1.0f));
	assert(paths.connectNodes(d, c, true, 500.0f));

	assert(paths.nearestNode(worlds::Vec3 { 9.0f, 0.0f, 0.0f }, 0, 0, 20.0f) == b);

	const int route = paths.findPath(a, c);
	assert(route != 0);
	assert(paths.routeCacheSize() == 1);
	assert(paths.routeLength(route) == 3);
	assert(paths.routeNode(route, 0) == a);
	assert(paths.routeNode(route, 1) == b);
	assert(paths.routeNode(route, 2) == c);

	worlds::Vec3 point;
	assert(paths.routePoint(route, 2, point));
	assertNear(point.x, 20.0f);

	assert(paths.destroyRoute(route));
	assert(paths.routeLength(route) == 0);

	const int cachedRoute = paths.findPath(a, c);
	assert(cachedRoute != 0);
	assert(paths.routeCacheSize() == 1);
	assert(paths.routeLength(cachedRoute) == 3);

	assert(paths.disconnectNodes(b, c, true));
	assert(paths.routeCacheSize() == 0);
}
} // namespace

int main()
{
	testCoreEntityLifecycle();
	testFallbackPersistenceRoundTrip();
	testSpatialGridQueries();
	testCoreEventCallbacks();
	testCropModuleGrowthAndHarvest();
	testVehicleModulePersistenceFallback();
	testPathModuleAStar();

	std::cout << "WorldSync unit tests passed\n";
	return 0;
}
