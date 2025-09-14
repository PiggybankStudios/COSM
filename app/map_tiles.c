/*
File:   map_tiles.c
Author: Taylor Robbins
Date:   09\09\2025
Description:
	** Holds functions that allow us to download rasterized tiles from tile.openstreetmap.org
*/

typedef plex MapTileHttpContext MapTileHttpContext;
plex MapTileHttpContext
{
	SparseSetV3i* mapTiles;
	v3i coord;
};

// +==============================+
// |     MapTileHttpCallback      |
// +==============================+
// void MapTileHttpCallback(plex HttpRequest* request)
HTTP_CALLBACK_DEF(MapTileHttpCallback)
{
	MapTileHttpContext* context = (MapTileHttpContext*)request->args.contextPntr;
	NotNull(context);
	NotNull(context->mapTiles);
	MapTile* mapTile = SparseSetV3iGet(MapTile, context->mapTiles, context->coord);
	NotNull(mapTile);
	FreeType(MapTileHttpContext, stdHeap, context); context = nullptr;
	
	bool wasSuccessful = false;
	if (request->state != HttpRequestState_Success) { PrintLine_E("Request to download MapTile(%d, %d, %d) failed! Status: %u %s Error: %s", mapTile->coord.X, mapTile->coord.Y, mapTile->coord.Z, request->statusCode, GetHttpStatusCodeDescription(request->statusCode), GetResultStr(request->error)); }
	else if (request->responseBytes.length == 0) { PrintLine_E("Request to download MapTile(%d, %d, %d) return empty response!", mapTile->coord.X, mapTile->coord.Y, mapTile->coord.Z); }
	else
	{
		Slice responseBytes = NewStr8(request->responseBytes.length, request->responseBytes.items);
		PrintLine_D("Got %llu byte response for MapTile(%d, %d, %d)", responseBytes.length, mapTile->coord.X, mapTile->coord.Y, mapTile->coord.Z);
		ScratchBegin(scratch);
		ImageData imageData = ZEROED;
		Result parseResult = TryParseImageFile(responseBytes, scratch, &imageData);
		if (parseResult != Result_Success) { PrintLine_E("Failed to parse response as image! %s", GetResultStr(parseResult)); }
		else
		{
			Str8 textureName = PrintInArenaStr(scratch, "Tile(%d,%d,%d)", mapTile->coord.X, mapTile->coord.Y, mapTile->coord.Z);
			Texture imageTexture = InitTexture(stdHeap, textureName, imageData.size, imageData.pixels, TextureFlag_None); //TODO: Pass NoMipmap when we have that working!
			if (imageTexture.error != Result_Success) { PrintLine_E("Failed to upload texture for MapTile(%d, %d, %d): %s", mapTile->coord.X, mapTile->coord.Y, mapTile->coord.Z, GetResultStr(imageTexture.error)); }
			else
			{
				wasSuccessful = true;
				mapTile->texture = imageTexture;
				mapTile->isLoaded = true;
				
				FilePath settingsFolderPath = OsGetSettingsSavePath(scratch, Str8_Empty, StrLit(PROJECT_FOLDER_NAME_STR), false); //TODO: This should really be like a "temporary" folder, not a "settings" folder
				FilePath tileFolderPath = JoinStringsInArena3(scratch,
					settingsFolderPath,
					DoesPathHaveTrailingSlash(settingsFolderPath) ? Str8_Empty : StrLit("/"),
					StrLit(TILES_FOLDERPATH "/"),
					false
				);
				Str8 fullTilePath = JoinStringsInArena(scratch, tileFolderPath, mapTile->fileName, false);
				bool writeSuccess = OsWriteBinFile(fullTilePath, responseBytes);
				DebugAssert(writeSuccess);
				if (writeSuccess) { mapTile->isOnDisk = true; }
			}
		}
		ScratchEnd(scratch);
	}
	
	mapTile->isDownloading = false;
	mapTile->downloadRequestId = 0;
	if (!wasSuccessful) { mapTile->failedToDownload = true; }
}

