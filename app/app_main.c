/*
File:   app_main.c
Author: Taylor Robbins
Date:   01\19\2025
Description: 
	** Contains the dll entry point and all exported functions that the platform
	** layer can lookup by name and call. Also includes all other source files
	** required for the application to compile.
*/

#include "build_config.h"
#include "defines.h"
#define PIG_CORE_IMPLEMENTATION BUILD_INTO_SINGLE_UNIT

#define HOXML_ENABLED 1

#include "base/base_all.h"
#include "std/std_all.h"
#include "file_fmt/file_fmt_all.h"
#include "gfx/gfx_all.h"
#include "gfx/gfx_system_global.h"
#include "input/input_all.h"
#include "lib/lib_all.h"
#include "mem/mem_all.h"
#include "misc/misc_all.h"
#include "os/os_all.h"
#include "parse/parse_all.h"
#include "phys/phys_all.h"
#include "struct/struct_all.h"
#include "ui/ui_all.h"

#if HOXML_ENABLED
#if COMPILER_IS_MSVC
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#define HOXML_IMPLEMENTATION
#include "third_party/hoxml/hoxml.h"
#if COMPILER_IS_MSVC
#pragma warning(pop)
#endif
#endif

// +--------------------------------------------------------------+
// |                         Header Files                         |
// +--------------------------------------------------------------+
#include "osm_pbf.pb-c.h"
#include "parse_xml.h"
#include "platform_interface.h"
#include "app_resources.h"
#include "map_projections.h"
#include "osm_carto.h"
#include "osm_map.h"
#include "app_main.h"

// +--------------------------------------------------------------+
// |                           Globals                            |
// +--------------------------------------------------------------+
static AppData* app = nullptr;
static AppInput* appIn = nullptr;
static Arena* uiArena = nullptr;

#if !BUILD_INTO_SINGLE_UNIT //NOTE: The platform layer already has these globals
static PlatformInfo* platformInfo = nullptr;
static PlatformApi* platform = nullptr;
static Arena* stdHeap = nullptr;
#endif

// +--------------------------------------------------------------+
// |                         Source Files                         |
// +--------------------------------------------------------------+
#include "osm_pbf.pb-c.c"
#include "parse_xml.c"
#include "main2d_shader.glsl.h"
#include "app_resources.c"
#include "osm_map.c"
#include "osm_map_serialization_osm.c"
#include "osm_map_serialization_pbf.c"
#include "app_clay_helpers.c"
#include "app_recent_files.c"
#include "app_helpers.c"
#include "map_tiles.c"
#include "map_view.c"
#include "app_clay.c"

// +==============================+
// |           DllMain            |
// +==============================+
#if (TARGET_IS_WINDOWS && !BUILD_INTO_SINGLE_UNIT)
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL, // handle to DLL module
	DWORD fdwReason,    // reason for calling function
	LPVOID lpReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpReserved);
	switch(fdwReason)
	{ 
		case DLL_PROCESS_ATTACH: break;
		case DLL_PROCESS_DETACH: break;
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
		default: break;
	}
	//If we don't return TRUE here then the LoadLibraryA call will return a failure!
	return TRUE;
}
#endif //(TARGET_IS_WINDOWS && !BUILD_INTO_SINGLE_UNIT)

void UpdateDllGlobals(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr, AppInput* appInput)
{
	#if !BUILD_INTO_SINGLE_UNIT
	platformInfo = inPlatformInfo;
	platform = inPlatformApi;
	stdHeap = inPlatformInfo->platformStdHeap;
	#else
	UNUSED(inPlatformApi);
	UNUSED(inPlatformInfo);
	#endif
	app = (AppData*)memoryPntr;
	appIn = appInput;
}

// +==============================+
// |           AppInit            |
// +==============================+
// void* AppInit(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi)
EXPORT_FUNC APP_INIT_DEF(AppInit)
{
	#if !BUILD_INTO_SINGLE_UNIT
	InitScratchArenasVirtual(Gigabytes(4));
	#endif
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	
	AppData* appData = AllocType(AppData, inPlatformInfo->platformStdHeap);
	ClearPointer(appData);
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, (void*)appData, nullptr);
	
	InitAppResources(&app->resources);
	
	#if BUILD_WITH_SOKOL_APP
	platform->SetWindowTitle(StrLit(PROJECT_READABLE_NAME_STR));
	LoadWindowIcon();
	#endif
	
	InitRandomSeriesDefault(&app->random);
	SeedRandomSeriesU64(&app->random, OsGetCurrentTimestamp(false));
	
	OsInitHttpRequestManager(stdHeap, &app->httpManager);
	
	InitCompiledShader(&app->mainShader, stdHeap, main2d);
	LoadMapBackTexture();
	
	InitVarArray(RecentFile, &app->recentFiles, stdHeap);
	AppLoadRecentFilesList();
	
	InitVarArray(u32, &app->kanjiCodepoints, stdHeap);
	app->uiFontSize = DEFAULT_UI_FONT_SIZE;
	app->largeFontSize = DEFAULT_LARGE_FONT_SIZE;
	app->mapFontSize = DEFAULT_MAP_FONT_SIZE;
	app->uiScale = 1.0f;
	bool fontBakeSuccess = AppCreateFonts();
	Assert(fontBakeSuccess);
	UNUSED(fontBakeSuccess);
	
	InitClayUIRenderer(stdHeap, V2_Zero, &app->clay);
	app->clayUiFontId = AddClayUIRendererFont(&app->clay, &app->uiFont, UI_FONT_STYLE);
	app->clayLargeFontId = AddClayUIRendererFont(&app->clay, &app->largeFont, LARGE_FONT_STYLE);
	
	InitUiResizableSplit(stdHeap, StrLit("SidebarSplit"), true, 0, 0.20f, &app->sidebarSplit);
	InitUiResizableSplit(stdHeap, StrLit("InfoLayersSplit"), false, 0, 0.50f, &app->infoLayersSplit);
	
	InitMapTiles();
	InitMapView(&app->view, MapProjection_Mercator);
	
	#if 0
	OpenOsmMap(StrLit(TEST_OSM_FILE), false);
	#else
	bool wasCmdPathGiven = false;
	//NOTE: Not really sure if we need to handle multiple argument paths being passed.
	// I guess if the first one fails doesn't point to a real file we can
	// open secondary one(s) but that really isn't super intuitive behavior
	uxx argIndex = 0;
	Str8 pathArgument = GetNamelessProgramArg(platformInfo->programArgs, argIndex);
	while (!IsEmptyStr(pathArgument))
	{
		wasCmdPathGiven = true;
		if (OsDoesFileExist(pathArgument))
		{
			OpenOsmMap(pathArgument, false);
			break;
		}
		else
		{
			PrintLine_E("Command line path does not point to a file: \"%.*s\"", StrPrint(pathArgument));
		}
		
		argIndex++;
		pathArgument = GetNamelessProgramArg(platformInfo->programArgs, argIndex);
	}
	if (!wasCmdPathGiven && app->recentFiles.length > 0)
	{
		RecentFile* mostRecentFile = VarArrayGetLast(RecentFile, &app->recentFiles);
		OpenOsmMap(mostRecentFile->path, false);
	}
	#endif
	
	app->initialized = true;
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	return (void*)app;
}

// +==============================+
// |       AppBeforeReload        |
// +==============================+
// bool AppBeforeReload(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr)
EXPORT_FUNC APP_BEFORE_RELOAD_DEF(AppBeforeReload)
{
	bool shouldReload = true;
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, nullptr);
	
	WriteLine_W("App is preparing for DLL reload...");
	//TODO: Anything that needs to be saved before the DLL reload should be done here
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	return shouldReload;
}

// +==============================+
// |        AppAfterReload        |
// +==============================+
// void AppAfterReload(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr)
EXPORT_FUNC APP_AFTER_RELOAD_DEF(AppAfterReload)
{
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, nullptr);
	
	WriteLine_I("New app DLL was loaded!");
	if (!StrExactEquals(app->mapBackTexturePath, StrLit(MAP_BACKGROUND_TEXTURE_PATH)))
	{
		PrintLine_W("Loading background texture from \"%s\" (was \"%.*s\")", MAP_BACKGROUND_TEXTURE_PATH, StrPrint(app->mapBackTexturePath));
		LoadMapBackTexture();
	}
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
}

