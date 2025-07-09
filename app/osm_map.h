/*
File:   osm_map.h
Author: Taylor Robbins
Date:   07\06\2025
*/

#ifndef _OSM_MAP_H
#define _OSM_MAP_H

// <tag k="highway" v="secondary"/>
typedef struct OsmTag OsmTag;
struct OsmTag
{
	Str8 key;
	Str8 value;
};

// <node id="30139418" visible="true" version="5" changeset="50213102" timestamp="2017-07-11T21:17:35Z" user="Natfoot" uid="567792" lat="47.7801029" lon="-122.1907513"/>
typedef struct OsmNode OsmNode;
struct OsmNode
{
	u64 id;
	bool visible;
	u8 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	GeoLoc location;
	VarArray tags; //OsmTag
};

// <way id="428033729" visible="true" version="5" changeset="81153531" timestamp="2020-02-18T06:41:24Z" user="thadekam" uid="10455646">
typedef struct OsmWay OsmWay;
struct OsmWay
{
	u64 id;
	bool visible;
	u8 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	VarArray nodeIds; //u64 <nd ref="136907743"/>
	VarArray tags; //OsmTag
};

typedef struct OsmMap OsmMap;
struct OsmMap
{
	Arena* arena;
	
	u64 nextNodeId;
	VarArray nodes; //OsmNode
	u64 nextWayId;
	VarArray ways; //OsmWay
};

// +--------------------------------------------------------------+
// |                      XML Osm File Types                      |
// +--------------------------------------------------------------+
typedef struct xOsmBounds xOsmBounds;
struct xOsmBounds
{
	r64 minlat;
	r64 minlon;
	r64 maxlat;
	r64 maxlon;
};
#define xOsmBounds_SrlInfo { "bounds", sizeof(xOsmBounds), 4, { \
	{ SRL_REQ, "minlat", STRUCT_VAR_OFFSET(xOsmBounds, minlat), STRUCT_VAR_SIZE(xOsmBounds, minlat), SrlMemberType_R64, "" }, \
	{ SRL_REQ, "minlon", STRUCT_VAR_OFFSET(xOsmBounds, minlon), STRUCT_VAR_SIZE(xOsmBounds, minlon), SrlMemberType_R64, "" }, \
	{ SRL_REQ, "maxlat", STRUCT_VAR_OFFSET(xOsmBounds, maxlat), STRUCT_VAR_SIZE(xOsmBounds, maxlat), SrlMemberType_R64, "" }, \
	{ SRL_REQ, "maxlon", STRUCT_VAR_OFFSET(xOsmBounds, maxlon), STRUCT_VAR_SIZE(xOsmBounds, maxlon), SrlMemberType_R64, "" }, \
} }

typedef struct xOsmTag xOsmTag;
struct xOsmTag
{
	Str8 key;
	Str8 value;
};
#define xOsmTag_SrlInfo { "tag", sizeof(xOsmTag), 2, { \
	{ SRL_REQ, "k", STRUCT_VAR_OFFSET(xOsmTag, key),     STRUCT_VAR_SIZE(xOsmTag, key),   SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "v", STRUCT_VAR_OFFSET(xOsmTag, value),   STRUCT_VAR_SIZE(xOsmTag, value), SrlMemberType_Str8, "" }, \
} }

