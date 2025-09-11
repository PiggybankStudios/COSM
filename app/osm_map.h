/*
File:   osm_map.h
Author: Taylor Robbins
Date:   07\06\2025
*/

#ifndef _OSM_MAP_H
#define _OSM_MAP_H

typedef enum OsmPrimitiveType OsmPrimitiveType;
enum OsmPrimitiveType
{
	OsmPrimitiveType_None = 0,
	OsmPrimitiveType_Node,
	OsmPrimitiveType_Way,
	OsmPrimitiveType_Count,
};
const char* GetOsmPrimitiveTypeStr(OsmPrimitiveType enumValue)
{
	switch (enumValue)
	{
		case OsmPrimitiveType_None:  return "None";
		case OsmPrimitiveType_Node:  return "Node";
		case OsmPrimitiveType_Way:   return "Way";
		default: return UNKNOWN_STR;
	}
}

// <tag k="highway" v="secondary"/>
typedef plex OsmTag OsmTag;
plex OsmTag
{
	Str8 key;
	Str8 value;
};

// <node id="30139418" visible="true" version="5" changeset="50213102" timestamp="2017-07-11T21:17:35Z" user="Natfoot" uid="567792" lat="47.7801029" lon="-122.1907513"/>
typedef plex OsmNode OsmNode;
plex OsmNode
{
	u64 id;
	bool visible;
	i32 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	v2d location;
	VarArray tags; //OsmTag
	
	bool isSelected;
	bool isHovered;
};

typedef plex OsmNodeRef OsmNodeRef;
plex OsmNodeRef
{
	u64 id;
	OsmNode* pntr;
};

typedef enum OsmRenderLayer OsmRenderLayer;
enum OsmRenderLayer
{
	OsmRenderLayer_None = 0,
	OsmRenderLayer_Bottom,
	OsmRenderLayer_Middle,
	OsmRenderLayer_Top,
	OsmRenderLayer_Selection,
	OsmRenderLayer_Count,
};
const char* GetOsmRenderLayerStr(OsmRenderLayer enumValue)
{
	switch (enumValue)
	{
		case OsmRenderLayer_None:      return "None";
		case OsmRenderLayer_Bottom:    return "Bottom";
		case OsmRenderLayer_Middle:    return "Middle";
		case OsmRenderLayer_Top:       return "Top";
		case OsmRenderLayer_Selection: return "Selection";
		default: return UNKNOWN_STR;
	}
}

// <way id="428033729" visible="true" version="5" changeset="81153531" timestamp="2020-02-18T06:41:24Z" user="thadekam" uid="10455646">
typedef plex OsmWay OsmWay;
plex OsmWay
{
	u64 id;
	bool visible;
	i32 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	
	VarArray nodes; //OsmNodeRef
	VarArray tags; //OsmTag
	recd nodeBounds;
	
	bool colorsChosen;
	bool isClosedLoop;
	OsmRenderLayer renderLayer;
	r32 lineThickness; //for non-closed-loops
	Color32 fillColor;
	r32 borderThickness;
	Color32 borderColor;
	
	bool attemptedTriangulation;
	uxx numTriIndices;
	uxx* triIndices;
	VertBuffer triVertBuffer; //these vertices are normalized within bounds
	
	bool isSelected;
	bool isHovered;
};

typedef enum OsmRelationMemberType OsmRelationMemberType;
enum OsmRelationMemberType
{
	OsmRelationMemberType_None = 0,
	OsmRelationMemberType_Node,
	OsmRelationMemberType_Way,
	OsmRelationMemberType_Relation,
	OsmRelationMemberType_Count,
};
const char* GetOsmRelationMemberTypeStr(OsmRelationMemberType enumValue)
{
	switch (enumValue)
	{
		case OsmRelationMemberType_None:     return "None";
		case OsmRelationMemberType_Node:     return "Node";
		case OsmRelationMemberType_Way:      return "Way";
		case OsmRelationMemberType_Relation: return "Relation";
		default: return UNKNOWN_STR;
	}
}
const char* GetOsmRelationMemberTypeXmlStr(OsmRelationMemberType enumValue)
{
	switch (enumValue)
	{
		case OsmRelationMemberType_Node:     return "node";
		case OsmRelationMemberType_Way:      return "way";
		case OsmRelationMemberType_Relation: return "relation";
		default: return "";
	}
}

typedef plex OsmRelationMember OsmRelationMember;
plex OsmRelationMember
{
	u64 id;
	u32 role;
	OsmRelationMemberType type;
	VarArray locations; //v2d
	union { void* pntr; OsmNode* nodePntr; OsmWay* wayPntr; plex OsmRelation* relationPntr; };
};

typedef plex OsmRelation OsmRelation;
plex OsmRelation
{
	u64 id;
	bool visible;
	i32 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	
	VarArray tags; //OsmTag
	VarArray members; //OsmRelationMember
};

typedef plex OsmSelectedItem OsmSelectedItem;
plex OsmSelectedItem
{
	OsmPrimitiveType type;
	u64 id;
	union { void* pntr; OsmNode* nodePntr; OsmWay* wayPntr; };
};

typedef plex OsmMap OsmMap;
plex OsmMap
{
	Arena* arena;
	recd bounds;
	Str8 versionStr;
	Str8 generatorStr;
	Str8 copyrightStr;
	Str8 attributionStr;
	Str8 licenseStr;
	
	bool areNodesSorted;
	u64 nextNodeId;
	VarArray nodes; //OsmNode
	bool areWaysSorted;
	u64 nextWayId;
	VarArray ways; //OsmWay
	bool areRelationsSorted;
	u64 nextRelationId;
	VarArray relations; //OsmRelation
	
	VarArray selectedItems; //OsmSelectedItem
};

#endif //  _OSM_MAP_H
