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

#define CartoFillBackground_Value       0xFFF2EFE9
#define CartoFillResidential_Value      0xFFE0DFDF //Grey
#define CartoFillRetail_Value           0xFFFFD6D1 //Pink
#define CartoBorderRetail_Value         0xFFEABBB5
#define CartoFillRetailBuilding_Value   0xFFFFEFED //Lighter Pink
#define CartoBorderRetailBuilding_Value 0xFFF4C9AF
#define CartoFillForest_Value           0xFFADD19E //Green
#define CartoFillIndustrial_Value       0xFFEBDBE8 //FadedPurple
#define CartoFillPark_Value             0xFFC8FACC //LightGreen
#define CartoFillPlayground_Value       0xFFDFFCE2 //LighterGreen
#define CartoFillSports_Value           0xFF88E0BE //DarkGreen
#define CartoFillWater_Value            0xFFAAD3DF //Blue
#define CartoBorderWater_Value          0xFF82CADD
#define CartoFillBuilding_Value         0xFFD9D0C9 //BrownGray
#define CartoBorderBuilding_Value       0xFFC4B5AA
#define CartoFillSchool_Value           0xFFFFFFE5 //LightYellow
#define CartoStrokeTrunk_Value          0xFFF9B29C //Brick
#define CartoStrokeResidential_Value    0xFFFFFFFF //White
#define CartoStrokeSecondary_Value      0xFFF7FABF //Yellow
#define CartoFillParking_Value          0xFFEEEEEE //Grey1
#define CartoBorderParking_Value        0xFFD6C9C8
#define CartoFillCommercial_Value       0xFFF2DAD9 //Light Brick
#define CartoBorderCommercial_Value     0xFFE1C7C5
#define CartoFillReligious_Value        0xFFD0D0D0 //Grey2
#define CartoBorderReligious_Value      0xFFBEBDBC
#define CartoFillBridge_Value           0xFFB8B8B8 //Grey3
#define CartoFillPublicTransit_Value    0xFFBBBBBB //Grey3.1
#define CartoBorderPublicTransit_Value  0xFF828282
#define CartoFillGrass_Value            0xFFCDEBB0 //Grass Green

#define CartoFillBackground       NewColorU32(CartoFillBackground_Value)
#define CartoFillResidential      NewColorU32(CartoFillResidential_Value)
#define CartoFillRetail           NewColorU32(CartoFillRetail_Value)
#define CartoBorderRetail         NewColorU32(CartoBorderRetail_Value)
#define CartoFillRetailBuilding   NewColorU32(CartoFillRetailBuilding_Value)
#define CartoBorderRetailBuilding NewColorU32(CartoBorderRetailBuilding_Value)
#define CartoFillForest           NewColorU32(CartoFillForest_Value)
#define CartoFillIndustrial       NewColorU32(CartoFillIndustrial_Value)
#define CartoFillPark             NewColorU32(CartoFillPark_Value)
#define CartoFillPlayground       NewColorU32(CartoFillPlayground_Value)
#define CartoFillSports           NewColorU32(CartoFillSports_Value)
#define CartoFillWater            NewColorU32(CartoFillWater_Value)
#define CartoBorderWater          NewColorU32(CartoBorderWater_Value)
#define CartoFillBuilding         NewColorU32(CartoFillBuilding_Value)
#define CartoBorderBuilding       NewColorU32(CartoBorderBuilding_Value)
#define CartoFillSchool           NewColorU32(CartoFillSchool_Value)
#define CartoStrokeTrunk          NewColorU32(CartoStrokeTrunk_Value)
#define CartoStrokeResidential    NewColorU32(CartoStrokeResidential_Value)
#define CartoStrokeSecondary      NewColorU32(CartoStrokeSecondary_Value)
#define CartoFillParking          NewColorU32(CartoFillParking_Value)
#define CartoBorderParking        NewColorU32(CartoBorderParking_Value)
#define CartoFillCommercial       NewColorU32(CartoFillCommercial_Value)
#define CartoBorderCommercial     NewColorU32(CartoBorderCommercial_Value)
#define CartoFillReligious        NewColorU32(CartoFillReligious_Value)
#define CartoBorderReligious      NewColorU32(CartoBorderReligious_Value)
#define CartoFillBridge           NewColorU32(CartoFillBridge_Value)
#define CartoFillPublicTransit    NewColorU32(CartoFillPublicTransit_Value)
#define CartoBorderPublicTransit  NewColorU32(CartoBorderPublicTransit_Value)
#define CartoFillGrass            NewColorU32(CartoFillGrass_Value)

#endif //  _OSM_CARTO_H
