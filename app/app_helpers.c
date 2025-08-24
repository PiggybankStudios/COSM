/*
File:   app_helpers.c
Author: Taylor Robbins
Date:   02\25\2025
Description: 
	** None
*/

ImageData LoadImageData(Arena* arena, const char* path)
{
	ScratchBegin1(scratch, arena);
	Slice fileContents = Slice_Empty;
	Result readFileResult = TryReadAppResource(&app->resources, scratch, FilePathLit(path), false, &fileContents);
	Assert(readFileResult == Result_Success);
	UNUSED(readFileResult);
	ImageData imageData = ZEROED;
	Result parseResult = TryParseImageFile(fileContents, arena, &imageData);
	Assert(parseResult == Result_Success);
	UNUSED(parseResult);
	ScratchEnd(scratch);
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

bool AppCreateFonts()
{
	FontCharRange fontCharRanges[] = {
		FontCharRange_ASCII,
		NewFontCharRangeSingle(UNICODE_ELLIPSIS_CODEPOINT),
		NewFontCharRangeSingle(UNICODE_RIGHT_ARROW_CODEPOINT),
	};
	FontCharRange fontExtendedCharRanges[] = {
		FontCharRange_LatinSupplementAccent,
		FontCharRange_LatinExtA,
	};
	
	PigFont newUiFont = ZEROED;
	{
		newUiFont = InitFont(stdHeap, StrLit("uiFont"));
		Result attachResult = AttachOsTtfFileToFont(&newUiFont, StrLit(UI_FONT_NAME), app->uiFontSize, UI_FONT_STYLE);
		Assert(attachResult == Result_Success);
		
		bool bakedSuccessfully = false;
		i32 atlasSize = MIN_FONT_ATLAS_SIZE;
		while (atlasSize <= MAX_FONT_ATLAS_SIZE)
		{
			Result bakeResult = BakeFontAtlas(&newUiFont, app->uiFontSize, UI_FONT_STYLE, NewV2i(atlasSize, atlasSize), ArrayCount(fontCharRanges), &fontCharRanges[0]);
			if (bakeResult == Result_Success) { bakedSuccessfully = true; break; }
			atlasSize *= 2;
		}
		if (!bakedSuccessfully)
		{
			RemoveAttachedTtfFile(&newUiFont);
			FreeFont(&newUiFont);
			return false;
		}
		
		FillFontKerningTable(&newUiFont);
		RemoveAttachedTtfFile(&newUiFont);
	}
	
	PigFont newLargeFont = ZEROED;
	{
		newLargeFont = InitFont(stdHeap, StrLit("largeFont"));
		Result attachResult = AttachOsTtfFileToFont(&newLargeFont, StrLit(LARGE_FONT_NAME), app->largeFontSize, LARGE_FONT_STYLE);
		Assert(attachResult == Result_Success);
		
		bool bakedSuccessfully = false;
		i32 atlasSize = MIN_FONT_ATLAS_SIZE;
		while (atlasSize <= MAX_FONT_ATLAS_SIZE)
		{
			Result bakeResult = BakeFontAtlas(&newLargeFont, app->largeFontSize, LARGE_FONT_STYLE, NewV2i(atlasSize, atlasSize), ArrayCount(fontCharRanges), &fontCharRanges[0]);
			if (bakeResult == Result_Success) { bakedSuccessfully = true; break; }
			atlasSize *= 2;
		}
		if (!bakedSuccessfully)
		{
			RemoveAttachedTtfFile(&newLargeFont);
			FreeFont(&newLargeFont);
			FreeFont(&newUiFont);
			return false;
		}
		
		FillFontKerningTable(&newLargeFont);
		RemoveAttachedTtfFile(&newLargeFont);
	}
	
	PigFont newMapFont = ZEROED;
	{
		newMapFont = InitFont(stdHeap, StrLit("mapFont"));
		Result attachResult = AttachOsTtfFileToFont(&newMapFont, StrLit(MAP_FONT_NAME), app->mapFontSize, MAP_FONT_STYLE);
		Assert(attachResult == Result_Success);
		
		bool bakedBasicSuccessfully = false;
		bool bakedExtendedSuccessfully = false;
		i32 atlasSize = MIN_FONT_ATLAS_SIZE;
		while (atlasSize <= MAX_FONT_ATLAS_SIZE)
		{
			if (!bakedBasicSuccessfully)
			{
				Result bakeResult = BakeFontAtlas(&newMapFont, app->mapFontSize, MAP_FONT_STYLE, NewV2i(atlasSize, atlasSize), ArrayCount(fontCharRanges), &fontCharRanges[0]);
				if (bakeResult == Result_Success) { bakedBasicSuccessfully = true; }
			}
			if (!bakedExtendedSuccessfully)
			{
				Result bakeResult = BakeFontAtlas(&newMapFont, app->mapFontSize, MAP_FONT_STYLE, NewV2i(atlasSize, atlasSize), ArrayCount(fontExtendedCharRanges), &fontExtendedCharRanges[0]);
				if (bakeResult == Result_Success) { bakedExtendedSuccessfully = true; }
			}
			if (bakedBasicSuccessfully && bakedExtendedSuccessfully) { break; }
			atlasSize *= 2;
		}
		
		FillFontKerningTable(&newMapFont);
		RemoveAttachedTtfFile(&newMapFont);
		
		bool bakedKanjiSuccessfully = true;
		if (app->kanjiCodepoints.length > 0 && bakedBasicSuccessfully && bakedExtendedSuccessfully)
		{
			ScratchBegin(scratch);
			attachResult = AttachOsTtfFileToFont(&newMapFont, StrLit(MAP_ASIAN_FONT_NAME), app->mapFontSize, MAP_FONT_STYLE);
			Assert(attachResult == Result_Success);
			
			uxx numRanges = app->kanjiCodepoints.length;
			FontCharRange* ranges = AllocArray(FontCharRange, scratch, numRanges);
			NotNull(ranges);
			VarArrayLoop(&app->kanjiCodepoints, cIndex)
			{
				VarArrayLoopGetValue(u32, codepoint, &app->kanjiCodepoints, cIndex);
				ranges[cIndex] = NewFontCharRangeSingle(codepoint);
			}
			
			bakedKanjiSuccessfully = false;
			atlasSize = MIN_FONT_ATLAS_SIZE;
			while (atlasSize <= MAX_FONT_ATLAS_SIZE)
			{
				Result bakeResult = BakeFontAtlas(&newMapFont, app->mapFontSize, MAP_FONT_STYLE, NewV2i(atlasSize, atlasSize), numRanges, ranges);
				if (bakeResult == Result_Success) { bakedKanjiSuccessfully = true; break; }
				atlasSize *= 2;
			}
			
			RemoveAttachedTtfFile(&newMapFont);
			ScratchEnd(scratch);
		}
		
		if (!bakedBasicSuccessfully || !bakedExtendedSuccessfully || !bakedKanjiSuccessfully)
		{
			FreeFont(&newMapFont);
			FreeFont(&newLargeFont);
			FreeFont(&newUiFont);
			return false;
		}
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

void FindInternationalCodepointsInMapNames(OsmMap* map, VarArray* codepointsOut)
{
	VarArrayClear(codepointsOut);
	VarArrayLoop(&app->map.nodes, nIndex)
	{
		VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex);
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

void OpenOsmMap(FilePath filePath)
{
	TracyCZoneN(funcZone, "OpenOsmMap", true);
	ScratchBegin(scratch);
	Str8 fileContents = Str8_Empty;
	TracyCZoneN(_ReadTextFile, "OsReadTextFile", true);
	bool openedSelectedFile = OsReadTextFile(filePath, scratch, &fileContents);
	TracyCZoneEnd(_ReadTextFile);
	if (openedSelectedFile)
	{
		PrintLine_I("Opened \"%.*s\", %llu bytes", StrPrint(filePath), fileContents.length);
		OsmMap newMap = ZEROED;
		Result parseResult = TryParseOsmMap(stdHeap, fileContents, &newMap);
		if (parseResult == Result_Success)
		{
			FreeStr8(stdHeap, &app->loadedFilePath);
			FreeOsmMap(&app->map);
			MyMemCopy(&app->map, &newMap, sizeof(OsmMap));
			PrintLine_I("Parsed map! %llu node%s, %llu way%s",
				app->map.nodes.length, Plural(app->map.nodes.length, "s"),
				app->map.ways.length, Plural(app->map.ways.length, "s")
			);
			
			v2d targetLocation = AddV2d(app->map.bounds.TopLeft, ShrinkV2d(app->map.bounds.Size, 2.0));
			app->view.position = MapProject(app->view.projection, targetLocation, NewRecdV(V2d_Zero, app->view.mapRec.Size));
			app->view.zoom = MinR64(
				1.0 / (app->map.bounds.SizeLon / 360.0),
				1.0 / (app->map.bounds.SizeLat / 180.0)
			);
			
			app->loadedFilePath = AllocStr8(stdHeap, filePath);
			FindInternationalCodepointsInMapNames(&app->map, &app->kanjiCodepoints);
			bool fontBakeSuccess = AppCreateFonts();
			Assert(fontBakeSuccess);
		}
		else { PrintLine_E("Failed to parse as OpenStreetMaps XML data! Error: %s", GetResultStr(parseResult)); }
	}
	else { PrintLine_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
}
