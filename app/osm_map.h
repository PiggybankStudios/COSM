/*
File:   osm_map.h
Author: Taylor Robbins
Date:   07\06\2025
*/

#ifndef _OSM_MAP_H
#define _OSM_MAP_H

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
	u8 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	v2d location;
	VarArray tags; //OsmTag
};

typedef plex OsmNodeRef OsmNodeRef;
plex OsmNodeRef
{
	u64 id;
	OsmNode* pntr;
};

// <way id="428033729" visible="true" version="5" changeset="81153531" timestamp="2020-02-18T06:41:24Z" user="thadekam" uid="10455646">
typedef plex OsmWay OsmWay;
plex OsmWay
{
	u64 id;
	bool visible;
	u8 version;
	u64 changeset;
	Str8 timestampStr;
	Str8 user;
	u64 uid;
	
	VarArray nodes; //OsmNodeRef
	VarArray tags; //OsmTag
	recd nodeBounds;
	
	bool isClosedLoop;
	uxx numTriIndices;
	uxx* triIndices;
	VertBuffer triVertBuffer; //these vertices are normalized within bounds
};

typedef plex OsmMap OsmMap;
plex OsmMap
{
	Arena* arena;
	recd bounds;
	
	u64 nextNodeId;
	VarArray nodes; //OsmNode
	u64 nextWayId;
	VarArray ways; //OsmWay
};

#endif //  _OSM_MAP_H
