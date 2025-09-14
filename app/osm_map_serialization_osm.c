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
		
		// +==============================+
		// |         Parse Nodes          |
		// +==============================+
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
		
		// +==============================+
		// |          Sort Nodes          |
		// +==============================+
		if (!mapOut->areNodesSorted || !areNodesSorted)
		{
			TracyCZoneN(Zone_SortNodes, "SortNodes", true);
			QuickSortVarArrayUintMember(OsmNode, id, &mapOut->nodes);
			mapOut->areNodesSorted = true;
			TracyCZoneEnd(Zone_SortNodes);
		}
		
		// +==============================+
		// |          Parse Ways          |
		// +==============================+
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
			if (!IsEmptyStr(visibleStr)   && !TryParseBool(visibleStr,  &visible,   nullptr)) { PrintLine_W("Failed to parse visible attribute as bool on way %llu: \"%.*s\"", id, StrPrint(visibleStr)); }
			if (!IsEmptyStr(versionStr)   && !TryParseI32(versionStr,   &version,   nullptr)) { PrintLine_W("Failed to parse version attribute as i32 on way %llu: \"%.*s\"", id, StrPrint(versionStr)); }
			if (!IsEmptyStr(changesetStr) && !TryParseU64(changesetStr, &changeset, nullptr)) { PrintLine_W("Failed to parse changeset attribute as u64 on way %llu: \"%.*s\"", id, StrPrint(changesetStr)); }
			if (!IsEmptyStr(uidStr)       && !TryParseU64(uidStr,       &uid,       nullptr)) { PrintLine_W("Failed to parse uid attribute as u64 on way %llu: \"%.*s\"", id, StrPrint(uidStr)); }
			
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
		
		// +==============================+
		// |          Sort Ways           |
		// +==============================+
		if (!mapOut->areWaysSorted || !areWaysSorted)
		{
			TracyCZoneN(Zone_SortWays, "SortWays", true);
			QuickSortVarArrayUintMember(OsmWay, id, &mapOut->ways);
			mapOut->areWaysSorted = true;
			TracyCZoneEnd(Zone_SortWays);
		}
		
		// +==============================+
		// |       Parse Relations        |
		// +==============================+
		XmlElement* xmlRelation = nullptr;
		while ((xmlRelation = XmlGetNextChild(&xml, root, StrLit("relation"), xmlRelation)) != nullptr)
		{
			u64 id = XmlGetAttributeU64OrBreak(&xml, xmlRelation, StrLit("id"));
			Str8 visibleStr   = XmlGetAttributeOrDefault(&xml, xmlRelation, StrLit("visible"),   Str8_Empty);
			Str8 versionStr   = XmlGetAttributeOrDefault(&xml, xmlRelation, StrLit("version"),   Str8_Empty);
			Str8 changesetStr = XmlGetAttributeOrDefault(&xml, xmlRelation, StrLit("changeset"), Str8_Empty);
			Str8 timestampStr = XmlGetAttributeOrDefault(&xml, xmlRelation, StrLit("timestamp"), Str8_Empty);
			Str8 userStr      = XmlGetAttributeOrDefault(&xml, xmlRelation, StrLit("user"),      Str8_Empty);
			Str8 uidStr       = XmlGetAttributeOrDefault(&xml, xmlRelation, StrLit("uid"),       Str8_Empty);
			bool visible = true;
			i32 version = 0;
			u64 changeset = 0;
			u64 uid = 0;
			if (!IsEmptyStr(visibleStr)   && !TryParseBool(visibleStr,  &visible,   nullptr)) { PrintLine_W("Failed to parse visible attribute as bool on relation %llu: \"%.*s\"", id, StrPrint(visibleStr)); }
			if (!IsEmptyStr(versionStr)   && !TryParseI32(versionStr,   &version,   nullptr)) { PrintLine_W("Failed to parse version attribute as i32 on relation %llu: \"%.*s\"", id, StrPrint(versionStr)); }
			if (!IsEmptyStr(changesetStr) && !TryParseU64(changesetStr, &changeset, nullptr)) { PrintLine_W("Failed to parse changeset attribute as u64 on relation %llu: \"%.*s\"", id, StrPrint(changesetStr)); }
			if (!IsEmptyStr(uidStr)       && !TryParseU64(uidStr,       &uid,       nullptr)) { PrintLine_W("Failed to parse uid attribute as u64 on relation %llu: \"%.*s\"", id, StrPrint(uidStr)); }
			XmlElement* xmlRelationBounds = XmlGetChild(&xml, xmlRelation, StrLit("bounds"), 0);
			
			u64 numMembersInRelation = 0;
			VarArrayLoop(&xmlRelation->children, cIndex)
			{
				VarArrayLoopGet(XmlElement, xmlChild, &xmlRelation->children, cIndex);
				if (StrExactEquals(xmlChild->type, StrLit("member"))) { numMembersInRelation++; }
			}
			
			OsmRelation* newRelation = AddOsmRelation(mapOut, id, numMembersInRelation);
			if (xmlRelationBounds != nullptr)
			{
				r64 relationBoundsMinLon = XmlGetAttributeR64OrBreak(&xml, xmlRelationBounds, StrLit("minlon"));
				r64 relationBoundsMinLat = XmlGetAttributeR64OrBreak(&xml, xmlRelationBounds, StrLit("minlat"));
				r64 relationBoundsMaxLon = XmlGetAttributeR64OrBreak(&xml, xmlRelationBounds, StrLit("maxlon"));
				r64 relationBoundsMaxLat = XmlGetAttributeR64OrBreak(&xml, xmlRelationBounds, StrLit("maxlat"));
				newRelation->bounds = NewRecdBetween(relationBoundsMinLon, relationBoundsMinLat, relationBoundsMaxLon, relationBoundsMaxLat);
			}
			newRelation->visible = visible;
			newRelation->version = version;
			newRelation->changeset = changeset;
			newRelation->timestampStr = (!IsEmptyStr(timestampStr) ? AllocStr8(arena, timestampStr) : Str8_Empty);
			newRelation->user = (!IsEmptyStr(userStr) ? AllocStr8(arena, userStr) : Str8_Empty);
			newRelation->uid = uid;
			
			XmlElement* xmlMember = nullptr;
			while ((xmlMember = XmlGetNextChild(&xml, xmlRelation, StrLit("member"), xmlMember)) != nullptr)
			{
				Str8 typeStr = XmlGetAttributeOrBreak(&xml, xmlMember, StrLit("type"));
				u64 refId = XmlGetAttributeU64OrBreak(&xml, xmlMember, StrLit("ref"));
				Str8 roleStr = XmlGetAttributeOrBreak(&xml, xmlMember, StrLit("role"));
				r64 latitude = XmlGetAttributeR64OrDefault(&xml, xmlMember, StrLit("lat"), INFINITY);
				r64 longitude = XmlGetAttributeR64OrDefault(&xml, xmlMember, StrLit("lon"), INFINITY);
				
				OsmRelationMemberType type = OsmRelationMemberType_None;
				for (uxx eIndex = 1; eIndex < OsmRelationMemberType_Count; eIndex++)
				{
					const char* enumValueStrNt = GetOsmRelationMemberTypeXmlStr((OsmRelationMemberType)eIndex);
					if (StrAnyCaseEquals(typeStr, StrLit(enumValueStrNt)))
					{
						type = (OsmRelationMemberType)eIndex;
						break;
					}
				}
				if (type == OsmRelationMemberType_None) { xml.error = Result_InvalidType; xml.errorStr = typeStr; xml.errorElement = xmlRelation; break; }
				OsmRelationMemberRole role = OsmRelationMemberRole_None;
				if (!IsEmptyStr(roleStr))
				{
					for (uxx eIndex = 1; eIndex < OsmRelationMemberRole_Count; eIndex++)
					{
						const char* enumValueStrNt = GetOsmRelationMemberRoleXmlStr((OsmRelationMemberRole)eIndex);
						if (StrAnyCaseEquals(roleStr, StrLit(enumValueStrNt)))
						{
							role = (OsmRelationMemberRole)eIndex;
							break;
						}
					}
					if (role == OsmRelationMemberRole_None) { PrintLine_W("Warning: Unknown role type \"%.*s\" on member[%llu] in relation %llu", StrPrint(roleStr), newRelation->members.length, newRelation->id); }
				}
				
				OsmRelationMember* newMember = VarArrayAdd(OsmRelationMember, &newRelation->members);
				NotNull(newMember);
				ClearPointer(newMember);
				newMember->id = refId;
				newMember->type = type;
				newMember->role = role;
				
				if (newMember->type == OsmRelationMemberType_Node && !IsInfiniteOrNanR64(latitude) && !IsInfiniteOrNanR64(longitude))
				{
					InitVarArrayWithInitial(v2d, &newMember->locations, arena, 1);
					VarArrayAddValue(v2d, &newMember->locations, NewV2d(longitude, latitude));
				}
				else if (newMember->type == OsmRelationMemberType_Way)
				{
					uxx numNodeLocations = 0;
					VarArrayLoop(&xmlMember->children, cIndex)
					{
						VarArrayLoopGet(XmlElement, xmlChild, &xmlMember->children, cIndex);
						if (StrExactEquals(xmlChild->type, StrLit("nd"))) { numNodeLocations++; }
					}
					
					InitVarArrayWithInitial(v2d, &newMember->locations, arena, numNodeLocations);
					XmlElement* xmlNodeLocation = nullptr;
					while ((xmlNodeLocation = XmlGetNextChild(&xml, xmlMember, StrLit("nd"), xmlNodeLocation)) != nullptr)
					{
						r64 nodeLatitude = XmlGetAttributeR64OrBreak(&xml, xmlNodeLocation, StrLit("lat"));
						r64 nodeLongitude = XmlGetAttributeR64OrBreak(&xml, xmlNodeLocation, StrLit("lon"));
						VarArrayAddValue(v2d, &newMember->locations, NewV2d(nodeLongitude, nodeLatitude));
					}
					if (xml.error != Result_None) { break; }
				}
			}
			if (xml.error != Result_None) { break; }
			
			XmlElement* xmlTag = nullptr;
			while ((xmlTag = XmlGetNextChild(&xml, xmlRelation, StrLit("tag"), xmlTag)) != nullptr)
			{
				Str8 keyStr = XmlGetAttributeOrBreak(&xml, xmlTag, StrLit("k"));
				Str8 valueStr = XmlGetAttributeOrBreak(&xml, xmlTag, StrLit("v"));
				OsmTag* newTag = VarArrayAdd(OsmTag, &newRelation->tags);
				NotNull(newTag);
				ClearPointer(newTag);
				newTag->key = AllocStr8(arena, keyStr);
				newTag->value = AllocStr8(arena, valueStr);
			}
			if (xml.error != Result_None) { break; }
		}
		if (xml.error != Result_None) { break; }
	} while(false);
	
	if (xml.error == Result_None)
	{
		VarArrayLoop(&mapOut->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, relation, &mapOut->relations, rIndex);
			UpdateOsmRelationPntrs(mapOut, relation);
		}
		UpdateOsmNodeWayBackPntrs(mapOut);
		UpdateOsmRelationBackPntrs(mapOut);
	}
	else
	{
		NotNull(xml.errorElement);
		PrintLine_E("XML Parsing Error: %s \"%.*s\" on \"%.*s\" element", GetResultStr(xml.error), StrPrint(xml.errorStr), StrPrint(xml.errorElement->type));
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
		ScratchBegin1(scratch, arena);
		TwoPassStrNt(&result, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		TwoPassStrNt(&result, "<osm version=\"0.6\" generator=\"COSM 1.0\" copyright=\"OpenStreetMap and contributors\" attribution=\"http://www.openstreetmap.org/copyright\" license=\"http://opendatacommons.org/licenses/odbl/1-0/\">\n");
		
		v2d boundsMin = NewV2d(MinR64(map->bounds.Lon, map->bounds.Lon + map->bounds.SizeLon), MinR64(map->bounds.Lat, map->bounds.Lat + map->bounds.SizeLat));
		v2d boundsMax = NewV2d(MaxR64(map->bounds.Lon, map->bounds.Lon + map->bounds.SizeLon), MaxR64(map->bounds.Lat, map->bounds.Lat + map->bounds.SizeLat));
		TwoPassPrint(&result, "\t<bounds minlat=\"%lf\" minlon=\"%lf\" maxlat=\"%lf\" maxlon=\"%lf\"/>\n", boundsMin.Lat, boundsMin.Lon, boundsMax.Lat, boundsMax.Lon);
		
		VarArrayLoop(&map->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
			uxx scratchMark = ArenaGetMark(scratch);
			TwoPassPrint(&result, "\t<node id=\"%llu\" visible=\"%s\"", node->id, node->visible ? "true" : "false");
			if (node->version != 0) { TwoPassPrint(&result, " version=\"%d\"", node->version); }
			if (node->changeset != 0) { TwoPassPrint(&result, " changeset=\"%llu\"", node->changeset); }
			if (!IsEmptyStr(node->timestampStr))
			{
				Str8 escapedTimestampStr = EscapeXmlString(scratch, node->timestampStr, false);
				TwoPassPrint(&result, " timestamp=\"%.*s\"", StrPrint(escapedTimestampStr));
			}
			if (!IsEmptyStr(node->user))
			{
				Str8 escapedUser = EscapeXmlString(scratch, node->user, false);
				TwoPassPrint(&result, " user=\"%.*s\"", StrPrint(escapedUser));
			}
			if (node->uid != 0) { TwoPassPrint(&result, " uid=\"%llu\"", node->uid); }
			TwoPassPrint(&result, " lat=\"%lf\" lon=\"%lf\"", node->location.Lat, node->location.Lon);
			
			if (node->tags.length > 0)
			{
				TwoPassStrNt(&result, ">\n");
				VarArrayLoop(&node->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, tag, &node->tags, tIndex);
					Str8 escapedKey = EscapeXmlString(scratch, tag->key, false);
					Str8 escapedValue = EscapeXmlString(scratch, tag->value, false);
					TwoPassPrint(&result, "\t\t<tag k=\"%.*s\" v=\"%.*s\"/>\n", StrPrint(escapedKey), StrPrint(escapedValue));
				}
				TwoPassStrNt(&result, "\t</node>\n");
			}
			else { TwoPassStrNt(&result, "/>\n"); }
			ArenaResetToMark(scratch, scratchMark);
		}
		
		
		VarArrayLoop(&map->ways, wIndex)
		{
			VarArrayLoopGet(OsmWay, way, &map->ways, wIndex);
			uxx scratchMark = ArenaGetMark(scratch);
			TwoPassPrint(&result, "\t<way id=\"%llu\" visible=\"%s\"", way->id, way->visible ? "true" : "false");
			if (way->version != 0) { TwoPassPrint(&result, " version=\"%d\"", way->version); }
			if (way->changeset != 0) { TwoPassPrint(&result, " changeset=\"%llu\"", way->changeset); }
			if (!IsEmptyStr(way->timestampStr))
			{
				Str8 escapedTimestampStr = EscapeXmlString(scratch, way->timestampStr, false);
				TwoPassPrint(&result, " timestamp=\"%.*s\"", StrPrint(escapedTimestampStr));
			}
			if (!IsEmptyStr(way->user))
			{
				Str8 escapedUser = EscapeXmlString(scratch, way->user, false);
				TwoPassPrint(&result, " user=\"%.*s\"", StrPrint(escapedUser));
			}
			if (way->uid != 0) { TwoPassPrint(&result, " uid=\"%llu\"", way->uid); }
			
			if (way->nodes.length > 0 || way->tags.length > 0)
			{
				TwoPassStrNt(&result, ">\n");
				VarArrayLoop(&way->nodes, nIndex)
				{
					VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
					TwoPassPrint(&result, "\t\t<nd ref=\"%llu\"/>\n", nodeRef->id);
				}
				VarArrayLoop(&way->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, tag, &way->tags, tIndex);
					Str8 escapedKey = EscapeXmlString(scratch, tag->key, false);
					Str8 escapedValue = EscapeXmlString(scratch, tag->value, false);
					TwoPassPrint(&result, "\t\t<tag k=\"%.*s\" v=\"%.*s\"/>\n", StrPrint(escapedKey), StrPrint(escapedValue));
				}
				TwoPassStrNt(&result, "\t</way>\n");
			}
			else { TwoPassStrNt(&result, "/>\n"); }
			ArenaResetToMark(scratch, scratchMark);
		}
		
		VarArrayLoop(&map->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, relation, &map->relations, rIndex);
			uxx scratchMark = ArenaGetMark(scratch);
			TwoPassPrint(&result, "\t<relation id=\"%llu\" visible=\"%s\"", relation->id, relation->visible ? "true" : "false");
			if (relation->version != 0) { TwoPassPrint(&result, " version=\"%d\"", relation->version); }
			if (relation->changeset != 0) { TwoPassPrint(&result, " changeset=\"%llu\"", relation->changeset); }
			if (!IsEmptyStr(relation->timestampStr))
			{
				Str8 escapedTimestampStr = EscapeXmlString(scratch, relation->timestampStr, false);
				TwoPassPrint(&result, " timestamp=\"%.*s\"", StrPrint(escapedTimestampStr));
			}
			if (!IsEmptyStr(relation->user))
			{
				Str8 escapedUser = EscapeXmlString(scratch, relation->user, false);
				TwoPassPrint(&result, " user=\"%.*s\"", StrPrint(escapedUser));
			}
			if (relation->uid != 0) { TwoPassPrint(&result, " uid=\"%llu\"", relation->uid); }
			
			if (relation->members.length > 0 || relation->tags.length > 0)
			{
				TwoPassStrNt(&result, ">\n");
				VarArrayLoop(&relation->members, mIndex)
				{
					VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
					TwoPassPrint(&result, "\t\t<member type=\"%s\" ref=\"%llu\" role=\"%s\"/>\n", GetOsmRelationMemberTypeXmlStr(member->type), member->id, GetOsmRelationMemberRoleXmlStr(member->role));
					//TODO: Check if location information is present, either attach as attributes for single location (for node) or as children (for ways)
				}
				VarArrayLoop(&relation->tags, tIndex)
				{
					VarArrayLoopGet(OsmTag, tag, &relation->tags, tIndex);
					Str8 escapedKey = EscapeXmlString(scratch, tag->key, false);
					Str8 escapedValue = EscapeXmlString(scratch, tag->value, false);
					TwoPassPrint(&result, "\t\t<tag k=\"%.*s\" v=\"%.*s\"/>\n", StrPrint(escapedKey), StrPrint(escapedValue));
				}
				TwoPassStrNt(&result, "\t</relation>\n");
			}
			else { TwoPassStrNt(&result, "/>\n"); }
			ArenaResetToMark(scratch, scratchMark);
		}
		
		TwoPassStrNt(&result, "</osm>\n");
		ScratchEnd(scratch);
		TwoPassStr8LoopEnd(&result);
	}
	return result.str;
}
