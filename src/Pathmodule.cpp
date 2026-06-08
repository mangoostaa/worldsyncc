#include "Pathmodule.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>

namespace worlds
{
namespace
{
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

std::vector<std::string> split(const std::string& value, char delimiter)
{
	std::vector<std::string> result;
	std::string item;
	std::stringstream stream(value);
	while (std::getline(stream, item, delimiter))
	{
		if (!item.empty())
		{
			result.push_back(item);
		}
	}
	return result;
}

float parseFloat(const std::string& value, float fallback)
{
	try
	{
		return std::stof(value);
	}
	catch (...)
	{
		return fallback;
	}
}

int parseInt(const std::string& value, int fallback)
{
	try
	{
		return std::stoi(value);
	}
	catch (...)
	{
		return fallback;
	}
}

struct OpenNode
{
	int node = 0;
	float fScore = 0.0f;

	bool operator<(const OpenNode& other) const
	{
		return fScore > other.fScore;
	}
};
} // namespace

PathModule::~PathModule()
{
	clearPathDebug();
	if (npcs_)
	{
		npcs_->getEventDispatcher().removeEventHandler(this);
	}
}

int PathModule::createNode(Vec3 position, int virtualWorld, int interior)
{
	invalidateRouteCache();
	return core_.createEntity(PATH_NODE_TYPE, position, virtualWorld, interior);
}

bool PathModule::isNode(int nodeID) const
{
	std::string type;
	return core_.getType(nodeID, type) && type == PATH_NODE_TYPE;
}

bool PathModule::connectNodes(int fromNode, int toNode, bool bidirectional, float cost)
{
	if (!isNode(fromNode) || !isNode(toNode) || fromNode == toNode)
	{
		return false;
	}

	const float finalCost = cost > 0.0f ? cost : distance(fromNode, toNode);
	const bool forward = upsertEdge(fromNode, toNode, finalCost);
	const bool backward = !bidirectional || upsertEdge(toNode, fromNode, finalCost);
	if (forward && backward)
	{
		invalidateRouteCache();
	}
	return forward && backward;
}

bool PathModule::disconnectNodes(int fromNode, int toNode, bool bidirectional)
{
	if (!isNode(fromNode) || !isNode(toNode))
	{
		return false;
	}

	const bool forward = removeEdge(fromNode, toNode);
	const bool backward = !bidirectional || removeEdge(toNode, fromNode);
	if (forward || backward)
	{
		invalidateRouteCache();
	}
	return forward || backward;
}

int PathModule::nearestNode(Vec3 position, int virtualWorld, int interior, float maxDistance) const
{
	return core_.findNearestEntity(position, virtualWorld, interior, maxDistance, PATH_NODE_TYPE);
}

int PathModule::findPath(int startNode, int endNode)
{
	if (!isNode(startNode) || !isNode(endNode))
	{
		return 0;
	}
	if (startNode == endNode)
	{
		return storeRoute({ startNode });
	}

	const std::string cacheKey = routeCacheKey(startNode, endNode);
	const auto cached = routeCache_.find(cacheKey);
	if (cached != routeCache_.end())
	{
		return storeRoute(cached->second);
	}

	std::priority_queue<OpenNode> open;
	std::unordered_map<int, int> cameFrom;
	std::unordered_map<int, float> gScore;
	std::unordered_set<int> closed;

	gScore[startNode] = 0.0f;
	open.push(OpenNode { startNode, distance(startNode, endNode) });

	while (!open.empty())
	{
		const int current = open.top().node;
		open.pop();
		if (closed.find(current) != closed.end())
		{
			continue;
		}
		if (current == endNode)
		{
			std::vector<int> nodes;
			int cursor = endNode;
			nodes.push_back(cursor);
			while (cursor != startNode)
			{
				cursor = cameFrom[cursor];
				nodes.push_back(cursor);
			}
			std::reverse(nodes.begin(), nodes.end());

			routeCache_[cacheKey] = nodes;
			return storeRoute(std::move(nodes));
		}

		closed.insert(current);
		for (const PathEdge& edge : getEdges(current))
		{
			if (!isNode(edge.node))
			{
				continue;
			}
			const float tentative = gScore[current] + edge.cost;
			const auto existing = gScore.find(edge.node);
			if (existing == gScore.end() || tentative < existing->second)
			{
				cameFrom[edge.node] = current;
				gScore[edge.node] = tentative;
				open.push(OpenNode { edge.node, tentative + distance(edge.node, endNode) });
			}
		}
	}

	return 0;
}

bool PathModule::destroyRoute(int routeID)
{
	return routes_.erase(routeID) > 0;
}

void PathModule::clearRouteCache()
{
	routeCache_.clear();
}

int PathModule::routeCacheSize() const
{
	return static_cast<int>(routeCache_.size());
}

int PathModule::routeLength(int routeID) const
{
	const auto it = routes_.find(routeID);
	return it == routes_.end() ? 0 : static_cast<int>(it->second.nodes.size());
}

int PathModule::routeNode(int routeID, int index) const
{
	const auto it = routes_.find(routeID);
	if (it == routes_.end() || index < 0 || index >= static_cast<int>(it->second.nodes.size()))
	{
		return 0;
	}
	return it->second.nodes[static_cast<size_t>(index)];
}

bool PathModule::routePoint(int routeID, int index, Vec3& out) const
{
	const int node = routeNode(routeID, index);
	return node != 0 && core_.getPosition(node, out);
}

int PathModule::createNPCPath(int routeID, float stopRange)
{
	if (!npcs_)
	{
		return 0;
	}

	const auto it = routes_.find(routeID);
	if (it == routes_.end() || it->second.nodes.empty())
	{
		return 0;
	}

	const int pathID = npcs_->createPath();
	for (int nodeID : it->second.nodes)
	{
		Vec3 position;
		if (core_.getPosition(nodeID, position))
		{
			npcs_->addPointToPath(pathID, toVector3(position), stopRange);
		}
	}
	return pathID;
}

bool PathModule::moveNPCByRoute(int npcID, int routeID, NPCMoveType moveType, float speed, bool reverse)
{
	if (!npcs_)
	{
		return false;
	}

	INPC* npc = npcs_->get(npcID);
	if (!npc)
	{
		return false;
	}

	const int pathID = createNPCPath(routeID, 1.0f);
	if (!pathID)
	{
		return false;
	}

	return npc->moveByPath(pathID, moveType, speed, reverse);
}

int PathModule::npcGoTo(int npcID, Vec3 destination, int virtualWorld, int interior, float nodeSearchRadius, NPCMoveType moveType, float speed)
{
	if (!npcs_)
	{
		return 0;
	}

	INPC* npc = npcs_->get(npcID);
	if (!npc)
	{
		return 0;
	}

	const Vector3 npcPosition = npc->getPosition();
	const int fromWorld = virtualWorld >= 0 ? virtualWorld : npc->getVirtualWorld();
	const int fromInterior = interior >= 0 ? interior : static_cast<int>(npc->getInterior());
	const float searchRadius = nodeSearchRadius > 0.0f ? nodeSearchRadius : 80.0f;

	const int startNode = nearestNode(Vec3 { npcPosition.x, npcPosition.y, npcPosition.z }, fromWorld, fromInterior, searchRadius);
	const int endNode = nearestNode(destination, fromWorld, fromInterior, searchRadius);
	if (!startNode || !endNode)
	{
		return 0;
	}

	const int routeID = findPath(startNode, endNode);
	if (!routeID)
	{
		return 0;
	}

	if (!moveNPCByRoute(npcID, routeID, moveType, speed, false))
	{
		destroyRoute(routeID);
		return 0;
	}

	return routeID;
}

int PathModule::createPatrol(int npcID, int routeID, bool loop, NPCMoveType moveType, float speed)
{
	if (!npcs_ || routes_.find(routeID) == routes_.end() || !npcs_->get(npcID))
	{
		return 0;
	}

	const int patrolID = nextPatrolID_++;
	Patrol patrol;
	patrol.id = patrolID;
	patrol.npcID = npcID;
	patrol.routeID = routeID;
	patrol.loop = loop;
	patrol.moveType = moveType;
	patrol.speed = speed;
	patrols_[patrolID] = patrol;
	return patrolID;
}

bool PathModule::startPatrol(int patrolID)
{
	Patrol* patrol = findPatrol(patrolID);
	return patrol && startPatrol(*patrol);
}

bool PathModule::stopPatrol(int patrolID)
{
	Patrol* patrol = findPatrol(patrolID);
	if (!patrol || !npcs_)
	{
		return false;
	}

	INPC* npc = npcs_->get(patrol->npcID);
	if (npc)
	{
		npc->stopPath();
	}
	if (patrol->npcPathID)
	{
		npcs_->destroyPath(patrol->npcPathID);
		patrol->npcPathID = 0;
	}
	patrol->active = false;
	return true;
}

bool PathModule::pausePatrol(int patrolID)
{
	const Patrol* patrol = findPatrol(patrolID);
	if (!patrol || !npcs_)
	{
		return false;
	}

	INPC* npc = npcs_->get(patrol->npcID);
	if (!npc || !patrol->active)
	{
		return false;
	}
	npc->pausePath();
	return true;
}

bool PathModule::resumePatrol(int patrolID)
{
	const Patrol* patrol = findPatrol(patrolID);
	if (!patrol || !npcs_)
	{
		return false;
	}

	INPC* npc = npcs_->get(patrol->npcID);
	if (!npc || !patrol->active)
	{
		return false;
	}
	npc->resumePath();
	return true;
}

bool PathModule::destroyPatrol(int patrolID)
{
	stopPatrol(patrolID);
	return patrols_.erase(patrolID) > 0;
}

bool PathModule::isPatrolActive(int patrolID) const
{
	const Patrol* patrol = findPatrol(patrolID);
	return patrol && patrol->active;
}

int PathModule::getPatrolRoute(int patrolID) const
{
	const Patrol* patrol = findPatrol(patrolID);
	return patrol ? patrol->routeID : 0;
}

int PathModule::getPatrolNPC(int patrolID) const
{
	const Patrol* patrol = findPatrol(patrolID);
	return patrol ? patrol->npcID : 0;
}

bool PathModule::setPathDebug(bool enabled)
{
	pathDebugEnabled_ = enabled;
	clearPathDebug();
	if (!enabled)
	{
		return true;
	}
	if (!textLabels_)
	{
		return false;
	}

	int created = 0;
	for (const Entity& entity : core_.entities())
	{
		if (entity.type != PATH_NODE_TYPE)
		{
			continue;
		}

		std::ostringstream label;
		label << "WS node " << entity.id;
		if (createDebugLabel(label.str(), Vec3 { entity.position.x, entity.position.y, entity.position.z + 0.65f }, Colour::Yellow(), entity.world))
		{
			++created;
		}

		for (const PathEdge& edge : getEdges(entity.id))
		{
			Vec3 target;
			if (!core_.getPosition(edge.node, target))
			{
				continue;
			}

			std::ostringstream edgeLabel;
			edgeLabel << entity.id << " -> " << edge.node << " (" << edge.cost << ")";
			if (createDebugLabel(edgeLabel.str(), midpoint(entity.position, target), Colour::Cyan(), entity.world))
			{
				++created;
			}
		}
	}

	if (omp_)
	{
		omp_->printLn("[WorldSync/Path] Debug visual: %d labels creados.", created);
	}
	return created > 0;
}

bool PathModule::showRouteDebug(int routeID)
{
	clearPathDebug();
	if (!textLabels_)
	{
		return false;
	}

	const auto it = routes_.find(routeID);
	if (it == routes_.end() || it->second.nodes.empty())
	{
		return false;
	}

	int created = 0;
	for (size_t i = 0; i < it->second.nodes.size(); ++i)
	{
		const int nodeID = it->second.nodes[i];
		Vec3 position;
		if (!core_.getPosition(nodeID, position))
		{
			continue;
		}

		std::ostringstream label;
		label << "route " << routeID << " [" << i << "] node " << nodeID;
		if (createDebugLabel(label.str(), Vec3 { position.x, position.y, position.z + 0.85f }, Colour::White(), core_.getWorld(nodeID)))
		{
			++created;
		}

		if (i + 1 < it->second.nodes.size())
		{
			Vec3 next;
			const int nextNode = it->second.nodes[i + 1];
			if (core_.getPosition(nextNode, next))
			{
				std::ostringstream edgeLabel;
				edgeLabel << "[" << i << "] -> [" << (i + 1) << "]";
				if (createDebugLabel(edgeLabel.str(), midpoint(position, next), Colour::Cyan(), core_.getWorld(nodeID)))
				{
					++created;
				}
			}
		}
	}

	if (omp_)
	{
		omp_->printLn("[WorldSync/Path] Ruta %d debug: %d labels creados.", routeID, created);
	}
	return created > 0;
}

void PathModule::clearPathDebug()
{
	if (textLabels_)
	{
		for (int labelID : debugLabelIDs_)
		{
			textLabels_->release(labelID);
		}
	}
	debugLabelIDs_.clear();
}

void PathModule::onNPCFinishMovePathPoint(INPC& npc, int pathId, int pointId)
{
	Patrol* patrol = findPatrolByNPCPath(pathId);
	if (!patrol || patrol->npcID != npc.getID())
	{
		return;
	}
	firePawnCallback("OnWorldSyncPatrolPoint", patrol->id, patrol->npcID, pointId);
}

void PathModule::onNPCFinishMovePath(INPC& npc, int pathId)
{
	Patrol* patrol = findPatrolByNPCPath(pathId);
	if (!patrol || patrol->npcID != npc.getID())
	{
		return;
	}

	firePawnCallback("OnWorldSyncPatrolComplete", patrol->id, patrol->npcID, patrol->routeID);
	if (!npcs_)
	{
		return;
	}

	if (patrol->npcPathID)
	{
		npcs_->destroyPath(patrol->npcPathID);
		patrol->npcPathID = 0;
	}
	patrol->active = false;

	if (patrol->loop)
	{
		startPatrol(*patrol);
	}
}

std::vector<PathEdge> PathModule::getEdges(int nodeID) const
{
	std::string encoded;
	if (!core_.getState(nodeID, PATH_KEY_EDGES, encoded) || encoded.empty())
	{
		return {};
	}

	std::vector<PathEdge> edges;
	for (const std::string& item : split(encoded, ','))
	{
		const size_t separator = item.find(':');
		if (separator == std::string::npos)
		{
			continue;
		}

		const int node = parseInt(item.substr(0, separator), 0);
		const float cost = parseFloat(item.substr(separator + 1), 0.0f);
		if (node > 0 && cost > 0.0f)
		{
			edges.push_back(PathEdge { node, cost });
		}
	}
	return edges;
}

void PathModule::setEdges(int nodeID, const std::vector<PathEdge>& edges)
{
	std::ostringstream stream;
	bool first = true;
	for (const PathEdge& edge : edges)
	{
		if (!first)
		{
			stream << ',';
		}
		stream << edge.node << ':' << edge.cost;
		first = false;
	}
	core_.setState(nodeID, PATH_KEY_EDGES, stream.str());
}

bool PathModule::upsertEdge(int fromNode, int toNode, float cost)
{
	std::vector<PathEdge> edges = getEdges(fromNode);
	for (PathEdge& edge : edges)
	{
		if (edge.node == toNode)
		{
			edge.cost = cost;
			setEdges(fromNode, edges);
			return true;
		}
	}
	edges.push_back(PathEdge { toNode, cost });
	setEdges(fromNode, edges);
	return true;
}

bool PathModule::removeEdge(int fromNode, int toNode)
{
	std::vector<PathEdge> edges = getEdges(fromNode);
	const size_t before = edges.size();
	edges.erase(std::remove_if(edges.begin(), edges.end(), [toNode](const PathEdge& edge) {
		return edge.node == toNode;
	}), edges.end());
	if (edges.size() == before)
	{
		return false;
	}
	setEdges(fromNode, edges);
	return true;
}

float PathModule::distance(int a, int b) const
{
	Vec3 pa;
	Vec3 pb;
	if (!core_.getPosition(a, pa) || !core_.getPosition(b, pb))
	{
		return std::numeric_limits<float>::max() / 4.0f;
	}
	return distance(pa, pb);
}

float PathModule::distance(Vec3 a, Vec3 b) const
{
	const float dx = a.x - b.x;
	const float dy = a.y - b.y;
	const float dz = a.z - b.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Vector3 PathModule::toVector3(Vec3 value) const
{
	return Vector3(value.x, value.y, value.z);
}

Vec3 PathModule::midpoint(Vec3 a, Vec3 b) const
{
	return Vec3 {
		(a.x + b.x) * 0.5f,
		(a.y + b.y) * 0.5f,
		(a.z + b.z) * 0.5f + 0.45f
	};
}

bool PathModule::createDebugLabel(const std::string& text, Vec3 position, Colour colour, int world)
{
	if (!textLabels_)
	{
		return false;
	}

	ITextLabel* label = textLabels_->create(StringView(text.data(), text.size()), colour, toVector3(position), 80.0f, world, false);
	if (!label)
	{
		return false;
	}
	debugLabelIDs_.push_back(label->getID());
	return true;
}

void PathModule::firePawnCallback(const char* name, int a, int b, int c)
{
	if (!pawn_)
	{
		return;
	}

	auto callScript = [name, a, b, c](IPawnScript* script) {
		if (!script) return;
		script->Call(name, DefaultReturnValue_True, a, b, c);
	};

	callScript(pawn_->mainScript());
	for (IPawnScript* script : pawn_->sideScripts())
	{
		callScript(script);
	}
}

Patrol* PathModule::findPatrol(int patrolID)
{
	const auto it = patrols_.find(patrolID);
	return it == patrols_.end() ? nullptr : &it->second;
}

const Patrol* PathModule::findPatrol(int patrolID) const
{
	const auto it = patrols_.find(patrolID);
	return it == patrols_.end() ? nullptr : &it->second;
}

Patrol* PathModule::findPatrolByNPCPath(int npcPathID)
{
	for (auto& item : patrols_)
	{
		if (item.second.npcPathID == npcPathID)
		{
			return &item.second;
		}
	}
	return nullptr;
}

bool PathModule::startPatrol(Patrol& patrol)
{
	if (!npcs_)
	{
		return false;
	}

	INPC* npc = npcs_->get(patrol.npcID);
	if (!npc)
	{
		return false;
	}

	if (patrol.npcPathID)
	{
		npcs_->destroyPath(patrol.npcPathID);
		patrol.npcPathID = 0;
	}

	patrol.npcPathID = createNPCPath(patrol.routeID, 1.0f);
	if (!patrol.npcPathID)
	{
		patrol.active = false;
		return false;
	}

	patrol.active = npc->moveByPath(patrol.npcPathID, patrol.moveType, patrol.speed, false);
	if (patrol.active)
	{
		firePawnCallback("OnWorldSyncPatrolStart", patrol.id, patrol.npcID, patrol.routeID);
	}
	return patrol.active;
}

void PathModule::invalidateRouteCache()
{
	if (!routeCache_.empty() && omp_)
	{
		omp_->printLn("[WorldSync/Path] Route cache invalidated (%d entries).", static_cast<int>(routeCache_.size()));
	}
	routeCache_.clear();
}

std::string PathModule::routeCacheKey(int startNode, int endNode) const
{
	return std::to_string(startNode) + ":" + std::to_string(endNode);
}

int PathModule::storeRoute(std::vector<int> nodes)
{
	const int routeID = nextRouteID_++;
	routes_[routeID] = Route { routeID, std::move(nodes) };
	return routeID;
}

static PathModule* s_pathModule = nullptr;

static cell AMX_NATIVE_CALL n_WS_CreatePathNode(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 5) return 0;
	const Vec3 position { amx_ctof(params[1]), amx_ctof(params[2]), amx_ctof(params[3]) };
	return s_pathModule->createNode(position, static_cast<int>(params[4]), static_cast<int>(params[5]));
}

static cell AMX_NATIVE_CALL n_WS_ConnectPathNodes(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 4) return 0;
	return s_pathModule->connectNodes(static_cast<int>(params[1]), static_cast<int>(params[2]), params[3] != 0, amx_ctof(params[4])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_DisconnectPathNodes(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 3) return 0;
	return s_pathModule->disconnectNodes(static_cast<int>(params[1]), static_cast<int>(params[2]), params[3] != 0) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_GetNearestPathNode(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 6) return 0;
	const Vec3 position { amx_ctof(params[1]), amx_ctof(params[2]), amx_ctof(params[3]) };
	return s_pathModule->nearestNode(position, static_cast<int>(params[4]), static_cast<int>(params[5]), amx_ctof(params[6]));
}

static cell AMX_NATIVE_CALL n_WS_FindPath(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 2) return 0;
	return s_pathModule->findPath(static_cast<int>(params[1]), static_cast<int>(params[2]));
}