// +==============================+
// |          AppUpdate           |
// +==============================+
// bool AppUpdate(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr, AppInput* appInput)
EXPORT_FUNC APP_UPDATE_DEF(AppUpdate)
{
	TracyCZoneN(funcZone, "AppUpdate", true);
	
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	bool renderedFrame = true;
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, appInput);
	v2i screenSizei = appIn->screenSize;
	v2 screenSize = ToV2Fromi(appIn->screenSize);
	// v2 screenCenter = Div(screenSize, 2.0f);
	v2 mousePos = appIn->mouse.position;
	bool isMouseOverMainViewport = IsMouseOverClay(CLAY_ID("MainViewport"));
	bool isOverDisplayLimit = (app->map.nodes.length > DISPLAY_NODE_COUNT_LIMIT || app->map.ways.length > DISPLAY_WAY_COUNT_LIMIT);
	bool isHoveringMapPrimitive = false;
	FontNewFrame(&app->uiFont, appIn->programTime);
	FontNewFrame(&app->mapFont, appIn->programTime);
	FontNewFrame(&app->largeFont, appIn->programTime);
	
	TracyCZoneN(Zone_Update, "Update", true);
	// +==============================+
	// |            Update            |
	// +==============================+
	{
		UpdateMapTiles();
		OsUpdateHttpRequestManager(&app->httpManager, appIn->programTime);
		
		// +==============================+
		// |  Check Recent Files Changed  |
		// +==============================+
		if (app->recentFilesSaveWatch.arena != nullptr)
		{
			OsUpdateFileWatch(&app->recentFilesSaveWatch, appIn->programTime);
			if (app->recentFilesSaveWatch.change != OsFileWatchChange_None && TimeSinceBy(appIn->programTime, app->recentFilesSaveWatch.lastChangeTime) >= RECENT_FILES_RELOAD_DELAY)
			{
				OsResetFileWatch(&app->recentFilesSaveWatch, appIn->programTime);
				AppLoadRecentFilesList();
			}
		}
		
		// +==============================+
		// |     Handle Dropped Files     |
		// +==============================+
		//TODO: Handle multiple drops as opening 1 and then adding the rest on top
		if (appIn->droppedFilePaths.length == 1)
		{
			Str8 droppedFilePath = VarArrayGetFirstValue(Str8, &appIn->droppedFilePaths);
			PrintLine_I("Opening dropped file: \"%.*s\"", StrPrint(droppedFilePath));
			OpenOsmMap(droppedFilePath, false);
			isOverDisplayLimit = (app->map.nodes.length > DISPLAY_NODE_COUNT_LIMIT || app->map.ways.length > DISPLAY_WAY_COUNT_LIMIT);
		}
		
		// +==================================+
		// | Space Centered Selected Item(s)  |
		// +==================================+
		if (IsKeyboardKeyPressed(&appIn->keyboard, Key_Space, true) && app->map.selectedItems.length > 0)
		{
			v2d averageLocation = V2d_Zero;
			uxx averageCount = 0;
			VarArrayLoop(&app->map.selectedItems, sIndex)
			{
				VarArrayLoopGet(OsmSelectedItem, selectedItem, &app->map.selectedItems, sIndex);
				if (selectedItem->type == OsmPrimitiveType_Node)
				{
					averageLocation = AddV2d(averageLocation, selectedItem->nodePntr->location);
					averageCount++;
				}
				else if (selectedItem->type == OsmPrimitiveType_Way)
				{
					//TODO: This isn't a great way to find the center of an arbitrary polygon! Sides with more vertices are weighted heavier. Should we find the center of mass?
					v2d wayAverageLocation = V2d_Zero;
					uxx wayAverageCount = 0;
					VarArrayLoop(&selectedItem->wayPntr->nodes, nIndex)
					{
						VarArrayLoopGet(OsmNodeRef, nodeRef, &selectedItem->wayPntr->nodes, nIndex);
						wayAverageLocation = AddV2d(wayAverageLocation, nodeRef->pntr->location);
						wayAverageCount++;
					}
					wayAverageLocation = ShrinkV2d(wayAverageLocation, (r64)wayAverageCount);
					averageLocation = AddV2d(averageLocation, wayAverageLocation);
					averageCount++;
				}
			}
			averageLocation = ShrinkV2d(averageLocation, (r64)averageCount);
			app->view.position = MapProject(app->view.projection, averageLocation, app->view.mapRec);
		}
		
		// +==================================+
		// | Handle Ctrl+Plus/Minus/0/Scroll  |
		// +==================================+
		if (IsKeyboardKeyPressed(&appIn->keyboard, Key_Plus, true) && IsKeyboardKeyDown(&appIn->keyboard, Key_Control))
		{
			AppChangeFontSize(true);
		}
		if (IsKeyboardKeyPressed(&appIn->keyboard, Key_Minus, true) && IsKeyboardKeyDown(&appIn->keyboard, Key_Control))
		{
			AppChangeFontSize(false);
		}
		if (IsKeyboardKeyPressed(&appIn->keyboard, Key_0, true) && IsKeyboardKeyDown(&appIn->keyboard, Key_Control))
		{
			app->uiFontSize = DEFAULT_UI_FONT_SIZE;
			app->uiScale = 1.0f;
			bool fontBakeSuccess = AppCreateFonts();
			Assert(fontBakeSuccess);
		}
		if (IsKeyboardKeyDown(&appIn->keyboard, Key_Control) && appIn->mouse.scrollDelta.Y != 0)
		{
			AppChangeFontSize(appIn->mouse.scrollDelta.Y > 0);
		}
		
		// +==================================+
		// | Ctrl+Shift+O Open TEST_OSM_FILE  |
		// +==================================+
		if (IsKeyboardKeyDown(&appIn->keyboard, Key_Control) && IsKeyboardKeyDown(&appIn->keyboard, Key_Shift) && IsKeyboardKeyPressed(&appIn->keyboard, Key_O, false))
		{
			OpenOsmMap(StrLit(TEST_OSM_FILE), false);
			isOverDisplayLimit = (app->map.nodes.length > DISPLAY_NODE_COUNT_LIMIT || app->map.ways.length > DISPLAY_WAY_COUNT_LIMIT);
		}
		
		// +==============================+
		// |      Handle Key_Escape       |
		// +==============================+
		if (IsKeyboardKeyPressed(&appIn->keyboard, Key_Escape, true))
		{
			if (app->map.selectedItems.length > 0)
			{
				ClearMapSelection(&app->map);
			}
			else
			{
				//TODO: Anything else?
			}
		}
		
		// +====================================+
		// | Update Hover and Handle Selection  |
		// +====================================+
		if (app->map.arena != nullptr && !isOverDisplayLimit)
		{
			if (!isMouseOverMainViewport)
			{
				VarArrayLoop(&app->map.nodes, nIndex) { VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex); node->isHovered = false; }
				VarArrayLoop(&app->map.ways, wIndex) { VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex); way->isHovered = false; }
				// if (IsMouseBtnPressed(&appIn->mouse, MouseBtn_Left))
				// {
				// 	if (!IsKeyboardKeyDown(&appIn->keyboard, Key_Shift) && !IsKeyboardKeyDown(&appIn->keyboard, Key_Control)) { ClearMapSelection(&app->map); }
				// }
			}
			else
			{
				recd screenMapRec = GetMapScreenRec(&app->view);
				v2d mouseLocation = MapUnproject(app->view.projection, ToV2dFromf(appIn->mouse.position), screenMapRec);
				
				OsmNode* closestNode = nullptr;
				r64 closestNodeDistanceSqr = 0.0f;
				VarArrayLoop(&app->map.nodes, nIndex)
				{
					VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex);
					node->isHovered = false;
					r64 nodeDistanceSqr = LengthSquaredV2d(SubV2d(mouseLocation, node->location));
					if (closestNode == nullptr || nodeDistanceSqr < closestNodeDistanceSqr)
					{
						closestNode = node;
						closestNodeDistanceSqr = nodeDistanceSqr;
					}
				}
				
				OsmWay* closestWay = nullptr;
				v2d closestWayPoint = V2d_Zero;
				#if 0
				r64 closestWayDistanceSqr = 0.0f;
				//TODO; This doesn't seem to be working super well
				VarArrayLoop(&app->map.ways, wIndex)
				{
					VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex);
					way->isHovered = false;
					if (IsInsideRecd(way->nodeBounds, mouseLocation))
					{
						for (uxx nIndex = 1; nIndex < way->nodes.length; nIndex++)
						{
							OsmNodeRef* node1 = VarArrayGet(OsmNodeRef, &way->nodes, nIndex-1);
							OsmNodeRef* node2 = VarArrayGet(OsmNodeRef, &way->nodes, nIndex);
							Line2DR64 line = NewLine2DR64V(node1->pntr->location, node2->pntr->location);
							v2d closestPoint = V2d_Zero;
							r64 distanceToLine = DistanceToLine2DR64(line, mouseLocation, &closestPoint);
							if (closestWay == nullptr || distanceToLine*distanceToLine < closestWayDistanceSqr)
							{
								closestWay = way;
								closestWayDistanceSqr = distanceToLine*distanceToLine;
								closestWayPoint = closestPoint;
							}
						}
					}
				}
				if (closestWayDistanceSqr < closestNodeDistanceSqr) { closestNode = nullptr; }
				#else
				VarArrayLoop(&app->map.ways, wIndex) { VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex); way->isHovered = false; }
				#endif
				
				OsmNode* hoveredNode = nullptr;
				OsmWay* hoveredWay = nullptr;
				if (closestNode != nullptr)
				{
					v2 nodePosOnScreen = ToV2Fromd(MapProject(app->view.projection, closestNode->location, screenMapRec));
					if (LengthSquaredV2(SubV2(nodePosOnScreen, appIn->mouse.position)) < 10*10)
					{
						hoveredNode = closestNode;
						hoveredNode->isHovered = true;
					}
				}
				else if (closestWay != nullptr)
				{
					v2 pointPosOnScreen = ToV2Fromd(MapProject(app->view.projection, closestWayPoint, screenMapRec));
					if (LengthSquaredV2(SubV2(pointPosOnScreen, appIn->mouse.position)) < 10*10)
					{
						hoveredWay = closestWay;
						hoveredWay->isHovered = true;
					}
				}
				
				// Check if the node is part of a way and hover that way instead
				if (hoveredNode != nullptr)
				{
					OsmWay* nodeWay = nullptr;
					VarArrayLoop(&app->map.ways, wIndex)
					{
						VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex);
						VarArrayLoop(&way->nodes, nIndex)
						{
							VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
							if (nodeRef->id == hoveredNode->id)
							{
								nodeWay = way;
								break;
							}
						}
					}
					
					if (nodeWay != nullptr)
					{
						hoveredNode->isHovered = false;
						nodeWay->isHovered = true;
						hoveredNode = nullptr;
						hoveredWay = nodeWay;
					}
				}
				
				isHoveringMapPrimitive = (hoveredWay != nullptr || hoveredNode != nullptr);
				
				if (IsMouseBtnPressed(&appIn->mouse, MouseBtn_Left))
				{
					if (!IsKeyboardKeyDown(&appIn->keyboard, Key_Shift) && !IsKeyboardKeyDown(&appIn->keyboard, Key_Control)) { ClearMapSelection(&app->map); }
					if (hoveredNode != nullptr)
					{
						SetMapNodeSelected(&app->map, hoveredNode, IsKeyboardKeyDown(&appIn->keyboard, Key_Control) ? !hoveredNode->isSelected : true);
					}
					else if (hoveredWay != nullptr)
					{
						SetMapWaySelected(&app->map, hoveredWay, IsKeyboardKeyDown(&appIn->keyboard, Key_Control) ? !hoveredWay->isSelected : true);
					}
				}
			}
		}
		
		// +================================================+
		// | Ctrl+R Refreshes Way Colors and Triangulation  |
		// +================================================+
		if (IsKeyboardKeyDown(&appIn->keyboard, Key_Control) && IsKeyboardKeyPressed(&appIn->keyboard, Key_R, false) && !isOverDisplayLimit)
		{
			VarArrayLoop(&app->map.ways, wIndex)
			{
				VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex);
				if (way->triVertBuffer.arena != nullptr)
				{
					FreeVertBuffer(&way->triVertBuffer);
				}
				if (way->triIndices != nullptr)
				{
					FreeArray(uxx, app->map.arena, way->numTriIndices, way->triIndices);
					way->triIndices = nullptr;
					way->numTriIndices = 0;
				}
				way->attemptedTriangulation = false;
				way->colorsChosen = false;
			}
		}
		
		// +==============================+
		// |   Alt+R Reloads Background   |
		// +==============================+
		if (IsKeyboardKeyDown(&appIn->keyboard, Key_Alt) && IsKeyboardKeyPressed(&appIn->keyboard, Key_R, false))
		{
			PrintLine_W("Loading background texture from \"%s\" (was \"%.*s\")", MAP_BACKGROUND_TEXTURE_PATH, StrPrint(app->mapBackTexturePath));
			LoadMapBackTexture();
		}
		
		UpdateMapView(&app->view, isMouseOverMainViewport, &appIn->mouse, &appIn->keyboard);
	}
	TracyCZoneEnd(Zone_Update);
	
	// +==============================+
	// |          Rendering           |
	// +==============================+
	TracyCZoneN(Zone_BeginFrame, "BeginFrame", true);
	BeginFrame(platform->GetSokolSwapchain(), screenSizei, CartoFillBackground, 1.0f);
	TracyCZoneEnd(Zone_BeginFrame);
	TracyCZoneN(Zone_Render, "Render", true);
	{
		BindShader(&app->mainShader);
		ClearDepthBuffer(1.0f);
		SetDepth(1.0f);
		mat4 projMat = Mat4_Identity;
		TransformMat4(&projMat, MakeScaleXYZMat4(1.0f/(screenSize.Width/2.0f), 1.0f/(screenSize.Height/2.0f), 1.0f));
		TransformMat4(&projMat, MakeTranslateXYZMat4(-1.0f, -1.0f, 0.0f));
		TransformMat4(&projMat, MakeScaleYMat4(-1.0f));
		SetProjectionMat(projMat);
		SetViewMat(Mat4_Identity);
		
		uiArena = scratch3;
		FlagSet(uiArena->flags, ArenaFlag_DontPop);
		uxx uiArenaMark = ArenaGetMark(uiArena);
		UiWidgetContext uiContext = NewUiWidgetContext(uiArena, &app->clay, &appIn->keyboard, &appIn->mouse, app->uiScale, &app->uiFocusedElement, CursorShape_Default, platform->GetNativeWindowHandle());
		
		// +==============================+
		// |          Render Map          |
		// +==============================+
		rec mainViewportRec = GetClayElementDrawRecNt("MainViewport");
		i32 tileLevelZ = 0;
		i32 tileGridSize = 1;
		r64 tileLongSize = 360.0;
		if (mainViewportRec.Width > 0 && mainViewportRec.Height > 0)
		{
			app->view.minZoom = MinR64(mainViewportRec.Width / app->view.mapRec.Width, mainViewportRec.Height / app->view.mapRec.Height);
			if (app->view.zoom == 0.0) { app->view.zoom = app->view.minZoom; }
			recd mapScreenRec = GetMapScreenRec(&app->view);
			while (tileLevelZ < MAX_MAP_TILE_DEPTH && mapScreenRec.Width * (tileLongSize / MERCATOR_LONGITUDE_RANGE) > MAP_TILE_IMAGE_SIZE) { tileLevelZ++; tileLongSize /= 2.0; tileGridSize *= 2; }
			
			v2d viewportLocationTopLeft = MapUnproject(app->view.projection, ToV2dFromf(mainViewportRec.TopLeft), mapScreenRec);
			v2d viewportLocationBottomRight = MapUnproject(app->view.projection, ToV2dFromf(AddV2(mainViewportRec.TopLeft, mainViewportRec.Size)), mapScreenRec);
			RangeR64 viewableLongitude = NewRangeR64(viewportLocationTopLeft.Longitude, viewportLocationBottomRight.Longitude);
			RangeR64 viewableLatitude = NewRangeR64(viewportLocationTopLeft.Latitude, viewportLocationBottomRight.Latitude);
			
			v2 backSize = ToV2Fromi(app->mapBackTexture.size);
			rec backSourceRec = NewRec(
				0 * backSize.Width, 0.0f * backSize.Height,
				1 * backSize.Width, 1.0f * backSize.Height
			);
			DrawTexturedRectangleEx(ToRecFromd(mapScreenRec), White, &app->mapBackTexture, backSourceRec);
			
			// +==============================+
			// |       Render Map Tiles       |
			// +==============================+
			if (app->renderTiles)
			{
				const Color32 tileColor = {.r=100, .g=100, .b=100, .a=255};
				v2 tileScreenSize = V2_Zero;
				tileScreenSize.Width = (r32)(mapScreenRec.Width/(r64)tileGridSize);
				tileScreenSize.Height = (r32)(mapScreenRec.Height/(r64)tileGridSize);
				i32 minTileX = ClampI32((i32)FloorR64i(((mainViewportRec.X - mapScreenRec.X) / mapScreenRec.Width) * (r64)tileGridSize), 0, tileGridSize-1);
				i32 minTileY = ClampI32((i32)FloorR64i(((mainViewportRec.Y - mapScreenRec.Y) / mapScreenRec.Height) * (r64)tileGridSize), 0, tileGridSize-1);
				TracyCZoneN(_RenderTiles, "RenderTiles", true);
				for (i32 tileY = minTileY; tileY < tileGridSize; tileY++)
				{
					r32 tileScreenY = (r32)(mapScreenRec.Y + ((r64)tileScreenSize.Height * tileY));
					if (tileScreenY >= screenSize.Height) { break; }
					
					for (i32 tileX = minTileX; tileX < tileGridSize; tileX++)
					{
						r32 tileScreenX = (r32)(mapScreenRec.X + ((r64)tileScreenSize.Width * tileX));
						if (tileScreenX >= screenSize.Width) { break; }
						
						rec tileRec = NewRec(tileScreenX, tileScreenY, tileScreenSize.Width, tileScreenSize.Height);
						if (tileRec.X + tileRec.Width > 0 && tileRec.Y + tileRec.Height > 0)
						{
							bool isMouseHovered = (isMouseOverMainViewport && IsInsideRec(tileRec, appIn->mouse.position) && !isHoveringMapPrimitive);
							bool shouldDownload = (isMouseHovered && IsMouseBtnPressed(&appIn->mouse, MouseBtn_Left) && IsKeyboardKeyDown(&appIn->keyboard, Key_Control));
							TracyCZoneN(_GetMapTileTexture, "GetMapTileTexture", true);
							Texture* tileTexture = GetMapTileTexture(NewV3i(tileX, tileY, tileLevelZ), true, shouldDownload);
							TracyCZoneEnd(_GetMapTileTexture);
							bool hadDesiredTile = (tileTexture != nullptr);
							if (tileTexture != nullptr)
							{
								TracyCZoneN(_DrawTexturedRectangle, "DrawTexturedRectangle", true);
								DrawTexturedRectangle(tileRec, tileColor, tileTexture);
								TracyCZoneEnd(_DrawTexturedRectangle);
							}
							else
							{
								//If we don't have the actual tile we want to render, then lets walk up the layers and find a tile that we can render a portion from
								i32 upperLevelZ = tileLevelZ;
								i32 upperGridSize = tileGridSize;
								v2i upperTileCoord = NewV2i(tileX, tileY);
								i32 innerLevelZ = 0;
								i32 innerGridSize = 1;
								v2i innerTileCoord = V2i_Zero;
								TracyCZoneN(_FindUpperTile, "FindUpperTile", true);
								while (upperLevelZ > 0 && tileTexture == nullptr)
								{
									upperLevelZ--;
									upperGridSize /= 2;
									innerLevelZ++;
									innerGridSize *= 2;
									if ((upperTileCoord.X % 2) != 0) { innerTileCoord.X += innerGridSize/2; }
									if ((upperTileCoord.Y % 2) != 0) { innerTileCoord.Y += innerGridSize/2; }
									upperTileCoord.X /= 2;
									upperTileCoord.Y /= 2;
									tileTexture = GetMapTileTexture(NewV3i(upperTileCoord.X, upperTileCoord.Y, upperLevelZ), true, false);
								}
								TracyCZoneEnd(_FindUpperTile);
								if (tileTexture != nullptr)
								{
									rec sourceRec = NewRec(
										innerTileCoord.X * (256.0f / (r32)innerGridSize),
										innerTileCoord.Y * (256.0f / (r32)innerGridSize),
										(256.0f / (r32)innerGridSize), (256.0f / (r32)innerGridSize)
									);
									TracyCZoneN(_DrawTexturedRectangle2, "DrawTexturedRectangle2", true);
									DrawTexturedRectangleEx(tileRec, tileColor, tileTexture, sourceRec);
									TracyCZoneEnd(_DrawTexturedRectangle2);
								}
							}
							
							if (!hadDesiredTile && isMouseHovered && IsKeyboardKeyDown(&appIn->keyboard, Key_Control))
							{
								DrawRectangleOutlineEx(tileRec, 2, ColorWithAlpha(MonokaiPurple, 0.5f), false);
								DrawRectangle(tileRec, ColorWithAlpha(MonokaiPurple, 0.25f));
							}
						}
					}
				}
				TracyCZoneEnd(_RenderTiles);
			}
			
			DrawRectangleOutlineEx(ToRecFromd(mapScreenRec), 4.0f, MonokaiPurple, false);
			
			#if 0
			if (app->map.arena != nullptr && IsMouseBtnPressed(&appIn->mouse, MouseBtn_Right))
			{
				v2d clickedLocation = MapUnproject(app->view.projection, ToV2dFromf(appIn->mouse.position), mapScreenRec);
				OsmNode* newNode = AddOsmNode(&app->map, clickedLocation, 0);
				OsmTag* newTag1 = VarArrayAdd(OsmTag, &newNode->tags); NotNull(newTag1); newTag1->key = AllocStr8Nt(app->map.arena, "name"); newTag1->value = AllocStr8Nt(app->map.arena, "Mouse");
				OsmTag* newTag2 = VarArrayAdd(OsmTag, &newNode->tags); NotNull(newTag2); newTag2->key = AllocStr8Nt(app->map.arena, "population"); newTag2->value = AllocStr8Nt(app->map.arena, "1000000");
			}
			#endif
			
			#if 0
			if (app->mapFont.arena != nullptr)
			{
				v2 atlasPos = NewV2(10, 60);
				VarArrayLoop(&app->mapFont.atlases, aIndex)
				{
					VarArrayLoopGet(FontAtlas, atlas, &app->mapFont.atlases, aIndex);
					rec atlasRec = NewRec(atlasPos.X, atlasPos.Y, (r32)atlas->texture.Width, (r32)atlas->texture.Height);
					DrawTexturedRectangle(atlasRec, White, &atlas->texture);
					DrawRectangleOutline(atlasRec, 1.0f, MonokaiYellow);
					atlasPos.X += atlasRec.Width + 10;
				}
			}
			#endif
			
			// +==============================+
			// |         Render Ways          |
			// +==============================+
			if (!isOverDisplayLimit)
			{
				TracyCZoneN(_RenderWays, "RenderWays", true);
				for (uxx lIndex = 1; lIndex < OsmRenderLayer_Count; lIndex++)
				{
					OsmRenderLayer currentLayer = (OsmRenderLayer)lIndex;
					VarArrayLoop(&app->map.ways, wIndex)
					{
						VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex);
						UpdateOsmWayColorChoice(way);
						if (way->renderLayer == currentLayer && way->colorsChosen &&
							way->nodeBounds.Lon <= viewableLongitude.max && way->nodeBounds.Lat <= viewableLatitude.max &&
							way->nodeBounds.Lon + way->nodeBounds.SizeLon >= viewableLongitude.min && way->nodeBounds.Lat + way->nodeBounds.SizeLat >= viewableLatitude.min)
						{
							if (way->fillColor.a > 0 || (way->isClosedLoop && way->borderThickness > 0.0f && way->borderColor.a > 0))
							{
								if (way->isClosedLoop)
								{
									v2 boundsTopLeft = ToV2Fromd(MapProject(app->view.projection, way->nodeBounds.TopLeft, mapScreenRec));
									v2 boundsBottomRight = ToV2Fromd(MapProject(app->view.projection, AddV2d(way->nodeBounds.TopLeft, way->nodeBounds.Size), mapScreenRec));
									rec boundsRec = NewRecBetweenV(boundsTopLeft, boundsBottomRight);
									if (boundsRec.Width * boundsRec.Height < 50)
									{
										Color32 smallColor = (way->borderThickness > 0.0f && way->borderColor.a > 0) ? way->borderColor : way->fillColor;
										#if 0
										r32 radius = LengthV2(boundsRec.Size) / 2.0f;
										DrawCircle(NewCircleV(AddV2(boundsRec.TopLeft, ShrinkV2(boundsRec.Size, 2)), radius), smallColor);
										#else
										DrawRectangle(boundsRec, smallColor);
										#endif
									}
									else
									{
										UpdateOsmWayTriangulation(&app->map, way);
										Color32 fillColor = way->isSelected ? MonokaiGreen : (way->isHovered ? ColorLerpSimple(way->fillColor, MonokaiOrange, 0.2f) : way->fillColor);
										Color32 borderColor = (way->isSelected || way->isHovered) ? Transparent : way->borderColor;
										RenderWayFilled(way, mapScreenRec, boundsRec, fillColor, way->borderThickness, borderColor);
									}
								}
								
								if (!way->isClosedLoop && way->lineThickness > 0.0f)
								{
									RenderWayLine(way, mapScreenRec, way->lineThickness, way->fillColor);
								}
							}
						}
						
						if (currentLayer == OsmRenderLayer_Selection && (way->isSelected || way->isHovered))
						{
							Color32 borderColor = (way->isSelected ? CartoTextGreen : CartoTextOrange);
							RenderWayLine(way, mapScreenRec, 2.0f, borderColor);
						}
					}
				}
				TracyCZoneEnd(_RenderWays);
			}
			
			// +==============================+
			// |         Render Nodes         |
			// +==============================+
			if (!isOverDisplayLimit && true)
			{
				TracyCZoneN(_RenderNodes, "RenderNodes", true);
				VarArrayLoop(&app->map.nodes, nIndex)
				{
					VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex);
					if (node->location.Lon <= viewableLongitude.max && node->location.Lat <= viewableLatitude.max &&
						node->location.Lon >= viewableLongitude.min && node->location.Lat >= viewableLatitude.min)
					{
						Str8 populationStr = GetOsmNodeTagValue(node, StrLit("population"), Str8_Empty);
						Str8 railwayStr = GetOsmNodeTagValue(node, StrLit("railway"), Str8_Empty);
						u64 population = 0; TryParseU64(populationStr, &population, nullptr);
						r32 populationLerp = InverseLerpClampR32(Thousand(50), Thousand(500), (r32)population);
						r32 radius = (!IsEmptyStr(populationStr)) ? LerpR32(1.0, 10.0f, populationLerp) : 0.0f;
						if (radius == 0.0f && StrAnyCaseEquals(railwayStr, StrLit("stop"))) { radius = 5.0f; }
						if (app->map.ways.length == 0 && radius == 0.0f) { radius = 1.0f; } //Show all nodes when now ways were found
						if (app->renderNodes && radius == 0.0f && node->wayPntrs.length == 0) { radius = 1.0f; } //Show nodes that aren't part of ways
						
						Str8 radiusStr = GetOsmNodeTagValue(node, StrLit("radius"), Str8_Empty);
						if (!IsEmptyStr(radiusStr)) { TryParseR32(radiusStr, &radius, nullptr); }
						
						if (node->isSelected) { radius = 3.0f; }
						else if (node->isHovered) { radius = 2.0f; }
						
						if (radius > 0.0f)
						{
							Color32 outlineColor = Transparent;
							Color32 nodeColor = (population < Thousand(50)) ? MonokaiGray2 : ColorLerpSimple(CartoTextOrange, CartoTextGreen, populationLerp);
							Str8 colorStr = GetOsmNodeTagValue(node, StrLit("color"), Str8_Empty);
							if (!IsEmptyStr(colorStr)) { TryParseColor(colorStr, &nodeColor, nullptr); }
							
							if (node->isSelected) { nodeColor = MonokaiGreen; outlineColor = CartoTextGreen; }
							else if (node->isHovered) { nodeColor = MonokaiOrange; outlineColor = CartoTextOrange; }
							
							v2d nodePos = MapProject(app->view.projection, node->location, mapScreenRec);
							AlignV2d(&nodePos);
							if (outlineColor.a > 0) { DrawCircle(NewCircleV(ToV2Fromd(nodePos), radius + 1), outlineColor); }
							DrawCircle(NewCircleV(ToV2Fromd(nodePos), radius), nodeColor);
							
							Str8 japaneseNameStr = GetOsmNodeTagValue(node, StrLit("name:ja"), Str8_Empty);
							if (IsEmptyStr(japaneseNameStr)) { japaneseNameStr = GetOsmNodeTagValue(node, StrLit("name"), Str8_Empty); }
							Str8 englishNameStr = GetOsmNodeTagValue(node, StrLit("name:es"), Str8_Empty);
							if (IsEmptyStr(englishNameStr)) { englishNameStr = GetOsmNodeTagValue(node, StrLit("name:en"), Str8_Empty); }
							
							Color32 textColor = (outlineColor.a > 0) ? outlineColor : nodeColor;
							v2 namePos = AddV2(ToV2Fromd(nodePos), NewV2(0, -(radius + 5)));
							if (!IsEmptyStr(japaneseNameStr))
							{
								TextMeasure nameMeasure = MeasureTextEx(&app->mapFont, app->mapFontSize, MAP_FONT_STYLE, false, 0.0f, japaneseNameStr);
								BindFontEx(&app->mapFont, app->mapFontSize, MAP_FONT_STYLE);
								v2 textPos = AddV2(namePos, NewV2(-nameMeasure.Width/2, 0));
								// DrawText(japaneseNameStr, AddV2(textPos, NewV2(0, 2)), ColorLerpSimple(textColor, Black, 0.75f));
								DrawText(japaneseNameStr, textPos, textColor);
								namePos.Y -= nameMeasure.Height + 5;
							}
							if (!IsEmptyStr(englishNameStr))
							{
								TextMeasure nameMeasure = MeasureTextEx(&app->mapFont, app->mapFontSize, MAP_FONT_STYLE, false, 0.0f, englishNameStr);
								BindFontEx(&app->mapFont, app->mapFontSize, MAP_FONT_STYLE);
								v2 textPos = AddV2(namePos, NewV2(-nameMeasure.Width/2, 0));
								DrawText(englishNameStr, AddV2(textPos, NewV2(0, 2)), ColorLerpSimple(textColor, Black, 0.75f));
								DrawText(englishNameStr, textPos, textColor);
								namePos.Y -= nameMeasure.Height + 5;
							}
						}
					}
				}
				TracyCZoneEnd(_RenderNodes);
			}
			
			v2 boundsTopLeft = ToV2Fromd(MapProject(app->view.projection, app->map.bounds.TopLeft, mapScreenRec));
			v2 boundsBottomRight = ToV2Fromd(MapProject(app->view.projection, AddV2d(app->map.bounds.TopLeft, app->map.bounds.Size), mapScreenRec));
			rec boundsRec = NewRecBetweenV(boundsTopLeft, boundsBottomRight);
			DrawRectangleOutline(boundsRec, 2.0f, MonokaiRed);
			// DrawCircle(NewCircleV(boundsRec.TopLeft, 5), MonokaiRed);
			// DrawCircle(NewCircleV(Add(boundsRec.TopLeft, boundsRec.Size), 5), MonokaiOrange);
			
			// +==============================+
			// |    Render Center Reticle     |
			// +==============================+
			v2 reticleScreenPos = ToV2Fromd(AddV2d(MulV2d(app->view.position, mapScreenRec.Size), mapScreenRec.TopLeft));
			r32 reticleRadius = OscillateBy(appIn->programTime, 3.0f, 6.0f, 1000, 0);
			// DrawRoundedRectangleOutline(NewRecCenteredV(reticleScreenPos, FillV2(reticleRadius*2)), 1.5f, reticleRadius, ColorWithAlpha(Black, 0.3f));
			DrawRectangle(NewRecCentered(reticleScreenPos.X, reticleScreenPos.Y, reticleRadius*2, 1.5f), ColorWithAlpha(Black, 0.3f));
			DrawRectangle(NewRecCentered(reticleScreenPos.X, reticleScreenPos.Y, 1.5f, reticleRadius*2), ColorWithAlpha(Black, 0.3f));
		}
		
		// +==============================+
		// |          Render UI           |
		// +==============================+
		v2 scrollContainerInput = IsKeyboardKeyDown(&appIn->keyboard, Key_Control) ? V2_Zero : appIn->mouse.scrollDelta;
		UpdateClayScrolling(&app->clay.clay, 16.6f, false, scrollContainerInput, false);
		BeginClayUIRender(&app->clay.clay, screenSize, false, mousePos, IsMouseBtnDown(&appIn->mouse, MouseBtn_Left));
		{
			u16 fullscreenBorderThickness = (appIn->isWindowTopmost ? 1 : 0);
			CLAY({ .id = CLAY_ID("FullscreenContainer"),
				.layout = {
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing = { .width = CLAY_SIZING_GROW(0, screenSize.Width), .height = CLAY_SIZING_GROW(0, screenSize.Height) },
					.padding = CLAY_PADDING_ALL(fullscreenBorderThickness)
				},
				.border = {
					.color=SELECTED_BLUE,
					.width=CLAY_BORDER_OUTSIDE(UI_BORDER(fullscreenBorderThickness)),
				},
			})
			{
				// +==============================+
				// |            Topbar            |
				// +==============================+
				Clay_SizingAxis topbarHeight = CLAY_SIZING_FIT(0);
				CLAY({ .id = CLAY_ID("Topbar"),
					.layout = {
						.sizing = {
							.height = topbarHeight,
							.width = CLAY_SIZING_GROW(0),
						},
						.padding = { 0, 0, 0, UI_BORDER(1) },
						.childGap = 2,
						.childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
					},
					.backgroundColor = BACKGROUND_GRAY,
					.border = { .color=OUTLINE_GRAY, .width={ .bottom=UI_BORDER(1) } },
				})
				{
					bool showMenuHotkeys = IsKeyboardKeyDown(&appIn->keyboard, Key_Alt);
					
					// +==============================+
					// |          File Menu           |
					// +==============================+
					if (ClayTopBtn("File", showMenuHotkeys, &app->isFileMenuOpen, &app->keepFileMenuOpenUntilMouseOver, app->isOpenRecentSubmenuOpen))
					{
						bool isAdding = IsKeyboardKeyDown(&appIn->keyboard, Key_Shift);
						if (ClayBtn(isAdding ? "Add" UNICODE_ELLIPSIS_STR : "Open" UNICODE_ELLIPSIS_STR, "Ctrl+O", true, nullptr))
						{
							FilePath selectedFilePath = FilePath_Empty;
							TracyCZoneN(Zone_OsOpenFileDialog, "OsDoOpenFileDialog", true);
							Result openResult = OsDoOpenFileDialog(scratch, &selectedFilePath);
							TracyCZoneEnd(Zone_OsOpenFileDialog);
							if (openResult == Result_Success)
							{
								OpenOsmMap(selectedFilePath, isAdding);
								isOverDisplayLimit = (app->map.nodes.length > DISPLAY_NODE_COUNT_LIMIT || app->map.ways.length > DISPLAY_WAY_COUNT_LIMIT);
							}
							else if (openResult != Result_Canceled) { PrintLine_E("OpenFileDialog failed: %s", GetResultStr(openResult)); }
						} Clay__CloseElement();
						
						if (app->recentFiles.length > 0)
						{
							if (ClayTopSubmenu("Open Recent >", app->isFileMenuOpen, &app->isOpenRecentSubmenuOpen, &app->keepOpenRecentSubmenuOpenUntilMouseOver, nullptr))
							{
								for (uxx rIndex = app->recentFiles.length; rIndex > 0; rIndex--)
								{
									RecentFile* recentFile = VarArrayGet(RecentFile, &app->recentFiles, rIndex-1);
									Str8 displayPath = GetUniqueRecentFilePath(recentFile->path);
									bool isOpenFile = StrAnyCaseEquals(app->mapFilePath, recentFile->path);
									Str8 displayStr = isAdding ? JoinStringsInArena(uiArena, StrLit("Add "), displayPath, false) : AllocStr8(uiArena, displayPath);
									if (ClayBtnStrEx(recentFile->path, displayStr, StrLit(""), !isOpenFile && recentFile->fileExists, nullptr))
									{
										OpenOsmMap(recentFile->path, isAdding);
										isOverDisplayLimit = (app->map.nodes.length > DISPLAY_NODE_COUNT_LIMIT || app->map.ways.length > DISPLAY_WAY_COUNT_LIMIT);
										app->isOpenRecentSubmenuOpen = false;
										app->isFileMenuOpen = false;
									} Clay__CloseElement();
								}
								
								if (ClayBtn("Clear Recent Files", "", app->recentFiles.length > 0, nullptr))
								{
									#if 0
									OpenPopupDialog(stdHeap, &app->popup,
										ScratchPrintStr("Are you sure you want to clear all %llu recent file entr%s", app->recentFiles.length, PluralEx(app->recentFiles.length, "y", "ies")),
										AppClearRecentFilesPopupCallback, nullptr
									);
									AddPopupButton(&app->popup, 1, StrLit("Cancel"), PopupDialogResult_No, TEXT_GRAY);
									AddPopupButton(&app->popup, 2, StrLit("Delete"), PopupDialogResult_Yes, ERROR_RED);
									#else
									AppClearRecentFiles();
									AppSaveRecentFilesList();
									#endif
									app->isOpenRecentSubmenuOpen = false;
									app->isFileMenuOpen = false;
								} Clay__CloseElement();
								
								Clay__CloseElement();
								Clay__CloseElement();
							} Clay__CloseElement();
						}
						else
						{
							if (ClayBtn("Open Recent >", "", false, nullptr)) { } Clay__CloseElement();
						}
						
						if (ClayBtn("Save As...", "Ctrl+Shift+S", (app->map.arena != nullptr), nullptr))
						{
							Str8Pair extensions[] = {
								{ StrLit("All Files"), StrLit("*.*") },
								{ StrLit("OpenStreetMap XML"), StrLit("*.osm") },
								{ StrLit("OpenStreetMap Protobuf"), StrLit("*.osm.pbf") },
							};
							FilePath saveFilePath = FilePath_Empty;
							Result dialogResult = OsDoSaveFileDialog(ArrayCount(extensions), &extensions[0], 1, scratch, &saveFilePath);
							if (dialogResult == Result_Success)
							{
								if (SaveOsmMap(saveFilePath))
								{
									FreeStr8(stdHeap, &app->mapFilePath);
									app->mapFilePath = AllocStr8(stdHeap, saveFilePath);
								}
							}
						} Clay__CloseElement();
						
						if (ClayBtn("Close File", "Ctrl+W", true, nullptr))
						{
							FreeOsmMap(&app->map);
							FreeStr8(stdHeap, &app->mapFilePath);
						} Clay__CloseElement();
						
						Clay__CloseElement();
						Clay__CloseElement();
					} Clay__CloseElement();
					
					// +==============================+
					// |          View Menu           |
					// +==============================+
					if (ClayTopBtn("View", showMenuHotkeys, &app->isViewMenuOpen, &app->keepViewMenuOpenUntilMouseOver, false))
					{
						if (ClayBtnStr(ScratchPrintStr("%s Topmost", appIn->isWindowTopmost ? "Disable" : "Enable"), StrLit("Ctrl+T"), TARGET_IS_WINDOWS, nullptr))
						{
							platform->SetWindowTopmost(!appIn->isWindowTopmost);
						} Clay__CloseElement();
						
						#if DEBUG_BUILD
						if (ClayBtnStr(ScratchPrintStr("%s Clay UI Debug", Clay_IsDebugModeEnabled() ? "Hide" : "Show"), StrLit("~"), true, nullptr))
						{
							Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
						} Clay__CloseElement();
						#endif //DEBUG_BUILD
						
						if (ClayBtn("Close Window", "Ctrl+Shift+W", true, nullptr))
						{
							platform->RequestQuit();
							app->isFileMenuOpen = false;
						} Clay__CloseElement();
						
						Clay__CloseElement();
						Clay__CloseElement();
					} Clay__CloseElement();
					
					CLAY({ .layout = { .sizing = { .width=CLAY_SIZING_GROW(0) } } }) {}
					
					CLAY({ .id = CLAY_ID("FileNameDisplay") })
					{
						if (!IsEmptyStr(app->mapFilePath))
						{
							Str8 mapFileName = GetFileNamePart(app->mapFilePath, true);
							mapFileName = AllocStr8(uiArena, mapFileName);
							CLAY_TEXT(
								mapFileName,
								CLAY_TEXT_CONFIG({
									.fontId = app->clayUiFontId,
									.fontSize = (u16)app->uiFontSize,
									.textColor = TEXT_WHITE,
									.wrapMode = CLAY_TEXT_WRAP_NONE,
									.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
									.userData = { .contraction = TextContraction_ClipRight },
								})
							);
							
							if (isOverDisplayLimit)
							{
								CLAY({ .layout = { .sizing = { .width=CLAY_SIZING_FIXED(UI_R32(10)) } } }) {}
								CLAY_TEXT(
									StrLit("Over Display Limit!"),
									CLAY_TEXT_CONFIG({
										.fontId = app->clayUiFontId,
										.fontSize = (u16)app->uiFontSize,
										.textColor = ERROR_RED,
										.wrapMode = CLAY_TEXT_WRAP_NONE,
										.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
										.userData = { .contraction = TextContraction_ClipRight },
									})
								);
							}
							if (app->map.waysMissingNodes)
							{
								CLAY({ .layout = { .sizing = { .width=CLAY_SIZING_FIXED(UI_R32(10)) } } }) {}
								CLAY_TEXT(
									StrLit("Some ways are missing nodes!"),
									CLAY_TEXT_CONFIG({
										.fontId = app->clayUiFontId,
										.fontSize = (u16)app->uiFontSize,
										.textColor = ERROR_RED,
										.wrapMode = CLAY_TEXT_WRAP_NONE,
										.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
										.userData = { .contraction = TextContraction_ClipRight },
									})
								);
							}
							// if (app->map.relationsMissingMembers)
							// {
							// 	CLAY({ .layout = { .sizing = { .width=CLAY_SIZING_FIXED(UI_R32(10)) } } }) {}
							// 	CLAY_TEXT(
							// 		StrLit("Some relations are missing members!"),
							// 		CLAY_TEXT_CONFIG({
							// 			.fontId = app->clayUiFontId,
							// 			.fontSize = (u16)app->uiFontSize,
							// 			.textColor = ERROR_RED,
							// 			.wrapMode = CLAY_TEXT_WRAP_NONE,
							// 			.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
							// 			.userData = { .contraction = TextContraction_ClipRight },
							// 		})
							// 	);
							// }
							
							#if 1
							{
								CLAY({ .layout = { .sizing = { .width=CLAY_SIZING_FIXED(UI_R32(10)) } } }) {}
								// Str8 tileInfoStr = PrintInArenaStr(uiArena, "Z: %d (%dx%d)", tileLevelZ, tileGridSize, tileGridSize);
								Str8 tileInfoStr = PrintInArenaStr(uiArena, "Z: %d", tileLevelZ);
								CLAY_TEXT(
									tileInfoStr,
									CLAY_TEXT_CONFIG({
										.fontId = app->clayUiFontId,
										.fontSize = (u16)app->uiFontSize,
										.textColor = TEXT_WHITE,
										.wrapMode = CLAY_TEXT_WRAP_NONE,
										.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
										.userData = { .contraction = TextContraction_ClipRight },
									})
								);
							}
							#endif
						}
						else
						{
							CLAY_TEXT(
								StrLit("No file loaded..."),
								CLAY_TEXT_CONFIG({
									.fontId = app->clayUiFontId,
									.fontSize = (u16)app->uiFontSize,
									.textColor = TEXT_WHITE,
									.wrapMode = CLAY_TEXT_WRAP_NONE,
									.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
									.userData = { .contraction = TextContraction_ClipRight },
								})
							);
						}
					}
					
					#if 0
					CLAY({ .id = CLAY_ID("ViewPosDisplay") })
					{
						CLAY_TEXT(
							PrintInArenaStr(uiArena, "Pos (%lf, %lf) Scale %lf", app->view.position.X, app->view.position.Y, app->view.zoom),
							CLAY_TEXT_CONFIG({
								.fontId = app->clayUiFontId,
								.fontSize = (u16)app->uiFontSize,
								.textColor = TEXT_WHITE,
								.wrapMode = CLAY_TEXT_WRAP_NONE,
								.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
								.userData = { .contraction = TextContraction_ClipRight },
							})
						);
					}
					#endif
					
					CLAY({ .layout={ .sizing={ .width=CLAY_SIZING_FIXED(UI_R32(4)) } } }) {}
				}
				
				DoUiResizableSplitInterleaved(sidebarSplitSection, &uiContext, &app->sidebarSplit)
				{
					// +==============================+
					// |           Sidebar            |
					// +==============================+
					DoUiResizableSplitSection(sidebarSplitSection, Left)
					{
						CLAY({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, .backgroundColor = BACKGROUND_GRAY })
						{
							DoUiResizableSplitInterleaved(infoLayersSplitSection, &uiContext, &app->infoLayersSplit)
							{
								// +==============================+
								// |          Info Panel          |
								// +==============================+
								DoUiResizableSplitSection(infoLayersSplitSection, Top)
								{
									CLAY({ .layout = { .sizing = { .width=CLAY_SIZING_GROW(0), .height=CLAY_SIZING_GROW(0) }, .layoutDirection = CLAY_TOP_TO_BOTTOM } })
									{
										CLAY({ .layout = { .sizing = { .height=CLAY_SIZING_FIXED(UI_R32(5)) } } }) { }
										Str8 renderTilesStr = StrLit("Render Tiles");
										if (app->renderTiles)
										{
											uxx numTilesLoaded = 0;
											SparseSetV3iLoop(&app->mapTiles, tIndex)
											{
												SparseSetV3iLoopGet(MapTile, mapTile, &app->mapTiles, tIndex)
												{
													if (mapTile->isLoaded) { numTilesLoaded++; }
												}
											}
											renderTilesStr = PrintInArenaStr(uiArena, "%.*s (%llu/%llu)", StrPrint(renderTilesStr), numTilesLoaded, app->mapTiles.length);
										}
										DoUiCheckbox(&uiContext, StrLit("RenderTilesCheckbox"), &app->renderTiles, UI_R32(16), nullptr, renderTilesStr, Dir2_Right, &app->uiFont, app->uiFontSize, UI_FONT_STYLE);
										DoUiCheckbox(&uiContext, StrLit("RenderNodesCheckbox"), &app->renderNodes, UI_R32(16), nullptr, StrLit("Render Nodes"), Dir2_Right, &app->uiFont, app->uiFontSize, UI_FONT_STYLE);
										CLAY({ .layout = { .sizing = { .height=CLAY_SIZING_FIXED(UI_R32(10)) } } }) { }
										
										CLAY({ .id = CLAY_ID("InfoPanelTitle"),
											.layout = {
												.sizing = { .width=CLAY_SIZING_GROW(0), .height=CLAY_SIZING_FIT(0) },
												.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
												.padding = CLAY_PADDING_ALL(UI_U16(4)),
											},
											.backgroundColor = OUTLINE_GRAY,
										})
										{
											CLAY_TEXT(
												StrLit("Selected Info"),
												CLAY_TEXT_CONFIG({
													.fontId = app->clayUiFontId,
													.fontSize = (u16)app->uiFontSize,
													.textColor = TEXT_WHITE,
													.wrapMode = CLAY_TEXT_WRAP_NONE,
													.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
													.userData = { .contraction = TextContraction_ClipRight },
												})
											);
										}
										
										#define INFO_PANEL_TEXT(idNt, idIndex, text, color) DoUiLabel(&uiContext, StrLit(idNt), (idIndex), (text), (color), &app->uiFont, app->uiFontSize, UI_FONT_STYLE, true, nullptr);
										
										CLAY({ .id = CLAY_ID("InfoPanel"),
											.layout = {
												.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
												.padding = CLAY_PADDING_ALL(UI_U16(10)),
												.childGap = UI_U16(5),
												.layoutDirection = CLAY_TOP_TO_BOTTOM,
											},
											.scroll = {
												.vertical = true,
												.scrollLag = 5.0f,
											},
										})
										{
											Str8 infoStr = PrintInArenaStr(uiArena, "%llu nodes, %llu ways, %llu relations [color=A6E22E](%llu selected)[color]", app->map.nodes.length, app->map.ways.length, app->map.relations.length, app->map.selectedItems.length);
											CLAY_TEXT(
												infoStr,
												CLAY_TEXT_CONFIG({
													.fontId = app->clayUiFontId,
													.fontSize = (u16)app->uiFontSize,
													.textColor = TEXT_WHITE,
													.wrapMode = CLAY_TEXT_WRAP_WORDS,
													.textAlignment = CLAY_TEXT_ALIGN_LEFT,
													.userData = { .richText = true },
												})
											);
											
											if (app->map.selectedItems.length > 0)
											{
												VarArrayLoop(&app->map.selectedItems, sIndex)
												{
													VarArrayLoopGet(OsmSelectedItem, selectedItem, &app->map.selectedItems, sIndex);
													u64 itemId = 0;
													Str8 nameTag = Str8_Empty;
													bool visible = true;
													i32 version = 0;
													u64 changeset = 0;
													Str8 timestampStr = Str8_Empty;
													Str8 user = Str8_Empty;
													u64 uid = 0;
													VarArray* tagsArray = nullptr;
													VarArray* wayPntrsArray = nullptr;
													VarArray* relationPntrsArray = nullptr;
													if (selectedItem->type == OsmPrimitiveType_Node)
													{
														itemId = selectedItem->nodePntr->id;
														nameTag = GetOsmNodeTagValue(selectedItem->nodePntr, StrLit("name:en"), Str8_Empty);
														if (IsEmptyStr(nameTag)) { nameTag = GetOsmNodeTagValue(selectedItem->nodePntr, StrLit("name"), Str8_Empty); }
														visible = selectedItem->nodePntr->visible;
														version = selectedItem->nodePntr->version;
														changeset = selectedItem->nodePntr->changeset;
														timestampStr = selectedItem->nodePntr->timestampStr;
														user = selectedItem->nodePntr->user;
														uid = selectedItem->nodePntr->uid;
														tagsArray = &selectedItem->nodePntr->tags;
														wayPntrsArray = &selectedItem->nodePntr->wayPntrs;
														relationPntrsArray = &selectedItem->nodePntr->relationPntrs;
													}
													else if (selectedItem->type == OsmPrimitiveType_Way)
													{
														itemId = selectedItem->wayPntr->id;
														nameTag = GetOsmWayTagValue(selectedItem->wayPntr, StrLit("name:en"), Str8_Empty);
														if (IsEmptyStr(nameTag)) { nameTag = GetOsmWayTagValue(selectedItem->wayPntr, StrLit("name"), Str8_Empty); }
														visible = selectedItem->wayPntr->visible;
														version = selectedItem->wayPntr->version;
														changeset = selectedItem->wayPntr->changeset;
														timestampStr = selectedItem->wayPntr->timestampStr;
														user = selectedItem->wayPntr->user;
														uid = selectedItem->wayPntr->uid;
														tagsArray = &selectedItem->wayPntr->tags;
														relationPntrsArray = &selectedItem->wayPntr->relationPntrs;
													}
													Str8 displayName = PrintInArenaStr(uiArena, "> %s %llu \"%.*s\"%s", GetOsmPrimitiveTypeStr(selectedItem->type), itemId, StrPrint(nameTag), visible ? "" : " (visible=false)");
													INFO_PANEL_TEXT("Label_DisplayName", sIndex, displayName, MonokaiGreen);
													if (selectedItem->type == OsmPrimitiveType_Way)
													{
														Str8 wayMetaStr = PrintInArenaStr(uiArena, "    %llu nodes%s", selectedItem->wayPntr->nodes.length, selectedItem->wayPntr->isClosedLoop ? " closed loop" : "");
														INFO_PANEL_TEXT("Label_WayMeta", sIndex, wayMetaStr, TEXT_GRAY);
													}
													
													if (version != 0)
													{
														Str8 versionStr = PrintInArenaStr(uiArena, "    version: %d", version);
														INFO_PANEL_TEXT("Label_Version", sIndex, versionStr, TEXT_GRAY);
													}
													if (changeset != 0)
													{
														Str8 changesetStr = PrintInArenaStr(uiArena, "    changeset: %llu", changeset);
														INFO_PANEL_TEXT("Label_Changeset", sIndex, changesetStr, TEXT_GRAY);
													}
													if (!IsEmptyStr(timestampStr))
													{
														Str8 timestampDisplayStr = PrintInArenaStr(uiArena, "    timestampStr: %.*s", StrPrint(timestampStr));
														INFO_PANEL_TEXT("Label_Timestamp", sIndex, timestampDisplayStr, TEXT_GRAY);
													}
													if (!IsEmptyStr(user))
													{
														Str8 userDisplayStr = PrintInArenaStr(uiArena, "    user: %.*s", StrPrint(user));
														INFO_PANEL_TEXT("Label_User", sIndex, userDisplayStr, TEXT_GRAY);
													}
													if (uid != 0)
													{
														Str8 uidStr = PrintInArenaStr(uiArena, "    uid: %llu", uid);
														INFO_PANEL_TEXT("Label_UID", sIndex, uidStr, TEXT_GRAY);
													}
													
													if (relationPntrsArray != nullptr)
													{
														VarArrayLoop(relationPntrsArray, rIndex)
														{
															VarArrayLoopGetValue(OsmRelation*, relation, relationPntrsArray, rIndex);
															Str8 relationName = GetOsmRelationTagValue(relation, StrLit("name"), Str8_Empty);
															uxx memberIndex = UINTXX_MAX;
															OsmRelationMemberRole role = OsmRelationMemberRole_None;
															VarArrayLoop(&relation->members, mIndex)
															{
																VarArrayLoopGet(OsmRelationMember, member, &relation->members, mIndex);
																if (member->id == itemId) { memberIndex = mIndex; role = member->role; break; }
															}
															DebugAssert(memberIndex != UINTXX_MAX);
															Str8 relationStr = PrintInArenaStr(uiArena, "  In relation %llu \"%.*s\" [%llu/%llu] as %s", relation->id, StrPrint(relationName), memberIndex, relation->members.length, GetOsmRelationMemberRoleStr(role));
															INFO_PANEL_TEXT("Label_Relation", sIndex*Million(1) + rIndex, relationStr, MonokaiPurple);
														}
													}
													if (tagsArray != nullptr)
													{
														VarArrayLoop(tagsArray, tIndex)
														{
															VarArrayLoopGet(OsmTag, tag, tagsArray, tIndex);
															Str8 tagStr = PrintInArenaStr(uiArena, "  %.*s = \"%.*s\"", StrPrint(tag->key), StrPrint(tag->value));
															INFO_PANEL_TEXT("Label_Tag", sIndex*Million(1) + tIndex, tagStr, TEXT_WHITE);
														}
													}
												}
											}
											else
											{
												INFO_PANEL_TEXT("Lable_NothingSelected", 0, StrLit("No items selected..."), TEXT_WHITE);
											}
											
											#if 0
											{
												Str8 stdHeapText = PrintInArenaStr(uiArena, "Std: %llu used", stdHeap->used);
												INFO_PANEL_TEXT("Label_StdHeap", 0, stdHeapText, TEXT_WHITE);
												for (uxx sIndex = 0; sIndex < NUM_SCRATCH_ARENAS_PER_THREAD; sIndex++)
												{
													Arena* scratchArena = scratch;
													if (sIndex == 1) { scratchArena = scratch2; }
													else if (sIndex == 2) { scratchArena = scratch3; }
													Str8 scratchText = PrintInArenaStr(uiArena, "Scratch[%llu]: %llu/%llu", sIndex, scratchArena->committed, scratchArena->size);
													INFO_PANEL_TEXT("Label_Scratch", sIndex, scratchText, TEXT_WHITE);
												}
											}
											#endif
										}
									}
								}
								
								// +==============================+
								// |         Layers Panel         |
								// +==============================+
								DoUiResizableSplitSection(infoLayersSplitSection, Bottom)
								{
									CLAY({ .id = CLAY_ID("LayersPanelTitle"),
										.layout = {
											.sizing = { .width=CLAY_SIZING_GROW(0), .height=CLAY_SIZING_FIT(0) },
											.childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
											.padding = CLAY_PADDING_ALL(UI_U16(4)),
										},
										.backgroundColor = OUTLINE_GRAY,
									})
									{
										CLAY_TEXT(
											StrLit("Layers"),
											CLAY_TEXT_CONFIG({
												.fontId = app->clayUiFontId,
												.fontSize = (u16)app->uiFontSize,
												.textColor = TEXT_WHITE,
												.wrapMode = CLAY_TEXT_WRAP_NONE,
												.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
												.userData = { .contraction = TextContraction_ClipRight },
											})
										);
									}
								}
							}
						}
					}
					
					// +==============================+
					// |        Main Viewport         |
					// +==============================+
					DoUiResizableSplitSection(sidebarSplitSection, Right)
					{
						CLAY({ .id = CLAY_ID("MainViewport"),
							.layout = {
								.layoutDirection = CLAY_LEFT_TO_RIGHT,
								.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
							}
						})
						{
							//TODO: Render things here
						}
					}
				}
			}
		}
		
		Clay_RenderCommandArray clayRenderCommands = EndClayUIRender(&app->clay.clay);
		RenderClayCommandArray(&app->clay, &gfx, &clayRenderCommands);
		FlagUnset(uiArena->flags, ArenaFlag_DontPop);
		ArenaResetToMark(uiArena, uiArenaMark);
		uiArena = nullptr;
		
		platform->SetCursorShape(uiContext.cursorShape);
	}
	CommitAllFontTextureUpdates(&app->uiFont);
	CommitAllFontTextureUpdates(&app->mapFont);
	CommitAllFontTextureUpdates(&app->largeFont);
	TracyCZoneEnd(Zone_Render);
	TracyCZoneN(Zone_EndFrame, "EndFrame", true);
	EndFrame();
	TracyCZoneEnd(Zone_EndFrame);
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	
	TracyCZoneEnd(funcZone);
	// TracyCFrameMarkEnd("Game Loop");
	return renderedFrame;
}

// +==============================+
// |          AppClosing          |
// +==============================+
// void AppClosing(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr)
EXPORT_FUNC APP_CLOSING_DEF(AppClosing)
{
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, nullptr);
	
	#if BUILD_WITH_IMGUI
	igSaveIniSettingsToDisk(app->imgui->io->IniFilename);
	#endif
	
	AppSaveRecentFilesList();
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
}

// +==============================+
// |          AppGetApi           |
// +==============================+
// AppApi AppGetApi()
EXPORT_FUNC APP_GET_API_DEF(AppGetApi)
{
	AppApi result = ZEROED;
	result.AppInit = AppInit;
	result.AppUpdate = AppUpdate;
	result.AppClosing = AppClosing;
	result.AppBeforeReload = AppBeforeReload;
	result.AppAfterReload = AppAfterReload;
	return result;
}
