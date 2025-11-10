/*
File:   app_helpers.c
Author: Taylor Robbins
Date:   02\25\2025
Description: 
	** None
*/

ImageData LoadImageData(Arena* arena, const char* path)
{
	TracyCZoneN(funcZone, "LoadImageData", true);
	ScratchBegin1(scratch, arena);
	Slice fileContents = Slice_Empty;
	TracyCZoneN(_TryReadAppResource, "TryReadAppResource", true);
	Result readFileResult = TryReadAppResource(&app->resources, scratch, FilePathLit(path), false, &fileContents);
	TracyCZoneEnd(_TryReadAppResource);
	Assert(readFileResult == Result_Success);
	UNUSED(readFileResult);
	ImageData imageData = ZEROED;
	Result parseResult = TryParseImageFile(fileContents, arena, &imageData);
	Assert(parseResult == Result_Success);
	UNUSED(parseResult);
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
	return imageData;
}

void LoadWindowIcon()
{
	ScratchBegin(scratch);
	ImageData iconImageDatas[6];
	iconImageDatas[0] = LoadImageData(scratch, "resources/image/icon_16.png");
	iconImageDatas[1] = LoadImageData(scratch, "resources/image/icon_24.png");
	iconImageDatas[2] = LoadImageData(scratch, "resources/image/icon_32.png");
	iconImageDatas[3] = LoadImageData(scratch, "resources/image/icon_64.png");
	iconImageDatas[4] = LoadImageData(scratch, "resources/image/icon_120.png");
	iconImageDatas[5] = LoadImageData(scratch, "resources/image/icon_256.png");
	platform->SetWindowIcon(ArrayCount(iconImageDatas), &iconImageDatas[0]);
	ScratchEnd(scratch);
}

void LoadNotificationIcons()
{
	TracyCZoneN(funcZone, "LoadNotificationIcons", true);
	
	{
		ScratchBegin(scratch);
		ImageData imageData = LoadImageData(scratch, NOTIFICATION_ICONS_TEXTURE_PATH);
		AssertMsg(imageData.pixels != nullptr && imageData.size.Width > 0 && imageData.size.Height > 0, "Failed to load notification icons texture!");
		Texture newTexture = InitTexture(stdHeap, StrLit("notificationIcons"), imageData.size, imageData.pixels, 0x00);
		AssertMsg(newTexture.error == Result_Success, "Failed to init texture for notification icons!");
		FreeTexture(&app->notificationIconsTexture);
		app->notificationIconsTexture = newTexture;
		ScratchEnd(scratch);
	}
	
	const v2i sheetSize = { .X=2, .Y=2 };
	v2 cellSize = NewV2(
		(r32)app->notificationIconsTexture.Width / (r32)sheetSize.Width,
		(r32)app->notificationIconsTexture.Height / (r32)sheetSize.Height
	);
	r32 iconScale = NOTIFICATION_ICONS_SIZE / cellSize.Width;
	for (uxx lIndex = DbgLevel_Debug; lIndex < DbgLevel_Count; lIndex++)
	{
		DbgLevel dbgLevel = (DbgLevel)lIndex;
		v2i cellPos = NewV2i(0, 0);
		switch (dbgLevel)
		{
			// case DbgLevel_Debug:   cellPos = NewV2i(1, 0); break;
			case DbgLevel_Regular: cellPos = NewV2i(1, 0); break;
			case DbgLevel_Info:    cellPos = NewV2i(1, 0); break;
			case DbgLevel_Notify:  cellPos = NewV2i(1, 0); break;
			case DbgLevel_Other:   cellPos = NewV2i(1, 0); break;
			case DbgLevel_Warning: cellPos = NewV2i(0, 1); break;
			case DbgLevel_Error:   cellPos = NewV2i(1, 1); break;
		}
		if (cellPos.X != 0 || cellPos.Y != 0)
		{
			rec iconSourceRec = NewRec(
				cellSize.Width * cellPos.X,
				cellSize.Height * cellPos.Y,
				cellSize.Width, cellSize.Height
			);
			SetNotificationIconEx(&app->notificationQueue, dbgLevel, &app->notificationIconsTexture, iconScale, GetDbgLevelTextColor(dbgLevel), iconSourceRec);
		}
	}
	
	TracyCZoneEnd(funcZone);
}

void LoadMapBackTexture()
{
	TracyCZoneN(funcZone, "LoadMapBackTexture", true);
	ScratchBegin(scratch);
	ImageData mapBackImageData = LoadImageData(scratch, MAP_BACKGROUND_TEXTURE_PATH);
	if (mapBackImageData.pixels != nullptr && mapBackImageData.size.Width > 0 && mapBackImageData.size.Height > 0)
	{
		TracyCZoneN(_InitTexture, "InitTexture", true);
		Texture newTexture = InitTexture(stdHeap, StrLit("mapBackTexture"), mapBackImageData.size, mapBackImageData.pixels, 0x00);
		TracyCZoneEnd(_InitTexture);
		if (newTexture.error == Result_Success)
		{
			FreeTexture(&app->mapBackTexture);
			app->mapBackTexture = newTexture;
			FreeStr8(stdHeap, &app->mapBackTexturePath);
			app->mapBackTexturePath = AllocStr8Nt(stdHeap, MAP_BACKGROUND_TEXTURE_PATH);
		}
		else { NotifyPrint_E("Failed to parse/create background Texture from %dx%d ImageData!", mapBackImageData.size.Width, mapBackImageData.size.Height); }
	}
	else { NotifyPrint_E("Failed to load map background texture from \"%s\"", MAP_BACKGROUND_TEXTURE_PATH); }
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
}

