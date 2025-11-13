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

#define CartoFillBackground       MakeColorU32(CartoFillBackground_Value)
#define CartoTextGrey             MakeColorU32(CartoTextGrey_Value)
#define CartoTextGreen            MakeColorU32(CartoTextGreen_Value)
#define CartoTextOrange           MakeColorU32(CartoTextOrange_Value)
#define CartoTextBrown            MakeColorU32(CartoTextBrown_Value)
#define CartoTextDarkBlue         MakeColorU32(CartoTextDarkBlue_Value)
#define CartoTextPurple           MakeColorU32(CartoTextPurple_Value)
#define CartoTextBlue             MakeColorU32(CartoTextBlue_Value)
#define CartoTextMagenta          MakeColorU32(CartoTextMagenta_Value)
#define CartoTextRed              MakeColorU32(CartoTextRed_Value)
#define CartoTextTan              MakeColorU32(CartoTextTan_Value)
#define CartoTextBlack            MakeColorU32(CartoTextBlack_Value)
#define CartoFillResidential      MakeColorU32(CartoFillResidential_Value)
#define CartoFillRetail           MakeColorU32(CartoFillRetail_Value)
#define CartoBorderRetail         MakeColorU32(CartoBorderRetail_Value)
#define CartoFillRetailBuilding   MakeColorU32(CartoFillRetailBuilding_Value)
#define CartoBorderRetailBuilding MakeColorU32(CartoBorderRetailBuilding_Value)
#define CartoFillForest           MakeColorU32(CartoFillForest_Value)
#define CartoFillIndustrial       MakeColorU32(CartoFillIndustrial_Value)
#define CartoFillPark             MakeColorU32(CartoFillPark_Value)
#define CartoFillPlayground       MakeColorU32(CartoFillPlayground_Value)
#define CartoFillSports           MakeColorU32(CartoFillSports_Value)
#define CartoFillWater            MakeColorU32(CartoFillWater_Value)
#define CartoBorderWater          MakeColorU32(CartoBorderWater_Value)
#define CartoFillBuilding         MakeColorU32(CartoFillBuilding_Value)
#define CartoBorderBuilding       MakeColorU32(CartoBorderBuilding_Value)
#define CartoFillDarkerBuilding   MakeColorU32(CartoFillDarkerBuilding_Value)
#define CartoBorderDarkerBuilding MakeColorU32(CartoBorderDarkerBuilding_Value)
#define CartoFillSchool           MakeColorU32(CartoFillSchool_Value)
#define CartoFillParking          MakeColorU32(CartoFillParking_Value)
#define CartoBorderParking        MakeColorU32(CartoBorderParking_Value)
#define CartoFillCommercial       MakeColorU32(CartoFillCommercial_Value)
#define CartoBorderCommercial     MakeColorU32(CartoBorderCommercial_Value)
#define CartoFillReligious        MakeColorU32(CartoFillReligious_Value)
#define CartoFillCemetery         MakeColorU32(CartoFillCemetery_Value)
#define CartoBorderReligious      MakeColorU32(CartoBorderReligious_Value)
#define CartoFillBridge           MakeColorU32(CartoFillBridge_Value)
#define CartoFillPublicTransit    MakeColorU32(CartoFillPublicTransit_Value)
#define CartoBorderPublicTransit  MakeColorU32(CartoBorderPublicTransit_Value)
#define CartoFillGrass            MakeColorU32(CartoFillGrass_Value)
#define CartoStrokeTrunk          MakeColorU32(CartoStrokeTrunk_Value)
#define CartoStrokeRoad           MakeColorU32(CartoStrokeRoad_Value)
#define CartoStrokeSecondary      MakeColorU32(CartoStrokeSecondary_Value)
#define CartoStrokePath           MakeColorU32(CartoStrokePath_Value)
#define CartoStrokeBackPath       MakeColorU32(CartoStrokeBackPath_Value)
#define CartoStrokeTrack          MakeColorU32(CartoStrokeTrack_Value)
#define CartoStrokeCycleway       MakeColorU32(CartoStrokeCycleway_Value)
#define CartoStrokeHedge          MakeColorU32(CartoStrokeHedge_Value)
#define CartoStrokeFence          MakeColorU32(CartoStrokeFence_Value)
#define CartoStrokePowerline      MakeColorU32(CartoStrokePowerline_Value)
#define CartoStrokeRail           MakeColorU32(CartoStrokeRail_Value)

#endif //  _OSM_CARTO_H
