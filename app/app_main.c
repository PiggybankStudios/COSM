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
#include "os/os_all.h"
#include "mem/mem_all.h"
#include "struct/struct_all.h"
#include "misc/misc_all.h"
#include "input/input_all.h"
#include "file_fmt/file_fmt_all.h"
#include "ui/ui_all.h"
#include "gfx/gfx_all.h"
#include "gfx/gfx_system_global.h"
#include "phys/phys_all.h"
#include "parse/parse_all.h"

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
#include "app_clay_helpers.c"
#include "app_helpers.c"
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
	
	InitCompiledShader(&app->mainShader, stdHeap, main2d);
	
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
	
	InitMapView(&app->view, MapProjection_Mercator);
	OpenOsmMap(StrLit(TEST_OSM_FILE));
	
	#if 0
	Str8 testFileContents = Str8_Empty;
	bool foundTestFile = OsReadTextFile(FilePathLit(TEST_OSM_FILE), scratch, &testFileContents);
	if (foundTestFile)
	{
		PrintLine_I("Opened test file, %llu bytes", testFileContents.length);
		Result parseResult = TryParseOsmMap(stdHeap, testFileContents, &app->map);
		if (parseResult == Result_Success)
		{
			#if 0
			u64 nodeId1  = AddOsmNode(&app->map, NewGeoLoc(47.584399766577576, -122.48176574707033))->id;
			u64 nodeId2  = AddOsmNode(&app->map, NewGeoLoc(47.581620824334166, -122.49755859375001))->id;
			u64 nodeId3  = AddOsmNode(&app->map, NewGeoLoc(47.57606249728773,  -122.50030517578126))->id;
			u64 nodeId4  = AddOsmNode(&app->map, NewGeoLoc(47.573283112482144, -122.52502441406251))->id;
			u64 nodeId5  = AddOsmNode(&app->map, NewGeoLoc(47.59273570820418,  -122.5353240966797))->id;
			u64 nodeId6  = AddOsmNode(&app->map, NewGeoLoc(47.59921830048998,  -122.54768371582033))->id;
			u64 nodeId7  = AddOsmNode(&app->map, NewGeoLoc(47.60431120244565,  -122.54768371582033))->id;
			u64 nodeId8  = AddOsmNode(&app->map, NewGeoLoc(47.60199630847375,  -122.5572967529297))->id;
			u64 nodeId9  = AddOsmNode(&app->map, NewGeoLoc(47.593198777144636, -122.56347656250001))->id;
			u64 nodeId10 = AddOsmNode(&app->map, NewGeoLoc(47.59273570820418,  -122.5737762451172))->id;
			u64 nodeId11 = AddOsmNode(&app->map, NewGeoLoc(47.60107032220255,  -122.57789611816408))->id;
			u64 nodeId12 = AddOsmNode(&app->map, NewGeoLoc(47.61356975397398,  -122.57583618164064))->id;
			u64 nodeId13 = AddOsmNode(&app->map, NewGeoLoc(47.64318610543658,  -122.57789611816408))->id;
			u64 nodeId14 = AddOsmNode(&app->map, NewGeoLoc(47.66261271615866,  -122.59231567382814))->id;
			u64 nodeId15 = AddOsmNode(&app->map, NewGeoLoc(47.66492492654025,  -122.58544921875001))->id;
			u64 nodeId16 = AddOsmNode(&app->map, NewGeoLoc(47.67833372712059,  -122.56896972656251))->id;
			u64 nodeId17 = AddOsmNode(&app->map, NewGeoLoc(47.66538735632654,  -122.5682830810547))->id;
			u64 nodeId18 = AddOsmNode(&app->map, NewGeoLoc(47.674172743907384, -122.5627899169922))->id;
			u64 nodeId19 = AddOsmNode(&app->map, NewGeoLoc(47.692663462837984, -122.56622314453126))->id;
			u64 nodeId20 = AddOsmNode(&app->map, NewGeoLoc(47.70698926201497,  -122.56416320800783))->id;
			u64 nodeId21 = AddOsmNode(&app->map, NewGeoLoc(47.72223498082051,  -122.5518035888672))->id;
			u64 nodeId22 = AddOsmNode(&app->map, NewGeoLoc(47.719001413201916, -122.54287719726564))->id;
			u64 nodeId23 = AddOsmNode(&app->map, NewGeoLoc(47.7097615426664,   -122.54150390625001))->id;
			u64 nodeId24 = AddOsmNode(&app->map, NewGeoLoc(47.70375474821553,  -122.53189086914064))->id;
			u64 nodeId25 = AddOsmNode(&app->map, NewGeoLoc(47.700057915247314, -122.53257751464845))->id;
			u64 nodeId26 = AddOsmNode(&app->map, NewGeoLoc(47.69682297134991,  -122.54562377929689))->id;
			u64 nodeId27 = AddOsmNode(&app->map, NewGeoLoc(47.692663462837984, -122.54493713378908))->id;
			u64 nodeId28 = AddOsmNode(&app->map, NewGeoLoc(47.69820940045469,  -122.52708435058595))->id;
			u64 nodeId29 = AddOsmNode(&app->map, NewGeoLoc(47.706065135695745, -122.52708435058595))->id;
			u64 nodeId30 = AddOsmNode(&app->map, NewGeoLoc(47.7032926584311,   -122.50991821289064))->id;
			u64 nodeId31 = AddOsmNode(&app->map, NewGeoLoc(47.695898664798875, -122.50442504882814))->id;
			u64 nodeId32 = AddOsmNode(&app->map, NewGeoLoc(47.67602211074509,  -122.50923156738283))->id;
			u64 nodeId33 = AddOsmNode(&app->map, NewGeoLoc(47.66769944380821,  -122.50854492187501))->id;
			u64 nodeId34 = AddOsmNode(&app->map, NewGeoLoc(47.66307516642836,  -122.49824523925783))->id;
			u64 nodeId35 = AddOsmNode(&app->map, NewGeoLoc(47.655675470505955, -122.50854492187501))->id;
			u64 nodeId36 = AddOsmNode(&app->map, NewGeoLoc(47.65428791076272,  -122.52296447753908))->id;
			u64 nodeId37 = AddOsmNode(&app->map, NewGeoLoc(47.646886969413,    -122.50854492187501))->id;
			u64 nodeId38 = AddOsmNode(&app->map, NewGeoLoc(47.63960064341703,  -122.50313758850099))->id;
			u64 nodeId39 = AddOsmNode(&app->map, NewGeoLoc(47.633470089967,    -122.48961925506593))->id;
			u64 nodeId40 = AddOsmNode(&app->map, NewGeoLoc(47.62794619237425,  -122.49438285827638))->id;
			u64 nodeId41 = AddOsmNode(&app->map, NewGeoLoc(47.623752267682875, -122.49468326568605))->id;
			u64 nodeId42 = AddOsmNode(&app->map, NewGeoLoc(47.62097541515849,  -122.49206542968751))->id;
			u64 nodeId43 = AddOsmNode(&app->map, NewGeoLoc(47.62164071617851,  -122.49554157257081))->id;
			u64 nodeId44 = AddOsmNode(&app->map, NewGeoLoc(47.62346301909368,  -122.49670028686525))->id;
			u64 nodeId45 = AddOsmNode(&app->map, NewGeoLoc(47.6264711261907,   -122.50262260437013))->id;
			u64 nodeId46 = AddOsmNode(&app->map, NewGeoLoc(47.625140638632644, -122.50717163085939))->id;
			u64 nodeId47 = AddOsmNode(&app->map, NewGeoLoc(47.621062194032575, -122.51455307006837))->id;
			u64 nodeId48 = AddOsmNode(&app->map, NewGeoLoc(47.62242171092048,  -122.51631259918214))->id;
			u64 nodeId49 = AddOsmNode(&app->map, NewGeoLoc(47.62216138063622,  -122.52060413360596))->id;
			u64 nodeId50 = AddOsmNode(&app->map, NewGeoLoc(47.62036795900684,  -122.52541065216066))->id;
			u64 nodeId51 = AddOsmNode(&app->map, NewGeoLoc(47.62545880178175,  -122.5300455093384))->id;
			u64 nodeId52 = AddOsmNode(&app->map, NewGeoLoc(47.62606619877979,  -122.53807067871095))->id;
			u64 nodeId53 = AddOsmNode(&app->map, NewGeoLoc(47.62190104905555,  -122.53463745117189))->id;
			u64 nodeId54 = AddOsmNode(&app->map, NewGeoLoc(47.61680985980715,  -122.52021789550783))->id;
			u64 nodeId55 = AddOsmNode(&app->map, NewGeoLoc(47.616346999837226, -122.50442504882814))->id;
			u64 nodeId56 = AddOsmNode(&app->map, NewGeoLoc(47.61958693358351,  -122.50030517578126))->id;
			u64 nodeId57 = AddOsmNode(&app->map, NewGeoLoc(47.61032944737081,  -122.49687194824219))->id;
			u64 nodeId58 = AddOsmNode(&app->map, NewGeoLoc(47.60523713135211,  -122.50030517578126))->id;
			u64 nodeId59 = AddOsmNode(&app->map, NewGeoLoc(47.59875528481801,  -122.50167846679689))->id;
			u64 nodeId60 = AddOsmNode(&app->map, NewGeoLoc(47.595977104737685, -122.5188446044922))->id;
			u64 nodeId61 = AddOsmNode(&app->map, NewGeoLoc(47.5941249027327,   -122.50373840332033))->id;
			u64 nodeId62 = AddOsmNode(&app->map, NewGeoLoc(47.589494110887394, -122.49618530273438))->id;
			u64 nodeId63 = AddOsmNode(&app->map, NewGeoLoc(47.58625231278527,  -122.4858856201172))->id;
			u64 nodeId64 = AddOsmNode(&app->map, NewGeoLoc(47.584399766577576, -122.48107910156251))->id;
			u64 nodeId65 = AddOsmNode(&app->map, NewGeoLoc(47.583473468887405, -122.48176574707033))->id;
			u64 nodeId66 = AddOsmNode(&app->map, NewGeoLoc(47.584399766577576, -122.48107910156251))->id;
			u64 nodeId67 = AddOsmNode(&app->map, NewGeoLoc(47.58393661978137,  -122.48313903808595))->id;
			u64 wayIds[] = { nodeId1, nodeId2, nodeId3, nodeId4, nodeId5, nodeId6, nodeId7, nodeId8, nodeId9, nodeId10, nodeId11, nodeId12, nodeId13, nodeId14, nodeId15, nodeId16, nodeId17, nodeId18, nodeId19, nodeId20, nodeId21, nodeId22, nodeId23, nodeId24, nodeId25, nodeId26, nodeId27, nodeId28, nodeId29, nodeId30, nodeId31, nodeId32, nodeId33, nodeId34, nodeId35, nodeId36, nodeId37, nodeId38, nodeId39, nodeId40, nodeId41, nodeId42, nodeId43, nodeId44, nodeId45, nodeId46, nodeId47, nodeId48, nodeId49, nodeId50, nodeId51, nodeId52, nodeId53, nodeId54, nodeId55, nodeId56, nodeId57, nodeId58, nodeId59, nodeId60, nodeId61, nodeId62, nodeId63, nodeId64, nodeId65, nodeId66, nodeId67 };
			OsmWay* testWay = AddOsmWay(&app->map, ArrayCount(wayIds), &wayIds[0]);
			app->viewPos = NewV2(159.687271f, 99.153770f);
			app->view.zoom = 622.0f;
			#endif
			
			PrintLine_I("Parsed test file, %llu node%s %llu way%s", app->map.nodes.length, Plural(app->map.nodes.length, "s"), app->map.ways.length, Plural(app->map.ways.length, "s"));
		}
		else { PrintLine_E("Parse failure: %s", GetResultStr(parseResult)); }
	}
	else
	{
		PrintLine_E("Failed to find test file \"%s\"", TEST_OSM_FILE);
	}
	#endif
	
	app->initialized = true;
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	return (void*)app;
}