Result TryAttachLocalFontFile(PigFont* font, Str8 filePath, u8 styleFlags)
{
	NotNull(font);
	NotNull(font->arena);
	NotEmptyStr(filePath);
	ScratchBegin1(scratch, font->arena);
	Slice fontFileContents = Slice_Empty;
	if (!OsReadBinFile(filePath, scratch, &fontFileContents)) { ScratchEnd(scratch); return Result_FileNotFound; }
	Result attachResult = TryAttachFontFile(font, filePath, fontFileContents, styleFlags, true);
	ScratchEnd(scratch);
	return attachResult;
}

bool AppCreateFonts()
{
	FontCharRange basicRanges[] = {
		FontCharRange_ASCII,
		FontCharRange_LatinSupplementAccent,
		FontCharRange_LatinExtA,
		NewFontCharRangeSingle(UNICODE_ELLIPSIS_CODEPOINT),
		NewFontCharRangeSingle(UNICODE_RIGHT_ARROW_CODEPOINT),
	};
	FontCharRange otherRanges[] = {
		FontCharRange_Cyrillic,
	};
	FontCharRange kanaRanges[] = {
		FontCharRange_Hiragana,
		FontCharRange_Katakana,
	};
	
	Result attachResult = Result_None;
	Result bakeResult = Result_None;
	
	PigFont newUiFont = ZEROED;
	{
		newUiFont = InitFont(stdHeap, StrLit("uiFont"));
		attachResult = TryAttachOsTtfFileToFont(&newUiFont, StrLit(UI_FONT_NAME), app->uiFontSize, UI_FONT_STYLE); Assert(attachResult == Result_Success);
		attachResult = TryAttachLocalFontFile(&newUiFont, StrLit("resources/font/NotoSansJP-Regular.ttf"), UI_FONT_STYLE); Assert(attachResult == Result_Success);
		attachResult = TryAttachLocalFontFile(&newUiFont, StrLit("resources/font/NotoSansSymbols-Regular.ttf"), UI_FONT_STYLE); Assert(attachResult == Result_Success);
		
		bakeResult = TryBakeFontAtlas(&newUiFont, app->uiFontSize, UI_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, ArrayCount(basicRanges), &basicRanges[0]);
		if (bakeResult == Result_Success || bakeResult == Result_Partial)
		{
			FillFontKerningTable(&newUiFont);
			bakeResult = TryBakeFontAtlas(&newUiFont, app->uiFontSize, UI_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, ArrayCount(otherRanges), &otherRanges[0]);
			if (bakeResult == Result_Success || bakeResult == Result_Partial)
			{
				bakeResult = TryBakeFontAtlas(&newUiFont, app->uiFontSize, UI_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, ArrayCount(kanaRanges), &kanaRanges[0]);
			}
		}
		if (bakeResult != Result_Success && bakeResult != Result_Partial)
		{
			FreeFont(&newUiFont);
			return false;
		}
		
		MakeFontActive(&newUiFont, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, FONT_MAX_NUM_ATLASES, FONT_AUTO_EVICT_GLYPH_TIME, FONT_AUTO_EVICT_ATLAS_TIME);
	}
	
	PigFont newLargeFont = ZEROED;
	{
		newLargeFont = InitFont(stdHeap, StrLit("largeFont"));
		attachResult = TryAttachOsTtfFileToFont(&newLargeFont, StrLit(LARGE_FONT_NAME), app->largeFontSize, LARGE_FONT_STYLE); Assert(attachResult == Result_Success);
		
		bakeResult = TryBakeFontAtlas(&newLargeFont, app->largeFontSize, LARGE_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, ArrayCount(basicRanges), &basicRanges[0]);
		if (bakeResult != Result_Success && bakeResult != Result_Partial)
		{
			FreeFont(&newLargeFont);
			FreeFont(&newUiFont);
			return false;
		}
		
		FillFontKerningTable(&newLargeFont);
		RemoveAttachedFontFiles(&newLargeFont);
	}
	
	PigFont newMapFont = ZEROED;
	{
		newMapFont = InitFont(stdHeap, StrLit("mapFont"));
		attachResult = TryAttachOsTtfFileToFont(&newMapFont, StrLit(MAP_FONT_NAME), app->mapFontSize, MAP_FONT_STYLE); Assert(attachResult == Result_Success);
		attachResult = TryAttachLocalFontFile(&newMapFont, StrLit("resources/font/NotoSansJP-Regular.ttf"), MAP_FONT_STYLE); Assert(attachResult == Result_Success);
		attachResult = TryAttachLocalFontFile(&newMapFont, StrLit("resources/font/NotoSansSymbols-Regular.ttf"), MAP_FONT_STYLE); Assert(attachResult == Result_Success);
		
		bakeResult = TryBakeFontAtlas(&newMapFont, app->mapFontSize, MAP_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, ArrayCount(basicRanges), &basicRanges[0]);
		if (bakeResult == Result_Success || bakeResult == Result_Partial)
		{
			FillFontKerningTable(&newMapFont);
			bakeResult = TryBakeFontAtlas(&newMapFont, app->mapFontSize, MAP_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, ArrayCount(kanaRanges), &kanaRanges[0]);
			// if (bakeResult == Result_Success || bakeResult == Result_Partial)
			// {
			// 	if (app->kanjiCodepoints.length > 0)
			// 	{
			// 		ScratchBegin(scratch);
			// 		uxx numKanjiRanges = app->kanjiCodepoints.length;
			// 		FontCharRange* kanjiRanges = AllocArray(FontCharRange, scratch, numKanjiRanges);
			// 		NotNull(kanjiRanges);
			// 		VarArrayLoop(&app->kanjiCodepoints, cIndex)
			// 		{
			// 			VarArrayLoopGetValue(u32, codepoint, &app->kanjiCodepoints, cIndex);
			// 			kanjiRanges[cIndex] = NewFontCharRangeSingle(codepoint);
			// 		}
			// 		bakeResult = TryBakeFontAtlas(&newMapFont, app->mapFontSize, MAP_FONT_STYLE, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, numKanjiRanges, kanjiRanges);
			// 		ScratchEnd(scratch);
			// 	}
			// }
		}
		if (bakeResult != Result_Success && bakeResult != Result_Partial)
		{
			FreeFont(&newMapFont);
			FreeFont(&newLargeFont);
			FreeFont(&newUiFont);
			return false;
		}
		
		MakeFontActive(&newMapFont, MIN_FONT_ATLAS_SIZE, MAX_FONT_ATLAS_SIZE, FONT_MAX_NUM_ATLASES, FONT_AUTO_EVICT_GLYPH_TIME, FONT_AUTO_EVICT_ATLAS_TIME);
	}
	
	if (app->uiFont.arena != nullptr) { FreeFont(&app->uiFont); }
	if (app->largeFont.arena != nullptr) { FreeFont(&app->largeFont); }
	if (app->mapFont.arena != nullptr) { FreeFont(&app->mapFont); }
	app->uiFont = newUiFont;
	app->largeFont = newLargeFont;
	app->mapFont = newMapFont;
	
	return true;
}