void ClearMapTiles()
{
	SparseSetV3iLoop(&app->mapTiles, tIndex)
	{
		SparseSetV3iLoopGet(MapTile, mapTile, &app->mapTiles, tIndex)
		{
			FreeStr8(stdHeap, &mapTile->fileName);
			if (mapTile->isLoaded) { FreeTexture(&mapTile->texture); }
		}
	}
	SparseSetV3iClear(&app->mapTiles);
}

void InitMapTiles()
{
	ScratchBegin(scratch);
	InitSparseSetV3i(MapTile, &app->mapTiles, stdHeap);
	
	FilePath settingsFolderPath = OsGetSettingsSavePath(scratch, Str8_Empty, StrLit(PROJECT_FOLDER_NAME_STR), false); //TODO: This should really be like a "temporary" folder, not a "settings" folder
	FilePath tileFolderPath = JoinStringsInArena3(scratch,
		settingsFolderPath,
		DoesPathHaveTrailingSlash(settingsFolderPath) ? Str8_Empty : StrLit("/"),
		StrLit(TILES_FOLDERPATH),
		false
	);
	if (!OsDoesFolderExist(tileFolderPath))
	{
		Result createFoldersResult = OsCreateFolder(tileFolderPath, true);
		Assert(createFoldersResult == Result_Success);
	}
	
	// +================================+
	// | Find Existing Tile Image Files |
	// +================================+
	OsFileIter fileIter = OsIterateFiles(scratch, tileFolderPath, true, false);
	FilePath fileName = FilePath_Empty;
	while (OsIterFileStepEx(&fileIter, nullptr, &fileName, scratch, false))
	{
		Str8 fileExt = GetFileExtPart(fileName, true, true);
		if (StrAnyCaseEquals(fileExt, StrLit(".png")))
		{
			Str8 fileNameNoExt = GetFileNamePart(fileName, false);
			uxx underscoreIndex = 0;
			RangeUXX zRange = RangeUXX_Zero_Const;
			RangeUXX xRange = RangeUXX_Zero_Const;
			RangeUXX yRange = RangeUXX_Zero_Const;
			for (uxx cIndex = 0; cIndex < fileNameNoExt.length; cIndex++)
			{
				if (fileNameNoExt.chars[cIndex] == '_')
				{
					if (underscoreIndex >= 3) { underscoreIndex++; break; }
					if (underscoreIndex == 0) { zRange.min = cIndex+1; }
					else if (underscoreIndex == 1) { zRange.max = cIndex; xRange.min = cIndex+1; }
					else if (underscoreIndex == 2) { xRange.max = cIndex; yRange.min = cIndex+1; yRange.max = fileNameNoExt.length; }
					underscoreIndex++;
				}
			}
			if (underscoreIndex != 3) { continue; }
			Str8 zStr = StrSliceRange(fileNameNoExt, zRange);
			Str8 xStr = StrSliceRange(fileNameNoExt, xRange);
			Str8 yStr = StrSliceRange(fileNameNoExt, yRange);
			v3i coord = V3i_Zero;
			if (IsEmptyStr(zStr) || !TryParseI32(zStr, &coord.Z, nullptr)) { continue; }
			if (IsEmptyStr(xStr) || !TryParseI32(xStr, &coord.X, nullptr)) { continue; }
			if (IsEmptyStr(yStr) || !TryParseI32(yStr, &coord.Y, nullptr)) { continue; }
			
			MapTile* tileOnDisk = SparseSetV3iAdd(MapTile, &app->mapTiles, coord);
			NotNull(tileOnDisk);
			ClearPointer(tileOnDisk);
			tileOnDisk->coord = coord;
			tileOnDisk->fileName = AllocStr8(stdHeap, fileName);
			PrintLine_D("Found MapTile(%d, %d, %d) at \"%.*s\"", coord.X, coord.Y, coord.Z, StrPrint(tileOnDisk->fileName));
			tileOnDisk->isOnDisk = true;
		}
	}
	PrintLine_D("Found %llu map tiles in \"%.*s\"", app->mapTiles.length, StrPrint(tileFolderPath));
	ScratchEnd(scratch);
}

