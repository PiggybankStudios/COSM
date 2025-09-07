/*
File:   osm_map_serialization.c
Author: Taylor Robbins
Date:   09\06\2025
Description: 
	** None
*/

// +--------------------------------------------------------------+
// |                       .osm File Format                       |
// +--------------------------------------------------------------+
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
		
	} while(false);
	
	if (xml.error)
	{
		FreeOsmMap(mapOut);
	}
	
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
	return (xml.error == Result_None) ? Result_Success : xml.error;
}

// +--------------------------------------------------------------+
// |                       .pbf File Format                       |
// +--------------------------------------------------------------+
char GetPbfBlobTypeChar(u8 blobType)
{
	switch (blobType)
	{
		case 0: return 'V';
		case 1: return 'D';
		case 2: return 'S';
		case 5: return 'I';
		default: return '?';
	}
}

#define GetPbfString(stringTablePntr, stringId)                                            \
(                                                                                          \
	((stringId) > 0 && (size_t)(stringId) < (stringTablePntr)->n_s)                        \
	? NewStr8((stringTablePntr)->s[(stringId)].len, (stringTablePntr)->s[(stringId)].data) \
	: Str8_Empty                                                                           \
)

Result TryParsePbfMap(Arena* arena, DataStream* protobufStream, OsmMap* mapOut)
{
	TracyCZoneN(Zone_Func, "TryParsePbfMap", true);
	ScratchBegin1(scratch, arena);
	ProtobufCAllocator scratchAllocator = ProtobufAllocatorFromArena(scratch);
	Result result = Result_None;
	uxx blobIndex = 0;
	bool foundOsmHeader = false;
	bool foundOsmData = false;
	bool foundUnkownBlobTypes = false;
	
	while (!IsDataStreamFinished(protobufStream) && result == Result_None)
	{
		uxx scratchMark1 = ArenaGetMark(scratch);
		u32 headerLength = BinStreamReadU32(protobufStream, scratch,
			result = ((blobIndex == 0) ? Result_EmptyFile : result);
			break
		);
		FlipEndianU32(headerLength);
		if (headerLength == 0) { result = Result_ValueTooLow; break; }
		if (headerLength > Kilobytes(64)) { result = Result_ValueTooHigh; break; }
		u8* headerBytes = TryReadFromDataStream(protobufStream, headerLength, scratch);
		if (headerBytes == nullptr) { PrintLine_E("Failed to read %u byte header for blob[%llu]", headerLength, blobIndex); result = protobufStream->error; break; }
		// PrintLine_D("headerLength=%u", headerLength);
		
		TracyCZoneN(Zone_BlobHeader, "BlobHeader", true);
		OSMPBF__BlobHeader* blobHeader = osmpbf__blob_header__unpack(&scratchAllocator, headerLength, headerBytes);
		TracyCZoneEnd(Zone_BlobHeader);
		if (blobHeader == nullptr) { result = Result_ParsingFailure; break; }
		Str8 blobTypeStr = StrLit(blobHeader->type);
		// PrintLine_I("Parsed %u byte BlobHeader: %d byte \"%s\"", headerLength, blobHeader->datasize, blobHeader->type);
		// if (blobHeader->has_indexdata) { PrintLine_D("\tindexdata=%zu bytes %p", blobHeader->indexdata.len, blobHeader->indexdata.data); }
		if (blobHeader->datasize == 0) { result = Result_ValueTooLow; break; }
		if (blobHeader->datasize > Megabytes(32)) { result = Result_ValueTooHigh; break; }
		u8* blobBytes = TryReadFromDataStream(protobufStream, blobHeader->datasize, scratch);
		if (blobBytes == nullptr) { PrintLine_E("Failed to read %u byte blob[%llu]", blobHeader->datasize, blobIndex); result = protobufStream->error; break; }
		
		TracyCZoneN(Zone_Blob, "Blob", true);
		OSMPBF__Blob* blob = osmpbf__blob__unpack(&scratchAllocator, blobHeader->datasize, blobBytes);
		TracyCZoneEnd(Zone_Blob);
		if (blob == nullptr) { result = Result_ParsingFailure; break; }
		// PrintLine_I("\tParsed %d byte Blob!", blobHeader->datasize);
		// PrintLine_D("\t\traw_size=%d%s", blob->raw_size, blob->has_raw_size ? "" : " (Not Found)");
		const char* compressionTypeStr = "UNKNOWN";
		ProtobufCBinaryData* dataPntr = nullptr;
		switch (blob->data_case)
		{
			case OSMPBF__BLOB__DATA__NOT_SET:            compressionTypeStr = "_NOT_SET";                                              break;
			case OSMPBF__BLOB__DATA_RAW:                 compressionTypeStr = "RAW";            dataPntr = &blob->obsolete_bzip2_data; break;
			case OSMPBF__BLOB__DATA_ZLIB_DATA:           compressionTypeStr = "ZLIB";           dataPntr = &blob->lz4_data;            break;
			case OSMPBF__BLOB__DATA_LZMA_DATA:           compressionTypeStr = "LZMA";           dataPntr = &blob->lzma_data;           break;
			case OSMPBF__BLOB__DATA_OBSOLETE_BZIP2_DATA: compressionTypeStr = "OBSOLETE_BZIP2"; dataPntr = &blob->raw;                 break;
			case OSMPBF__BLOB__DATA_LZ4_DATA:            compressionTypeStr = "LZ4";            dataPntr = &blob->zlib_data;           break;
			case OSMPBF__BLOB__DATA_ZSTD_DATA:           compressionTypeStr = "ZSTD";           dataPntr = &blob->zstd_data;           break;
		}
		// PrintLineAt((dataPntr == nullptr ? DbgLevel_Error: DbgLevel_Info),
		// 	"Found Blob[%llu]: \"%s\" (%s %d->%d bytes)",
		// 	blobIndex,
		// 	blobHeader->type,
		// 	compressionTypeStr,
		// 	(dataPntr != nullptr) ? dataPntr->len : 0,
		// 	blob->raw_size
		// );
		
		Slice decompressedBuffer = Slice_Empty;
		if (blob->data_case == OSMPBF__BLOB__DATA_RAW)
		{
			decompressedBuffer = NewStr8((uxx)dataPntr->len, dataPntr->data);
		}
		else if (blob->data_case == OSMPBF__BLOB__DATA_ZLIB_DATA && blob->raw_size > 0)
		{
			TracyCZoneN(Zone_ZlibDecompress, "ZlibDecompress", true);
			decompressedBuffer = ZlibDecompressIntoArena(scratch, NewStr8((uxx)dataPntr->len, dataPntr->data), (uxx)blob->raw_size);
			TracyCZoneEnd(Zone_ZlibDecompress);
			if (decompressedBuffer.bytes == nullptr)
			{
				PrintLine_E("Failed to decompress blob[%llu] ZLIB %llu->%llu", blobIndex, (uxx)dataPntr->len, (uxx)blob->raw_size);
				result = Result_DecompressError;
				break;
			}
			// PrintLine_I("Decompressed ZLIB_DATA successfully: %llu bytes (%02X %02X %02X %02X)", decompressedBuffer.length, decompressedBuffer.bytes[0], decompressedBuffer.bytes[1], decompressedBuffer.bytes[2], decompressedBuffer.bytes[3]);
		}
		else if (blob->data_case != OSMPBF__BLOB__DATA__NOT_SET)
		{
			PrintLine_E("Unsupported compression %s on blob[%llu]", compressionTypeStr, blobIndex);
			result = Result_UnsupportedCompression;
			break;
		}
		
		// +==============================+
		// |        OSMHeader Blob        |
		// +==============================+
		if (StrExactEquals(blobTypeStr, StrLit("OSMHeader")))
		{
			if (foundOsmHeader) { PrintLine_E("Blob[%llu] was a second OSMHeader!", blobIndex); result = Result_Duplicate; break; }
			
			TracyCZoneN(Zone_OsmHeaderBlock, "OsmHeaderBlock", true);
			OSMPBF__HeaderBlock* headerBlock = osmpbf__header_block__unpack(&scratchAllocator, decompressedBuffer.length, decompressedBuffer.bytes);
			TracyCZoneEnd(Zone_OsmHeaderBlock);
			if (headerBlock == nullptr) { PrintLine_E("Failed to parse OSMPBF::HeaderBlock in blob[%llu]!", blobIndex); result = Result_ParsingFailure; break; }
			#if 0
			PrintLine_D("\tbbox: (%lf, %lf, %lf, %lf)",
				(r64)headerBlock->bbox->left * (r64)Nano(1),
				(r64)headerBlock->bbox->top * (r64)Nano(1),
				(r64)headerBlock->bbox->right * (r64)Nano(1),
				(r64)headerBlock->bbox->bottom * (r64)Nano(1)
			);
			PrintLine_D("\tn_required_features: %zu", headerBlock->n_required_features);
			for (size_t fIndex = 0; fIndex < headerBlock->n_required_features; fIndex++) { PrintLine_D("\t\trequired_feature[%zu]: \"%s\"", fIndex, headerBlock->required_features[fIndex]); }
			PrintLine_D("\tn_optional_features: %zu", headerBlock->n_optional_features);
			for (size_t fIndex = 0; fIndex < headerBlock->n_optional_features; fIndex++) { PrintLine_D("\t\toptional_feature[%zu]: \"%s\"", fIndex, headerBlock->optional_features[fIndex]); }
			PrintLine_D("\twritingprogram: \"%s\"", headerBlock->writingprogram); //ex. "osmconvert 0.8.10"
			PrintLine_D("\tsource: \"%s\"", headerBlock->source); //ex. "http://www.openstreetmap.org/api/0.6"
			if (headerBlock->has_osmosis_replication_timestamp) { PrintLine_D("\tosmosis_replication_timestamp: %lld", headerBlock->osmosis_replication_timestamp); }
			if (headerBlock->has_osmosis_replication_sequence_number) { PrintLine_D("\tosmosis_replication_sequence_number: %lld", headerBlock->osmosis_replication_sequence_number); }
			if (headerBlock->osmosis_replication_base_url != nullptr) { PrintLine_D("\tosmosis_replication_base_url: \"%s\"", headerBlock->osmosis_replication_base_url); }
			#endif
			
			foundOsmHeader = true;
			InitOsmMap(arena, mapOut, 0, 0);
			mapOut->bounds.X = (r64)headerBlock->bbox->left * (r64)Nano(1);
			mapOut->bounds.Y = (r64)headerBlock->bbox->top * (r64)Nano(1);
			mapOut->bounds.Width = ((r64)headerBlock->bbox->right * (r64)Nano(1)) - mapOut->bounds.X;
			mapOut->bounds.Height = ((r64)headerBlock->bbox->bottom * (r64)Nano(1)) - mapOut->bounds.Y;
			//TODO: Ensure that all the "required_features" are things we expect to handle
			//      "OsmSchema-V0.6", "DenseNodes", "Sort.Type_then_ID", "LocationsOnWays", "HistoricalInformation", etc.
		}
		// +==============================+
		// |         OSMData Blob         |
		// +==============================+
		else if (StrExactEquals(blobTypeStr, StrLit("OSMData")))
		{
			if (!foundOsmHeader) { PrintLine_E("Blob[%llu] was OSMData BEFORE we found OSMHeader blob!", blobIndex); result = Result_MissingHeader; break; }
			
			TracyCZoneN(Zone_OsmHeaderBlock, "OsmPrimitiveBlock", true);
			OSMPBF__PrimitiveBlock* primitiveBlock = osmpbf__primitive_block__unpack(&scratchAllocator, decompressedBuffer.length, decompressedBuffer.bytes);
			TracyCZoneEnd(Zone_OsmHeaderBlock);
			if (primitiveBlock == nullptr) { PrintLine_E("Failed to parse OSMPBF::PrimitiveBlock in blob[%llu]!", blobIndex); result = Result_ParsingFailure; break; }
			#if 0
			PrintLine_D("\tstrings: %zu", primitiveBlock->stringtable->n_s);
			// PrintLine_D("\tstrings at %p relative to %p or %d", primitiveBlock->stringtable->s, decompressedBuffer.bytes, (u8*)primitiveBlock->stringtable->s - decompressedBuffer.bytes);
			PrintLine_D("\tn_primitivegroup: %zu", primitiveBlock->n_primitivegroup);
			for (size_t gIndex = 0; gIndex < primitiveBlock->n_primitivegroup; gIndex++)
			{
				OSMPBF__PrimitiveGroup* primitiveGroup = primitiveBlock->primitivegroup[gIndex];
				PrintLine_D("\t\tgroup[%zu]: %zu nodes (%zu dense) - %zu ways - %zu relations - %zu changesets",
					gIndex,
					primitiveGroup->n_nodes,
					(primitiveGroup->dense != nullptr) ? MaxU64(MaxU64((u64)primitiveGroup->dense->n_id, (u64)primitiveGroup->dense->n_lat), (u64)primitiveGroup->dense->n_lon) : (size_t)0,
					primitiveGroup->n_ways,
					primitiveGroup->n_relations,
					primitiveGroup->n_changesets
				);
				if (primitiveGroup->dense != nullptr)
				{
					NotNull(primitiveGroup->dense->denseinfo);
					PrintLine_D("\t\t\tDenseNodes: %zu IDs, %zu lats, %zu lons, %zu kvps", primitiveGroup->dense->n_id, primitiveGroup->dense->n_lat, primitiveGroup->dense->n_lon, primitiveGroup->dense->n_keys_vals);
					PrintLine_D("\t\t\t Info: %zu versions, %zu timestamps, %zu changesets, %zu uids, %zu user_sids, %zu visibles", primitiveGroup->dense->denseinfo->n_version, primitiveGroup->dense->denseinfo->n_timestamp, primitiveGroup->dense->denseinfo->n_changeset, primitiveGroup->dense->denseinfo->n_uid, primitiveGroup->dense->denseinfo->n_user_sid, primitiveGroup->dense->denseinfo->n_visible);
				}
			}
			if (primitiveBlock->has_granularity) { PrintLine_D("\tgranularity: %d", primitiveBlock->granularity); }
			else { WriteLine_D("\tgranularity: default"); }
			if (primitiveBlock->has_lat_offset) { PrintLine_D("\tlat_offset: %lld", primitiveBlock->lat_offset); }
			else { WriteLine_D("\tlat_offset: default"); }
			if (primitiveBlock->has_lon_offset) { PrintLine_D("\tlon_offset: %lld", primitiveBlock->lon_offset); }
			else { WriteLine_D("\tlon_offset: default"); }
			if (primitiveBlock->has_date_granularity) { PrintLine_D("\tdate_granularity: %d", primitiveBlock->date_granularity); }
			else { WriteLine_D("\tdate_granularity: default"); }
			#endif
			
			r64 granularityMult = (r64)(primitiveBlock->has_granularity ? primitiveBlock->granularity : 1) * (r64)Billionth(100);
			v2d nodeOffset = NewV2d(
				(r64)(primitiveBlock->has_lon_offset ? primitiveBlock->lon_offset * granularityMult : 0),
				(r64)(primitiveBlock->has_lat_offset ? primitiveBlock->lat_offset * granularityMult : 0)
			);
			
			for (size_t gIndex = 0; gIndex < primitiveBlock->n_primitivegroup; gIndex++)
			{
				OSMPBF__PrimitiveGroup* primitiveGroup = primitiveBlock->primitivegroup[gIndex];
				// +==============================+
				// |          PBF Nodes           |
				// +==============================+
				TracyCZoneN(Zone_OsmNodes, "OsmNodes", true);
				for (size_t nIndex = 0; nIndex < primitiveGroup->n_nodes; nIndex++)
				{
					OSMPBF__Node* node = primitiveGroup->nodes[nIndex];
					UNUSED(node); //TODO: Implement me!
				}
				TracyCZoneEnd(Zone_OsmNodes);
				if (result != Result_None) { break; }
				
				// +==============================+
				// |       PBF Dense Nodes        |
				// +==============================+
				if (primitiveGroup->dense != nullptr)
				{
					TracyCZoneN(Zone_OsmDenseNodes, "OsmDenseNodes", true);
					OSMPBF__DenseNodes* denseNodes = primitiveGroup->dense;
					NotNull(denseNodes->denseinfo);
					if (denseNodes->n_id != denseNodes->n_lat) { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match Lat count %zu", blobIndex, denseNodes->n_id, denseNodes->n_lat); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (denseNodes->n_id != denseNodes->n_lon) { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match Lon count %zu", blobIndex, denseNodes->n_id, denseNodes->n_lon); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					bool haveVersions   = (denseNodes->denseinfo->n_version   != 0);
					bool haveTimestamps = (denseNodes->denseinfo->n_timestamp != 0);
					bool haveChangesets = (denseNodes->denseinfo->n_changeset != 0);
					bool haveUids       = (denseNodes->denseinfo->n_uid       != 0);
					bool haveUserSids   = (denseNodes->denseinfo->n_user_sid  != 0);
					bool haveVisibles   = (denseNodes->denseinfo->n_visible   != 0);
					if (haveVersions   && denseNodes->n_id != denseNodes->denseinfo->n_version)   { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match info->version count %zu",   blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_version);   result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveTimestamps && denseNodes->n_id != denseNodes->denseinfo->n_timestamp) { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match info->timestamp count %zu", blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_timestamp); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveChangesets && denseNodes->n_id != denseNodes->denseinfo->n_changeset) { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match info->changeset count %zu", blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_changeset); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveUids       && denseNodes->n_id != denseNodes->denseinfo->n_uid)       { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match info->uid count %zu",       blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_uid);       result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveUserSids   && denseNodes->n_id != denseNodes->denseinfo->n_user_sid)  { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match info->user_sid count %zu",  blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_user_sid);  result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveVisibles   && denseNodes->n_id != denseNodes->denseinfo->n_visible)   { PrintLine_E("DenseNode in blob[%llu] ID count %zu doesn't match info->visible count %zu",   blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_visible);   result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					
					VarArrayExpand(&mapOut->nodes, mapOut->nodes.length + (uxx)denseNodes->n_id);
					
					size_t currentKeyValIndex = 0;
					
					i64 prevNodeId = 0;
					i64 prevNodeLat = 0;
					i64 prevNodeLon = 0;
					i32 prevNodeVersion = 0;
					i64 prevNodeTimestamp = 0;
					i64 prevNodeChangeset = 0;
					i32 prevNodeUid = 0;
					i32 prevNodeUserSid = 0;
					for (size_t nIndex = 0; nIndex < denseNodes->n_id; nIndex++)
					{
						i64 nodeId = prevNodeId + denseNodes->id[nIndex];
						i64 nodeLat = prevNodeLat + denseNodes->lat[nIndex];
						i64 nodeLon = prevNodeLon + denseNodes->lon[nIndex];
						i32 nodeVersion   = (haveVersions   ? prevNodeVersion   + denseNodes->denseinfo->version[nIndex]   : -1);
						i64 nodeTimestamp = (haveTimestamps ? prevNodeTimestamp + denseNodes->denseinfo->timestamp[nIndex] : 0);
						i64 nodeChangeset = (haveChangesets ? prevNodeChangeset + denseNodes->denseinfo->changeset[nIndex] : 0);
						i32 nodeUid       = (haveUids       ? prevNodeUid       + denseNodes->denseinfo->uid[nIndex]       : 0);
						i32 nodeUserSid   = (haveUserSids   ? prevNodeUserSid   + denseNodes->denseinfo->user_sid[nIndex]  : 0);
						bool nodeVisible  = (haveVisibles   ? denseNodes->denseinfo->visible[nIndex]   : true);
						if (nodeId <= 0) { PrintLine_E("Invalid Node ID %lld in blob[%llu] group[%zu] denseNode[%zu]!", nodeId, blobIndex, gIndex, nIndex); result = Result_InvalidID; break; }
						if (nodeVersion < -1) { PrintLine_E("Invalid Node Version %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeVersion, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						if (nodeTimestamp < 0) { PrintLine_E("Invalid Node Timestamp %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeTimestamp, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						if (nodeChangeset < 0) { PrintLine_E("Invalid Node Changeset %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeChangeset, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						if (nodeUid < 0) { PrintLine_E("Invalid Node UID %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeUid, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						
						v2d nodeLocation = NewV2d(
							nodeOffset.X + ((r64)nodeLon * granularityMult),
							nodeOffset.Y + ((r64)nodeLat * granularityMult)
						);
						// if (nIndex < 10) { PrintLine_D("Node[%lld]: (%lld, %lld) -> (%lf, %lf) -> (%lf, %lf)", nodeId, nodeLat, nodeLon, (r64)nodeLon * granularityMult, (r64)nodeLat * granularityMult, nodeLocation.X, nodeLocation.Y); }
						
						OsmNode* newNode = AddOsmNode(mapOut, nodeLocation, (u64)nodeId);
						newNode->visible = nodeVisible;
						newNode->version = nodeVersion;
						newNode->changeset = (u64)nodeChangeset;
						// TODO: Str8 timestampStr;
						// TODO: Str8 user;
						newNode->uid = (u64)nodeUid;
						
						//Find node tags by walking keys_vals list 2 at a time until we find a 0 entry
						while (currentKeyValIndex <= denseNodes->n_keys_vals)
						{
							i32 keyStringId = denseNodes->keys_vals[currentKeyValIndex+0];
							if (keyStringId == 0 || currentKeyValIndex+1 >= denseNodes->n_keys_vals) { currentKeyValIndex++; break; } // 0 entry denotes following tags are for next node
							i32 valStringId = denseNodes->keys_vals[currentKeyValIndex+1];
							currentKeyValIndex += 2;
							Str8 keyStr = GetPbfString(primitiveBlock->stringtable, keyStringId);
							if (!IsEmptyStr(keyStr))
							{
								Str8 valueStr = GetPbfString(primitiveBlock->stringtable, valStringId);
								OsmTag* newTag = VarArrayAdd(OsmTag, &newNode->tags);
								NotNull(newTag);
								ClearPointer(newTag);
								newTag->key = AllocStr8(arena, keyStr);
								newTag->value = AllocStr8(arena, valueStr);
							}
						}
						
						prevNodeId = nodeId;
						prevNodeLat = nodeLat;
						prevNodeLon = nodeLon;
						prevNodeVersion = nodeVersion;
						prevNodeTimestamp = nodeTimestamp;
						prevNodeChangeset = nodeChangeset;
						prevNodeUid = nodeUid;
						prevNodeUserSid = nodeUserSid;
					}
					TracyCZoneEnd(Zone_OsmDenseNodes);
					if (result != Result_None) { break; }
					if (currentKeyValIndex < denseNodes->n_keys_vals) { PrintLine_W("There were %zu/%zu tags left over after parsing %zu denseNodes in blob[%llu]", denseNodes->n_keys_vals - currentKeyValIndex, denseNodes->n_keys_vals, denseNodes->n_id, blobIndex); }
				}
				
				// +==============================+
				// |           PBF Ways           |
				// +==============================+
				TracyCZoneN(Zone_OsmWays, "OsmWays", true);
				for (size_t wIndex = 0; wIndex < primitiveGroup->n_ways; wIndex++)
				{
					OSMPBF__Way* way = primitiveGroup->ways[wIndex];
					NotNull(way->info);
					if (way->id <= 0)                                         { PrintLine_E("Way[%zu] in blob[%llu] has invalid ID %lld",        wIndex, blobIndex, way->id); result = Result_InvalidID; break; }
					if (way->info->has_timestamp && way->info->timestamp < 0) { PrintLine_E("Way[%zu] in blob[%llu] has invalid timestamp %lld", wIndex, blobIndex, way->info->timestamp); result = Result_ValueTooLow; break; }
					if (way->info->has_uid       && way->info->uid       < 0) { PrintLine_E("Way[%zu] in blob[%llu] has invalid uid %d",         wIndex, blobIndex, way->info->uid); result = Result_ValueTooLow; break; }
					if (way->info->has_changeset && way->info->changeset < 0) { PrintLine_E("Way[%zu] in blob[%llu] has invalid changeset %lld", wIndex, blobIndex, way->info->changeset); result = Result_ValueTooLow; break; }
					if (way->n_keys != way->n_vals)                           { PrintLine_E("Way[%zu] in blob[%llu] key count %zu doesn't match value count %zu", wIndex, blobIndex, way->n_keys, way->n_vals); result = Result_Mismatch; break; }
					if (way->n_refs > 0)
					{
						uxx scratchMark2 = ArenaGetMark(scratch);
						uxx numNodesInWay = (uxx)way->n_refs;
						u64* nodeIds = AllocArray(u64, scratch, numNodesInWay);
						NotNull(nodeIds);
						u64 prevNodeId = 0;
						for (size_t rIndex = 0; rIndex < way->n_refs; rIndex++)
						{
							if (way->refs[rIndex] < -(i64)prevNodeId) { PrintLine_E("Node[%zu] in Way[%zu] in blob[%llu] has negative ID %llu%s%lld", rIndex, wIndex, blobIndex, prevNodeId, (way->refs[rIndex] >= 0) ? "+" : "", way->refs[rIndex]); result = Result_InvalidID; break; }
							if (way->refs[rIndex] > 0 && (u64)way->refs[rIndex] > UINT64_MAX - prevNodeId) { PrintLine_E("Node[%zu] in Way[%zu] in blob[%llu] has overflow ID %llu%s%lld", rIndex, wIndex, blobIndex, prevNodeId, (way->refs[rIndex] >= 0) ? "+" : "", way->refs[rIndex]); result = Result_InvalidID; break; }
							nodeIds[rIndex] = (u64)(prevNodeId + way->refs[rIndex]);
							prevNodeId = nodeIds[rIndex];
						}
						if (result != Result_None) { ArenaResetToMark(scratch, scratchMark2); break; }
						
						//TODO: Handle "LocationsOnWays" feature by looking at n_lat,n_lon and disregarding if the IDs map to nodes we loaded!
						OsmWay* newWay = AddOsmWay(mapOut, (u64)way->id, numNodesInWay, nodeIds);
						newWay->visible = (way->info->has_visible ? way->info->visible : true);
						newWay->version = (way->info->has_version ? way->info->version : -1);
						//TODO: Str8 timestampStr; newWay->timestamp = (way->info->has_timestamp ? (u64)way->info->timestamp : 0);
						newWay->uid = (way->info->has_uid ? (u64)way->info->uid : 0);
						//TODO: Str8 user; newWay->user = (way->info->has_user_sid ? LookupString(way->info->user_sid) : Str8_Empty);
						newWay->changeset = (way->info->has_changeset ? (u64)way->info->changeset : 0);
						VarArrayExpand(&newWay->tags, newWay->tags.length + (uxx)way->n_keys);
						for (size_t tIndex = 0; tIndex < way->n_keys; tIndex++)
						{
							Str8 keyStr = GetPbfString(primitiveBlock->stringtable, way->keys[tIndex]);
							if (!IsEmptyStr(keyStr))
							{
								Str8 valueStr = GetPbfString(primitiveBlock->stringtable, way->vals[tIndex]);
								OsmTag* newTag = VarArrayAdd(OsmTag, &newWay->tags);
								NotNull(newTag);
								ClearPointer(newTag);
								newTag->key = AllocStr8(arena, keyStr);
								newTag->value = AllocStr8(arena, valueStr);
							}
						}
						
						ArenaResetToMark(scratch, scratchMark2);
					}
					else { PrintLine_W("Way[%zu] in blob[%llu] had no node references!", wIndex, blobIndex); }
				}
				TracyCZoneEnd(Zone_OsmWays);
				if (result != Result_None) { break; }
				
				// +==============================+
				// |        PBF Relations         |
				// +==============================+
				TracyCZoneN(Zone_OsmRelations, "OsmRelations", true);
				for (size_t rIndex = 0; rIndex < primitiveGroup->n_relations; rIndex++)
				{
					OSMPBF__Relation* relation = primitiveGroup->relations[rIndex];
					UNUSED(relation); //TODO: Implement me!
				}
				TracyCZoneEnd(Zone_OsmRelations);
				if (result != Result_None) { break; }
				
				// +==============================+
				// |        PBF Changesets        |
				// +==============================+
				TracyCZoneN(Zone_OsmChangesets, "OsmChangesets", true);
				for (size_t cIndex = 0; cIndex < primitiveGroup->n_changesets; cIndex++)
				{
					OSMPBF__ChangeSet* changeset = primitiveGroup->changesets[cIndex];
					UNUSED(changeset); //TODO: Implement me!
				}
				TracyCZoneEnd(Zone_OsmChangesets);
				if (result != Result_None) { break; }
			}
			if (result != Result_None) { break; }
			
			foundOsmData = true;
		}
		else { PrintLine_W("Unhandled blob type \"%s\"", blobHeader->type); foundUnkownBlobTypes = true; }
		
		ArenaResetToMark(scratch, scratchMark1);
		blobIndex++;
	}
	
	if (result != Result_None) { PrintLine_E("Got through %llu blobs before encountering: %s", blobIndex, GetResultStr(result)); }
	if (!foundOsmData) { result = foundOsmHeader ? (foundUnkownBlobTypes ? Result_WrongInternalFormat : Result_MissingData) : Result_MissingHeader; }
	if (result == Result_None) { result = Result_Success; }
	else if (foundOsmHeader) { FreeOsmMap(mapOut); }
	ScratchEnd(scratch);
	TracyCZoneEnd(Zone_Func);
	return result;
}

Result TryParseMapFile(Arena* arena, FilePath filePath, OsmMap* mapOut)
{
	ScratchBegin1(scratch, arena);
	Result parseResult = Result_None;
	
	if (StrAnyCaseEndsWith(filePath, StrLit(".pbf")))
	{
		#if 1
		OsFile pbfFile = ZEROED;
		TracyCZoneN(_OsOpenFile, "OsOpenFile", true);
		bool openedSelectedFile = OsOpenFile(scratch, filePath, OsOpenFileMode_Read, false, &pbfFile);
		TracyCZoneEnd(_OpenBinFile);
		if (openedSelectedFile)
		{
			PrintLine_I("Opened binary \"%.*s\"", StrPrint(filePath));
			DataStream fileStream = ToDataStreamFromFile(&pbfFile);
			parseResult = TryParsePbfMap(stdHeap, &fileStream, mapOut);
			OsCloseFile(&pbfFile);
			if (parseResult != Result_Success) { PrintLine_E("Failed to parse as OpenStreetMaps Protobuf data! Error: %s", GetResultStr(parseResult)); }
		}
		else { PrintLine_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
		#else
		Slice fileContents = Slice_Empty;
		TracyCZoneN(_ReadBinFile, "OsReadBinFile", true);
		bool openedSelectedFile = OsReadBinFile(filePath, scratch, &fileContents);
		TracyCZoneEnd(_ReadBinFile);
		if (openedSelectedFile)
		{
			PrintLine_I("Opened binary \"%.*s\", %llu bytes", StrPrint(filePath), fileContents.length);
			DataStream fileStream = ToDataStreamFromBuffer(fileContents);
			parseResult = TryParsePbfMap(stdHeap, &fileStream, mapOut);
			if (parseResult != Result_Success) { PrintLine_E("Failed to parse as OpenStreetMaps Protobuf data! Error: %s", GetResultStr(parseResult)); }
		}
		else { PrintLine_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
		#endif
	}
	else if (StrAnyCaseEndsWith(filePath, StrLit(".osm")))
	{
		Str8 fileContents = Str8_Empty;
		TracyCZoneN(_ReadTextFile, "OsReadTextFile", true);
		bool openedSelectedFile = OsReadTextFile(filePath, scratch, &fileContents);
		TracyCZoneEnd(_ReadTextFile);
		if (openedSelectedFile)
		{
			PrintLine_I("Opened text \"%.*s\", %llu bytes", StrPrint(filePath), fileContents.length);
			parseResult = TryParseOsmMap(stdHeap, fileContents, mapOut);
			if (parseResult != Result_Success) { PrintLine_E("Failed to parse as OpenStreetMaps XML data! Error: %s", GetResultStr(parseResult)); }
		}
		else { PrintLine_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
	}
	else
	{
		PrintLine_E("Unknown file extension \"%.*s\", expected .osm or .pbf files!", StrPrint(GetFileNamePart(filePath, true)));
		parseResult = Result_UnsupportedFileFormat;
	}
	
	ScratchEnd(scratch);
	return parseResult;
}

