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
	FreeVarArray(&way->nodeIds);
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
}

void InitOsmMap(Arena* arena, OsmMap* mapOut, u64 numNodesExpected, u64 numWaysExpected)
{
	NotNull(arena);
	NotNull(mapOut);
	ClearPointer(mapOut);
	mapOut->arena = arena;
	mapOut->nextNodeId = 1;
	mapOut->nextWayId = 1;
	InitVarArrayWithInitial(OsmNode, &mapOut->nodes, arena, numNodesExpected);
	InitVarArrayWithInitial(OsmWay, &mapOut->ways, arena, numWaysExpected);
}

Result TryParseOsmMap(Arena* arena, Str8 xmlFileContents, OsmMap* mapOut)
{
	ScratchBegin1(scratch, arena);
	XmlFile xml = ZEROED;
	Result parseResult = TryParseXml(xmlFileContents, scratch, &xml);
	if (parseResult != Result_Success) { ScratchEnd(scratch); return parseResult; }
	
	
	
	InitOsmMap(arena, mapOut, numNodesInFile, numWaysInFile);
	
	do
	{
		XmlElement* root = XmlGetOneChildOrBreak(&xml, nullptr, StrLit("osm"));
		Str8 versionStr = XmlGetAttributeOrDefault(&xml, root, StrLit("version"), Str8_Empty);
		// Str8 generator = XmlGetAttributeOrDefault(&xml, root, StrLit("generator"), Str8_Empty);
		// Str8 copyright = XmlGetAttributeOrDefault(&xml, root, StrLit("copyright"), Str8_Empty);
		// Str8 attribution = XmlGetAttributeOrDefault(&xml, root, StrLit("attribution"), Str8_Empty);
		// Str8 license = XmlGetAttributeOrDefault(&xml, root, StrLit("license"), Str8_Empty);
		XmlElement* bounds = XmlGetOneChildOrBreak(&xml, root, StrLit("bounds"));
		//TODO: Should these be r64?
		r32 boundsMinLat = XmlGetAttributeR32OrBreak(&xml, bounds, StrLit("minlat"));
		r32 boundsMinLon = XmlGetAttributeR32OrBreak(&xml, bounds, StrLit("minlon"));
		r32 boundsMaxLat = XmlGetAttributeR32OrBreak(&xml, bounds, StrLit("maxlat"));
		r32 boundsMaxLon = XmlGetAttributeR32OrBreak(&xml, bounds, StrLit("maxlon"));
		rec boundsRec = NewRec(boundsMinLon, boundsMinLat, boundsMaxLon - boundsMinLon, boundsMaxLat - boundsMinLat);
	} while(false);
	
	//TODO: Implement me!
	
	ScratchEnd(scratch);
	return Result_Success;
}

OsmNode* AddOsmNode(OsmMap* map, GeoLoc location)
{
	NotNull(map);
	NotNull(map->arena);
	OsmNode* result = VarArrayAdd(OsmNode, &map->nodes);
	result->id = map->nextNodeId;
	map->nextNodeId++;
	result->location = location;
	result->visible = true;
	InitVarArray(OsmTag, &result->tags, map->arena);
	return result;
}

OsmWay* AddOsmWay(OsmMap* map, u64 numNodes, u64* nodeIds)
{
	NotNull(map);
	NotNull(map->arena);
	Assert(numNodes == 0 || nodeIds != nullptr);
	OsmWay* result = VarArrayAdd(OsmWay, &map->ways);
	result->id = map->nextWayId;
	map->nextWayId++;
	result->visible = true;
	InitVarArrayWithInitial(u64, &result->nodeIds, map->arena, numNodes);
	for (u64 nIndex = 0; nIndex < numNodes; nIndex++)
	{
		VarArrayAddValue(u64, &result->nodeIds, nodeIds[nIndex]);
	}
	InitVarArray(OsmTag, &result->tags, map->arena);
	return result;
}