bool EvictUnusedLoadedMapTile()
{
	MapTile* oldestTile = nullptr;
	u64 oldestTileTime = 0;
	SparseSetV3iLoop(&app->mapTiles, tIndex)
	{
		SparseSetV3iLoopGet(MapTile, mapTile, &app->mapTiles, tIndex)
		{
			if (mapTile->isLoaded)
			{
				if (oldestTile == nullptr || mapTile->lastUsedTime == 0 || TimeSinceBy(appIn->programTime, mapTile->lastUsedTime) > oldestTileTime)
				{
					oldestTile = mapTile;
					oldestTileTime = (mapTile->lastUsedTime != 0) ? TimeSinceBy(appIn->programTime, mapTile->lastUsedTime) : UINT64_MAX;
				}
			}
		}
	}
	if (oldestTile != nullptr)
	{
		PrintLine_D("Evicting tile (%d, %d, %d) which was used %llums ago", oldestTile->coord.X, oldestTile->coord.Y, oldestTile->coord.Z, oldestTileTime);
		FreeTexture(&oldestTile->texture);
		oldestTile->isLoaded = false;
		return true;
	}
	else { return false; }
}

void UpdateMapTiles()
{
	uxx numDownloading = 0;
	uxx numLoaded = 0;
	uxx numOnDiskUnloaded = 0;
	uxx numNeedingDownload = 0;
	SparseSetV3iLoop(&app->mapTiles, tIndex)
	{
		SparseSetV3iLoopGet(MapTile, mapTile, &app->mapTiles, tIndex)
		{
			if (mapTile->isDownloading) { numDownloading++; }
			if (mapTile->isLoaded) { numLoaded++; }
			else if (mapTile->isOnDisk) { numOnDiskUnloaded++; }
			else if (!mapTile->isDownloading && !mapTile->failedToDownload && mapTile->lastUsedTime != 0) { numNeedingDownload++; }
		}
	}
	
	while (numLoaded > MAX_LOADED_MAP_TILES)
	{
		if (EvictUnusedLoadedMapTile()) { numLoaded--; }
		else { break; }
	}
	
	while (numDownloading < 3)
	{
		u64 mostRecentRequestTime = 0;
		MapTile* mostRecentRequestTile = 0;
		SparseSetV3iLoop(&app->mapTiles, tIndex2)
		{
			SparseSetV3iLoopGet(MapTile, mapTile, &app->mapTiles, tIndex2)
			{
				if (!mapTile->isDownloading && !mapTile->isLoaded && !mapTile->isOnDisk && !mapTile->failedToDownload && mapTile->lastUsedTime != 0)
				{
					u64 timeSinceUsed = TimeSinceBy(appIn->programTime, mapTile->lastUsedTime);
					if (mostRecentRequestTile == nullptr || timeSinceUsed < mostRecentRequestTime)
					{
						mostRecentRequestTime = timeSinceUsed;
						mostRecentRequestTile = mapTile;
					}
				}
			}
		}
		
		if (mostRecentRequestTile != nullptr)
		{
			ScratchBegin(scratch);
			HttpRequestArgs args = ZEROED;
			args.verb = HttpVerb_GET;
			args.urlStr = PrintInArenaStr(scratch, OSM_TILE_API_URL_FORMAT_STR, mostRecentRequestTile->coord.Z, mostRecentRequestTile->coord.X, mostRecentRequestTile->coord.Y);
			Str8Pair headers[] = {
				{ .key=StrLit("User-Agent"), .value=StrLit(OSM_TILE_API_USER_AGENT) },
			};
			args.numHeaders = ArrayCount(headers);
			args.headers = &headers[0];
			args.contentEncoding = MimeType_FormUrlEncoded;
			args.numContentItems = 0;
			args.contentItems = nullptr;
			args.callback = MapTileHttpCallback;
			MapTileHttpContext* callbackContext = AllocType(MapTileHttpContext, stdHeap);
			callbackContext->mapTiles = &app->mapTiles;
			callbackContext->coord = mostRecentRequestTile->coord;
			args.contextPntr = (void*)callbackContext;
			HttpRequest* newRequest = OsMakeHttpRequest(&app->httpManager, &args, appIn->programTime);
			if (newRequest != nullptr)
			{
				PrintLine_D("Downloading MapTile(%d, %d, %d)...", mostRecentRequestTile->coord.X, mostRecentRequestTile->coord.Y, mostRecentRequestTile->coord.Z);
				mostRecentRequestTile->isDownloading = true;
				mostRecentRequestTile->downloadRequestId = newRequest->id;
				numDownloading++;
			}
			else
			{
				PrintLine_W("Failed to start request to download MapTile(%d, %d, %d)!", mostRecentRequestTile->coord.X, mostRecentRequestTile->coord.Y, mostRecentRequestTile->coord.Z);
				mostRecentRequestTile->failedToDownload = true;
				break;
			}
			ScratchEnd(scratch);
		}
		else { break; }
	}
}

