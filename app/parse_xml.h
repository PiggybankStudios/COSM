/*
File:   parse_xml.h
Author: Taylor Robbins
Date:   07\07\2025
*/

#ifndef _PARSE_XML_H
#define _PARSE_XML_H

#define XML_MAX_DEPTH 16

typedef plex XmlAttribute XmlAttribute;
plex XmlAttribute
{
	Str8 key;
	Str8 value;
};

typedef plex XmlElement XmlElement;
plex XmlElement
{
	Str8 type;
	VarArray attributes; //XmlAttribute
	VarArray children; //XmlElement
};

typedef plex XmlFile XmlFile;
plex XmlFile
{
	Arena* arena;
	u64 numElements;
	VarArray roots; //XmlElement
	Result error;
};

#endif //  _PARSE_XML_H
