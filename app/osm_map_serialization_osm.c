/*
File:   osm_map_serialization_osm.c
Author: Taylor Robbins
Date:   09\11\2025
Description: 
	** Holds TryParseOsmMap and SerializeOsmMap which handle the XML-based .osm file format
*/

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
	
	InitOsmMap(arena, mapOut, 0, 0, 0);
	mapOut->areNodesSorted = true;
	mapOut->areWaysSorted = true;
	mapOut->areRelationsSorted = false; //TODO: Change me!
	
	do
	{
		XmlElement* root = XmlGetOneChildOrBreak(&xml, nullptr, StrLit("osm"));
		Str8 mapVersionStr  = XmlGetAttributeOrDefault(&xml, root, StrLit("version"),     Str8_Empty);
		Str8 generatorStr   = XmlGetAttributeOrDefault(&xml, root, StrLit("generator"),   Str8_Empty);
		Str8 copyrightStr   = XmlGetAttributeOrDefault(&xml, root, StrLit("copyright"),   Str8_Empty);
		Str8 attributionStr = XmlGetAttributeOrDefault(&xml, root, StrLit("attribution"), Str8_Empty);
		Str8 licenseStr     = XmlGetAttributeOrDefault(&xml, root, StrLit("license"),     Str8_Empty);
		mapOut->versionStr     = (!IsEmptyStr(mapVersionStr)  ? AllocStr8(arena, mapVersionStr)  : Str8_Empty);
		mapOut->generatorStr   = (!IsEmptyStr(generatorStr)   ? AllocStr8(arena, generatorStr)   : Str8_Empty);
		mapOut->copyrightStr   = (!IsEmptyStr(copyrightStr)   ? AllocStr8(arena, copyrightStr)   : Str8_Empty);
		mapOut->attributionStr = (!IsEmptyStr(attributionStr) ? AllocStr8(arena, attributionStr) : Str8_Empty);
		mapOut->licenseStr     = (!IsEmptyStr(licenseStr)     ? AllocStr8(arena, licenseStr)     : Str8_Empty);
		
		XmlElement* bounds = XmlGetOneChildOrBreak(&xml, root, StrLit("bounds"));
		r64 boundsMinLon = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("minlon"));
		r64 boundsMinLat = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("minlat"));
		r64 boundsMaxLon = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("maxlon"));
		r64 boundsMaxLat = XmlGetAttributeR64OrBreak(&xml, bounds, StrLit("maxlat"));
		//TODO: This does not properly handle if the rectangle passes over the international date line (max longitude will be less than min longitude)
		mapOut->bounds = NewRecdBetween(boundsMinLon, boundsMinLat, boundsMaxLon, boundsMaxLat);
		
		u64 prevNodeId = 0;
		bool areNodesSorted = true;
		u64 prevWayId = 0;
		bool areWaysSorted = true;
		
		XmlElement* xmlNode = nullptr;
		while ((xmlNode = XmlGetNextChild(&xml, root, StrLit("node"), xmlNode)) != nullptr)
		{
			u64 id        = XmlGetAttributeU64OrBreak(&xml, xmlNode, StrLit("id"));
			r64 longitude = XmlGetAttributeR64OrBreak(&xml, xmlNode, StrLit("lon"));
			r64 latitude  = XmlGetAttributeR64OrBreak(&xml, xmlNode, StrLit("lat"));
			Str8 visibleStr   = XmlGetAttributeOrDefault(&xml, xmlNode, StrLit("visible"),   Str8_Empty);
			Str8 versionStr   = XmlGetAttributeOrDefault(&xml, xmlNode, StrLit("version"),   Str8_Empty);
			Str8 changesetStr = XmlGetAttributeOrDefault(&xml, xmlNode, StrLit("changeset"), Str8_Empty);
			Str8 timestampStr = XmlGetAttributeOrDefault(&xml, xmlNode, StrLit("timestamp"), Str8_Empty);
			Str8 userStr      = XmlGetAttributeOrDefault(&xml, xmlNode, StrLit("user"),      Str8_Empty);
			Str8 uidStr       = XmlGetAttributeOrDefault(&xml, xmlNode, StrLit("uid"),       Str8_Empty);
			bool visible = true;
			i32 version = 0;
			u64 changeset = 0;
			u64 uid = 0;
			if (!IsEmptyStr(visibleStr)   && !TryParseBool(visibleStr,  &visible,   nullptr)) { PrintLine_W("Failed to parse visible attribute as bool on node %llu: \"%.*s\"", id, StrPrint(visibleStr)); }
			if (!IsEmptyStr(versionStr)   && !TryParseI32(versionStr,   &version,   nullptr)) { PrintLine_W("Failed to parse version attribute as i32 on node %llu: \"%.*s\"", id, StrPrint(versionStr)); }
			if (!IsEmptyStr(changesetStr) && !TryParseU64(changesetStr, &changeset, nullptr)) { PrintLine_W("Failed to parse changeset attribute as u64 on node %llu: \"%.*s\"", id, StrPrint(changesetStr)); }
			if (!IsEmptyStr(uidStr)       && !TryParseU64(uidStr,       &uid,       nullptr)) { PrintLine_W("Failed to parse uid attribute as u64 on node %llu: \"%.*s\"", id, StrPrint(uidStr)); }
			
			if (id <= prevNodeId) { areNodesSorted = false; }
			prevNodeId = id;
			
			OsmNode* newNode = AddOsmNode(mapOut, NewV2d(longitude, latitude), id);
			NotNull(newNode);
			newNode->visible = visible;
			newNode->version = version;
			newNode->changeset = changeset;
			newNode->timestampStr = (!IsEmptyStr(timestampStr) ? AllocStr8(arena, timestampStr) : Str8_Empty);
			newNode->user = (!IsEmptyStr(userStr) ? AllocStr8(arena, userStr) : Str8_Empty);
			newNode->uid = uid;
			
			XmlElement* xmlTag = nullptr;
			while ((xmlTag = XmlGetNextChild(&xml, xmlNode, StrLit("tag"), xmlTag)) != nullptr)
			{
				Str8 keyStr = XmlGetAttributeOrBreak(&xml, xmlTag, StrLit("k"));
				Str8 valueStr = XmlGetAttributeOrBreak(&xml, xmlTag, StrLit("v"));
				OsmTag* newTag = VarArrayAdd(OsmTag, &newNode->tags);
				NotNull(newTag);
				ClearPointer(newTag);
				newTag->key = AllocStr8(arena, keyStr);
				newTag->value = AllocStr8(arena, valueStr);
			}
			if (xml.error != Result_None) { break; }
		}
		if (xml.error != Result_None) { break; }
		
		if (!mapOut->areNodesSorted || !areNodesSorted)
		{
			TracyCZoneN(Zone_SortNodes, "SortNodes", true);
			QuickSortVarArrayUintMember(OsmNode, id, &mapOut->nodes);
			mapOut->areNodesSorted = true;
			TracyCZoneEnd(Zone_SortNodes);
		}
		
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
			Str8 visibleStr   = XmlGetAttributeOrDefault(&xml, xmlWay, StrLit("visible"),   Str8_Empty);
			Str8 versionStr   = XmlGetAttributeOrDefault(&xml, xmlWay, StrLit("version"),   Str8_Empty);
			Str8 changesetStr = XmlGetAttributeOrDefault(&xml, xmlWay, StrLit("changeset"), Str8_Empty);
			Str8 timestampStr = XmlGetAttributeOrDefault(&xml, xmlWay, StrLit("timestamp"), Str8_Empty);
			Str8 userStr      = XmlGetAttributeOrDefault(&xml, xmlWay, StrLit("user"),      Str8_Empty);
			Str8 uidStr       = XmlGetAttributeOrDefault(&xml, xmlWay, StrLit("uid"),       Str8_Empty);
			bool visible = true;
			i32 version = 0;
			u64 changeset = 0;
			u64 uid = 0;
			if (!IsEmptyStr(visibleStr)   && !TryParseBool(visibleStr,  &visible,   nullptr)) { PrintLine_W("Failed to parse visible attribute as bool on node %llu: \"%.*s\"", id, StrPrint(visibleStr)); }
			if (!IsEmptyStr(versionStr)   && !TryParseI32(versionStr,   &version,   nullptr)) { PrintLine_W("Failed to parse version attribute as i32 on node %llu: \"%.*s\"", id, StrPrint(versionStr)); }
			if (!IsEmptyStr(changesetStr) && !TryParseU64(changesetStr, &changeset, nullptr)) { PrintLine_W("Failed to parse changeset attribute as u64 on node %llu: \"%.*s\"", id, StrPrint(changesetStr)); }
			if (!IsEmptyStr(uidStr)       && !TryParseU64(uidStr,       &uid,       nullptr)) { PrintLine_W("Failed to parse uid attribute as u64 on node %llu: \"%.*s\"", id, StrPrint(uidStr)); }
			
			if (id <= prevWayId) { areWaysSorted = false; }
			prevWayId = id;
			
			OsmWay* newWay = AddOsmWay(mapOut, id, numNodesInWay, nodeIds);
			NotNull(newWay);
			newWay->visible = visible;
			newWay->version = version;
			newWay->changeset = changeset;
			newWay->timestampStr = (!IsEmptyStr(timestampStr) ? AllocStr8(arena, timestampStr) : Str8_Empty);
			newWay->user = (!IsEmptyStr(userStr) ? AllocStr8(arena, userStr) : Str8_Empty);
			newWay->uid = uid;
			
			XmlElement* xmlTag = nullptr;
			while ((xmlTag = XmlGetNextChild(&xml, xmlWay, StrLit("tag"), xmlTag)) != nullptr)
			{
				Str8 keyStr = XmlGetAttributeOrBreak(&xml, xmlTag, StrLit("k"));
				Str8 valueStr = XmlGetAttributeOrBreak(&xml, xmlTag, StrLit("v"));
				OsmTag* newTag = VarArrayAdd(OsmTag, &newWay->tags);
				NotNull(newTag);
				ClearPointer(newTag);
				newTag->key = AllocStr8(arena, keyStr);
				newTag->value = AllocStr8(arena, valueStr);
			}
			if (xml.error != Result_None) { break; }
			
			ArenaResetToMark(scratch, mark);
		}
		if (xml.error != Result_None) { break; }
		
		if (!mapOut->areWaysSorted || !areWaysSorted)
		{
			TracyCZoneN(Zone_SortWays, "SortWays", true);
			QuickSortVarArrayUintMember(OsmWay, id, &mapOut->ways);
			mapOut->areWaysSorted = true;
			TracyCZoneEnd(Zone_SortWays);
		}
		
	} while(false);
	
	if (xml.error)
	{
		FreeOsmMap(mapOut);
	}
	
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
	return (xml.error == Result_None) ? Result_Success : xml.error;
}

