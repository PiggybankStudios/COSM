/*
File:   app_main.h
Author: Taylor Robbins
Date:   02\25\2025
*/

#ifndef _APP_MAIN_H
#define _APP_MAIN_H

typedef plex MapView MapView;
plex MapView
{
	MapProjection projection;
	recd mapRec;
	v2d position; //this is relative to mapRec
	r64 minZoom;
	r64 zoom;
	bool isDragPanning;
	v2d dragPanningPos; //relative to mapRec
};

typedef plex RecentFile RecentFile;
plex RecentFile
{
	Str8 path;
	bool fileExists;
};

typedef plex AppData AppData;
plex AppData
{
	bool initialized;
	RandomSeries random;
	AppResources resources;
	
	Shader mainShader;
	PigFont uiFont;
	VarArray kanjiCodepoints; //u32
	PigFont mapFont;
	PigFont largeFont;
	
	Str8 mapBackTexturePath;
	Texture mapBackTexture;
	
	ClayUIRenderer clay;
	r32 uiScale;
	r32 uiFontSize;
	r32 largeFontSize;
	r32 mapFontSize;
	u16 clayUiFontId;
	u16 clayLargeFontId;
	bool isFileMenuOpen;
	bool keepFileMenuOpenUntilMouseOver;
	bool isOpenRecentSubmenuOpen;
	bool keepOpenRecentSubmenuOpenUntilMouseOver;
	bool isViewMenuOpen;
	bool keepViewMenuOpenUntilMouseOver;
	void* uiFocusedElement;
	UiResizableSplit sidebarSplit;
	UiResizableSplit infoLayersSplit;
	
	OsFileWatch recentFilesSaveWatch;
	VarArray recentFiles; //RecentFile
	
	Str8 mapFilePath;
	OsmMap map;
	MapView view;
};

#endif //  _APP_MAIN_H
