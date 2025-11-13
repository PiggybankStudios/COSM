/*
File:   osm_map_serialization_pbf.c
Author: Taylor Robbins
Date:   09\11\2025
Description: 
	** Holds TryParsePbfMap and (eventually) SerializePbfMap which handle the Protobuf-based .pbf file format
*/

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

#define GetPbfString(stringTablePntr, stringId)                                                    \
(                                                                                                  \
	((stringId) > 0 && (size_t)(stringId) < (stringTablePntr)->n_s)                                \
	? MakeStr8((stringTablePntr)->s[(stringId)].len, (char*)(stringTablePntr)->s[(stringId)].data) \
	: Str8_Empty                                                                                   \
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
		if (headerBytes == nullptr) { NotifyPrint_E("Failed to read %u byte header for blob[%llu]", headerLength, blobIndex); result = protobufStream->error; break; }
		// PrintLine_D("headerLength=%u", headerLength);
		
		TracyCZoneN(Zone_BlobHeader, "BlobHeader", true);
		OSMPBF__BlobHeader* blobHeader = osmpbf__blob_header__unpack(&scratchAllocator, headerLength, headerBytes);
		TracyCZoneEnd(Zone_BlobHeader);
		if (blobHeader == nullptr) { result = Result_ParsingFailure; break; }
		Str8 blobTypeStr = MakeStr8Nt(blobHeader->type);
		// PrintLine_I("Parsed %u byte BlobHeader: %d byte \"%s\"", headerLength, blobHeader->datasize, blobHeader->type);
		// if (blobHeader->has_indexdata) { PrintLine_D("\tindexdata=%zu bytes %p", blobHeader->indexdata.len, blobHeader->indexdata.data); }
		if (blobHeader->datasize == 0) { result = Result_ValueTooLow; break; }
		if (blobHeader->datasize > Megabytes(32)) { result = Result_ValueTooHigh; break; }
		u8* blobBytes = TryReadFromDataStream(protobufStream, blobHeader->datasize, scratch);
		if (blobBytes == nullptr) { NotifyPrint_E("Failed to read %u byte blob[%llu]", blobHeader->datasize, blobIndex); result = protobufStream->error; break; }
		
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
			decompressedBuffer = MakeSlice((uxx)dataPntr->len, dataPntr->data);
		}
		else if (blob->data_case == OSMPBF__BLOB__DATA_ZLIB_DATA && blob->raw_size > 0)
		{
			TracyCZoneN(Zone_ZlibDecompress, "ZlibDecompress", true);
			decompressedBuffer = ZlibDecompressIntoArena(scratch, MakeSlice((uxx)dataPntr->len, dataPntr->data), (uxx)blob->raw_size);
			TracyCZoneEnd(Zone_ZlibDecompress);
			if (decompressedBuffer.bytes == nullptr)
			{
				NotifyPrint_E("Failed to decompress blob[%llu] ZLIB %llu->%llu", blobIndex, (uxx)dataPntr->len, (uxx)blob->raw_size);
				result = Result_DecompressError;
				break;
			}
			// PrintLine_I("Decompressed ZLIB_DATA successfully: %llu bytes (%02X %02X %02X %02X)", decompressedBuffer.length, decompressedBuffer.bytes[0], decompressedBuffer.bytes[1], decompressedBuffer.bytes[2], decompressedBuffer.bytes[3]);
		}
		else if (blob->data_case != OSMPBF__BLOB__DATA__NOT_SET)
		{
			NotifyPrint_E("Unsupported compression %s on blob[%llu]", compressionTypeStr, blobIndex);
			result = Result_UnsupportedCompression;
			break;
		}
		
		// +==============================+
		// |        OSMHeader Blob        |
		// +==============================+
		if (StrExactEquals(blobTypeStr, StrLit("OSMHeader")))
		{
			if (foundOsmHeader) { NotifyPrint_E("Blob[%llu] was a second OSMHeader!", blobIndex); result = Result_Duplicate; break; }
			
			TracyCZoneN(Zone_OsmHeaderBlock, "OsmHeaderBlock", true);
			OSMPBF__HeaderBlock* headerBlock = osmpbf__header_block__unpack(&scratchAllocator, decompressedBuffer.length, decompressedBuffer.bytes);
			TracyCZoneEnd(Zone_OsmHeaderBlock);
			if (headerBlock == nullptr) { NotifyPrint_E("Failed to parse OSMPBF::HeaderBlock in blob[%llu]!", blobIndex); result = Result_ParsingFailure; break; }
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
			InitOsmMap(arena, mapOut, 0, 0, 0);
			mapOut->areNodesSorted = true;
			mapOut->areWaysSorted = true;
			mapOut->areRelationsSorted = true;
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
			if (!foundOsmHeader) { NotifyPrint_E("Blob[%llu] was OSMData BEFORE we found OSMHeader blob!", blobIndex); result = Result_MissingHeader; break; }
			
			TracyCZoneN(Zone_OsmHeaderBlock, "OsmPrimitiveBlock", true);
			OSMPBF__PrimitiveBlock* primitiveBlock = osmpbf__primitive_block__unpack(&scratchAllocator, decompressedBuffer.length, decompressedBuffer.bytes);
			TracyCZoneEnd(Zone_OsmHeaderBlock);
			if (primitiveBlock == nullptr) { NotifyPrint_E("Failed to parse OSMPBF::PrimitiveBlock in blob[%llu]!", blobIndex); result = Result_ParsingFailure; break; }
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
			v2d nodeOffset = MakeV2d(
				(r64)(primitiveBlock->has_lon_offset ? primitiveBlock->lon_offset * granularityMult : 0),
				(r64)(primitiveBlock->has_lat_offset ? primitiveBlock->lat_offset * granularityMult : 0)
			);
			
			for (size_t gIndex = 0; gIndex < primitiveBlock->n_primitivegroup; gIndex++)
			{
				OSMPBF__PrimitiveGroup* primitiveGroup = primitiveBlock->primitivegroup[gIndex];
				
				// +==============================+
				// |          PBF Nodes           |
				// +==============================+
				if (primitiveGroup->n_nodes > 0)
				{
					TracyCZoneN(Zone_OsmNodes, "OsmNodes", true);
					for (size_t nIndex = 0; nIndex < primitiveGroup->n_nodes; nIndex++)
					{
						OSMPBF__Node* node = primitiveGroup->nodes[nIndex];
						UNUSED(node); //TODO: Implement me!
					}
					TracyCZoneEnd(Zone_OsmNodes);
					if (result != Result_None) { break; }
				}
				
				// +==============================+
				// |       PBF Dense Nodes        |
				// +==============================+
				if (primitiveGroup->dense != nullptr)
				{
					TracyCZoneN(Zone_OsmDenseNodes, "OsmDenseNodes", true);
					OSMPBF__DenseNodes* denseNodes = primitiveGroup->dense;
					NotNull(denseNodes->denseinfo);
					if (denseNodes->n_id != denseNodes->n_lat) { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match Lat count %zu", blobIndex, denseNodes->n_id, denseNodes->n_lat); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (denseNodes->n_id != denseNodes->n_lon) { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match Lon count %zu", blobIndex, denseNodes->n_id, denseNodes->n_lon); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					bool haveVersions   = (denseNodes->denseinfo->n_version   != 0);
					bool haveTimestamps = (denseNodes->denseinfo->n_timestamp != 0);
					bool haveChangesets = (denseNodes->denseinfo->n_changeset != 0);
					bool haveUids       = (denseNodes->denseinfo->n_uid       != 0);
					bool haveUserSids   = (denseNodes->denseinfo->n_user_sid  != 0);
					bool haveVisibles   = (denseNodes->denseinfo->n_visible   != 0);
					if (haveVersions   && denseNodes->n_id != denseNodes->denseinfo->n_version)   { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match info->version count %zu",   blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_version);   result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveTimestamps && denseNodes->n_id != denseNodes->denseinfo->n_timestamp) { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match info->timestamp count %zu", blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_timestamp); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveChangesets && denseNodes->n_id != denseNodes->denseinfo->n_changeset) { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match info->changeset count %zu", blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_changeset); result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveUids       && denseNodes->n_id != denseNodes->denseinfo->n_uid)       { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match info->uid count %zu",       blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_uid);       result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveUserSids   && denseNodes->n_id != denseNodes->denseinfo->n_user_sid)  { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match info->user_sid count %zu",  blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_user_sid);  result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					if (haveVisibles   && denseNodes->n_id != denseNodes->denseinfo->n_visible)   { NotifyPrint_E("DenseNode in blob[%llu] ID count %zu doesn't match info->visible count %zu",   blobIndex, denseNodes->n_id, denseNodes->denseinfo->n_visible);   result = Result_Mismatch; TracyCZoneEnd(Zone_OsmDenseNodes); break; }
					
					VarArrayExpand(&mapOut->nodes, mapOut->nodes.length + (uxx)denseNodes->n_id);
					
					size_t currentKeyValIndex = 0;
					
					bool areNewNodesSorted = true;
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
						if (nodeId <= 0) { NotifyPrint_E("Invalid Node ID %lld in blob[%llu] group[%zu] denseNode[%zu]!", nodeId, blobIndex, gIndex, nIndex); result = Result_InvalidID; break; }
						if (nodeVersion < -1) { NotifyPrint_E("Invalid Node Version %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeVersion, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						if (nodeTimestamp < 0) { NotifyPrint_E("Invalid Node Timestamp %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeTimestamp, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						if (nodeChangeset < 0) { NotifyPrint_E("Invalid Node Changeset %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeChangeset, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						if (nodeUid < 0) { NotifyPrint_E("Invalid Node UID %d in blob[%llu] group[%zu] denseNode[%zu]!", nodeUid, blobIndex, gIndex, nIndex); result = Result_ValueTooLow; break; }
						
						if (nIndex == 0)
						{
							OsmNode* lastNode = VarArrayGetLastSoft(OsmNode, &mapOut->nodes);
							if (lastNode != nullptr && lastNode->id >= (u64)nodeId) { areNewNodesSorted = false; }
						}
						else if (prevNodeId >= nodeId) { areNewNodesSorted = false; }
						v2d nodeLocation = MakeV2d(
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
					if (currentKeyValIndex < denseNodes->n_keys_vals) { NotifyPrint_W("There were %zu/%zu tags left over after parsing %zu denseNodes in blob[%llu]", denseNodes->n_keys_vals - currentKeyValIndex, denseNodes->n_keys_vals, denseNodes->n_id, blobIndex); }
					
					if (!areNewNodesSorted || !mapOut->areNodesSorted)
					{
						TracyCZoneN(Zone_SortNodes, "SortNodes", true);
						QuickSortVarArrayUintMember(OsmNode, id, &mapOut->nodes);
						mapOut->areNodesSorted = true;
						TracyCZoneEnd(Zone_SortNodes);
						
						//TODO: We really should be waiting till the end of parsing the entire file before hook up NodeRefs with valid pointers
						if (mapOut->ways.length > 0)
						{
							TracyCZoneN(_FixNodeRefs, "FixNodeRefs", true);
							VarArrayLoop(&mapOut->ways, wIndex)
							{
								VarArrayLoopGet(OsmWay, way, &mapOut->ways, wIndex);
								VarArrayLoop(&way->nodes, nIndex)
								{
									VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
									nodeRef->pntr = FindOsmNode(mapOut, nodeRef->id);
								}
							}
							TracyCZoneEnd(_FixNodeRefs);
						}
					}
				}
				
				// +==============================+
				// |           PBF Ways           |
				// +==============================+
				if (primitiveGroup->n_ways > 0)
				{
					TracyCZoneN(Zone_OsmWays, "OsmWays", true);
					bool areNewWaysSorted = true;
					u64 prevWayId = 0;
					for (size_t wIndex = 0; wIndex < primitiveGroup->n_ways; wIndex++)
					{
						OSMPBF__Way* way = primitiveGroup->ways[wIndex];
						NotNull(way->info);
						if (way->id <= 0)                                         { NotifyPrint_E("Way[%zu] in blob[%llu] has invalid ID %lld",        wIndex, blobIndex, way->id); result = Result_InvalidID; break; }
						if (way->info->has_timestamp && way->info->timestamp < 0) { NotifyPrint_E("Way[%zu] in blob[%llu] has invalid timestamp %lld", wIndex, blobIndex, way->info->timestamp); result = Result_ValueTooLow; break; }
						if (way->info->has_uid       && way->info->uid       < 0) { NotifyPrint_E("Way[%zu] in blob[%llu] has invalid uid %d",         wIndex, blobIndex, way->info->uid); result = Result_ValueTooLow; break; }
						if (way->info->has_changeset && way->info->changeset < 0) { NotifyPrint_E("Way[%zu] in blob[%llu] has invalid changeset %lld", wIndex, blobIndex, way->info->changeset); result = Result_ValueTooLow; break; }
						if (way->n_keys != way->n_vals)                           { NotifyPrint_E("Way[%zu] in blob[%llu] key count %zu doesn't match value count %zu", wIndex, blobIndex, way->n_keys, way->n_vals); result = Result_Mismatch; break; }
						if (way->n_refs > 0)
						{
							uxx scratchMark2 = ArenaGetMark(scratch);
							uxx numNodesInWay = (uxx)way->n_refs;
							u64* nodeIds = AllocArray(u64, scratch, numNodesInWay);
							NotNull(nodeIds);
							u64 prevNodeId = 0;
							for (size_t rIndex = 0; rIndex < way->n_refs; rIndex++)
							{
								if (way->refs[rIndex] < -(i64)prevNodeId) { NotifyPrint_E("Node[%zu] in Way[%zu] in blob[%llu] has negative ID %llu%s%lld", rIndex, wIndex, blobIndex, prevNodeId, (way->refs[rIndex] >= 0) ? "+" : "", way->refs[rIndex]); result = Result_InvalidID; break; }
								if (way->refs[rIndex] > 0 && (u64)way->refs[rIndex] > UINT64_MAX - prevNodeId) { NotifyPrint_E("Node[%zu] in Way[%zu] in blob[%llu] has overflow ID %llu%s%lld", rIndex, wIndex, blobIndex, prevNodeId, (way->refs[rIndex] >= 0) ? "+" : "", way->refs[rIndex]); result = Result_InvalidID; break; }
								nodeIds[rIndex] = (u64)(prevNodeId + way->refs[rIndex]);
								prevNodeId = nodeIds[rIndex];
							}
							if (result != Result_None) { ArenaResetToMark(scratch, scratchMark2); break; }
							
							if (prevWayId == 0)
							{
								OsmWay* lastWay = VarArrayGetLastSoft(OsmWay, &mapOut->ways);
								if (lastWay != nullptr && lastWay->id >= (u64)way->id) { areNewWaysSorted = false; }
							}
							else if (prevWayId >= (u64)way->id) { areNewWaysSorted = false; }
							prevWayId = (u64)way->id;
							
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
					
					if (!areNewWaysSorted || !mapOut->areWaysSorted)
					{
						TracyCZoneN(Zone_SortWays, "SortWays", true);
						QuickSortVarArrayUintMember(OsmWay, id, &mapOut->ways);
						mapOut->areWaysSorted = true;
						TracyCZoneEnd(Zone_SortWays);
					}
				}
				
				// +==============================+
				// |        PBF Relations         |
				// +==============================+
				if (primitiveGroup->n_relations > 0)
				{
					TracyCZoneN(Zone_OsmRelations, "OsmRelations", true);
					bool areNewRelationsSorted = true;
					u64 prevRelationId = 0;
					for (size_t rIndex = 0; rIndex < primitiveGroup->n_relations; rIndex++)
					{
						OSMPBF__Relation* relation = primitiveGroup->relations[rIndex];
						NotNull(relation->info);
						if (relation->id <= 0)                                              { NotifyPrint_E("Relation[%zu] in blob[%llu] has invalid ID %lld", rIndex, blobIndex, relation->id); result = Result_InvalidID; break; }
						if (relation->info->has_timestamp && relation->info->timestamp < 0) { NotifyPrint_E("Relation[%zu] in blob[%llu] has invalid timestamp %lld", rIndex, blobIndex, relation->info->timestamp); result = Result_ValueTooLow; break; }
						if (relation->info->has_uid       && relation->info->uid       < 0) { NotifyPrint_E("Relation[%zu] in blob[%llu] has invalid uid %d", rIndex, blobIndex, relation->info->uid); result = Result_ValueTooLow; break; }
						if (relation->info->has_changeset && relation->info->changeset < 0) { NotifyPrint_E("Relation[%zu] in blob[%llu] has invalid changeset %lld", rIndex, blobIndex, relation->info->changeset); result = Result_ValueTooLow; break; }
						if (relation->n_keys != relation->n_vals)                           { NotifyPrint_E("Relation[%zu] in blob[%llu] key count %zu doesn't match value count %zu", rIndex, blobIndex, relation->n_keys, relation->n_vals); result = Result_Mismatch; break; }
						if (relation->n_memids != relation->n_roles_sid)                    { NotifyPrint_E("Relation[%zu] in blob[%llu] member ID count %zu doesn't match roles SID count %zu", rIndex, blobIndex, relation->n_memids, relation->n_roles_sid); result = Result_Mismatch; break; }
						if (relation->n_memids != relation->n_types)                        { NotifyPrint_E("Relation[%zu] in blob[%llu] member ID count %zu doesn't match types count %zu", rIndex, blobIndex, relation->n_memids, relation->n_types); result = Result_Mismatch; break; }
						if (relation->n_memids > 0)
						{
							if (prevRelationId == 0)
							{
								OsmRelation* lastRelation = VarArrayGetLastSoft(OsmRelation, &mapOut->relations);
								if (lastRelation != nullptr && lastRelation->id >= (u64)relation->id) { areNewRelationsSorted = false; }
							}
							else if (prevRelationId >= (u64)relation->id) { areNewRelationsSorted = false; }
							prevRelationId = (u64)relation->id;
							
							OsmRelation* newRelation = AddOsmRelation(mapOut, (u64)relation->id, (uxx)relation->n_memids);
							newRelation->visible = (relation->info->has_visible ? relation->info->visible : true);
							newRelation->version = (relation->info->has_version ? relation->info->version : -1);
							//TODO: Str8 timestampStr; newRelation->timestamp = (relation->info->has_timestamp ? (u64)relation->info->timestamp : 0);
							newRelation->uid = (relation->info->has_uid ? (u64)relation->info->uid : 0);
							//TODO: Str8 user; newRelation->user = (relation->info->has_user_sid ? LookupString(relation->info->user_sid) : Str8_Empty);
							newRelation->changeset = (relation->info->has_changeset ? (u64)relation->info->changeset : 0);
							VarArrayExpand(&newRelation->tags, newRelation->tags.length + (uxx)relation->n_keys);
							for (size_t tIndex = 0; tIndex < relation->n_keys; tIndex++)
							{
								Str8 keyStr = GetPbfString(primitiveBlock->stringtable, relation->keys[tIndex]);
								if (!IsEmptyStr(keyStr))
								{
									Str8 valueStr = GetPbfString(primitiveBlock->stringtable, relation->vals[tIndex]);
									OsmTag* newTag = VarArrayAdd(OsmTag, &newRelation->tags);
									NotNull(newTag);
									ClearPointer(newTag);
									newTag->key = AllocStr8(arena, keyStr);
									newTag->value = AllocStr8(arena, valueStr);
								}
							}
							
							i64 prevMemberId = 0;
							for (size_t mIndex = 0; mIndex < relation->n_memids; mIndex++)
							{
								Str8 roleStr = GetPbfString(primitiveBlock->stringtable, relation->roles_sid[mIndex]);
								i64 memberId = prevMemberId + relation->memids[mIndex];
								prevMemberId = memberId;
								OSMPBF__Relation__MemberType memberType = relation->types[mIndex];
								
								OsmRelationMemberRole role = OsmRelationMemberRole_None;
								if (!IsEmptyStr(roleStr))
								{
									for (uxx roleIndex = 1; roleIndex < OsmRelationMemberRole_Count; roleIndex++)
									{
										const char* roleEnumStrNt = GetOsmRelationMemberRoleXmlStr((OsmRelationMemberRole)roleIndex);
										if (StrAnyCaseEquals(roleStr, MakeStr8Nt(roleEnumStrNt)))
										{
											role = (OsmRelationMemberRole)roleIndex;
											break;
										}
									}
									if (role == OsmRelationMemberRole_None) { PrintLine_W("Warning: Uknown role type \"%.*s\" on member[%llu] in relation[%zu] in blob[%llu]", StrPrint(roleStr), mIndex, rIndex, blobIndex); }
								}
								
								if (memberId <= 0) { NotifyPrint_E("Member[%zu] in Relation[%zu] in blob[%llu] has invalid ID %lld", mIndex, rIndex, blobIndex, memberId); result = Result_InvalidID; break; }
								else
								{
									OsmRelationMember* newMember = VarArrayAdd(OsmRelationMember, &newRelation->members);
									NotNull(newMember);
									ClearPointer(newMember);
									newMember->id = (u64)memberId;
									newMember->role = role;
									newMember->pntr = nullptr; //We'll look it up later
									switch (memberType)
									{
										case OSMPBF__RELATION__MEMBER_TYPE__NODE: newMember->type = OsmRelationMemberType_Node; break;
										case OSMPBF__RELATION__MEMBER_TYPE__WAY: newMember->type = OsmRelationMemberType_Way; break;
										case OSMPBF__RELATION__MEMBER_TYPE__RELATION: newMember->type = OsmRelationMemberType_Relation; break;
										default: NotifyPrint_E("Member[%zu] in Relation[%zu] in blob[%llu] has unhandled type %d", mIndex, rIndex, blobIndex, memberType); result = Result_InvalidType; break;
									}
									if (result != Result_None) { break; }
								}
							}
							if (result != Result_None) { break; }
						}
						else { PrintLine_W("Relation[%zu] in blob[%llu] had no members!", rIndex, blobIndex); }
					}
					TracyCZoneEnd(Zone_OsmRelations);
					if (result != Result_None) { break; }
					
					if (!areNewRelationsSorted || !mapOut->areRelationsSorted)
					{
						TracyCZoneN(Zone_SortRelations, "SortRelations", true);
						QuickSortVarArrayUintMember(OsmRelation, id, &mapOut->relations);
						mapOut->areRelationsSorted = true;
						TracyCZoneEnd(Zone_SortRelations);
					}
				}
				
				// +==============================+
				// |        PBF Changesets        |
				// +==============================+
				if (primitiveGroup->n_changesets > 0)
				{
					TracyCZoneN(Zone_OsmChangesets, "OsmChangesets", true);
					for (size_t cIndex = 0; cIndex < primitiveGroup->n_changesets; cIndex++)
					{
						OSMPBF__ChangeSet* changeset = primitiveGroup->changesets[cIndex];
						UNUSED(changeset); //TODO: Implement me!
					}
					TracyCZoneEnd(Zone_OsmChangesets);
					if (result != Result_None) { break; }
				}
			}
			if (result != Result_None) { break; }
			
			foundOsmData = true;
		}
		else { PrintLine_W("Unhandled blob type \"%s\"", blobHeader->type); foundUnkownBlobTypes = true; }
		
		ArenaResetToMark(scratch, scratchMark1);
		blobIndex++;
	}
	
	if (result != Result_None) { NotifyPrint_E("Got through %llu blobs before encountering: %s", blobIndex, GetResultStr(result)); }
	if (!foundOsmData) { result = foundOsmHeader ? (foundUnkownBlobTypes ? Result_WrongInternalFormat : Result_MissingData) : Result_MissingHeader; }
	if (result == Result_None)
	{
		result = Result_Success;
		VarArrayLoop(&mapOut->relations, rIndex)
		{
			VarArrayLoopGet(OsmRelation, relation, &mapOut->relations, rIndex);
			UpdateOsmRelationPntrs(mapOut, relation);
		}
		UpdateOsmNodeWayBackPntrs(mapOut);
		UpdateOsmRelationBackPntrs(mapOut);
	}
	else if (foundOsmHeader) { FreeOsmMap(mapOut); }
	ScratchEnd(scratch);
	TracyCZoneEnd(Zone_Func);
	return result;
}
