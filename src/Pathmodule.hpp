#pragma once

#include "WorldSync.hpp"

#include <Server/Components/NPCs/npcs.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/TextLabels/textlabels.hpp>
#include <sdk.hpp>
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
	int npcPathID = 0;
	bool loop = false;
	bool active = false;
	NPCMoveType moveType = NPCMoveType_Auto;
	float speed = NPC_MOVE_SPEED_AUTO;
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
	int routeLength(int routeID) const;
	int routeNode(int routeID, int index) const;
	bool routePoint(int routeID, int index, Vec3& out) const;

	int createNPCPath(int routeID, float stopRange);
	bool moveNPCByRoute(int npcID, int routeID, NPCMoveType moveType, float speed, bool reverse);
	int npcGoTo(int npcID, Vec3 destination, int virtualWorld, int interior, float nodeSearchRadius, NPCMoveType moveType, float speed);

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

	void registerNatives(IPawnScript& script);
	IPawnComponent* pawnRef() const { return pawn_; }

private:
	void firePawnCallback(const char* name, int a, int b, int c);
	Patrol* findPatrol(int patrolID);
	const Patrol* findPatrol(int patrolID) const;
	Patrol* findPatrolByNPCPath(int npcPathID);
	bool startPatrol(Patrol& patrol);

	std::vector<PathEdge> getEdges(int nodeID) const;
	void setEdges(int nodeID, const std::vector<PathEdge>& edges);
	bool upsertEdge(int fromNode, int toNode, float cost);
	bool removeEdge(int fromNode, int toNode);
	float distance(int a, int b) const;
	float distance(Vec3 a, Vec3 b) const;
	Vec3 midpoint(Vec3 a, Vec3 b) const;
	Vector3 toVector3(Vec3 value) const;
	bool createDebugLabel(const std::string& text, Vec3 position, Colour colour, int world);

	WorldSyncCore& core_;
	IPawnComponent* pawn_;
	INPCComponent* npcs_;
	ITextLabelsComponent* textLabels_;
	ICore* omp_;
	int nextRouteID_ = 1;
	int nextPatrolID_ = 1;
	std::unordered_map<int, Route> routes_;
	std::unordered_map<int, Patrol> patrols_;
	std::vector<int> debugLabelIDs_;
	bool pathDebugEnabled_ = false;
};
} // namespace worlds
