/*
File:   app_main.h
Author: Taylor Robbins
Date:   02\25\2025
*/

#ifndef _APP_MAIN_H
#define _APP_MAIN_H

typedef plex AppData AppData;
plex AppData
{
	bool initialized;
	RandomSeries random;
	AppResources resources;
	
	Shader mainShader;
	PigFont uiFont;
	PigFont largeFont;
	PigFont kanjiFont;
	
	ClayUIRenderer clay;
	r32 uiScale;
	r32 uiFontSize;
	r32 largeFontSize;
	u16 clayUiFontId;
	u16 clayLargeFontId;
	bool isFileMenuOpen;
	bool keepFileMenuOpenUntilMouseOver;
	bool isViewMenuOpen;
	bool keepViewMenuOpenUntilMouseOver;
	
	OsmMap map;
	MapProjection projection;
	recd mapRec;
	v2d viewPos;
	r64 minZoom;
	r64 viewZoom;
	
	Str8 loadedFilePath;
};

#endif //  _APP_MAIN_H