Texture* GetMapTileTexture(v3i coord, bool loadFromDisk, bool download)
{
	MapTile* resultTile = nullptr;
	SparseSetV3iLoop(&app->mapTiles, tIndex)
	{
		SparseSetV3iLoopGet(MapTile, mapTile, &app->mapTiles, tIndex)
		{
			if (AreEqualV3i(mapTile->coord, coord))
			{
				resultTile = mapTile;
				break;
			}
		}
	}
	
	if (resultTile == nullptr && download)
	{
		resultTile = SparseSetV3iAdd(MapTile, &app->mapTiles, coord);
		NotNull(resultTile);
		ClearPointer(resultTile);
		resultTile->coord = coord;
		resultTile->fileName = PrintInArenaStr(stdHeap, "tile_%04d_%04d_%04d.png", coord.Z, coord.X, coord.Y);
	}
	if (resultTile != nullptr && resultTile->isOnDisk && !resultTile->isLoaded && loadFromDisk)
	{
		ScratchBegin(scratch);
		FilePath settingsFolderPath = OsGetSettingsSavePath(scratch, Str8_Empty, StrLit(PROJECT_FOLDER_NAME_STR), false); //TODO: This should really be like a "temporary" folder, not a "settings" folder
		FilePath tileFolderPath = JoinStringsInArena3(scratch,
			settingsFolderPath,
			DoesPathHaveTrailingSlash(settingsFolderPath) ? Str8_Empty : StrLit("/"),
			StrLit(TILES_FOLDERPATH "/"),
			false
		);
		Str8 fullTilePath = JoinStringsInArena(scratch, tileFolderPath, resultTile->fileName, false);
		PrintLine_D("Loading MapTile(%d, %d, %d) at \"%.*s\"...", resultTile->coord.X, resultTile->coord.Y, resultTile->coord.Z, StrPrint(fullTilePath));
		Slice imageFileContents = Slice_Empty;
		if (OsReadBinFile(fullTilePath, scratch, &imageFileContents))
		{
			ImageData imageData = ZEROED;
			Result imageParseResult = TryParseImageFile(imageFileContents, scratch, &imageData);
			if (imageParseResult == Result_Success)
			{
				Str8 textureName = PrintInArenaStr(scratch, "Tile(%d,%d,%d)", resultTile->coord.X, resultTile->coord.Y, resultTile->coord.Z);
				Texture imageTexture = InitTexture(stdHeap, textureName, imageData.size, imageData.pixels, TextureFlag_None); //TODO: Pass NoMipmap when we have that working!
				if (imageTexture.error == Result_Success)
				{
					resultTile->texture = imageTexture;
					resultTile->isLoaded = true;
				}
				else { DebugAssert(false); PrintLine_E("Failed to load texture for MapTile(%d, %d, %d): %s", resultTile->coord.X, resultTile->coord.Y, resultTile->coord.Z, GetResultStr(imageTexture.error)); resultTile->isOnDisk = false; }
			}
			else { DebugAssert(false); PrintLine_E("Failed to parse %llu byte map tile image at \"%.*s\"", imageFileContents.length, StrPrint(fullTilePath)); resultTile->isOnDisk = false; }
		}
		else { DebugAssert(false); PrintLine_W("Failed to open map tile at \"%.*s\"", StrPrint(fullTilePath)); resultTile->isOnDisk = false; }
	}
	
	if (resultTile != nullptr) { resultTile->lastUsedTime = (appIn != nullptr) ? appIn->programTime : 0; }
	return (resultTile != nullptr && resultTile->isLoaded) ? &resultTile->texture : nullptr;
}