typedef struct xOsmNode xOsmNode;
struct xOsmNode
{
	u64 id;
	bool visible;
	Str8 version;
	u64 changeset;
	Str8 timestamp;
	Str8 user;
	u64 uid;
	r64 lat;
	r64 lon;
	VarArray tags; //xOsmTag
};
#define xOsmNode_SrlInfo { "node", sizeof(xOsmNode), 10, { \
	{ SRL_REQ, "id",        STRUCT_VAR_OFFSET(xOsmNode, id),        STRUCT_VAR_SIZE(xOsmNode, id),        SrlMemberType_U64,  "" }, \
	{ SRL_REQ, "visible",   STRUCT_VAR_OFFSET(xOsmNode, visible),   STRUCT_VAR_SIZE(xOsmNode, visible),   SrlMemberType_Bool, "" }, \
	{ SRL_REQ, "version",   STRUCT_VAR_OFFSET(xOsmNode, version),   STRUCT_VAR_SIZE(xOsmNode, version),   SrlMemberType_Str8, "" }, \
	{ SRL_OPT, "changeset", STRUCT_VAR_OFFSET(xOsmNode, changeset), STRUCT_VAR_SIZE(xOsmNode, changeset), SrlMemberType_U64,  "" }, \
	{ SRL_OPT, "timestamp", STRUCT_VAR_OFFSET(xOsmNode, timestamp), STRUCT_VAR_SIZE(xOsmNode, timestamp), SrlMemberType_Str8, "" }, \
	{ SRL_OPT, "user",      STRUCT_VAR_OFFSET(xOsmNode, user),      STRUCT_VAR_SIZE(xOsmNode, user),      SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "uid",       STRUCT_VAR_OFFSET(xOsmNode, uid),       STRUCT_VAR_SIZE(xOsmNode, uid),       SrlMemberType_U64,  "" }, \
	{ SRL_REQ, "lat",       STRUCT_VAR_OFFSET(xOsmNode, lat),       STRUCT_VAR_SIZE(xOsmNode, lat),       SrlMemberType_R64,  "" }, \
	{ SRL_REQ, "lon",       STRUCT_VAR_OFFSET(xOsmNode, lon),       STRUCT_VAR_SIZE(xOsmNode, lon),       SrlMemberType_R64,  "" }, \
	{ SRL_OPT, "[CHILDREN]",STRUCT_VAR_OFFSET(xOsmNode, tags),      STRUCT_VAR_SIZE(xOsmNode, tags),      SrlMemberType_VarArray, "tag" }, \
} }

typedef struct xOsmRoot xOsmRoot;
struct xOsmRoot
{
	Str8 version;
	Str8 generator;
	Str8 copyright;
	Str8 attribution;
	Str8 license;
	xOsmBounds bounds;
	VarArray nodes; //xOsmNode
};
#define xOsmRoot_SrlInfo { "osm", sizeof(xOsmRoot), 7, { \
	{ SRL_OPT, "version",     STRUCT_VAR_OFFSET(xOsmRoot, version),     STRUCT_VAR_SIZE(xOsmRoot, version),     SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "generator",   STRUCT_VAR_OFFSET(xOsmRoot, generator),   STRUCT_VAR_SIZE(xOsmRoot, generator),   SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "copyright",   STRUCT_VAR_OFFSET(xOsmRoot, copyright),   STRUCT_VAR_SIZE(xOsmRoot, copyright),   SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "attribution", STRUCT_VAR_OFFSET(xOsmRoot, attribution), STRUCT_VAR_SIZE(xOsmRoot, attribution), SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "license",     STRUCT_VAR_OFFSET(xOsmRoot, license),     STRUCT_VAR_SIZE(xOsmRoot, license),     SrlMemberType_Str8, "" }, \
	{ SRL_REQ, "bounds",      STRUCT_VAR_OFFSET(xOsmRoot, bounds),      STRUCT_VAR_SIZE(xOsmRoot, bounds),      SrlMemberType_SrlType, "bounds" }, \
	{ SRL_REQ, "[CHILDREN]",  STRUCT_VAR_OFFSET(xOsmRoot, nodes),       STRUCT_VAR_SIZE(xOsmRoot, nodes),       SrlMemberType_VarArray, "node" }, \
} }

const SrlInfo xOsmSrlInfos[] = {
	xOsmBounds_SrlInfo,
	xOsmTag_SrlInfo,
	xOsmNode_SrlInfo,
	xOsmRoot_SrlInfo,
};

#endif //  _OSM_MAP_H