bool AppChangeFontSize(bool increase)
{
	if (increase)
	{
		app->uiFontSize += 1;
		app->uiScale = app->uiFontSize / (r32)DEFAULT_UI_FONT_SIZE;
		app->largeFontSize = RoundR32(DEFAULT_LARGE_FONT_SIZE * app->uiScale);
		app->mapFontSize = RoundR32(DEFAULT_MAP_FONT_SIZE * app->uiScale);
		if (!AppCreateFonts())
		{
			app->uiFontSize -= 1;
			app->uiScale = app->uiFontSize / (r32)DEFAULT_UI_FONT_SIZE;
			app->largeFontSize = RoundR32(DEFAULT_LARGE_FONT_SIZE * app->uiScale);
			app->mapFontSize = RoundR32(DEFAULT_MAP_FONT_SIZE * app->uiScale);
		}
		return true;
	}
	else if (AreSimilarOrGreaterR32(app->uiFontSize - 1.0f, MIN_UI_FONT_SIZE, DEFAULT_R32_TOLERANCE))
	{
		app->uiFontSize -= 1;
		app->uiScale = app->uiFontSize / (r32)DEFAULT_UI_FONT_SIZE;
		app->largeFontSize = RoundR32(DEFAULT_LARGE_FONT_SIZE * app->uiScale);
		app->mapFontSize = RoundR32(DEFAULT_MAP_FONT_SIZE * app->uiScale);
		AppCreateFonts();
		return true;
	}
	else { return false; }
}

recd GetMapScreenRec(MapView* view)
{
	recd result = view->mapRec;
	result.TopLeft = SubV2d(result.TopLeft, view->position);
	result = ScaleRecd(result, view->zoom);
	
	rec mainViewportRec = GetClayElementDrawRecNt("MainViewport");
	if (mainViewportRec.Width > 0 && mainViewportRec.Height > 0)
	{
		result.TopLeft = AddV2d(result.TopLeft, ToV2dFromf(AddV2(mainViewportRec.TopLeft, ShrinkV2(mainViewportRec.Size, 2.0f))));
	}
	
	return result;
}

void SetMapItemSelected(OsmMap* map, OsmPrimitiveType type, void* itemPntr, bool selected)
{
	NotNull(map);
	NotNull(itemPntr);
	u64 itemId = 0;
	if (type == OsmPrimitiveType_Node) { itemId = ((OsmNode*)itemPntr)->id; }
	else if (type == OsmPrimitiveType_Way) { itemId = ((OsmWay*)itemPntr)->id; }
	else { Assert(false); }
	
	bool isAlreadySelected = false;
	uxx selectedItemIndex = 0;
	VarArrayLoop(&map->selectedItems, sIndex)
	{
		VarArrayLoopGet(OsmSelectedItem, selectedItem, &map->selectedItems, sIndex);
		if (selectedItem->type == type && selectedItem->id == itemId)
		{
			isAlreadySelected = true;
			selectedItemIndex = sIndex;
			break;
		}
	}
	
	if (selected)
	{
		if (isAlreadySelected) { return; }
		OsmSelectedItem* newSelectedItem = VarArrayAdd(OsmSelectedItem, &map->selectedItems);
		NotNull(newSelectedItem);
		ClearPointer(newSelectedItem);
		newSelectedItem->type = type;
		newSelectedItem->id = itemId;
		newSelectedItem->pntr = itemPntr;
		if (type == OsmPrimitiveType_Node) { ((OsmNode*)itemPntr)->isSelected = true; }
		else if (type == OsmPrimitiveType_Way) { ((OsmWay*)itemPntr)->isSelected = true; }
		else { Assert(false); }
	}
	else
	{
		if (!isAlreadySelected) { return; }
		if (type == OsmPrimitiveType_Node) { ((OsmNode*)itemPntr)->isSelected = false; }
		else if (type == OsmPrimitiveType_Way) { ((OsmWay*)itemPntr)->isSelected = false; }
		else { Assert(false); }
		VarArrayRemoveAt(OsmSelectedItem, &map->selectedItems, selectedItemIndex);
	}
}
void SetMapNodeSelected(OsmMap* map, OsmNode* node, bool selected) { SetMapItemSelected(map, OsmPrimitiveType_Node, (void*)node, selected); }
void SetMapWaySelected(OsmMap* map, OsmWay* way, bool selected) { SetMapItemSelected(map, OsmPrimitiveType_Way, (void*)way, selected); }

