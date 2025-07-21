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
		FontCharRange_LatinExt,
		NewFontCharRangeSingle(UNICODE_ELLIPSIS_CODEPOINT),
		NewFontCharRangeSingle(UNICODE_RIGHT_ARROW_CODEPOINT),
	};
	
	PigFont newUiFont = ZEROED;
	{
		newUiFont = InitFont(stdHeap, StrLit("uiFont"));
		Result attachResult = AttachOsTtfFileToFont(&newUiFont, StrLit(UI_FONT_NAME), app->uiFontSize, UI_FONT_STYLE);
		Assert(attachResult == Result_Success);
		UNUSED(attachResult);
		Result bakeResult = BakeFontAtlas(&newUiFont, app->uiFontSize, UI_FONT_STYLE, NewV2i(256, 256), ArrayCount(fontCharRanges), &fontCharRanges[0]);
		if (bakeResult != Result_Success)
		{
			bakeResult = BakeFontAtlas(&newUiFont, app->uiFontSize, UI_FONT_STYLE, NewV2i(512, 512), ArrayCount(fontCharRanges), &fontCharRanges[0]);
			if (bakeResult != Result_Success)
			{
				RemoveAttachedTtfFile(&newUiFont);
				FreeFont(&newUiFont);
				return false;
			}
		}
		Assert(bakeResult == Result_Success);
		FillFontKerningTable(&newUiFont);
		RemoveAttachedTtfFile(&newUiFont);
	}
	
	PigFont newLargeFont = ZEROED;
	{
		newLargeFont = InitFont(stdHeap, StrLit("largeFont"));
		Result attachResult = AttachOsTtfFileToFont(&newLargeFont, StrLit(LARGE_FONT_NAME), app->largeFontSize, LARGE_FONT_STYLE);
		Assert(attachResult == Result_Success);
		UNUSED(attachResult);
		Result bakeResult = BakeFontAtlas(&newLargeFont, app->largeFontSize, LARGE_FONT_STYLE, NewV2i(256, 256), ArrayCount(fontCharRanges), &fontCharRanges[0]);
		if (bakeResult != Result_Success)
		{
			bakeResult = BakeFontAtlas(&newLargeFont, app->largeFontSize, LARGE_FONT_STYLE, NewV2i(512, 512), ArrayCount(fontCharRanges), &fontCharRanges[0]);
			if (bakeResult != Result_Success)
			{
				RemoveAttachedTtfFile(&newLargeFont);
				FreeFont(&newLargeFont);
				FreeFont(&newUiFont);
				return false;
			}
		}
		Assert(bakeResult == Result_Success);
		FillFontKerningTable(&newLargeFont);
		RemoveAttachedTtfFile(&newLargeFont);
	}
	
	if (app->uiFont.arena != nullptr) { FreeFont(&app->uiFont); }
	if (app->largeFont.arena != nullptr) { FreeFont(&app->largeFont); }
	app->uiFont = newUiFont;
	app->largeFont = newLargeFont;
	
	return true;
}

bool AppChangeFontSize(bool increase)
{
	if (increase)
	{
		app->uiFontSize += 1;
		app->uiScale = app->uiFontSize / (r32)DEFAULT_UI_FONT_SIZE;
		app->largeFontSize = RoundR32(DEFAULT_LARGE_FONT_SIZE * app->uiScale);
		if (!AppCreateFonts())
		{
			app->uiFontSize -= 1;
			app->uiScale = app->uiFontSize / (r32)DEFAULT_UI_FONT_SIZE;
			app->largeFontSize = RoundR32(DEFAULT_LARGE_FONT_SIZE * app->uiScale);
		}
		return true;
	}
	else if (AreSimilarOrGreaterR32(app->uiFontSize - 1.0f, MIN_UI_FONT_SIZE, DEFAULT_R32_TOLERANCE))
	{
		app->uiFontSize -= 1;
		app->uiScale = app->uiFontSize / (r32)DEFAULT_UI_FONT_SIZE;
		app->largeFontSize = RoundR32(DEFAULT_LARGE_FONT_SIZE * app->uiScale);
		AppCreateFonts();
		return true;
	}
	else { return false; }
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
			FreeOsmMap(&app->map);
			MyMemCopy(&app->map, &newMap, sizeof(OsmMap));
			PrintLine_I("Parsed map! %llu node%s, %llu way%s",
				app->map.nodes.length, Plural(app->map.nodes.length, "s"),
				app->map.ways.length, Plural(app->map.ways.length, "s")
			);
			
			v2d targetLocation = AddV2d(app->map.bounds.TopLeft, ShrinkV2d(app->map.bounds.Size, 2.0));
			app->viewPos = ProjectMercator(targetLocation, NewRecV(V2_Zero, app->mapRec.Size));
			app->viewZoom = (r32)MinR64(
				1.0 / (app->map.bounds.SizeLon / 360.0),
				1.0 / (app->map.bounds.SizeLat / 180.0)
			);
		}
		else { PrintLine_E("Failed to parse as OpenStreetMaps XML data! Error: %s", GetResultStr(parseResult)); }
	}
	else { PrintLine_E("Failed to open \"%.*s\"", StrPrint(filePath)); }
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
}
