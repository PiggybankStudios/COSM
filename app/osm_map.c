/*
File:   osm_map.c
Author: Taylor Robbins
Date:   07\06\2025
Description: 
	** None
*/

void FreeOsmTag(Arena* arena, OsmTag* tag)
{
	NotNull(arena);
	NotNull(tag);
	FreeStr8(arena, &tag->key);
	FreeStr8(arena, &tag->value);
	ClearPointer(tag);
}

void FreeOsmNode(Arena* arena, OsmNode* node)
{
	NotNull(arena);
	NotNull(node);
	FreeStr8(arena, &node->timestampStr);
	FreeStr8(arena, &node->user);
	VarArrayLoop(&node->tags, tIndex)
	{
		VarArrayLoopGet(OsmTag, tag, &node->tags, tIndex);
		FreeOsmTag(arena, tag);
	}
	FreeVarArray(&node->tags);
	if (node->wayPntrs.arena != nullptr) { FreeVarArray(&node->wayPntrs); }
	if (node->wayPntrs.arena != nullptr) { FreeVarArray(&node->relationPntrs); }
	ClearPointer(node);
}

void FreeOsmWay(Arena* arena, OsmWay* way)
{
	NotNull(arena);
	NotNull(way);
	FreeStr8(arena, &way->timestampStr);
	FreeStr8(arena, &way->user);
	FreeVarArray(&way->nodes);
	VarArrayLoop(&way->tags, tIndex)
	{
		VarArrayLoopGet(OsmTag, tag, &way->tags, tIndex);
		FreeOsmTag(arena, tag);
	}
	FreeVarArray(&way->tags);
	if (way->relationPntrs.arena != nullptr) { FreeVarArray(&way->relationPntrs); }
	if (way->triIndices != nullptr) { FreeArray(uxx, arena, way->numTriIndices, way->triIndices); }
	FreeVertBuffer(&way->triVertBuffer);
	ClearPointer(way);
}

void FreeOsmRelation(Arena* arena, OsmRelation* relation)
{
	NotNull(arena);
	NotNull(relation);
	FreeStr8(arena, &relation->timestampStr);
	FreeStr8(arena, &relation->user);
	VarArrayLoop(&relation->tags, tIndex)
	{
		VarArrayLoopGet(OsmTag, tag, &relation->tags, tIndex);
		FreeOsmTag(arena, tag);
	}
	FreeVarArray(&relation->tags);
	VarArrayLoop(&relation->members, mIndex)
	{
		VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
		FreeVarArray(&member->locations);
	}
	FreeVarArray(&relation->members);
	if (relation->relationPntrs.arena != nullptr) { FreeVarArray(&relation->relationPntrs); }
	ClearPointer(relation);
}

void FreeOsmMap(OsmMap* map)
{
	TracyCZoneN(funcZone, "FreeOsmMap", true);
	NotNull(map);
	if (map->arena != nullptr)
	{
		VarArrayLoop(&map->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
			FreeOsmNode(map->arena, node);
		}
		FreeVarArray(&map->nodes);
		VarArrayLoop(&map->ways, wIndex)
		{
			VarArrayLoopGet(OsmWay, way, &map->ways, wIndex);
			FreeOsmWay(map->arena, way);
		}
		FreeVarArray(&map->ways);
		FreeVarArray(&map->selectedItems);
	}
	ClearPointer(map);
	TracyCZoneEnd(funcZone);
}

void InitOsmMap(Arena* arena, OsmMap* mapOut, u64 numNodesExpected, u64 numWaysExpected, u64 numRelationsExpected)
{
	TracyCZoneN(funcZone, "InitOsmMap", true);
	NotNull(arena);
	NotNull(mapOut);
	ClearPointer(mapOut);
	mapOut->arena = arena;
	mapOut->nextNodeId = 1;
	mapOut->nextWayId = 1;
	mapOut->nextRelationId = 1;
	InitVarArrayWithInitial(OsmNode, &mapOut->nodes, arena, numNodesExpected);
	InitVarArrayWithInitial(OsmWay, &mapOut->ways, arena, numWaysExpected);
	InitVarArrayWithInitial(OsmRelation, &mapOut->relations, arena, numRelationsExpected);
	InitVarArray(OsmSelectedItem, &mapOut->selectedItems, arena);
	TracyCZoneEnd(funcZone);
}