void ClearMapSelection(OsmMap* map)
{
	VarArrayLoop(&map->selectedItems, sIndex)
	{
		VarArrayLoopGet(OsmSelectedItem, selectedItem, &map->selectedItems, sIndex);
		if (selectedItem->type == OsmPrimitiveType_Node) { selectedItem->nodePntr->isSelected = false; }
		else if (selectedItem->type == OsmPrimitiveType_Way) { selectedItem->wayPntr->isSelected = false; }
	}
	VarArrayClear(&map->selectedItems);
}

void FindInternationalCodepointsInMapNames(OsmMap* map, VarArray* codepointsOut)
{
	VarArrayClear(codepointsOut);
	VarArrayLoop(&map->nodes, nIndex)
	{
		VarArrayLoopGet(OsmNode, node, &map->nodes, nIndex);
		Str8 nameStr = GetOsmNodeTagValue(node, StrLit("name:ja"), Str8_Empty);
		if (IsEmptyStr(nameStr)) { nameStr = GetOsmNodeTagValue(node, StrLit("name"), Str8_Empty); }
		for (uxx bIndex = 0; bIndex < nameStr.length; bIndex++)
		{
			u32 codepoint = 0;
			u8 codepointSize = GetCodepointForUtf8Str(nameStr, bIndex, &codepoint);
			if (codepointSize == 0) { continue; }
			if (codepointSize > 1)
			{
				bool alreadyFound = false;
				VarArrayLoop(codepointsOut, cIndex)
				{
					VarArrayLoopGetValue(u32, knownCodepoint, codepointsOut, cIndex);
					if (knownCodepoint == codepoint) { alreadyFound = true; break; }
				}
				if (!alreadyFound)
				{
					VarArrayAddValue(u32, codepointsOut, codepoint);
				}
			}
			if (codepointSize > 1) { bIndex += codepointSize-1; }
		}
	}
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
		TracyCZoneEnd(_OsOpenFile);
		if (openedSelectedFile)
		{
			PrintLine_I("Opened binary \"%.*s\"", StrPrint(filePath));
			DataStream fileStream = ToDataStreamFromFile(&pbfFile);
			parseResult = TryParsePbfMap(stdHeap, &fileStream, mapOut);
			OsCloseFile(&pbfFile);
			if (parseResult != Result_Success) { NotifyPrint_E("Failed to parse as OpenStreetMaps Protobuf data! Error: %s", GetResultStr(parseResult)); }
		}
		else { NotifyPrint_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
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
			if (parseResult != Result_Success) { NotifyPrint_E("Failed to parse as OpenStreetMaps XML data! Error: %s", GetResultStr(parseResult)); }
		}
		else { NotifyPrint_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
	}
	else
	{
		NotifyPrint_E("Unknown file extension \"%.*s\", expected .osm or .pbf files!", StrPrint(GetFileNamePart(filePath, true)));
		parseResult = Result_UnsupportedFileFormat;
	}
	
	ScratchEnd(scratch);
	return parseResult;
}

void OpenOsmMap(FilePath filePath, bool addToMap)
{
	TracyCZoneN(funcZone, "OpenOsmMap", true);
	ScratchBegin(scratch);
	
	OsmMap newMap = ZEROED;
	Result parseResult = TryParseMapFile(scratch, filePath, &newMap);
	if (parseResult == Result_Success)
	{
		PrintLine_I("Parsed map! %llu node%s, %llu way%s, %llu relation%s",
			newMap.nodes.length, Plural(newMap.nodes.length, "s"),
			newMap.ways.length, Plural(newMap.ways.length, "s"),
			newMap.relations.length, Plural(newMap.relations.length, "s")
		);
		
		if (addToMap && app->map.arena != nullptr)
		{
			OsmAddFromMap(&app->map, &newMap);
			FreeOsmMap(&newMap);
		}
		else
		{
			FreeStr8(stdHeap, &app->mapFilePath);
			FreeOsmMap(&app->map);
			MyMemCopy(&app->map, &newMap, sizeof(OsmMap));
			app->mapFilePath = AllocStr8(stdHeap, filePath);
		}
		AppRememberRecentFile(filePath);
		
		v2d boundsOnMapTopLeft = MapProject(app->view.projection, app->map.bounds.TopLeft, app->view.mapRec);
		v2d boundsOnMapBottomRight = MapProject(app->view.projection, AddV2d(app->map.bounds.TopLeft, app->map.bounds.Size), app->view.mapRec);
		recd boundsOnMap = NewRecdBetweenV(boundsOnMapTopLeft, boundsOnMapBottomRight);
		app->view.position = AddV2d(boundsOnMap.TopLeft, ShrinkV2d(boundsOnMap.Size, 2.0));
		rec mainViewportRec = GetClayElementDrawRecNt("MainViewport");
		if (boundsOnMap.Width > 0 && boundsOnMap.Height > 0 && mainViewportRec.Width > 0)
		{
			app->view.zoom = MinR64(
				mainViewportRec.Width / boundsOnMap.Width,
				mainViewportRec.Height / boundsOnMap.Height
			);
		}
		
		TracyCZoneN(_FindInternationalCodepoints, "FindInternationalCodepoints", true);
		FindInternationalCodepointsInMapNames(&app->map, &app->kanjiCodepoints);
		TracyCZoneEnd(_FindInternationalCodepoints);
		
		//TODO: This is taking like 40-50ms now. We should really work on changing how we display international codepoints
		TracyCZoneN(_CreatingFonts, "CreatingFonts", true);
		bool fontBakeSuccess = AppCreateFonts();
		TracyCZoneEnd(_CreatingFonts);
		
		Assert(fontBakeSuccess);
	}
	
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
}

