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
	
	ClayUIRenderer clay;
	r32 uiScale;
	r32 uiFontSize;
	r32 largeFontSize;
	r32 mapFontSize;
	u16 clayUiFontId;
	u16 clayLargeFontId;
	bool isFileMenuOpen;
	bool keepFileMenuOpenUntilMouseOver;
	bool isViewMenuOpen;
	bool keepViewMenuOpenUntilMouseOver;
	void* uiFocusedElement;
	UiResizableSplit sidebarSplit;
	
	OsmMap map;
	MapView view;
	
	Str8 loadedFilePath;
};

#endif //  _APP_MAIN_H
