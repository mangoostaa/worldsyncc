#include "Cropmodule.hpp"
#include "Pathmodule.hpp"
#include "WorldSync.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
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
	const int id = core.createEntity("door", worlds::Vec3 { 1.0f, 2.0f, 3.0f }, 4, 5);
	assert(id == 1);
	assert(core.hasEntity(id));
	assert(core.setState(id, "locked", "1"));

	std::string value;
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

void testCropModuleGrowthAndHarvest()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	worlds::CropModule crops(core, nullptr, nullptr);
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

void testPathModuleAStar()
{
	cleanStorage();

	worlds::WorldSyncCore core;
	worlds::PathModule paths(core, nullptr, nullptr, nullptr);

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
	assert(paths.routeLength(route) == 3);
	assert(paths.routeNode(route, 0) == a);
	assert(paths.routeNode(route, 1) == b);
	assert(paths.routeNode(route, 2) == c);

	worlds::Vec3 point;
	assert(paths.routePoint(route, 2, point));
	assertNear(point.x, 20.0f);

	assert(paths.destroyRoute(route));
	assert(paths.routeLength(route) == 0);
}
} // namespace

int main()
{
	testCoreEntityLifecycle();
	testFallbackPersistenceRoundTrip();
	testCropModuleGrowthAndHarvest();
	testPathModuleAStar();

	std::cout << "WorldSync unit tests passed\n";
	return 0;
}
