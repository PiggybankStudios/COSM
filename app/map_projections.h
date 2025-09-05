/*
File:   map_projections.h
Author: Taylor Robbins
Date:   07\06\2025
Descriptions:
	** Holds functions that help us
*/

#ifndef _MAP_PROJECTIONS_H
#define _MAP_PROJECTIONS_H

typedef enum MapProjection MapProjection;
enum MapProjection
{
	MapProjection_None = 0,
	MapProjection_Mercator,
	MapProjection_Count,
};
const char* GetMapProjectionStr(MapProjection enumValue)
{
	switch (enumValue)
	{
		case MapProjection_None:     return "None";
		case MapProjection_Mercator: return "Mercator";
		default: return UNKNOWN_STR;
	}
}

// +--------------------------------------------------------------+
// |                     Mercator Projection                      |
// +--------------------------------------------------------------+
// https://stackoverflow.com/questions/14329691/convert-latitude-longitude-point-to-a-pixels-x-y-on-mercator-projection
v2d MapProject(MapProjection projection, v2d geoLoc, recd mapRec)
{
	switch (projection)
	{
		case MapProjection_Mercator:
		{
			v2d result;
			result.X = mapRec.X + ((geoLoc.Longitude + 180.0) * (mapRec.Width / 360.0));
			r64 latRadians = ToRadians64(geoLoc.Latitude);
			r64 mercN = LnR64(TanR64(QuarterPi64 + (latRadians/2.0)));
			result.Y = mapRec.Y + (mapRec.Height/2.0) - (mapRec.Width * mercN / TwoPi64);
			return result;
		} break;
		
		default: AssertMsg(false, "MapProjection doesn't have implementation in MapProject"); return geoLoc;
	}
}

v2d MapUnproject(MapProjection projection, v2d mapPos, recd mapRec)
{
	switch (projection)
	{
		case MapProjection_Mercator:
		{
			v2d result;
			
			result.Longitude = ((mapPos.X - mapRec.X) / (mapRec.Width / 360.0)) - 180.0;
			if (!IsInfiniteOrNanR64(result.Longitude)) { result.Longitude = (((result.Longitude+180)/360) - FloorR64((result.Longitude+180)/360))*360 - 180; } //wrap around
			
			r64 relativeY = mapPos.Y - (mapRec.Y + (mapRec.Height/2.0));
			r64 mercN = -(relativeY * TwoPi64 / mapRec.Width);
			r64 latRadians = (AtanJoinedR64(PowR64(e64, mercN)) - QuarterPi64) * 2.0;
			result.Latitude = ToDegrees64(latRadians);
			
			return result;
		} break;
		
		default: AssertMsg(false, "MapProjection doesn't have implementation in MapUnproject"); return mapPos;
	}
}

#endif //  _MAP_PROJECTIONS_H