OsmNode* FindOsmNode(OsmMap* map, u64 nodeId)
{
	TracyCZoneN(funcZone, "FindOsmNode", true);
	if (map->areNodesSorted)
	{
		uxx foundIndex = BinarySearchVarArrayUintMember(OsmNode, id, &map->nodes, &nodeId);
		if (foundIndex < map->nodes.length) { TracyCZoneEnd(funcZone); return VarArrayGet(OsmNode, &map->nodes, foundIndex); }
	}
	else
	{
		VarArrayLoop(&map->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
			if (node->id == nodeId) { TracyCZoneEnd(funcZone); return node; }
		}
	}
	TracyCZoneEnd(funcZone);
	return nullptr;
}
OsmWay* FindOsmWay(OsmMap* map, u64 wayId)
{
	TracyCZoneN(funcZone, "FindOsmWay", true);
	if (map->areWaysSorted)
	{
		uxx foundIndex = BinarySearchVarArrayUintMember(OsmWay, id, &map->ways, &wayId);
		if (foundIndex < map->ways.length) { TracyCZoneEnd(funcZone); return VarArrayGet(OsmWay, &map->ways, foundIndex); }
	}
	else
	{
		VarArrayLoop(&map->ways, wIndex)
		{
			VarArrayLoopGet(OsmWay, way, &map->ways, wIndex);
			if (way->id == wayId) { TracyCZoneEnd(funcZone); return way; }
		}
	}
	TracyCZoneEnd(funcZone);
	return nullptr;
}
OsmRelation* FindOsmRelation(OsmMap* map, u64 relationId)
{
	TracyCZoneN(funcZone, "FindOsmRelation", true);
	VarArrayLoop(&map->relations, rIndex)
	{
		VarArrayLoopGet(OsmRelation, relation, &map->relations, rIndex);
		if (relation->id == relationId) { TracyCZoneEnd(funcZone); return relation; }
	}
	TracyCZoneEnd(funcZone);
	return nullptr;
}

OsmNode* AddOsmNode(OsmMap* map, v2d location, u64 id)
{
	TracyCZoneN(funcZone, "AddOsmNode", true);
	NotNull(map);
	NotNull(map->arena);
	OsmNode* result = VarArrayAdd(OsmNode, &map->nodes);
	NotNull(result);
	ClearPointer(result);
	result->id = (id == 0) ? map->nextNodeId : id;
	if (id == 0) { map->nextNodeId++; }
	else if (map->nextNodeId <= id) { map->nextNodeId = id+1; }
	result->location = location;
	result->visible = true;
	InitVarArray(OsmTag, &result->tags, map->arena);
	TracyCZoneEnd(funcZone);
	return result;
}

OsmWay* AddOsmWay(OsmMap* map, u64 id, u64 numNodes, u64* nodeIds)
{
	TracyCZoneN(funcZone, "AddOsmWay", true);
	NotNull(map);
	NotNull(map->arena);
	Assert(numNodes == 0 || nodeIds != nullptr);
	OsmWay* result = VarArrayAdd(OsmWay, &map->ways);
	NotNull(result);
	ClearPointer(result);
	result->id = (id == 0) ? map->nextWayId : id;
	if (id == 0) { map->nextWayId++; }
	else if (map->nextWayId <= id) { map->nextWayId = id+1; }
	result->visible = true;
	InitVarArrayWithInitial(OsmNodeRef, &result->nodes, map->arena, numNodes);
	bool foundFirstNode = false;
	for (u64 nIndex = 0; nIndex < numNodes; nIndex++)
	{
		OsmNodeRef* newRef = VarArrayAdd(OsmNodeRef, &result->nodes);
		NotNull(newRef);
		ClearPointer(newRef);
		newRef->id = nodeIds[nIndex];
		newRef->pntr = FindOsmNode(map, newRef->id);
		if (newRef->pntr == nullptr) { map->waysMissingNodes = true; }
		else
		{
			if (!foundFirstNode) { result->nodeBounds = NewRecd(newRef->pntr->location.Lon, newRef->pntr->location.Lat, 0, 0); foundFirstNode = true; }
			else { result->nodeBounds = BothRecd(result->nodeBounds, NewRecdV(newRef->pntr->location, V2d_Zero)); }
		}
	}
	result->isClosedLoop = (numNodes >= 3 && nodeIds[0] == nodeIds[numNodes-1]);
	InitVarArray(OsmTag, &result->tags, map->arena);
	TracyCZoneEnd(funcZone);
	return result;
}

OsmRelation* AddOsmRelation(OsmMap* map, u64 id, uxx numMembersExpected)
{
	TracyCZoneN(funcZone, "AddOsmRelation", true);
	NotNull(map);
	NotNull(map->arena);
	OsmRelation* result = VarArrayAdd(OsmRelation, &map->relations);
	NotNull(result);
	ClearPointer(result);
	result->id = (id == 0) ? map->nextRelationId : id;
	if (id == 0) { map->nextRelationId++; }
	else if (map->nextRelationId <= id) { map->nextRelationId = id+1; }
	result->visible = true;
	InitVarArray(OsmTag, &result->tags, map->arena);
	InitVarArrayWithInitial(OsmRelationMember, &result->members, map->arena, numMembersExpected);
	TracyCZoneEnd(funcZone);
	return result;
}

