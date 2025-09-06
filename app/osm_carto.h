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
#define CartoTextGrey_Value             0xFF555555
#define CartoTextGreen_Value            0xFF0E8517
#define CartoTextOrange_Value           0xFFC77400
#define CartoTextBrown_Value            0xFF734A08
#define CartoTextDarkBlue_Value         0xFF5768EC
#define CartoTextPurple_Value           0xFF8461C4
#define CartoTextBlue_Value             0xFF0092DA
#define CartoTextMagenta_Value          0xFFAC39AC
#define CartoTextRed_Value              0xFFBF0000
#define CartoTextTan_Value              0xFFB9A99E
#define CartoTextBlack_Value            0xFF000000
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
#define CartoFillDarkerBuilding_Value   0xFFC4B6AB
#define CartoBorderDarkerBuilding_Value 0xFFB19F91
#define CartoFillSchool_Value           0xFFFFFFE5 //LightYellow
#define CartoFillParking_Value          0xFFEEEEEE //Grey1
#define CartoBorderParking_Value        0xFFD6C9C8
#define CartoFillCommercial_Value       0xFFF2DAD9 //Light Brick
#define CartoBorderCommercial_Value     0xFFE1C7C5
#define CartoFillReligious_Value        0xFFD0D0D0 //Grey2
#define CartoFillCemetery_Value         0xFFAACBAF //Slightly Bluish Green
#define CartoBorderReligious_Value      0xFFBEBDBC
#define CartoFillBridge_Value           0xFFB8B8B8 //Grey3
#define CartoFillPublicTransit_Value    0xFFBBBBBB //Grey3.1
#define CartoBorderPublicTransit_Value  0xFF828282
#define CartoFillGrass_Value            0xFFCDEBB0 //Grass Green
#define CartoStrokeTrunk_Value          0xFFF9B29C //Brick
#define CartoStrokeRoad_Value           0xFFFFFFFF //White
#define CartoStrokeSecondary_Value      0xFFF7FABF //Yellow
#define CartoStrokePath_Value           0xFFFA8173 //Darker Brick
#define CartoStrokeBackPath_Value       0x80FFFFFF //Half-transparent White
#define CartoStrokeTrack_Value          0xFFAC8331 //Brown
#define CartoStrokeCycleway_Value       0xFF1C1CFD //Blue
#define CartoStrokeHedge_Value          0xFFB2D3A5 //Leaf Green
#define CartoStrokeFence_Value          0xFF929191 //Mid Grey
#define CartoStrokePowerline_Value      0xAA929191 //Slightly Transparent Grey
#define CartoStrokeRail_Value           0xFF707070 //Dark Grey

#define CartoFillBackground       NewColorU32(CartoFillBackground_Value)
#define CartoTextGrey             NewColorU32(CartoTextGrey_Value)
#define CartoTextGreen            NewColorU32(CartoTextGreen_Value)
#define CartoTextOrange           NewColorU32(CartoTextOrange_Value)
#define CartoTextBrown            NewColorU32(CartoTextBrown_Value)
#define CartoTextDarkBlue         NewColorU32(CartoTextDarkBlue_Value)
#define CartoTextPurple           NewColorU32(CartoTextPurple_Value)
#define CartoTextBlue             NewColorU32(CartoTextBlue_Value)
#define CartoTextMagenta          NewColorU32(CartoTextMagenta_Value)
#define CartoTextRed              NewColorU32(CartoTextRed_Value)
#define CartoTextTan              NewColorU32(CartoTextTan_Value)
#define CartoTextBlack            NewColorU32(CartoTextBlack_Value)
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
#define CartoFillDarkerBuilding   NewColorU32(CartoFillDarkerBuilding_Value)
#define CartoBorderDarkerBuilding NewColorU32(CartoBorderDarkerBuilding_Value)
#define CartoFillSchool           NewColorU32(CartoFillSchool_Value)
#define CartoFillParking          NewColorU32(CartoFillParking_Value)
#define CartoBorderParking        NewColorU32(CartoBorderParking_Value)
#define CartoFillCommercial       NewColorU32(CartoFillCommercial_Value)
#define CartoBorderCommercial     NewColorU32(CartoBorderCommercial_Value)
#define CartoFillReligious        NewColorU32(CartoFillReligious_Value)
#define CartoFillCemetery         NewColorU32(CartoFillCemetery_Value)
#define CartoBorderReligious      NewColorU32(CartoBorderReligious_Value)
#define CartoFillBridge           NewColorU32(CartoFillBridge_Value)
#define CartoFillPublicTransit    NewColorU32(CartoFillPublicTransit_Value)
#define CartoBorderPublicTransit  NewColorU32(CartoBorderPublicTransit_Value)
#define CartoFillGrass            NewColorU32(CartoFillGrass_Value)
#define CartoStrokeTrunk          NewColorU32(CartoStrokeTrunk_Value)
#define CartoStrokeRoad           NewColorU32(CartoStrokeRoad_Value)
#define CartoStrokeSecondary      NewColorU32(CartoStrokeSecondary_Value)
#define CartoStrokePath           NewColorU32(CartoStrokePath_Value)
#define CartoStrokeBackPath       NewColorU32(CartoStrokeBackPath_Value)
#define CartoStrokeTrack          NewColorU32(CartoStrokeTrack_Value)
#define CartoStrokeCycleway       NewColorU32(CartoStrokeCycleway_Value)
#define CartoStrokeHedge          NewColorU32(CartoStrokeHedge_Value)
#define CartoStrokeFence          NewColorU32(CartoStrokeFence_Value)
#define CartoStrokePowerline      NewColorU32(CartoStrokePowerline_Value)
#define CartoStrokeRail           NewColorU32(CartoStrokeRail_Value)
#endif //  _OSM_CARTO_H
