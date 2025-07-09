/*
File:   parse_xml.h
Author: Taylor Robbins
Date:   07\07\2025
*/

#ifndef _PARSE_XML_H
#define _PARSE_XML_H

typedef enum SrlMemberType SrlMemberType;
enum SrlMemberType
{
	SrlMemberType_None = 0,
	SrlMemberType_Bool,
	SrlMemberType_U8,
	SrlMemberType_U16,
	SrlMemberType_U32,
	SrlMemberType_U64,
	SrlMemberType_UXX,
	SrlMemberType_I8,
	SrlMemberType_I16,
	SrlMemberType_I32,
	SrlMemberType_I64,
	SrlMemberType_IXX,
	SrlMemberType_R32,
	SrlMemberType_R64,
	SrlMemberType_Str8,
	SrlMemberType_VarArray,
	SrlMemberType_SrlType,
	SrlMemberType_Count,
};

typedef struct SrlMemberInfo SrlMemberInfo;
struct SrlMemberInfo
{
	bool optional;
	const char* serializedName;
	uxx offset;
	uxx size;
	SrlMemberType type;
	const char* srlTypeName; //for VarArray and SrlType
};

typedef struct SrlInfo SrlInfo;
struct SrlInfo
{
	const char* serializedName;
	uxx structSize;
	uxx numMembers;
	SrlMemberInfo members[64];
};

#define SRL_MEMBER_CHILDREN_NAME "[CHILDREN]"
#define SRL_REQ false
#define SRL_OPT true
#define SRL_MAX_NUM_MEMBERS 64

typedef struct XmlStackElem XmlStackElem;
struct XmlStackElem
{
	const SrlInfo* info;
	SrlMemberInfo* childrenMemberInfo;
	const SrlInfo* childrenTypeInfo;
	
	u64 structSize;
	void* structPntr;
	VarArray* childrenVarArray;
	
	bool foundMember[SRL_MAX_NUM_MEMBERS];
	
	u64 openTagLine;
	u64 closeTagLine;
};

typedef struct XmlResult XmlResult;
struct XmlResult
{
	Arena* arena;
	Result result;
	const SrlInfo* rootInfo;
	u64 rootSize;
	void* rootPntr;
};

#endif //  _PARSE_XML_H
