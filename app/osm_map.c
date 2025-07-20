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
	ClearPointer(way);
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
	}
	ClearPointer(map);
	TracyCZoneEnd(funcZone);
}

void InitOsmMap(Arena* arena, OsmMap* mapOut, u64 numNodesExpected, u64 numWaysExpected)
{
	TracyCZoneN(funcZone, "InitOsmMap", true);
	NotNull(arena);
	NotNull(mapOut);
	ClearPointer(mapOut);
	mapOut->arena = arena;
	mapOut->nextNodeId = 1;
	mapOut->nextWayId = 1;
	InitVarArrayWithInitial(OsmNode, &mapOut->nodes, arena, numNodesExpected);
	InitVarArrayWithInitial(OsmWay, &mapOut->ways, arena, numWaysExpected);
	TracyCZoneEnd(funcZone);
}

OsmNode* FindOsmNode(OsmMap* map, u64 nodeId)
{
	TracyCZoneN(funcZone, "FindOsmNode", true);
	VarArrayLoop(&map->nodes, nIndex)
	{
		VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
		if (node->id == nodeId) { TracyCZoneEnd(funcZone); return node; }
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
	ClearPointer(result);
	result->id = (id == 0) ? map->nextNodeId : id;
	if (id == 0) { map->nextNodeId++; }
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
	ClearPointer(result);
	result->id = (id == 0) ? map->nextWayId : id;
	if (id == 0) { map->nextWayId++; }
	result->visible = true;
	InitVarArrayWithInitial(OsmNodeRef, &result->nodes, map->arena, numNodes);
	for (u64 nIndex = 0; nIndex < numNodes; nIndex++)
	{
		OsmNodeRef* newRef = VarArrayAdd(OsmNodeRef, &result->nodes);
		NotNull(newRef);
		ClearPointer(newRef);
		newRef->id = nodeIds[nIndex];
		newRef->pntr = FindOsmNode(map, newRef->id);
		NotNull(newRef->pntr);
	}
	InitVarArray(OsmTag, &result->tags, map->arena);
	TracyCZoneEnd(funcZone);
	return result;
}

Result TryParseOsmMap(Arena* arena, Str8 xmlFileContents, OsmMap* mapOut)
{
	TracyCZoneN(funcZone, "TryParseOsmMap", true);
	ScratchBegin1(scratch, arena);
	XmlFile xml = ZEROED;
	Result parseResult = TryParseXml(xmlFileContents, scratch, &xml);
	if (parseResult != Result_Success)
	{
		ScratchEnd(scratch);
		TracyCZoneEnd(funcZone);
		return parseResult;
	}
	
	InitOsmMap(arena, mapOut, 0, 0);
	
	do
	{
		XmlElement* root = XmlGetOneChildOrBreak(&xml, nullptr, StrLit("osm"));
		// Str8 versionStr = XmlGetAttributeOrDefault(&xml, root, StrLit("version"), Str8_Empty);
		// Str8 generator = XmlGetAttributeOrDefault(&xml, root, StrLit("generator"), Str8_Empty);
		// Str8 copyright = XmlGetAttributeOrDefault(&xml, root, StrLit("copyright"), Str8_Empty);
		// Str8 attribution = XmlGetAttributeOrDefault(&xml, root, StrLit("attribution"), Str8_Empty);
		// Str8 license = XmlGetAttributeOrDefault(&xml, root, StrLit("license"), Str8_Empty);
		XmlElement* bounds = XmlGetOneChildOrBreak(&xml, root, StrLit("bounds"));
		//TODO: Should these be r64?
		r64 boundsMinLon = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("minlon"));
		r64 boundsMinLat = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("minlat"));
		r64 boundsMaxLon = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("maxlon"));
		r64 boundsMaxLat = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("maxlat"));
		//TODO: This does not properly handle if the rectangle passes over the international date line (max longitude will be less than min longitude)
		mapOut->bounds = NewRecd(boundsMinLon, boundsMinLat, boundsMaxLon - boundsMinLon, boundsMaxLat - boundsMinLat);
		
		XmlElement* xmlNode = nullptr;
		while ((xmlNode = XmlGetNextChild(&xml, root, StrLit("node"), xmlNode)) != nullptr)
		{
			u64 id = XmlGetAttributeU64OrBreak(&xml, xmlNode, StrLit("id"));
			r64 longitude = XmlGetAttributeR64OrBreak(&xml, xmlNode, StrLit("lon"));
			r64 latitude = XmlGetAttributeR64OrBreak(&xml, xmlNode, StrLit("lat"));
			OsmNode* newNode = AddOsmNode(mapOut, NewV2d(longitude, latitude), id);
			NotNull(newNode);
			UNUSED(newNode);
			// TODO: Parse "visible" attribute
			// TODO: Parse "version" attribute
			// TODO: Parse "changeset" attribute
			// TODO: Parse "timestamp" attribute
			// TODO: Parse "user" attribute
			// TODO: Parse "uid" attribute
		}
		if (xml.error != Result_None) { break; }
		
		XmlElement* xmlWay = nullptr;
		while ((xmlWay = XmlGetNextChild(&xml, root, StrLit("way"), xmlWay)) != nullptr)
		{
			u64 mark = ArenaGetMark(scratch);
			u64 numNodesInWay = 0;
			VarArrayLoop(&xmlWay->children, cIndex)
			{
				VarArrayLoopGet(XmlElement, xmlChild, &xmlWay->children, cIndex);
				if (StrExactEquals(xmlChild->type, StrLit("nd"))) { numNodesInWay++; }
			}
			
			u64* nodeIds = (numNodesInWay != 0) ? AllocArray(u64, scratch, numNodesInWay) : nullptr;
			u64 nIndex = 0;
			VarArrayLoop(&xmlWay->children, cIndex)
			{
				VarArrayLoopGet(XmlElement, xmlChild, &xmlWay->children, cIndex);
				if (StrExactEquals(xmlChild->type, StrLit("nd")))
				{
					nodeIds[nIndex] = XmlGetAttributeU64OrBreak(&xml, xmlChild, StrLit("ref"));
					nIndex++;
				}
			}
			if (xml.error != Result_None) { break; }
			
			u64 id = XmlGetAttributeU64OrBreak(&xml, xmlWay, StrLit("id"));
			// TODO: Parse "visible" attribute
			// TODO: Parse "version" attribute
			// TODO: Parse "changeset" attribute
			// TODO: Parse "timestamp" attribute
			// TODO: Parse "user" attribute
			// TODO: Parse "uid" attribute
			OsmWay* newWay = AddOsmWay(mapOut, id, numNodesInWay, nodeIds);
			NotNull(newWay);
			UNUSED(newWay);
			//TODO: Parse the <tag> elements under the <way>
			ArenaResetToMark(scratch, mark);
		}
		if (xml.error != Result_None) { break; }
		
	} while(false);
	
	if (xml.error)
	{
		FreeOsmMap(mapOut);
	}
	
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
	return (xml.error == Result_None) ? Result_Success : xml.error;
}