bool SaveOsmMap(FilePath filePath)
{
	ScratchBegin(scratch);
	bool result = false;
	if (StrAnyCaseEndsWith(filePath, StrLit(".osm")))
	{
		Str8 xmlFileContents = SerializeOsmMap(scratch, &app->map);
		if (!IsEmptyStr(xmlFileContents))
		{
			if (OsWriteTextFile(filePath, xmlFileContents))
			{
				NotifyPrint_I("Successfully saved %llu byte XML to \"%.*s\"", xmlFileContents.length, StrPrint(filePath));
				result = true;
			}
			else { NotifyPrint_E("Failed to write %llu byte XML to \"%.*s\"!", xmlFileContents.length, StrPrint(filePath)); }
		}
		else { Notify_E("Failed to serialize map to OpenStreetMap XML format!"); }
	}
	// else if (StrAnyCaseEndsWith(filePath, StrLit(".pbf")))
	// {
	// 	Unimplemented(); //TODO: Implement me!
	// }
	else
	{
		Str8 fileName = GetFileNamePart(filePath, true);
		NotifyPrint_E("Unsupported extension \"%.*s\"", StrPrint(fileName));
	}
	ScratchEnd(scratch);
	return result;
}

void UpdateOsmWayColorChoice(OsmWay* way)
{
	if (!way->colorsChosen)
	{
		// PrintLine_D("Choosing color for way[%llu]...", way->id);
		way->colorsChosen = true;
		way->borderThickness = 0.0f;
		way->borderColor = Transparent;
		
		if (way->isClosedLoop)
		{
			way->fillColor = NewColorU32(0x80FF00FF);
			way->renderLayer = OsmRenderLayer_Bottom;
			if (way->tags.length == 0) { way->fillColor = Transparent; } //TODO: We should see if there are any relations referencing this way and maybe color this way based off that
			else
			{
				Str8 landuseStr = GetOsmWayTagValue(way, StrLit("landuse"), Str8_Empty);
				if (StrAnyCaseEquals(landuseStr, StrLit("retail"))) { way->fillColor = CartoFillRetail; way->borderThickness = 1.0f; way->borderColor = CartoBorderRetail; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("residential"))) { way->fillColor = CartoFillResidential; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("commercial"))) { way->fillColor = CartoFillCommercial; way->borderThickness = 1.0f; way->borderColor = CartoBorderCommercial; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("forest"))) { way->fillColor = CartoFillForest; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("railway")) ||
					StrAnyCaseEquals(landuseStr, StrLit("industrial"))) { way->fillColor = CartoFillIndustrial; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("religious"))) { way->fillColor = CartoFillReligious; way->borderThickness = 1.0f; way->borderColor = CartoBorderReligious; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("cemetery"))) { way->fillColor = CartoFillCemetery; }
				else if (StrAnyCaseEquals(landuseStr, StrLit("grass")) ||
					StrAnyCaseEquals(landuseStr, StrLit("flowerbed"))) { way->fillColor = CartoFillGrass; }
				else
				{
					Str8 leisureStr = GetOsmWayTagValue(way, StrLit("leisure"), Str8_Empty);
					if (StrAnyCaseEquals(leisureStr, StrLit("park"))) { way->fillColor = CartoFillPark; }
					else if (StrAnyCaseEquals(leisureStr, StrLit("playground")) ||
						StrAnyCaseEquals(landuseStr, StrLit("recreation_ground"))) { way->fillColor = CartoFillPlayground; }
					else if (StrAnyCaseEquals(leisureStr, StrLit("sports_centre"))) { way->renderLayer = OsmRenderLayer_Top; way->fillColor = CartoFillPlayground; }
					else if (StrAnyCaseEquals(leisureStr, StrLit("pitch"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillSports; }
					else if (StrAnyCaseEquals(leisureStr, StrLit("marina"))) { way->fillColor = CartoFillWater; }
					else if (StrAnyCaseEquals(leisureStr, StrLit("swimming_pool"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillWater; way->borderThickness = 1.0f; way->borderColor = CartoBorderWater; }
					else if (StrAnyCaseEquals(leisureStr, StrLit("garden"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillGrass; }
					{
						Str8 buildingStr = GetOsmWayTagValue(way, StrLit("building"), Str8_Empty);
						if (StrAnyCaseEquals(buildingStr, StrLit("yes")) ||
							StrAnyCaseEquals(buildingStr, StrLit("apartments")) ||
							StrAnyCaseEquals(buildingStr, StrLit("residential")) ||
							StrAnyCaseEquals(buildingStr, StrLit("public")) ||
							StrAnyCaseEquals(buildingStr, StrLit("office")) ||
							StrAnyCaseEquals(buildingStr, StrLit("hotel")) ||
							StrAnyCaseEquals(buildingStr, StrLit("school")) ||
							StrAnyCaseEquals(buildingStr, StrLit("house")) ||
							StrAnyCaseEquals(buildingStr, StrLit("commercial")) ||
							StrAnyCaseEquals(buildingStr, StrLit("kindergarten")) ||
							StrAnyCaseEquals(buildingStr, StrLit("service")) ||
							StrAnyCaseEquals(buildingStr, StrLit("bridge")) ||
							StrAnyCaseEquals(buildingStr, StrLit("roof"))) { way->renderLayer = OsmRenderLayer_Top; way->fillColor = CartoFillBuilding; way->borderThickness = 2.0f; way->borderColor = CartoBorderBuilding; }
						else if (StrAnyCaseEquals(buildingStr, StrLit("garage"))) { way->renderLayer = OsmRenderLayer_Top; way->fillColor = CartoFillParking; way->borderThickness = 1.0f; way->borderColor = CartoBorderParking; }
						else if (StrAnyCaseEquals(buildingStr, StrLit("train_station"))) { way->renderLayer = OsmRenderLayer_Top; way->fillColor = CartoFillDarkerBuilding; way->borderThickness = 1.0f; way->borderColor = CartoBorderDarkerBuilding; }
						else if (StrAnyCaseEquals(buildingStr, StrLit("retail"))) { way->renderLayer = OsmRenderLayer_Top; way->fillColor = CartoFillRetailBuilding; way->borderThickness = 1.0f; way->borderColor = CartoBorderRetailBuilding; }
						// else if (StrAnyCaseEquals(buildingStr, StrLit("train_station"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillPublicTransit; }
						else
						{
							Str8 amenityStr = GetOsmWayTagValue(way, StrLit("amenity"), Str8_Empty);
							if (StrAnyCaseEquals(amenityStr, StrLit("school"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillSchool; }
							else if (StrAnyCaseEquals(amenityStr, StrLit("parking"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillParking; way->borderThickness = 1.0f; way->borderColor = CartoBorderParking; }
							else if (StrAnyCaseEquals(amenityStr, StrLit("place_of_worship"))) { way->fillColor = CartoFillReligious; way->borderThickness = 1.0f; way->borderColor = CartoBorderReligious; }
							else
							{
								Str8 waterStr = GetOsmWayTagValue(way, StrLit("water"), Str8_Empty);
								Str8 waterwayStr = GetOsmWayTagValue(way, StrLit("waterway"), Str8_Empty);
								Str8 naturalStr = GetOsmWayTagValue(way, StrLit("natural"), Str8_Empty);
								if (StrAnyCaseEquals(waterStr, StrLit("lake")) ||
									StrAnyCaseEquals(waterStr, StrLit("river")) ||
									StrAnyCaseEquals(waterStr, StrLit("pond")) ||
									StrAnyCaseEquals(naturalStr, StrLit("water"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillWater; }
								else if (StrAnyCaseEquals(waterwayStr, StrLit("stream"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillWater; }
								else if (StrAnyCaseEquals(naturalStr, StrLit("scrub"))) { way->fillColor = CartoFillGrass; }
								else if (StrAnyCaseEquals(naturalStr, StrLit("wood"))) { way->fillColor = CartoFillForest; }
								else
								{
									Str8 demolishedBuildingStr = GetOsmWayTagValue(way, StrLit("demolished:building"), Str8_Empty);
									Str8 buildingPartStr = GetOsmWayTagValue(way, StrLit("building:part"), Str8_Empty);
									Str8 wasBuildingStr = GetOsmWayTagValue(way, StrLit("was:building"), Str8_Empty);
									if (StrAnyCaseEquals(demolishedBuildingStr, StrLit("yes"))) { way->fillColor = Transparent; }
									else if (StrAnyCaseEquals(wasBuildingStr, StrLit("yes"))) { way->fillColor = Transparent; }
									else if (!IsEmptyStr(buildingPartStr)) { way->fillColor = Transparent; }
									else
									{
										Str8 railwayStr = GetOsmWayTagValue(way, StrLit("railway"), Str8_Empty);
										Str8 manMadeStr = GetOsmWayTagValue(way, StrLit("man_made"), Str8_Empty);
										if (StrAnyCaseEquals(railwayStr, StrLit("platform"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillPublicTransit; way->borderThickness = 1.0f; way->borderColor = CartoBorderPublicTransit; }
										else if (StrAnyCaseEquals(manMadeStr, StrLit("bridge"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillBridge; }
										else
										{
											if (!IsEmptyStr(GetOsmWayTagValue(way, StrLit("highway"), Str8_Empty)) ||
												!IsEmptyStr(GetOsmWayTagValue(way, StrLit("barrier"), Str8_Empty))) { way->isClosedLoop = false; }
										}
									}
								}
							}
						}
					}
				}
			}
			
			if (way->isClosedLoop)
			{
				#if 0
				if (way->fillColor.valueU32 == 0x80FF00FF)
				{
					PrintLine_D("Way[%llu] failed to choose a fillColor with %llu tags", way->id, way->tags.length);
					VarArrayLoop(&way->tags, tIndex)
					{
						VarArrayLoopGet(OsmTag, tag, &way->tags, tIndex);
						PrintLine_D("\tTag[%llu] \"%.*s\" = \"%.*s\"", tIndex, StrPrint(tag->key), StrPrint(tag->value));
					}
				}
				#endif
				
				Str8 colorStr = GetOsmWayTagValue(way, StrLit("color"), Str8_Empty);
				if (IsEmptyStr(colorStr)) { colorStr = GetOsmWayTagValue(way, StrLit("colour"), Str8_Empty); }
				if (!IsEmptyStr(colorStr))
				{
					TryParseColor(colorStr, &way->fillColor, nullptr);
				}
				else
				{
					VarArrayLoop(&way->relationPntrs, rIndex)
					{
						VarArrayLoopGetValue(OsmRelation*, relation, &way->relationPntrs, rIndex);
						Str8 relationColorStr = GetOsmRelationTagValue(relation, StrLit("color"), Str8_Empty);
						if (IsEmptyStr(relationColorStr)) { relationColorStr = GetOsmRelationTagValue(relation, StrLit("colour"), Str8_Empty); }
						if (!IsEmptyStr(relationColorStr))
						{
							TryParseColor(relationColorStr, &way->fillColor, nullptr);
						}
					}
				}
			}
		}
		
		if (!way->isClosedLoop)
		{
			way->renderLayer = OsmRenderLayer_Top;
			way->fillColor = Black;
			way->lineThickness = 1.0f;
			Str8 highwayStr = GetOsmWayTagValue(way, StrLit("highway"), Str8_Empty);
			if (StrAnyCaseEquals(highwayStr, StrLit("trunk"))) { way->fillColor = CartoStrokeTrunk; way->lineThickness = 5.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("tertiary"))) { way->fillColor = CartoStrokeRoad; way->lineThickness = 3.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("residential")) ||
				StrAnyCaseEquals(highwayStr, StrLit("unclassified"))) { way->fillColor = CartoStrokeRoad; way->lineThickness = 2.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("service"))) { way->fillColor = CartoStrokeRoad; way->lineThickness = 1.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("secondary"))) { way->fillColor = CartoStrokeSecondary; way->lineThickness = 3.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("path")) ||
				StrAnyCaseEquals(highwayStr, StrLit("footway"))) { way->fillColor = CartoStrokePath; way->lineThickness = 1.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("cycleway"))) { way->fillColor = CartoStrokeCycleway; way->lineThickness = 1.0f; }
			else if (StrAnyCaseEquals(highwayStr, StrLit("track"))) { way->fillColor = CartoStrokeTrack; way->lineThickness = 2.0f; }
			else
			{
				Str8 railwayStr = GetOsmWayTagValue(way, StrLit("railway"), Str8_Empty);
				Str8 waterwayStr = GetOsmWayTagValue(way, StrLit("waterway"), Str8_Empty);
				Str8 barrierStr = GetOsmWayTagValue(way, StrLit("barrier"), Str8_Empty);
				if (StrAnyCaseEquals(waterwayStr, StrLit("stream"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoFillWater; way->lineThickness = 2.0f; }
				else if (StrAnyCaseEquals(barrierStr, StrLit("hedge"))) { way->fillColor = CartoStrokeHedge; way->lineThickness = 3.0f; }
				else if (StrAnyCaseEquals(barrierStr, StrLit("fence"))) { way->fillColor = CartoStrokeFence; way->lineThickness = 1.0f; }
				else if (StrAnyCaseEquals(railwayStr, StrLit("rail"))) { way->renderLayer = OsmRenderLayer_Middle; way->fillColor = CartoStrokeRail; way->lineThickness = 2.0f; }
				else
				{
					Str8 powerStr = GetOsmWayTagValue(way, StrLit("power"), Str8_Empty);
					if (StrAnyCaseEquals(powerStr, StrLit("line"))) { way->renderLayer = OsmRenderLayer_Top; way->fillColor = CartoStrokePowerline; way->lineThickness = 1.0f; }
					
				}
			}
			
			Str8 thicknessStr = GetOsmWayTagValue(way, StrLit("thickness"), Str8_Empty);
			if (!IsEmptyStr(thicknessStr)) { TryParseR32(thicknessStr, &way->lineThickness, nullptr); }
			Str8 colorStr = GetOsmWayTagValue(way, StrLit("color"), Str8_Empty);
			if (IsEmptyStr(colorStr)) { colorStr = GetOsmWayTagValue(way, StrLit("colour"), Str8_Empty); }
			if (!IsEmptyStr(colorStr))
			{
				TryParseColor(colorStr, &way->fillColor, nullptr);
			}
			else
			{
				VarArrayLoop(&way->relationPntrs, rIndex)
				{
					VarArrayLoopGetValue(OsmRelation*, relation, &way->relationPntrs, rIndex);
					Str8 relationColorStr = GetOsmRelationTagValue(relation, StrLit("color"), Str8_Empty);
					if (IsEmptyStr(relationColorStr)) { relationColorStr = GetOsmRelationTagValue(relation, StrLit("colour"), Str8_Empty); }
					if (!IsEmptyStr(relationColorStr))
					{
						TryParseColor(relationColorStr, &way->fillColor, nullptr);
					}
				}
			}
		}
	}
}

void UpdateOsmWayTriangulation(OsmMap* map, OsmWay* way)
{
	if (!way->isClosedLoop) { return; }
	if (way->triIndices == nullptr && !way->attemptedTriangulation)
	{
		TracyCZoneN(_TriangulatingWay, "TriangulatingWay", true);
		ScratchBegin(scratch);
		way->attemptedTriangulation = true;
		
		uxx numPolygonVerts = way->nodes.length;
		v2d* polygonVerts = AllocArray(v2d, scratch, numPolygonVerts);
		VarArrayLoop(&way->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
			if (nodeRef->pntr == nullptr) { way->attemptedTriangulation = true; TracyCZoneEnd(_TriangulatingWay); return; }
			polygonVerts[nIndex] = nodeRef->pntr->location;
		}
		// PrintLine_D("Triangulating way %llu (%llu nodes)", way->id, way->nodes.length);
		way->triIndices = Triangulate2DEarClipR64(map->arena, numPolygonVerts, polygonVerts, &way->numTriIndices);
		if (way->triIndices == nullptr)
		{
			//Reverse the way, try triangulating it with reverse winding
			for (uxx vIndex = 0; vIndex < numPolygonVerts/2; vIndex++)
			{
				SwapValues(v2d, polygonVerts[vIndex], polygonVerts[numPolygonVerts-1 - vIndex]);
			}
			// PrintLine_D("Triangulating reversed way %llu (%llu nodes)", way->id, way->nodes.length);
			way->triIndices = Triangulate2DEarClipR64(map->arena, numPolygonVerts, polygonVerts, &way->numTriIndices);
			if (way->triIndices == nullptr) { PrintLine_W("Failed to triangulate way %llu (%llu nodes)", way->id, way->nodes.length); }
		}
		
		if (way->triIndices != nullptr && way->numTriIndices > 0)
		{
			// PrintLine_D("Triangulated way[%llu]...", wayIndex);
			uxx numBufferVertices = way->numTriIndices;
			Vertex2D* bufferVertices = AllocArray(Vertex2D, scratch, numBufferVertices);
			NotNull(bufferVertices);
			for (uxx iIndex = 0; iIndex+3 <= way->numTriIndices; iIndex += 3)
			{
				//NOTE: We need to change the winding order of all triangles to account for the Y-axis flip above and Triangle2DEarClip giving us clockwise triangles in this flipped coordinates system
				for (uxx tIndex = 0; tIndex < 3; tIndex++)
				{
					uxx offset = tIndex;
					if (offset == 1) { offset = 2; }
					else if (offset == 2) { offset = 1; }
					uxx vertIndex = way->triIndices[iIndex + offset];
					v2 normalizedPosition = NewV2(
						(r32)InverseLerpClampR64(way->nodeBounds.Lon, way->nodeBounds.Lon + way->nodeBounds.Width, polygonVerts[vertIndex].Lon),
						(r32)InverseLerpClampR64(way->nodeBounds.Lat + way->nodeBounds.Height, way->nodeBounds.Lat, polygonVerts[vertIndex].Lat)
					);
					bufferVertices[iIndex + tIndex] = NewVertex2D(normalizedPosition, normalizedPosition, (v4)V4_One_Const);
				}
			}
			way->triVertBuffer = InitVertBuffer2D(map->arena, StrLit("Way_TriVertBuffer"), VertBufferUsage_Static, numBufferVertices, bufferVertices, false);
		}
		ScratchEnd(scratch);
		TracyCZoneEnd(_TriangulatingWay);
	}
}

void RenderWayLine(OsmWay* way, recd mapScreenRec, r32 thickness, Color32 color)
{
	#if 1
	TracyCZoneN(funcZone, "RenderWayLine", true);
	ScratchBegin(scratch);
	SimpPolygonR64 simpPoly = ZEROED;
	simpPoly.numVertices = way->nodes.length;
	simpPoly.vertices = AllocArray(SimpPolyVertR64, scratch, simpPoly.numVertices);
	VarArrayLoop(&way->nodes, nIndex)
	{
		VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
		if (nodeRef->pntr == nullptr) { ScratchEnd(scratch); TracyCZoneEnd(funcZone); return; }
		simpPoly.vertices[nIndex].state = 0;
		simpPoly.vertices[nIndex].pos = nodeRef->pntr->location;
	}
	r64 epsilonDegrees = ((r64)WAY_SIMPLIFYING_EPSILON_PX / mapScreenRec.Width) * MERCATOR_LONGITUDE_RANGE;
	TracyCZoneN(_SimplifyPolygonR64, "SimplifyPolygonR64", true);
	SimplifyPolygonR64(&simpPoly, epsilonDegrees);
	TracyCZoneEnd(_SimplifyPolygonR64);
	v2d prevPos = V2d_Zero;
	uxx drawIndex = 0;
	for (uxx vIndex = 0; vIndex < simpPoly.numVertices; vIndex++)
	{
		if (simpPoly.vertices[vIndex].state > 0)
		{
			v2d nodePos = MapProject(app->view.projection, simpPoly.vertices[vIndex].pos, mapScreenRec);
			if (drawIndex > 0) { DrawLine(ToV2Fromd(prevPos), ToV2Fromd(nodePos), thickness, color); }
			prevPos = nodePos;
			drawIndex++;
		}
	}
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
	#else
	v2d prevPos = V2d_Zero;
	VarArrayLoop(&way->nodes, nIndex)
	{
		VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
		v2d nodePos = MapProject(app->view.projection, nodeRef->pntr->location, mapScreenRec);
		if (nIndex > 0) { DrawLine(ToV2Fromd(prevPos), ToV2Fromd(nodePos), thickness, color); }
		prevPos = nodePos;
	}
	#endif
}

void RenderWayFilled(OsmWay* way, recd mapScreenRec, rec wayOnScreenBoundsRec, Color32 fillColor, r32 borderThickness, Color32 borderColor)
{
	bool renderedFill = false;
	if (way->triVertBuffer.arena != nullptr && way->triVertBuffer.numVertices > 0)
	{
		mat4 worldMat = Mat4_Identity;
		TransformMat4(&worldMat, Make2DScaleMat4(wayOnScreenBoundsRec.Size));
		TransformMat4(&worldMat, MakeTranslateXYZMat4(wayOnScreenBoundsRec.X, wayOnScreenBoundsRec.Y, gfx.state.depth));
		SetWorldMat(worldMat);
		SetTintColor(fillColor);
		BindTexture(&gfx.pixelTexture);
		SetSourceRec(Rec_Default);
		BindVertBuffer(&way->triVertBuffer);
		DrawVertices();
		renderedFill = true;
	}
	else if (way->numTriIndices > 0 && way->triIndices != nullptr)
	{
		for (uxx iIndex = 0; iIndex < way->numTriIndices; iIndex += 3)
		{
			OsmNodeRef* node0 = VarArrayGet(OsmNodeRef, &way->nodes, way->triIndices[iIndex+0]);
			OsmNodeRef* node1 = VarArrayGet(OsmNodeRef, &way->nodes, way->triIndices[iIndex+1]);
			OsmNodeRef* node2 = VarArrayGet(OsmNodeRef, &way->nodes, way->triIndices[iIndex+2]);
			v2 vert0 = ToV2Fromd(MapProject(app->view.projection, node0->pntr->location, mapScreenRec));
			v2 vert1 = ToV2Fromd(MapProject(app->view.projection, node1->pntr->location, mapScreenRec));
			v2 vert2 = ToV2Fromd(MapProject(app->view.projection, node2->pntr->location, mapScreenRec));
			DrawLine(vert0, vert1, 2.0f, fillColor);
			DrawLine(vert1, vert2, 2.0f, fillColor);
			DrawLine(vert2, vert0, 2.0f, fillColor);
		}
		renderedFill = true;
	}
	if (way->attemptedTriangulation && !renderedFill)
	{
		RenderWayLine(way, mapScreenRec, 2.0f, fillColor);
	}
	else if (borderThickness > 0 && borderColor.a > 0)
	{
		RenderWayLine(way, mapScreenRec, borderThickness, borderColor);
	}
}
