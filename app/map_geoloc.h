/*
File:   map_geoloc.h
Author: Taylor Robbins
Date:   07\06\2025
*/

#ifndef _MAP_GEOLOC_H
#define _MAP_GEOLOC_H

typedef union GeoLoc GeoLoc;
union GeoLoc
{
	r64 values[2];
	struct
	{
		union { r64 lon; r64 longitude; }; //aka phi
		union { r64 lat; r64 latitude;  }; //aka omega
	};
};

static inline GeoLoc NewGeoLoc(r64 latitude, r64 longitude)
{
	GeoLoc result;
	result.latitude = latitude;
	result.longitude = longitude;
	return result;
}

#define GeoLoc_Zero NewGeoLoc(0.0, 0.0)
#define GeoLoc_Zero_Const (GeoLoc){ 0.0, 0.0 }

#endif //  _MAP_GEOLOC_H
