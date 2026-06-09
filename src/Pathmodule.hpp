#pragma once

#include "WorldSync.hpp"

#include <Server/Components/NPCs/npcs.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/TextLabels/textlabels.hpp>
#include <sdk.hpp>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace worlds
{
constexpr const char* PATH_NODE_TYPE = "path_node";
constexpr const char* PATH_KEY_EDGES = "edges";

struct PathEdge
{
	int node = 0;
	float cost = 0.0f;
};

struct Route
{
	int id = 0;
	std::vector<int> nodes;
};

struct Patrol
{
	int id = 0;
	int npcID = 0;
	int routeID = 0;
	int npcPathID = -1;
	bool loop = false;
	bool active = false;
	NPCMoveType moveType = NPCMoveType_Auto;
	float speed = NPC_MOVE_SPEED_AUTO;
};

struct NPCSpatialEntry
{
	int id = 0;
	Vec3 position;
	int world = 0;
	int interior = 0;
};

class PathModule : public NPCEventHandler
{
public:
	PathModule(WorldSyncCore& core, IPawnComponent* pawn, INPCComponent* npcs, ITextLabelsComponent* textLabels, ICore* omp)
		: core_(core)
		, pawn_(pawn)
		, npcs_(npcs)
		, textLabels_(textLabels)
		, omp_(omp)
	{
		if (npcs_)
		{
			npcs_->getEventDispatcher().addEventHandler(this);
		}
	}

	~PathModule();

	int createNode(Vec3 position, int virtualWorld = 0, int interior = 0);
	bool isNode(int nodeID) const;
	bool connectNodes(int fromNode, int toNode, bool bidirectional, float cost = 0.0f);
	bool disconnectNodes(int fromNode, int toNode, bool bidirectional);
	int nearestNode(Vec3 position, int virtualWorld, int interior, float maxDistance) const;

	int findPath(int startNode, int endNode);
	bool destroyRoute(int routeID);
	void clearRouteCache();
	int routeCacheSize() const;
	int routeLength(int routeID) const;
	int routeNode(int routeID, int index) const;
	bool routePoint(int routeID, int index, Vec3& out) const;

	int createNPCPath(int routeID, float stopRange);
	bool moveNPCByRoute(int npcID, int routeID, NPCMoveType moveType, float speed, bool reverse);
	int npcGoTo(int npcID, Vec3 destination, int virtualWorld, int interior, float nodeSearchRadius, NPCMoveType moveType, float speed);
	void tickNPCSpatialGrid(std::chrono::milliseconds elapsed);
	int findNearestNPC(Vec3 position, int virtualWorld, int interior, float maxDistance);
	std::vector<int> findNPCsInRange(Vec3 position, int virtualWorld, int interior, float radius, size_t maxResults);
	int findNearestPlayerToNPC(int npcID, float maxDistance, bool includeBots);
	bool isPlayerInNPCSight(int npcID, int playerID, float maxDistance, float fovDegrees, bool includeBots);

	int createPatrol(int npcID, int routeID, bool loop, NPCMoveType moveType, float speed);
	bool startPatrol(int patrolID);
	bool stopPatrol(int patrolID);
	bool pausePatrol(int patrolID);
	bool resumePatrol(int patrolID);
	bool destroyPatrol(int patrolID);
	bool isPatrolActive(int patrolID) const;
	int getPatrolRoute(int patrolID) const;
	int getPatrolNPC(int patrolID) const;

	bool setPathDebug(bool enabled);
	bool showRouteDebug(int routeID);
	void clearPathDebug();

	void onNPCFinishMovePathPoint(INPC& npc, int pathId, int pointId) override;
	void onNPCFinishMovePath(INPC& npc, int pathId) override;
	void onNPCCreate(INPC& npc) override;
	void onNPCDestroy(INPC& npc) override;

	void registerNatives(IPawnScript& script);
	IPawnComponent* pawnRef() const { return pawn_; }

private:
	void firePawnCallback(const char* name, int a, int b, int c);
	Patrol* findPatrol(int patrolID);
	const Patrol* findPatrol(int patrolID) const;
	Patrol* findPatrolByNPCPath(int npcPathID);
	bool startPatrol(Patrol& patrol);
	void invalidateRouteCache();
	std::string routeCacheKey(int startNode, int endNode) const;
	int storeRoute(std::vector<int> nodes);
	void rebuildNPCSpatialGrid();

	const std::vector<PathEdge>& getEdges(int nodeID) const;
	void setEdges(int nodeID, const std::vector<PathEdge>& edges);
	bool upsertEdge(int fromNode, int toNode, float cost);
	bool removeEdge(int fromNode, int toNode);
	float distance(int a, int b) const;
	float distance(Vec3 a, Vec3 b) const;
	Vec3 midpoint(Vec3 a, Vec3 b) const;
	Vector3 toVector3(Vec3 value) const;
	bool createDebugLabel(const std::string& text, Vec3 position, Colour colour, int world);

	struct NPCSpatialCell
	{
		int world = 0;
		int interior = 0;
		int x = 0;
		int y = 0;

		bool operator==(const NPCSpatialCell& other) const
		{
			return world == other.world && interior == other.interior && x == other.x && y == other.y;
		}
	};

	struct NPCSpatialCellHash
	{
		size_t operator()(const NPCSpatialCell& cell) const;
	};

	NPCSpatialCell npcSpatialCellFor(Vec3 position, int world, int interior) const;

	WorldSyncCore& core_;
	IPawnComponent* pawn_;
	INPCComponent* npcs_;
	ITextLabelsComponent* textLabels_;
	ICore* omp_;
	int nextRouteID_ = 1;
	int nextPatrolID_ = 1;
	std::unordered_map<int, Route> routes_;
	std::unordered_map<std::string, std::vector<int>> routeCache_;
	std::unordered_map<int, Patrol> patrols_;
	mutable std::unordered_map<int, std::vector<PathEdge>> edgeCache_;
	std::unordered_map<NPCSpatialCell, std::vector<int>, NPCSpatialCellHash> npcSpatialGrid_;
	std::unordered_map<int, NPCSpatialEntry> npcSpatialEntries_;
	std::vector<int> debugLabelIDs_;
	std::chrono::milliseconds npcSpatialElapsed_ { 0 };
	bool pathDebugEnabled_ = false;
	bool npcSpatialDirty_ = true;
};
} // namespace worlds
