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
	//TODO: Make sure the file is parseable as XML
	
	u64 numNodesInFile = 0;
	u64 numWaysInFile = 0;
	//TODO: Figure out how many nodes and ways there are in the file
	
	InitOsmMap(arena, mapOut, numNodesInFile, numWaysInFile);
	
	//TODO: Implement me!
	
	return Result_Success;
}
