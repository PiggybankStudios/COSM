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
	rec mapRec;
	v2 viewPos;
	r32 viewZoom;
};

#endif //  _APP_MAIN_H
