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
	VarArray wayPntrs; //OsmWay*
	VarArray relationPntrs; //OsmRelation*
	
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
	VarArray relationPntrs; //OsmRelation*
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

typedef enum OsmRelationMemberRole OsmRelationMemberRole;
enum OsmRelationMemberRole
{
	OsmRelationMemberRole_None = 0,
	OsmRelationMemberRole_Outer,
	OsmRelationMemberRole_Inner,
	OsmRelationMemberRole_Label,
	OsmRelationMemberRole_North,
	OsmRelationMemberRole_South,
	OsmRelationMemberRole_East,
	OsmRelationMemberRole_West,
	OsmRelationMemberRole_From,
	OsmRelationMemberRole_To,
	OsmRelationMemberRole_Backward,
	OsmRelationMemberRole_Forward,
	OsmRelationMemberRole_Platform,
	OsmRelationMemberRole_Stop,
	OsmRelationMemberRole_Via,
	OsmRelationMemberRole_AdminCentre,
	OsmRelationMemberRole_Spring,
	OsmRelationMemberRole_Building,
	OsmRelationMemberRole_Roof,
	OsmRelationMemberRole_Station,
	OsmRelationMemberRole_Entrance,
	OsmRelationMemberRole_Access,
	OsmRelationMemberRole_StopEntryOnly,
	OsmRelationMemberRole_PlatformArea,
	OsmRelationMemberRole_PlatformEntryOnly,
	OsmRelationMemberRole_PlatformExitOnly,
	OsmRelationMemberRole_BackwardStop,
	OsmRelationMemberRole_ForwardStop,
	OsmRelationMemberRole_BusStop,
	OsmRelationMemberRole_Street,
	OsmRelationMemberRole_Connector,
	OsmRelationMemberRole_Outline,
	OsmRelationMemberRole_Both,
	OsmRelationMemberRole_Part,
	OsmRelationMemberRole_Defunct,
	OsmRelationMemberRole_Device,
	OsmRelationMemberRole_Count,
};
const char* GetOsmRelationMemberRoleStr(OsmRelationMemberRole enumValue)
{
	switch (enumValue)
	{
		case OsmRelationMemberRole_None:              return "None";
		case OsmRelationMemberRole_Outer:             return "Outer";
		case OsmRelationMemberRole_Inner:             return "Inner";
		case OsmRelationMemberRole_Label:             return "Label";
		case OsmRelationMemberRole_North:             return "North";
		case OsmRelationMemberRole_South:             return "South";
		case OsmRelationMemberRole_East:              return "East";
		case OsmRelationMemberRole_West:              return "West";
		case OsmRelationMemberRole_From:              return "From";
		case OsmRelationMemberRole_To:                return "To";
		case OsmRelationMemberRole_Backward:          return "Backward";
		case OsmRelationMemberRole_Forward:           return "Forward";
		case OsmRelationMemberRole_Platform:          return "Platform";
		case OsmRelationMemberRole_Stop:              return "Stop";
		case OsmRelationMemberRole_Via:               return "Via";
		case OsmRelationMemberRole_AdminCentre:       return "AdminCentre";
		case OsmRelationMemberRole_Spring:            return "Spring";
		case OsmRelationMemberRole_Building:          return "Building";
		case OsmRelationMemberRole_Roof:              return "Roof";
		case OsmRelationMemberRole_Station:           return "Station";
		case OsmRelationMemberRole_Entrance:          return "Entrance";
		case OsmRelationMemberRole_Access:            return "Access";
		case OsmRelationMemberRole_StopEntryOnly:     return "StopEntryOnly";
		case OsmRelationMemberRole_PlatformArea:      return "PlatformArea";
		case OsmRelationMemberRole_PlatformEntryOnly: return "PlatformEntryOnly";
		case OsmRelationMemberRole_PlatformExitOnly:  return "PlatformExitOnly";
		case OsmRelationMemberRole_BackwardStop:      return "BackwardStop";
		case OsmRelationMemberRole_ForwardStop:       return "ForwardStop";
		case OsmRelationMemberRole_BusStop:           return "BusStop";
		case OsmRelationMemberRole_Street:            return "Street";
		case OsmRelationMemberRole_Connector:         return "Connector";
		case OsmRelationMemberRole_Outline:           return "Outline";
		case OsmRelationMemberRole_Both:              return "Both";
		case OsmRelationMemberRole_Part:              return "Part";
		case OsmRelationMemberRole_Defunct:           return "Defunct";
		case OsmRelationMemberRole_Device:            return "Device";
		default: return UNKNOWN_STR;
	}
}

const char* GetOsmRelationMemberRoleXmlStr(OsmRelationMemberRole enumValue)
{
	switch (enumValue)
	{
		case OsmRelationMemberRole_Outer:             return "outer";
		case OsmRelationMemberRole_Inner:             return "inner";
		case OsmRelationMemberRole_Label:             return "label";
		case OsmRelationMemberRole_North:             return "north";
		case OsmRelationMemberRole_South:             return "south";
		case OsmRelationMemberRole_East:              return "east";
		case OsmRelationMemberRole_West:              return "west";
		case OsmRelationMemberRole_From:              return "from";
		case OsmRelationMemberRole_To:                return "to";
		case OsmRelationMemberRole_Backward:          return "backward";
		case OsmRelationMemberRole_Forward:           return "forward";
		case OsmRelationMemberRole_Platform:          return "platform";
		case OsmRelationMemberRole_Stop:              return "stop";
		case OsmRelationMemberRole_Via:               return "via";
		case OsmRelationMemberRole_AdminCentre:       return "admin_centre";
		case OsmRelationMemberRole_Spring:            return "spring";
		case OsmRelationMemberRole_Building:          return "building";
		case OsmRelationMemberRole_Roof:              return "roof";
		case OsmRelationMemberRole_Station:           return "station";
		case OsmRelationMemberRole_Entrance:          return "entrance";
		case OsmRelationMemberRole_Access:            return "access";
		case OsmRelationMemberRole_StopEntryOnly:     return "stop_entry_only";
		case OsmRelationMemberRole_PlatformArea:      return "platform_area";
		case OsmRelationMemberRole_PlatformEntryOnly: return "platform_entry_only";
		case OsmRelationMemberRole_PlatformExitOnly:  return "platform_exit_only";
		case OsmRelationMemberRole_BackwardStop:      return "backward_stop";
		case OsmRelationMemberRole_ForwardStop:       return "forward_stop";
		case OsmRelationMemberRole_BusStop:           return "bus_stop";
		case OsmRelationMemberRole_Street:            return "street";
		case OsmRelationMemberRole_Connector:         return "connector";
		case OsmRelationMemberRole_Outline:           return "outline";
		case OsmRelationMemberRole_Both:              return "both";
		case OsmRelationMemberRole_Part:              return "part";
		case OsmRelationMemberRole_Defunct:           return "defunct";
		case OsmRelationMemberRole_Device:            return "device";
		default: return "";
	}
}

typedef plex OsmRelationMember OsmRelationMember;
plex OsmRelationMember
{
	u64 id;
	OsmRelationMemberType type;
	OsmRelationMemberRole role;
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
	recd bounds;
	
	VarArray tags; //OsmTag
	VarArray members; //OsmRelationMember
	VarArray relationPntrs; //OsmRelation*
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
	bool waysMissingNodes;
	u64 nextWayId;
	VarArray ways; //OsmWay
	
	bool areRelationsSorted;
	bool relationsMissingMembers;
	u64 nextRelationId;
	VarArray relations; //OsmRelation
	
	VarArray selectedItems; //OsmSelectedItem
};

#endif //  _OSM_MAP_H
