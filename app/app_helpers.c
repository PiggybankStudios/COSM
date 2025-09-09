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
		else { PrintLine_E("Failed to parse create Texture from %dx%d ImageData!", mapBackImageData.size.Width, mapBackImageData.size.Height); }
	}
	else { PrintLine_E("Failed to load map background texture from \"%s\"", MAP_BACKGROUND_TEXTURE_PATH); }
	ScratchEnd(scratch);
	TracyCZoneEnd(funcZone);
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

void OpenOsmMap(FilePath filePath)
{
	TracyCZoneN(funcZone, "OpenOsmMap", true);
	ScratchBegin(scratch);
	
	OsmMap newMap = ZEROED;
	Result parseResult = TryParseMapFile(scratch, filePath, &newMap);
	if (parseResult == Result_Success)
	{
		FreeStr8(stdHeap, &app->mapFilePath);
		FreeOsmMap(&app->map);
		MyMemCopy(&app->map, &newMap, sizeof(OsmMap));
		app->mapFilePath = AllocStr8(stdHeap, filePath);
		PrintLine_I("Parsed map! %llu node%s, %llu way%s",
			app->map.nodes.length, Plural(app->map.nodes.length, "s"),
			app->map.ways.length, Plural(app->map.ways.length, "s")
		);
		AppRememberRecentFile(filePath);
		
		#if 0
		if (!IsVarArraySortedUintMember(OsmNode, id, &app->map.nodes))
		{
			TracyCZoneN(_SortOsmNodes, "SortOsmNodes", true);
			PrintLine_D("Sorting %llu nodes...", app->map.nodes.length);
			// VarArrayLoop(&app->map.nodes, nIndex) { VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex); PrintLine_D("Before Node[%llu]: ID %llu", nIndex, node->id); if (nIndex >= 10) { break; } }
			QuickSortVarArrayUintMember(OsmNode, id, &app->map.nodes);
			app->map.areNodesSorted = true;
			// VarArrayLoop(&app->map.nodes, nIndex) { VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex); PrintLine_D("After Node[%llu]: ID %llu", nIndex, node->id); if (nIndex >= 10) { break; } }
			TracyCZoneEnd(_SortOsmNodes);
			
			
			TracyCZoneN(_FixNodeRefs, "FixNodeRefs", true);
			VarArrayLoop(&app->map.ways, wIndex)
			{
				VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex);
				VarArrayLoop(&way->nodes, nIndex)
				{
					VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
					nodeRef->pntr = FindOsmNode(&app->map, nodeRef->id);
				}
			}
			TracyCZoneEnd(_FixNodeRefs);
		}
		else { PrintLine_D("%llu nodes are already sorted!", app->map.nodes.length); }
		
		TracyCZoneN(_SortOsmWays, "SortOsmWays", true);
		PrintLine_D("Sorting %llu ways...", app->map.ways.length);
		// VarArrayLoop(&app->map.ways, wIndex) { VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex); PrintLine_D("Before Way[%llu]: ID %llu", wIndex, way->id); if (wIndex >= 10) { break; } }
		QuickSortVarArrayIntMember(OsmWay, id, &app->map.ways);
		// VarArrayLoop(&app->map.ways, wIndex) { VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex); PrintLine_D("After Way[%llu]: ID %llu", wIndex, way->id); if (wIndex >= 10) { break; } }
		TracyCZoneEnd(_SortOsmWays);
		#endif
		
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
				if (!IsEmptyStr(colorStr))
				{
					if (TryParseColor(colorStr, &way->fillColor, nullptr)) { way->renderLayer = OsmRenderLayer_Middle; }
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
			if (!IsEmptyStr(colorStr)) { TryParseColor(colorStr, &way->fillColor, nullptr); }
		}
	}
}

void UpdateOsmWayTriangulation(OsmMap* map, OsmWay* way)
{
	if (!way->isClosedLoop) { return; }
	if (way->triIndices == nullptr && !way->attemptedTriangulation)
	{
		ScratchBegin(scratch);
		way->attemptedTriangulation = true;
		
		uxx numPolygonVerts = way->nodes.length;
		v2d* polygonVerts = AllocArray(v2d, scratch, numPolygonVerts);
		VarArrayLoop(&way->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
			polygonVerts[nIndex] = nodeRef->pntr->location;
		}
		way->triIndices = Triangulate2DEarClipR64(map->arena, numPolygonVerts, polygonVerts, &way->numTriIndices);
		if (way->triIndices == nullptr)
		{
			//Reverse the way, try triangulating it with reverse winding
			for (uxx vIndex = 0; vIndex < numPolygonVerts/2; vIndex++)
			{
				SwapValues(v2d, polygonVerts[vIndex], polygonVerts[numPolygonVerts-1 - vIndex]);
			}
			way->triIndices = Triangulate2DEarClipR64(map->arena, numPolygonVerts, polygonVerts, &way->numTriIndices);
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
	}
}

void RenderWayFilled(OsmWay* way, recd mapRec, rec wayOnScreenBoundsRec, Color32 fillColor, r32 borderThickness, Color32 borderColor)
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
			v2 vert0 = ToV2Fromd(MapProject(app->view.projection, node0->pntr->location, mapRec));
			v2 vert1 = ToV2Fromd(MapProject(app->view.projection, node1->pntr->location, mapRec));
			v2 vert2 = ToV2Fromd(MapProject(app->view.projection, node2->pntr->location, mapRec));
			DrawLine(vert0, vert1, 2.0f, fillColor);
			DrawLine(vert1, vert2, 2.0f, fillColor);
			DrawLine(vert2, vert0, 2.0f, fillColor);
		}
		renderedFill = true;
	}
	bool needToRenderOutlineAsFill = (way->attemptedTriangulation && !renderedFill);
	if (needToRenderOutlineAsFill || (borderThickness > 0 && borderColor.a > 0))
	{
		r32 actualBorderThickness = (needToRenderOutlineAsFill) ? 2.0f : borderThickness;
		Color32 actualBorderColor = (needToRenderOutlineAsFill) ? fillColor : borderColor;
		v2d prevPos = V2d_Zero;
		VarArrayLoop(&way->nodes, nIndex)
		{
			VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
			v2d nodePos = MapProject(app->view.projection, nodeRef->pntr->location, mapRec);
			if (nIndex > 0) { DrawLine(ToV2Fromd(prevPos), ToV2Fromd(nodePos), actualBorderThickness, actualBorderColor); }
			prevPos = nodePos;
		}
	}
}
