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

// +--------------------------------------------------------------+
// |                         Header Files                         |
// +--------------------------------------------------------------+
#include "platform_interface.h"
#include "app_resources.h"
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
#include "main2d_shader.glsl.h"
#include "app_resources.c"
#include "osm_map.c"
#include "app_clay_helpers.c"
#include "app_helpers.c"
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
	
	app->uiFontSize = DEFAULT_UI_FONT_SIZE;
	app->uiScale = 1.0f;
	bool fontBakeSuccess = AppCreateFonts();
	Assert(fontBakeSuccess);
	
	InitClayUIRenderer(stdHeap, V2_Zero, &app->clay);
	app->clayUiFontId = AddClayUIRendererFont(&app->clay, &app->uiFont, UI_FONT_STYLE);
	
	Str8 testFileContents = Str8_Empty;
	bool foundTestFile = OsReadTextFile(FilePathLit(TEST_OSM_FILE), scratch, &testFileContents);
	if (foundTestFile)
	{
		PrintLine_I("Opened test file, %llu bytes", testFileContents.length);
		Result parseResult = TryParseOsmMap(stdHeap, testFileContents, &app->map);
		if (parseResult == Result_Success)
		{
			PrintLine_I("Parsed test file, %llu node%s %llu way%s", app->map.nodes.length, Plural(app->map.nodes.length, "s"), app->map.ways.length, Plural(app->map.ways.length, "s"));
		}
		else { PrintLine_E("Parse failure: %s", GetResultStr(parseResult)); }
	}
	else
	{
		PrintLine_E("Failed to find test file \"%s\"", TEST_OSM_FILE);
	}
	
	app->initialized = true;
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	return (void*)app;
}

// +==============================+
// |          AppUpdate           |
// +==============================+
// bool AppUpdate(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr, AppInput* appInput)
EXPORT_FUNC APP_UPDATE_DEF(AppUpdate)
{
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	bool renderedFrame = true;
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, appInput);
	v2i screenSizei = appIn->screenSize;
	v2 screenSize = ToV2Fromi(appIn->screenSize);
	v2 screenCenter = Div(screenSize, 2.0f);
	v2 mousePos = appIn->mouse.position;
	
	// +==============================+
	// |            Update            |
	// +==============================+
	{
		//TODO: Implement me!
	}
	
	// +==============================+
	// |          Rendering           |
	// +==============================+
	BeginFrame(platform->GetSokolSwapchain(), screenSizei, BACKGROUND_BLACK, 1.0f);
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
							//TODO: Implement me!
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
				}
				
				// +==============================+
				// |        Main Viewport         |
				// +==============================+
				CLAY({
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
		
		Clay_RenderCommandArray clayRenderCommands = EndClayUIRender(&app->clay.clay);
		RenderClayCommandArray(&app->clay, &gfx, &clayRenderCommands);
		FlagUnset(uiArena->flags, ArenaFlag_DontPop);
		ArenaResetToMark(uiArena, uiArenaMark);
		uiArena = nullptr;
	}
	EndFrame();
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
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
	return result;
}