static cell AMX_NATIVE_CALL n_WS_DestroyPath(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->destroyRoute(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_ClearPathCache(AMX*, cell*)
{
	if (!s_pathModule) return 0;
	s_pathModule->clearRouteCache();
	return 1;
}

static cell AMX_NATIVE_CALL n_WS_GetPathCacheSize(AMX*, cell*)
{
	return s_pathModule ? s_pathModule->routeCacheSize() : 0;
}

static cell AMX_NATIVE_CALL n_WS_GetPathLength(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->routeLength(static_cast<int>(params[1]));
}

static cell AMX_NATIVE_CALL n_WS_GetPathNode(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 2) return 0;
	return s_pathModule->routeNode(static_cast<int>(params[1]), static_cast<int>(params[2]));
}

static cell AMX_NATIVE_CALL n_WS_GetPathPoint(AMX* amx, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 5) return 0;
	Vec3 point;
	if (!s_pathModule->routePoint(static_cast<int>(params[1]), static_cast<int>(params[2]), point)) return 0;
	return setPawnCell(amx, s_pathModule->pawnRef(), params[3], amx_ftoc(point.x))
		&& setPawnCell(amx, s_pathModule->pawnRef(), params[4], amx_ftoc(point.y))
		&& setPawnCell(amx, s_pathModule->pawnRef(), params[5], amx_ftoc(point.z))
		? 1
		: 0;
}

