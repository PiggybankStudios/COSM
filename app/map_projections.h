/*
File:   map_projections.h
Author: Taylor Robbins
Date:   07\06\2025
Descriptions:
	** Holds functions that help us
*/

#ifndef _MAP_PROJECTIONS_H
#define _MAP_PROJECTIONS_H

// +--------------------------------------------------------------+
// |                     Mercator Projection                      |
// +--------------------------------------------------------------+
// https://stackoverflow.com/questions/14329691/convert-latitude-longitude-point-to-a-pixels-x-y-on-mercator-projection
//TODO: Do we need 64-bit float v2 and rec?
v2 ProjectMercator(v2d geoLoc, rec mapRec)
{
	v2 result;
	result.X = mapRec.X + ((r32)(geoLoc.Longitude + 180.0) * (mapRec.Width / 360.0f));
	r64 latRadians = ToRadians64(geoLoc.Latitude);
	r64 mercN = LnR64(TanR64(QuarterPi64 + (latRadians/2.0)));
	result.Y = mapRec.Y + (mapRec.Height/2.0f) - (mapRec.Width * (r32)mercN / TwoPi32);
	return result;
}

v2d UnprojectMercator(v2 mapPos, rec mapRec)
{
	//TODO: Implement me!
	return V2d_Zero;
}

#endif //  _MAP_PROJECTIONS_H
