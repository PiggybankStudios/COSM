/*
File:   osm_carto.h
Author: Taylor Robbins
Date:   08\24\2025
Description:
	** Contains defines that relate to the OSM-Carto stylesheet
	** https://wiki.openstreetmap.org/wiki/OpenStreetMap_Carto
*/

#ifndef _OSM_CARTO_H
#define _OSM_CARTO_H

#define CartoFillBackgroundValue    0xFFF2EFE9
#define CartoFillResidentialValue   0xFFE0DFDF //Grey
#define CartoFillRetailValue        0xFFFFD6D1 //Pink
#define CartoFillForestValue        0xFFADD19E //Green
#define CartoFillIndustrialValue    0xFFEBDBE8 //FadedPurple
#define CartoFillParkValue          0xFFC8FACC //LightGreen
#define CartoFillPlaygroundValue    0xFFDFFCE2 //LighterGreen
#define CartoFillSportsValue        0xFF88E0BE //DarkGreen
#define CartoFillWaterValue         0xFFAAD3DF //Blue
#define CartoFillBuildingValue      0xFFD9D0C9 //BrownGray
#define CartoFillSchoolValue        0xFFFFFFE5 //LightYellow
#define CartoStrokeTrunkValue       0xFFF9B29C //Brick
#define CartoStrokeResidentialValue 0xFFFFFFFF //White
#define CartoStrokeSecondaryValue   0xFFF7FABF //Yellow

#define CartoFillBackground    NewColorU32(CartoFillBackgroundValue)
#define CartoFillResidential   NewColorU32(CartoFillResidentialValue)
#define CartoFillRetail        NewColorU32(CartoFillRetailValue)
#define CartoFillForest        NewColorU32(CartoFillForestValue)
#define CartoFillIndustrial    NewColorU32(CartoFillIndustrialValue)
#define CartoFillPark          NewColorU32(CartoFillParkValue)
#define CartoFillPlayground    NewColorU32(CartoFillPlaygroundValue)
#define CartoFillSports        NewColorU32(CartoFillSportsValue)
#define CartoFillWater         NewColorU32(CartoFillWaterValue)
#define CartoFillBuilding      NewColorU32(CartoFillBuildingValue)
#define CartoFillSchool        NewColorU32(CartoFillSchoolValue)
#define CartoStrokeTrunk       NewColorU32(CartoStrokeTrunkValue)
#define CartoStrokeResidential NewColorU32(CartoStrokeResidentialValue)
#define CartoStrokeSecondary   NewColorU32(CartoStrokeSecondaryValue)

#endif //  _OSM_CARTO_H
