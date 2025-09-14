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

void OsmAddFromMap(OsmMap* dstMap, const OsmMap* srcMap)
{
	dstMap->bounds = BothRecd(dstMap->bounds, srcMap->bounds);
	
	// +==============================+
	// |          Add Nodes           |
	// +==============================+
	{
		VarArrayExpand(&dstMap->nodes, dstMap->nodes.length + srcMap->nodes.length);
		
		u64 prevNodeId = 0;
		if (dstMap->nodes.length > 0) { prevNodeId = VarArrayGetLast(OsmNode, &dstMap->nodes)->id; }
		VarArrayLoop(&srcMap->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNode, srcNode, &srcMap->nodes, nIndex);
			OsmNode* existingNode = FindOsmNode(dstMap, srcNode->id);
			if (existingNode == nullptr)
			{
				if (srcNode->id <= prevNodeId) { dstMap->areNodesSorted = false; }
				prevNodeId = srcNode->id;
				OsmNode* dstNode = AddOsmNode(dstMap, srcNode->location, srcNode->id);
				NotNull(dstNode);
				dstNode->visible = srcNode->visible;
				dstNode->version = srcNode->version;
				dstNode->changeset = srcNode->changeset;
				dstNode->timestampStr = (!IsEmptyStr(srcNode->timestampStr) ? AllocStr8(dstMap->arena, srcNode->timestampStr) : Str8_Empty);
				dstNode->user = (!IsEmptyStr(srcNode->user) ? AllocStr8(dstMap->arena, srcNode->user) : Str8_Empty);
				dstNode->uid = srcNode->uid;
				VarArrayLoop(&srcNode->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, srcTag, &srcNode->tags, tIndex);
					OsmTag* dstTag = VarArrayAdd(OsmTag, &dstNode->tags);
					NotNull(dstTag);
					dstTag->key = AllocStr8(dstMap->arena, srcTag->key);
					dstTag->value = AllocStr8(dstMap->arena, srcTag->value);
				}
			}
		}
		
		//Sort the nodes if needed
		if (!dstMap->areNodesSorted)
		{
			QuickSortVarArrayUintMember(OsmNode, id, &dstMap->nodes);
			dstMap->areNodesSorted = true;
		}
		
		//Update NodeRef pntrs for existing ways since we expanded the node array and it may have been reallocated
		VarArrayLoop(&dstMap->ways, wIndex)
		{
			VarArrayLoopGet(OsmWay, way, &dstMap->ways, wIndex);
			VarArrayLoop(&way->nodes, nIndex)
			{
				VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
				nodeRef->pntr = FindOsmNode(dstMap, nodeRef->id);
			}
		}
	}
	
	// +==============================+
	// |           Add Ways           |
	// +==============================+
	{
		VarArrayExpand(&dstMap->ways, dstMap->ways.length + srcMap->ways.length);
		
		u64 prevWayId = 0;
		if (dstMap->ways.length > 0) { prevWayId = VarArrayGetLast(OsmWay, &dstMap->ways)->id; }
		VarArrayLoop(&srcMap->ways, wIndex)
		{
			VarArrayLoopGet(OsmWay, srcWay, &srcMap->ways, wIndex);
			OsmWay* existingWay = FindOsmWay(dstMap, srcWay->id);
			if (existingWay == nullptr)
			{
				if (srcWay->id <= prevWayId) { dstMap->areWaysSorted = false; }
				prevWayId = srcWay->id;
				ScratchBegin1(scratch, dstMap->arena);
				u64* nodeIds = (srcWay->nodes.length > 0) ? AllocArray(u64, scratch, srcWay->nodes.length) : nullptr;
				VarArrayLoop(&srcWay->nodes, nIndex) { VarArrayLoopGet(OsmNodeRef, nodeRef, &srcWay->nodes, nIndex); nodeIds[nIndex] = nodeRef->id; }
				OsmWay* dstWay = AddOsmWay(dstMap, srcWay->id, srcWay->nodes.length, nodeIds);
				ScratchEnd(scratch);
				NotNull(dstWay);
				dstWay->visible = srcWay->visible;
				dstWay->version = srcWay->version;
				dstWay->changeset = srcWay->changeset;
				dstWay->timestampStr = (!IsEmptyStr(srcWay->timestampStr) ? AllocStr8(dstMap->arena, srcWay->timestampStr) : Str8_Empty);
				dstWay->user = (!IsEmptyStr(srcWay->user) ? AllocStr8(dstMap->arena, srcWay->user) : Str8_Empty);
				dstWay->uid = srcWay->uid;
				VarArrayLoop(&srcWay->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, srcTag, &srcWay->tags, tIndex);
					OsmTag* dstTag = VarArrayAdd(OsmTag, &dstWay->tags);
					NotNull(dstTag);
					dstTag->key = AllocStr8(dstMap->arena, srcTag->key);
					dstTag->value = AllocStr8(dstMap->arena, srcTag->value);
				}
			}
		}
		
		//Sort the ways if needed
		if (!dstMap->areWaysSorted)
		{
			QuickSortVarArrayUintMember(OsmWay, id, &dstMap->ways);
			dstMap->areWaysSorted = true;
		}
		
		UpdateOsmNodeWayBackPntrs(dstMap);
		
		//Update Relation Member Pntrs
		VarArrayLoop(&dstMap->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, relation, &dstMap->relations, rIndex);
			VarArrayLoop(&relation->members, mIndex)
			{
				VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
				if (member->type == OsmRelationMemberType_Node) { member->nodePntr = FindOsmNode(dstMap, member->id); }
				else if (member->type == OsmRelationMemberType_Way) { member->wayPntr = FindOsmWay(dstMap, member->id); }
				else if (member->type == OsmRelationMemberType_Relation) { /*member->relationPntr = FindOsmRelation(dstMap, member->id);*/ } //NOTE: We'll do this for all relations later, after we've added the new ones
				else { Assert(false); }
			}
		}
	}
	
	// +==============================+
	// |        Add Relations         |
	// +==============================+
	{
		VarArrayExpand(&dstMap->relations, dstMap->relations.length + srcMap->relations.length);
		
		u64 prevRelationId = 0;
		if (dstMap->relations.length > 0) { prevRelationId = VarArrayGetLast(OsmRelation, &dstMap->relations)->id; }
		VarArrayLoop(&srcMap->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, srcRelation, &srcMap->relations, rIndex);
			OsmRelation* existingRelation = FindOsmRelation(dstMap, srcRelation->id);
			if (existingRelation == nullptr)
			{
				if (srcRelation->id <= prevRelationId) { dstMap->areRelationsSorted = false; }
				prevRelationId = srcRelation->id;
				OsmRelation* dstRelation = AddOsmRelation(dstMap, srcRelation->id, srcRelation->members.length);
				NotNull(dstRelation);
				dstRelation->visible = srcRelation->visible;
				dstRelation->version = srcRelation->version;
				dstRelation->changeset = srcRelation->changeset;
				dstRelation->timestampStr = (!IsEmptyStr(srcRelation->timestampStr) ? AllocStr8(dstMap->arena, srcRelation->timestampStr) : Str8_Empty);
				dstRelation->user = (!IsEmptyStr(srcRelation->user) ? AllocStr8(dstMap->arena, srcRelation->user) : Str8_Empty);
				dstRelation->uid = srcRelation->uid;
				VarArrayLoop(&srcRelation->members, mIndex)
				{
					VarArrayLoopGet(OsmRelationMember, srcMember, &srcRelation->members, mIndex);
					OsmRelationMember* dstMember = VarArrayAdd(OsmRelationMember, &dstRelation->members);
					NotNull(dstMember);
					ClearPointer(dstMember);
					dstMember->id = srcMember->id;
					dstMember->type = srcMember->type;
					dstMember->role = srcMember->role;
					//TODO: Copy the locations!
					if (dstMember->type == OsmRelationMemberType_Node) { dstMember->nodePntr = FindOsmNode(dstMap, dstMember->id); }
					else if (dstMember->type == OsmRelationMemberType_Way) { dstMember->wayPntr = FindOsmWay(dstMap, dstMember->id); }
					else if (dstMember->type == OsmRelationMemberType_Relation) { /*dstMember->relationPntr = FindOsmRelation(dstMap, dstMember->id);*/ } //NOTE: We'll do this for all relations later, after we've added the new ones
					else { Assert(false); }
				}
				VarArrayLoop(&srcRelation->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, srcTag, &srcRelation->tags, tIndex);
					OsmTag* dstTag = VarArrayAdd(OsmTag, &dstRelation->tags);
					NotNull(dstTag);
					dstTag->key = AllocStr8(dstMap->arena, srcTag->key);
					dstTag->value = AllocStr8(dstMap->arena, srcTag->value);
				}
			}
		}
		
		//Sort the relations if needed
		if (!dstMap->areRelationsSorted)
		{
			QuickSortVarArrayUintMember(OsmRelation, id, &dstMap->relations);
			dstMap->areRelationsSorted = true;
		}
		
		//Update Relation Members that are Relation Type
		VarArrayLoop(&dstMap->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, relation, &dstMap->relations, rIndex);
			VarArrayLoop(&relation->members, mIndex)
			{
				VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
				if (member->type == OsmRelationMemberType_Relation) { member->relationPntr = FindOsmRelation(dstMap, member->id); }
			}
		}
		
		UpdateOsmRelationBackPntrs(dstMap);
	}
	
	// +==============================+
	// |    Update Selection Pntrs    |
	// +==============================+
	{
		VarArrayLoop(&dstMap->selectedItems, sIndex)
		{
			VarArrayLoopGet(OsmSelectedItem, selectedItem, &dstMap->selectedItems, sIndex);
			if (selectedItem->type == OsmPrimitiveType_Node) { selectedItem->nodePntr = FindOsmNode(dstMap, selectedItem->id); }
			else if (selectedItem->type == OsmPrimitiveType_Way) { selectedItem->wayPntr = FindOsmWay(dstMap, selectedItem->id); }
			else { Assert(false); }
		}
	}
}