void PrintArray(BktArray* array)
{
	PrintLine_D("BktArray: %llu buckets, %llu/%llu items (first=%p, last=%p)", array->numBuckets, array->length, array->allocLength, array->firstBucket, array->lastBucket);
	BktArrayBkt* bucket = array->firstBucket;
	uxx bIndex = 0;
	uxx index = 0;
	while (bucket != nullptr)
	{
		PrintLine_D("Bucket[%llu] %p: %llu/%llu%s%s", bIndex, bucket, bucket->length, bucket->allocLength, (bucket == array->firstBucket) ? " <- FIRST" : "", (bucket == array->lastBucket) ? " <- LAST" : "");
		for (uxx iIndex = 0; iIndex < bucket->length; iIndex++)
		{
			v2* itemPntr = BktArrayGet(v2, array, index + iIndex);
			NotNull(itemPntr);
			PrintLine_D("\t[%llu]: (%g, %g)", index + iIndex, itemPntr->X, itemPntr->Y);
		}
		
		index += bucket->length;
		bIndex++;
		bucket = bucket->next;
	}
	Assert(index == array->length);
	Assert(bIndex == array->numBuckets);
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
	//TODO: Anything that needs fixup after DLL reload should be done here
	
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
	
	TracyCZoneN(Zone_Update, "Update", true);
	// +==============================+
	// |            Update            |
	// +==============================+
	{
		// +==============================+
		// |       Test XML Parsers       |
		// +==============================+
		if (IsKeyboardKeyPressed(&appIn->keyboard, Key_Enter, false))
		{
			TracyCZoneN(_TestXML, "TestXML", true);
			TracyCMessageL("Enter Pressed");
			
			Str8 xmlFileContents = Str8_Empty;
			bool readSuccess = OsReadTextFile(FilePathLit(TEST_OSM_FILE), scratch, &xmlFileContents);
			if (readSuccess)
			{
				#if 0
				hoxml_context_t context;
				char* buffer = AllocMem(scratch, xmlFileContents.length);
				hoxml_init(&context, buffer, xmlFileContents.length);
				
				u64 codeIndex = 0;
				hoxml_code_t xmlCode;
				while ((xmlCode = hoxml_parse(&context, xmlFileContents.chars, xmlFileContents.length)) != HOXML_END_OF_DOCUMENT)
				{
					switch (xmlCode)
					{
						case HOXML_ELEMENT_BEGIN:
						{
							PrintLine_D("Opened <%s>", context.tag);
							bool foundSrlInfo = false;
							for (u64 sIndex = 0; sIndex < ArrayCount(xOsmSrlInfos); sIndex++)
							{
								const SrlInfo* info = &xOsmSrlInfos[sIndex];
								if (StrExactEquals(StrLit(context.tag), StrLit(info->serializedName)))
								{
									PrintLine_D("\tShould have %llu members!", info->numMembers);
									foundSrlInfo = true;
									break;
								}
							}
							if (!foundSrlInfo)
							{
								WriteLine_W("\tUnknown type!");
							}
						} break;
						case HOXML_ELEMENT_END:   PrintLine_D("Closed <%s>", context.tag); break;
						case HOXML_ATTRIBUTE: PrintLine_D("Attribute \"%s\" of <%s> has value: %s", context.attribute, context.tag, context.value); break;
						default: PrintLine_D("HoxmlCode[%llu]: %s", codeIndex, GetHoxmlCodeStr(xmlCode));
					}
					if (xmlCode == HOXML_ERROR_INSUFFICIENT_MEMORY) { break; }
					codeIndex++;
				}
				#endif
				
				#if HOXML_ENABLED
				XmlFile xmlFile = ZEROED;
				Result parseResult = TryParseXml(xmlFileContents, scratch, &xmlFile);
				if (parseResult == Result_Success)
				{
					PrintLine_I("Parsed xml file, %llu root elements, %llu total elements", xmlFile.roots.length, xmlFile.numElements);
				}
				else { PrintLine_E("Failed to parse XML file: %s", GetResultStr(parseResult)); }
				#endif
			}
			
			TracyCZoneEnd(_TestXML);
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
			OpenOsmMap(StrLit(TEST_OSM_FILE));
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
		if (app->map.arena != nullptr)
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
				r64 closestWayDistanceSqr = 0.0f;
				v2d closestWayPoint = V2d_Zero;
				#if 0
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
		if (IsKeyboardKeyDown(&appIn->keyboard, Key_Control) && IsKeyboardKeyPressed(&appIn->keyboard, Key_R, false))
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
		UiWidgetContext uiContext = NewUiWidgetContext(uiArena, &app->clay, &appIn->keyboard, &appIn->mouse, app->uiScale, &app->uiFocusedElement, CursorShape_Default);
		
		// +==============================+
		// |          Render Map          |
		// +==============================+
		rec mainViewportRec = GetClayElementDrawRecNt("MainViewport");
		if (mainViewportRec.Width > 0 && mainViewportRec.Height > 0)
		{
			app->view.minZoom = MinR64(mainViewportRec.Width / app->view.mapRec.Width, mainViewportRec.Height / app->view.mapRec.Height);
			if (app->view.zoom == 0.0) { app->view.zoom = app->view.minZoom; }
			recd mapScreenRec = GetMapScreenRec(&app->view);
			
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
			for (uxx lIndex = 1; lIndex < OsmRenderLayer_Count; lIndex++)
			{
				OsmRenderLayer currentLayer = (OsmRenderLayer)lIndex;
				VarArrayLoop(&app->map.ways, wIndex)
				{
					VarArrayLoopGet(OsmWay, way, &app->map.ways, wIndex);
					UpdateOsmWayColorChoice(way);
					if (way->renderLayer == currentLayer && way->colorsChosen)
					{
						if (way->fillColor.a > 0 || (way->isClosedLoop && way->borderThickness > 0.0f && way->borderColor.a > 0))
						{
							if (way->isClosedLoop)
							{
								v2 boundsTopLeft = ToV2Fromd(MapProject(app->view.projection, way->nodeBounds.TopLeft, mapScreenRec));
								v2 boundsBottomRight = ToV2Fromd(MapProject(app->view.projection, AddV2d(way->nodeBounds.TopLeft, way->nodeBounds.Size), mapScreenRec));
								rec boundsRec = NewRecBetweenV(boundsTopLeft, boundsBottomRight);
								if (boundsRec.X + boundsRec.Width >= 0 && boundsRec.Y + boundsRec.Height >= 0 &&
									boundsRec.X <= screenSize.Width && boundsRec.Y <= screenSize.Height)
								{
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
							}
							
							if (!way->isClosedLoop && way->lineThickness > 0.0f)
							{
								v2d prevPos = V2d_Zero;
								VarArrayLoop(&way->nodes, nIndex)
								{
									VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
									v2d nodePos = MapProject(app->view.projection, nodeRef->pntr->location, mapScreenRec);
									if (nIndex > 0)
									{
										DrawLine(ToV2Fromd(prevPos), ToV2Fromd(nodePos), way->lineThickness, way->fillColor);
									}
									prevPos = nodePos;
								}
							}
						}
					}
					
					if (currentLayer == OsmRenderLayer_Selection && (way->isSelected || way->isHovered))
					{
						Color32 borderColor = (way->isSelected ? CartoTextGreen : CartoTextOrange);
						
						v2d prevPos = V2d_Zero;
						VarArrayLoop(&way->nodes, nIndex)
						{
							VarArrayLoopGet(OsmNodeRef, nodeRef, &way->nodes, nIndex);
							v2d nodePos = MapProject(app->view.projection, nodeRef->pntr->location, mapScreenRec);
							if (nIndex > 0)
							{
								DrawLine(ToV2Fromd(prevPos), ToV2Fromd(nodePos), 2.0f, borderColor);
							}
							prevPos = nodePos;
						}
					}
				}
			}
			
			// +==============================+
			// |         Render Nodes         |
			// +==============================+
			#if 1
			VarArrayLoop(&app->map.nodes, nIndex)
			{
				VarArrayLoopGet(OsmNode, node, &app->map.nodes, nIndex);
				
				Str8 populationStr = GetOsmNodeTagValue(node, StrLit("population"), Str8_Empty);
				u64 population = 0; TryParseU64(populationStr, &population, nullptr);
				r32 populationLerp = InverseLerpClampR32(Thousand(50), Thousand(500), (r32)population);
				r32 radius = (!IsEmptyStr(populationStr)) ? LerpR32(1.0, 10.0f, populationLerp) : 0.0f;
				if (app->map.ways.length == 0 && radius == 0.0f) { radius = 1.0f; } //Show all nodes when now ways were found
				
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
			#endif
			
			v2 boundsTopLeft = ToV2Fromd(MapProject(app->view.projection, app->map.bounds.TopLeft, mapScreenRec));
			v2 boundsBottomRight = ToV2Fromd(MapProject(app->view.projection, AddV2d(app->map.bounds.TopLeft, app->map.bounds.Size), mapScreenRec));
			rec boundsRec = NewRecBetweenV(boundsTopLeft, boundsBottomRight);
			DrawRectangleOutline(boundsRec, 2.0f, MonokaiRed);
			// DrawCircle(NewCircleV(boundsRec.TopLeft, 5), MonokaiRed);
			// DrawCircle(NewCircleV(Add(boundsRec.TopLeft, boundsRec.Size), 5), MonokaiOrange);
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
					if (ClayTopBtn("File", showMenuHotkeys, &app->isFileMenuOpen, &app->keepFileMenuOpenUntilMouseOver, false))
					{
						if (ClayBtn("Open" UNICODE_ELLIPSIS_STR, "Ctrl+O", true, nullptr))
						{
							FilePath selectedFilePath = FilePath_Empty;
							TracyCZoneN(Zone_OsOpenFileDialog, "OsDoOpenFileDialog", true);
							Result openResult = OsDoOpenFileDialog(scratch, &selectedFilePath);
							TracyCZoneEnd(Zone_OsOpenFileDialog);
							if (openResult == Result_Success)
							{
								OpenOsmMap(selectedFilePath);
							}
							else if (openResult != Result_Canceled) { PrintLine_E("OpenFileDialog failed: %s", GetResultStr(openResult)); }
						} Clay__CloseElement();
						
						if (ClayBtn("Close File", "Ctrl+W", true, nullptr))
						{
							//TODO: Implement me!
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
						if (!IsEmptyStr(app->loadedFilePath))
						{
							CLAY_TEXT(
								GetFileNamePart(app->loadedFilePath, true),
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
					DoUiResizableSplitSection(sidebarSplitSection, Left)
					{
						// +==============================+
						// |           Sidebar            |
						// +==============================+
						CLAY({ .id = CLAY_ID("Sidebar"),
							.layout = {
								.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
								.padding = CLAY_PADDING_ALL(UI_U16(10)),
								.childGap = UI_U16(5),
								.layoutDirection = CLAY_TOP_TO_BOTTOM,
							},
							.backgroundColor = BACKGROUND_GRAY,
							.scroll = {
								.vertical = true,
								.scrollLag = 5.0f,
							},
						})
						{
							Str8 infoStr = PrintInArenaStr(uiArena, "%llu nodes, %llu ways [color=A6E22E](%llu selected)[color]", app->map.nodes.length, app->map.ways.length, app->map.selectedItems.length);
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
									VarArray* tagsArray = nullptr;
									if (selectedItem->type == OsmPrimitiveType_Node)
									{
										itemId = selectedItem->nodePntr->id;
										nameTag = GetOsmNodeTagValue(selectedItem->nodePntr, StrLit("name:en"), Str8_Empty);
										if (IsEmptyStr(nameTag)) { nameTag = GetOsmNodeTagValue(selectedItem->nodePntr, StrLit("name"), Str8_Empty); }
										tagsArray = &selectedItem->nodePntr->tags;
									}
									else if (selectedItem->type == OsmPrimitiveType_Way)
									{
										itemId = selectedItem->wayPntr->id;
										nameTag = GetOsmWayTagValue(selectedItem->wayPntr, StrLit("name:en"), Str8_Empty);
										if (IsEmptyStr(nameTag)) { nameTag = GetOsmWayTagValue(selectedItem->wayPntr, StrLit("name"), Str8_Empty); }
										tagsArray = &selectedItem->wayPntr->tags;
									}
									Str8 displayName = PrintInArenaStr(uiArena, "> %s %llu \"%.*s\"", GetOsmPrimitiveTypeStr(selectedItem->type), itemId, StrPrint(nameTag));
									CLAY_TEXT(
										displayName,
										CLAY_TEXT_CONFIG({
											.fontId = app->clayUiFontId,
											.fontSize = (u16)app->uiFontSize,
											.textColor = MonokaiGreen,
											.wrapMode = CLAY_TEXT_WRAP_WORDS,
											.textAlignment = CLAY_TEXT_ALIGN_LEFT,
										})
									);
									if (selectedItem->type == OsmPrimitiveType_Way)
									{
										Str8 wayMetaStr = PrintInArenaStr(uiArena, "    %llu nodes%s", selectedItem->wayPntr->nodes.length, selectedItem->wayPntr->isClosedLoop ? " closed loop" : "");
										CLAY_TEXT(
											wayMetaStr,
											CLAY_TEXT_CONFIG({
												.fontId = app->clayUiFontId,
												.fontSize = (u16)app->uiFontSize,
												.textColor = TEXT_GRAY,
												.wrapMode = CLAY_TEXT_WRAP_WORDS,
												.textAlignment = CLAY_TEXT_ALIGN_LEFT,
											})
										);
									}
									
									if (tagsArray != nullptr)
									{
										VarArrayLoop(tagsArray, tIndex)
										{
											VarArrayLoopGet(OsmTag, tag, tagsArray, tIndex);
											Str8 tagStr = PrintInArenaStr(uiArena, "  [bold][size=%g]%.*s[size][bold] = \"%.*s\"", app->uiFontSize+2, StrPrint(tag->key), StrPrint(tag->value));
											CLAY_TEXT(
												tagStr,
												CLAY_TEXT_CONFIG({
													.fontId = app->clayUiFontId,
													.fontSize = (u16)app->uiFontSize,
													.textColor = TEXT_WHITE,
													.wrapMode = CLAY_TEXT_WRAP_WORDS,
													.textAlignment = CLAY_TEXT_ALIGN_LEFT,
													.userData = { .richText = true },
												})
											);
										}
									}
								}
							}
							else
							{
								CLAY_TEXT(
									StrLit("No items selected..."),
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
					DoUiResizableSplitSection(sidebarSplitSection, Right)
					{
						// +==============================+
						// |        Main Viewport         |
						// +==============================+
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