static cell AMX_NATIVE_CALL n_WS_CreateNPCPath(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 2) return 0;
	return s_pathModule->createNPCPath(static_cast<int>(params[1]), amx_ctof(params[2]));
}

static cell AMX_NATIVE_CALL n_WS_MoveNPCByPath(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 5) return 0;
	return s_pathModule->moveNPCByRoute(
		static_cast<int>(params[1]),
		static_cast<int>(params[2]),
		static_cast<NPCMoveType>(params[3]),
		amx_ctof(params[4]),
		params[5] != 0)
		? 1
		: 0;
}

static cell AMX_NATIVE_CALL n_WS_NPCGoTo(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 9) return 0;
	const Vec3 destination { amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]) };
	return s_pathModule->npcGoTo(
		static_cast<int>(params[1]),
		destination,
		static_cast<int>(params[5]),
		static_cast<int>(params[6]),
		amx_ctof(params[7]),
		static_cast<NPCMoveType>(params[8]),
		amx_ctof(params[9]));
}

static cell AMX_NATIVE_CALL n_WS_SetPathDebug(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->setPathDebug(params[1] != 0) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_DebugPathRoute(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->showRouteDebug(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_ClearPathDebug(AMX*, cell*)
{
	if (!s_pathModule) return 0;
	s_pathModule->clearPathDebug();
	return 1;
}

static cell AMX_NATIVE_CALL n_WS_CreatePatrol(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 5) return 0;
	return s_pathModule->createPatrol(
		static_cast<int>(params[1]),
		static_cast<int>(params[2]),
		params[3] != 0,
		static_cast<NPCMoveType>(params[4]),
		amx_ctof(params[5]));
}

static cell AMX_NATIVE_CALL n_WS_StartPatrol(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->startPatrol(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_StopPatrol(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->stopPatrol(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_PausePatrol(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->pausePatrol(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_ResumePatrol(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->resumePatrol(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_DestroyPatrol(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->destroyPatrol(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_IsPatrolActive(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->isPatrolActive(static_cast<int>(params[1])) ? 1 : 0;
}

static cell AMX_NATIVE_CALL n_WS_GetPatrolRoute(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->getPatrolRoute(static_cast<int>(params[1]));
}

static cell AMX_NATIVE_CALL n_WS_GetPatrolNPC(AMX*, cell* params)
{
	if (!s_pathModule || !params || params[0] / static_cast<cell>(sizeof(cell)) < 1) return 0;
	return s_pathModule->getPatrolNPC(static_cast<int>(params[1]));
}

static const AMX_NATIVE_INFO PathNatives[] = {
	{ "WS_CreatePathNode", n_WS_CreatePathNode },
	{ "WS_ConnectPathNodes", n_WS_ConnectPathNodes },
	{ "WS_DisconnectPathNodes", n_WS_DisconnectPathNodes },
	{ "WS_GetNearestPathNode", n_WS_GetNearestPathNode },
	{ "WS_FindPath", n_WS_FindPath },
	{ "WS_DestroyPath", n_WS_DestroyPath },
	{ "WS_ClearPathCache", n_WS_ClearPathCache },
	{ "WS_GetPathCacheSize", n_WS_GetPathCacheSize },
	{ "WS_GetPathLength", n_WS_GetPathLength },
	{ "WS_GetPathNode", n_WS_GetPathNode },
	{ "WS_GetPathPoint", n_WS_GetPathPoint },
	{ "WS_CreateNPCPath", n_WS_CreateNPCPath },
	{ "WS_MoveNPCByPath", n_WS_MoveNPCByPath },
	{ "WS_NPCGoTo", n_WS_NPCGoTo },
	{ "WS_SetPathDebug", n_WS_SetPathDebug },
	{ "WS_DebugPathRoute", n_WS_DebugPathRoute },
	{ "WS_ClearPathDebug", n_WS_ClearPathDebug },
	{ "WS_CreatePatrol", n_WS_CreatePatrol },
	{ "WS_StartPatrol", n_WS_StartPatrol },
	{ "WS_StopPatrol", n_WS_StopPatrol },
	{ "WS_PausePatrol", n_WS_PausePatrol },
	{ "WS_ResumePatrol", n_WS_ResumePatrol },
	{ "WS_DestroyPatrol", n_WS_DestroyPatrol },
	{ "WS_IsPatrolActive", n_WS_IsPatrolActive },
	{ "WS_GetPatrolRoute", n_WS_GetPatrolRoute },
	{ "WS_GetPatrolNPC", n_WS_GetPatrolNPC },
};

void PathModule::registerNatives(IPawnScript& script)
{
	s_pathModule = this;
	script.Register(PathNatives, static_cast<int>(sizeof(PathNatives) / sizeof(PathNatives[0])));
}
} // namespace worlds
