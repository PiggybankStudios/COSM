/*
File:   defines.h
Author: Taylor Robbins
Date:   02\25\2025
*/

#ifndef _DEFINES_H
#define _DEFINES_H

#define MIN_FONT_ATLAS_SIZE 256 //px
#define MAX_FONT_ATLAS_SIZE 1024 //px

#define UI_FONT_NAME  "Consolas"
#define UI_FONT_STYLE FontStyleFlag_None
#define MIN_UI_FONT_SIZE       9
#define DEFAULT_UI_FONT_SIZE   14

// #define MAP_FONT_NAME  "Arial"
#define MAP_FONT_NAME  "Helvetica"
#define MAP_ASIAN_FONT_NAME  "Yu Mincho" //TODO: This isn't available by default on Win11 unless you install a japanese keyboard on Windows? Or maybe Windows 11 doesn't have the font by default?
// #define MAP_ASIAN_FONT_NAME  "Yu Gothic" //TODO: This breaks stb_truetype?
// #define MAP_ASIAN_FONT_NAME  "Meiryo UI"
// #define MAP_ASIAN_FONT_NAME  "Meiryo"
// #define MAP_ASIAN_FONT_NAME  "Ms Mincho" //TODO: This one can be found on Win11 can't be found on Win10 for some reason, even though MSMINCHO.ttf exists
// #define MAP_ASIAN_FONT_NAME  "MSMINCHO" //TODO: This one can be found on Windows 10 but does NOT contain CJK glyphs??
#define MAP_FONT_STYLE FontStyleFlag_Bold
#define DEFAULT_MAP_FONT_SIZE   20

#define LARGE_FONT_NAME  "Consolas"
#define LARGE_FONT_STYLE FontStyleFlag_Bold
#define DEFAULT_LARGE_FONT_SIZE   36

#define TOPBAR_ICONS_SIZE  16 //px
#define TOPBAR_ICONS_PADDING  8 //px

#define TEXT_WHITE           NewColor(221, 222, 224, 255) //FFDDDEE0
#define TEXT_LIGHT_GRAY      NewColor(175, 177, 179, 255) //FFAFB1B3
#define TEXT_GRAY            NewColor(107, 112, 120, 255) //FF6B7078
#define BACKGROUND_BLACK     NewColor(25, 27, 28, 255)    //FF191B1C
#define BACKGROUND_DARK_GRAY NewColor(31, 34, 35, 255)    //FF1F2223
#define BACKGROUND_GRAY      NewColor(39, 42, 43, 255)    //FF272A2B
#define OUTLINE_GRAY         NewColor(52, 58, 59, 255)    //FF343A3B
#define HOVERED_BLUE         NewColor(16, 60, 76, 255)    //FF103C4C
#define SELECTED_BLUE        NewColor(0, 121, 166, 255)   //FF0079A6
#define ERROR_RED            NewColor(255, 102, 102, 255) //FFFF6666

#define OSM_API_URL "https://api.openstreetmap.org/api"
#define OHM_API_URL "https://api.openhistoricalmap.org/api"

// #define TEST_OSM_FILE "bracke.osm" //download a portion of OSM from https://www.openstreetmap.org/export and put it in the _data folder
#define TEST_OSM_FILE "Q:/downtown_seattle.osm.pbf"

#define MERCATOR_MAP_ASPECT_RATIO   (2.0/1.0) //2:1 aspect ratio for Mercator projection maps
#define MAP_MAX_ZOOM                Million(100) //multiplier TODO: Change to a unit of measurement that isn't dependent on screen size and mapRec size!

#define DISPLAY_NODE_COUNT_LIMIT     Thousand(30)
#define DISPLAY_WAY_COUNT_LIMIT      Thousand(30)

#endif //  _DEFINES_H