void UpdateOsmRelationPntrs(OsmMap* map, OsmRelation* relation)
{
	NotNull(map);
	NotNull(relation);
	VarArrayLoop(&relation->members, mIndex)
	{
		VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
		if (member->type == OsmRelationMemberType_Node)
		{
			member->nodePntr = FindOsmNode(map, member->id);
			if (member->nodePntr != nullptr) { map->relationsMissingMembers = true; }
		}
		else if (member->type == OsmRelationMemberType_Way)
		{
			member->wayPntr = FindOsmWay(map, member->id);
			if (member->wayPntr != nullptr) { map->relationsMissingMembers = true; }
		}
		else if (member->type == OsmRelationMemberType_Relation)
		{
			member->relationPntr = FindOsmRelation(map, member->id);
			if (member->relationPntr != nullptr) { map->relationsMissingMembers = true; }
		}
	}
}

void UpdateOsmNodeWayBackPntrs(OsmMap* map)
{
	VarArrayLoop(&map->nodes, nIndex)
	{
		VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
		if (node->wayPntrs.arena != nullptr) { VarArrayClear(&node->wayPntrs); }
	}
	
	VarArrayLoop(&map->ways, wIndex)
	{
		VarArrayLoopGet(OsmWay, way, &map->ways, wIndex);
		VarArrayLoop(&way->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
			if (nodeRef->pntr != nullptr)
			{
				if (nodeRef->pntr->wayPntrs.arena == nullptr) { InitVarArray(OsmWay*, &nodeRef->pntr->wayPntrs, map->arena); }
				VarArrayAddValue(OsmWay*, &nodeRef->pntr->wayPntrs, way);
			}
		}
	}
}

void UpdateOsmRelationBackPntrs(OsmMap* map)
{
	VarArrayLoop(&map->nodes, nIndex)
	{
		VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
		if (node->relationPntrs.arena != nullptr) { VarArrayClear(&node->relationPntrs); }
	}
	VarArrayLoop(&map->ways, wIndex)
	{
		VarArrayLoopGet(OsmWay, way, &map->ways, wIndex);
		if (way->relationPntrs.arena != nullptr) { VarArrayClear(&way->relationPntrs); }
	}
	VarArrayLoop(&map->relations, rIndex)
	{
		VarArrayLoopGet(OsmRelation, relation, &map->relations, rIndex);
		if (relation->relationPntrs.arena != nullptr) { VarArrayClear(&relation->relationPntrs); }
	}
	
	VarArrayLoop(&map->relations, rIndex)
	{
		VarArrayLoopGet(OsmRelation, relation, &map->relations, rIndex);
		VarArrayLoop(&relation->members, mIndex)
		{
			VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
			if (member->type == OsmRelationMemberType_Node && member->nodePntr != nullptr)
			{
				if (member->nodePntr->relationPntrs.arena == nullptr) { InitVarArray(OsmRelation*, &member->nodePntr->relationPntrs, map->arena); }
				VarArrayAddValue(OsmRelation*, &member->nodePntr->relationPntrs, relation);
			}
			else if (member->type == OsmRelationMemberType_Way && member->wayPntr != nullptr)
			{
				if (member->wayPntr->relationPntrs.arena == nullptr) { InitVarArray(OsmRelation*, &member->wayPntr->relationPntrs, map->arena); }
				VarArrayAddValue(OsmRelation*, &member->wayPntr->relationPntrs, relation);
			}
			else if (member->type == OsmRelationMemberType_Relation && member->relationPntr != nullptr)
			{
				if (member->relationPntr->relationPntrs.arena == nullptr) { InitVarArray(OsmRelation*, &member->relationPntr->relationPntrs, map->arena); }
				VarArrayAddValue(OsmRelation*, &member->relationPntr->relationPntrs, relation);
			}
		}
	}
}

Str8 GetOsmNodeTagValue(OsmNode* node, Str8 tagKey, Str8 defaultValue)
{
	if (node == nullptr) { return defaultValue; }
	VarArrayLoop(&node->tags, tIndex)
	{
		VarArrayLoopGet(OsmTag, tag, &node->tags, tIndex);
		if (StrAnyCaseEquals(tag->key, tagKey)) { return tag->value; }
	}
	return defaultValue;
}
Str8 GetOsmWayTagValue(OsmWay* way, Str8 tagKey, Str8 defaultValue)
{
	if (way == nullptr) { return defaultValue; }
	VarArrayLoop(&way->tags, tIndex)
	{
		VarArrayLoopGet(OsmTag, tag, &way->tags, tIndex);
		if (StrAnyCaseEquals(tag->key, tagKey)) { return tag->value; }
	}
	return defaultValue;
}
Str8 GetOsmRelationTagValue(OsmRelation* relation, Str8 tagKey, Str8 defaultValue)
{
	if (relation == nullptr) { return defaultValue; }
	VarArrayLoop(&relation->tags, tIndex)
	{
		VarArrayLoopGet(OsmTag, tag, &relation->tags, tIndex);
		if (StrAnyCaseEquals(tag->key, tagKey)) { return tag->value; }
	}
	return defaultValue;
}