Str8 SerializeOsmMap(Arena* arena, OsmMap* map)
{
	TwoPassStr8Loop(result, arena, false)
	{
		TwoPassStrNt(&result, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		TwoPassStrNt(&result, "<osm version=\"0.6\" generator=\"COSM 1.0\" copyright=\"OpenStreetMap and contributors\" attribution=\"http://www.openstreetmap.org/copyright\" license=\"http://opendatacommons.org/licenses/odbl/1-0/\">\n");
		
		v2d boundsMin = NewV2d(MinR64(map->bounds.Lon, map->bounds.Lon + map->bounds.SizeLon), MinR64(map->bounds.Lat, map->bounds.Lat + map->bounds.SizeLat));
		v2d boundsMax = NewV2d(MaxR64(map->bounds.Lon, map->bounds.Lon + map->bounds.SizeLon), MaxR64(map->bounds.Lat, map->bounds.Lat + map->bounds.SizeLat));
		TwoPassPrint(&result, "\t<bounds minlat=\"%lf\" minlat=\"%lf\" maxlat=\"%lf\" maxlat=\"%lf\"/>\n", boundsMin.Lat, boundsMin.Lon, boundsMax.Lat, boundsMax.Lon);
		
		VarArrayLoop(&map->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
			TwoPassPrint(&result, "\t<node id=\"%llu\" visible=\"%s\"", node->id, node->visible ? "true" : "false");
			if (node->version != 0) { TwoPassPrint(&result, " version=\"%d\"", node->version); }
			if (node->changeset != 0) { TwoPassPrint(&result, " changeset=\"%llu\"", node->changeset); }
			if (!IsEmptyStr(node->timestampStr)) { TwoPassPrint(&result, " timestamp=\"%.*s\"", StrPrint(node->timestampStr)); }
			if (!IsEmptyStr(node->user)) { TwoPassPrint(&result, " user=\"%.*s\"", StrPrint(node->user)); }
			if (node->uid != 0) { TwoPassPrint(&result, " uid=\"%llu\"", node->uid); }
			TwoPassPrint(&result, " lat=\"%lf\" lon=\"%lf\"", node->location.Lat, node->location.Lon);
			
			if (node->tags.length > 0)
			{
				TwoPassStrNt(&result, ">\n");
				VarArrayLoop(&node->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, tag, &node->tags, tIndex);
					TwoPassPrint(&result, "\t\t<tag k=\"%.*s\" v=\"%.*s\"/>\n", StrPrint(tag->key), StrPrint(tag->value));
				}
				TwoPassStrNt(&result, "\t</node>\n");
			}
			else { TwoPassStrNt(&result, "/>\n"); }
		}
		
		
		VarArrayLoop(&map->ways, wIndex)
		{
			VarArrayLoopGet(OsmWay, way, &map->ways, wIndex);
			TwoPassPrint(&result, "\t<way id=\"%llu\" visible=\"%s\"", way->id, way->visible ? "true" : "false");
			if (way->version != 0) { TwoPassPrint(&result, " version=\"%d\"", way->version); }
			if (way->changeset != 0) { TwoPassPrint(&result, " changeset=\"%llu\"", way->changeset); }
			if (!IsEmptyStr(way->timestampStr)) { TwoPassPrint(&result, " timestamp=\"%.*s\"", StrPrint(way->timestampStr)); }
			if (!IsEmptyStr(way->user)) { TwoPassPrint(&result, " user=\"%.*s\"", StrPrint(way->user)); }
			if (way->uid != 0) { TwoPassPrint(&result, " uid=\"%llu\"", way->uid); }
			
			if (way->nodes.length > 0 || way->tags.length > 0)
			{
				TwoPassStrNt(&result, ">\n");
				VarArrayLoop(&way->nodes, nIndex)
				{
					VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
					TwoPassPrint(&result, "\t\t<nd ref=\"%llu\"/>", nodeRef->id);
				}
				VarArrayLoop(&way->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, tag, &way->tags, tIndex);
					TwoPassPrint(&result, "\t\t<tag k=\"%.*s\" v=\"%.*s\"/>\n", StrPrint(tag->key), StrPrint(tag->value));
				}
				TwoPassStrNt(&result, "\t</way>\n");
			}
			else { TwoPassStrNt(&result, "/>\n"); }
		}
		
		VarArrayLoop(&map->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, relation, &map->relations, rIndex);
			TwoPassPrint(&result, "\t<relation id=\"%llu\" visible=\"%s\"", relation->id, relation->visible ? "true" : "false");
			if (relation->version != 0) { TwoPassPrint(&result, " version=\"%d\"", relation->version); }
			if (relation->changeset != 0) { TwoPassPrint(&result, " changeset=\"%llu\"", relation->changeset); }
			if (!IsEmptyStr(relation->timestampStr)) { TwoPassPrint(&result, " timestamp=\"%.*s\"", StrPrint(relation->timestampStr)); }
			if (!IsEmptyStr(relation->user)) { TwoPassPrint(&result, " user=\"%.*s\"", StrPrint(relation->user)); }
			if (relation->uid != 0) { TwoPassPrint(&result, " uid=\"%llu\"", relation->uid); }
			
			if (relation->members.length > 0 || relation->tags.length > 0)
			{
				TwoPassStrNt(&result, ">\n");
				VarArrayLoop(&relation->members, mIndex)
				{
					VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
					//TODO: Add role!
					TwoPassPrint(&result, "\t\t<member type=\"%s\" ref=\"%llu\"/>", GetOsmRelationMemberTypeXmlStr(member->type), member->id);
				}
				VarArrayLoop(&relation->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, tag, &relation->tags, tIndex);
					TwoPassPrint(&result, "\t\t<tag k=\"%.*s\" v=\"%.*s\"/>\n", StrPrint(tag->key), StrPrint(tag->value));
				}
				TwoPassStrNt(&result, "\t</relation>\n");
			}
			else { TwoPassStrNt(&result, "/>\n"); }
		}
		
		TwoPassStrNt(&result, "</osm>\n");
		TwoPassStr8LoopEnd(&result);
	}
	return result.str;
}
