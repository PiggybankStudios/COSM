/*
File:   parse_xml.c
Author: Taylor Robbins
Date:   07\07\2025
Description: 
	** None
*/

const char* GetHoxmlCodeStr(hoxml_code_t code)
{
	switch (code)
	{
		case HOXML_ERROR_INVALID_INPUT:                     return "HOXML_ERROR_INVALID_INPUT";
		case HOXML_ERROR_INTERNAL:                          return "HOXML_ERROR_INTERNAL";
		case HOXML_ERROR_INSUFFICIENT_MEMORY:               return "HOXML_ERROR_INSUFFICIENT_MEMORY";
		case HOXML_ERROR_UNEXPECTED_EOF:                    return "HOXML_ERROR_UNEXPECTED_EOF";
		case HOXML_ERROR_SYNTAX:                            return "HOXML_ERROR_SYNTAX";
		case HOXML_ERROR_ENCODING:                          return "HOXML_ERROR_ENCODING";
		case HOXML_ERROR_TAG_MISMATCH:                      return "HOXML_ERROR_TAG_MISMATCH";
		case HOXML_ERROR_INVALID_DOCUMENT_TYPE_DECLARATION: return "HOXML_ERROR_INVALID_DOCUMENT_TYPE_DECLARATION";
		case HOXML_ERROR_INVALID_DOCUMENT_DECLARATION:      return "HOXML_ERROR_INVALID_DOCUMENT_DECLARATION";
		case HOXML_END_OF_DOCUMENT:                         return "HOXML_END_OF_DOCUMENT";
		case HOXML_ELEMENT_BEGIN:                           return "HOXML_ELEMENT_BEGIN";
		case HOXML_ELEMENT_END:                             return "HOXML_ELEMENT_END";
		case HOXML_ATTRIBUTE:                               return "HOXML_ATTRIBUTE";
		case HOXML_PROCESSING_INSTRUCTION_BEGIN:            return "HOXML_PROCESSING_INSTRUCTION_BEGIN";
		case HOXML_PROCESSING_INSTRUCTION_END:              return "HOXML_PROCESSING_INSTRUCTION_END";
		default: return UNKNOWN_STR;
	}
}

Result TryParseXml(Str8 xmlContents, Arena* arena, XmlFile* fileOut)
{
	NotNullStr(xmlContents);
	NotNull(arena);
	NotNull(fileOut);
	ScratchBegin1(scratch, arena);
	
	ClearPointer(fileOut);
	fileOut->arena = arena;
	InitVarArray(XmlElement, &fileOut->roots, arena);
	
	u32 stackSize = 0;
	XmlElement* stack[XML_MAX_DEPTH];
	
	uxx hoxmlBufferSize = MaxUXX(xmlContents.length, 128);
	char* hoxmlBuffer = (char*)AllocMem(scratch, hoxmlBufferSize);
	hoxml_context_t hoxml;
	hoxml_init(&hoxml, hoxmlBuffer, (size_t)hoxmlBufferSize);
	
	bool insideProcessingInstruction = false;
	Result result = Result_None;
	hoxml_code_t code;
	while ((code = hoxml_parse(&hoxml, xmlContents.chars, (size_t)xmlContents.length)) != HOXML_END_OF_DOCUMENT)
	{
		switch (code)
		{
			case HOXML_PROCESSING_INSTRUCTION_BEGIN: insideProcessingInstruction = true; break;
			
			case HOXML_ELEMENT_BEGIN:
			{
				if (stackSize >= XML_MAX_DEPTH) { result = Result_StackOverflow; break; }
				
				XmlElement* parent = (stackSize > 0) ? parent = stack[stackSize-1] : nullptr;
				VarArray* elementArray = (parent != nullptr) ? &parent->children : &fileOut->roots;
				XmlElement* newElement = VarArrayAdd(XmlElement, elementArray);
				NotNull(newElement);
				ClearPointer(newElement);
				newElement->type = AllocStr8Nt(arena, hoxml.tag);
				InitVarArray(XmlAttribute, &newElement->attributes, arena);
				InitVarArray(XmlElement, &newElement->children, arena);
				stack[stackSize] = newElement;
				stackSize++;
				fileOut->numElements++;
			} break;
			
			case HOXML_ELEMENT_END:
			{
				if (stackSize == 0) { result = Result_UnexpectedEndElement; break; }
				XmlElement* element = stack[stackSize-1];
				if (!StrExactEquals(StrLit(hoxml.tag), element->type)) { result = Result_WrongEndElementType; break; }
				stackSize--;
			} break;
			
			case HOXML_ATTRIBUTE:
			{
				// if (insideProcessingInstruction) { PrintLine_D("Ignoring processing attribute %s", hoxml.attribute); break; }
				if (stackSize == 0) { result = Result_UnexpectedAttribute; break; }
				XmlElement* element = stack[stackSize-1];
				XmlAttribute* newAttribute = VarArrayAdd(XmlAttribute, &element->attributes);
				NotNull(newAttribute);
				ClearPointer(newAttribute);
				newAttribute->key = AllocStr8Nt(arena, hoxml.attribute);
				newAttribute->value = AllocStr8Nt(arena, hoxml.value);
			} break;
			
			case HOXML_PROCESSING_INSTRUCTION_END: insideProcessingInstruction = false; break;
			
			case HOXML_ERROR_INSUFFICIENT_MEMORY: result = Result_Overflow; break;
			case HOXML_ERROR_INVALID_INPUT: result = Result_InvalidInput; break;
			case HOXML_ERROR_INTERNAL: result = Result_Failure; break;
			case HOXML_ERROR_UNEXPECTED_EOF: result = Result_NoMoreBytes; break;
			case HOXML_ERROR_SYNTAX: result = Result_InvalidSyntax; break;
			case HOXML_ERROR_ENCODING: result = Result_InvalidUtf8; break;
			case HOXML_ERROR_TAG_MISMATCH: result = Result_WrongEndElementType; break;
			case HOXML_ERROR_INVALID_DOCUMENT_TYPE_DECLARATION: result = Result_MissingFileHeader; break;
			case HOXML_ERROR_INVALID_DOCUMENT_DECLARATION: result = Result_MissingFileHeader; break;
			
			default: PrintLine_W("Unhandled HOXML code: \"%s\"", GetHoxmlCodeStr(code)); break;
		}
		
		if (result != Result_None) { break; }
	}
	
	if (result == Result_None)
	{
		if (fileOut->roots.length == 0) { result = Result_EmptyFile; }
		else if (stackSize > 0) { result = Result_MissingEndElement; }
		else { result = Result_Success; }
	}
	
	ScratchEnd(scratch);
	return result;
}

XmlElement* XmlGetOneChild(XmlFile* file, XmlElement* parent, Str8 type)
{
	NotNull(file);
	VarArray* childArray = (parent != nullptr) ? &parent->children : &file->roots;
	XmlElement* result = nullptr;
	VarArrayLoop(childArray, cIndex)
	{
		VarArrayLoopGet(XmlElement, child, childArray, cIndex);
		if (StrExactEquals(child->type, type))
		{
			if (result != nullptr) { file->error = Result_Duplicate; return nullptr; }
			result = child;
		}
	}
	if (result == nullptr) { file->error = Result_NotFound; }
	return result;
}

XmlElement* XmlGetChild(XmlFile* file, XmlElement* parent, Str8 type, u64 index)
{
	NotNull(file);
	VarArray* childArray = (parent != nullptr) ? &parent->children : &file->roots;
	XmlElement* result = nullptr;
	u64 findIndex = 0;
	VarArrayLoop(childArray, cIndex)
	{
		VarArrayLoopGet(XmlElement, child, childArray, cIndex);
		if (StrExactEquals(child->type, type))
		{
			if (findIndex >= index) { return child; }
			findIndex++;
		}
	}
	if (result == nullptr) { file->error = Result_NotFound; }
	return result;
}

Str8 XmlGetAttribute(XmlFile* file, XmlElement* element, Str8 attributeName)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName)) { return attribute->value; }
	}
	file->error = Result_NotFound;
	return Str8_Empty;
}

Str8 XmlGetAttributeOrDefault(XmlFile* file, XmlElement* element, Str8 attributeName, Str8 defaultValue)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName)) { return attribute->value; }
	}
	return defaultValue;
}

r32 XmlGetAttributeR32(XmlFile* file, XmlElement* element, Str8 attributeName)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName))
		{
			r32 valueR32 = 0.0f;
			Result error = Result_None;
			if (TryParseR32(attribute->value, &valueR32, &error)) { return valueR32; }
			else { file->error = Result_FloatParseFailure; return 0.0f; }
		}
	}
	file->error = Result_NotFound;
	return 0.0f;
}
r32 XmlGetAttributeR32OrDefault(XmlFile* file, XmlElement* element, Str8 attributeName, r32 defaultValue)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName))
		{
			r32 valueR32 = 0.0f;
			Result error = Result_None;
			if (TryParseR32(attribute->value, &valueR32, &error)) { return valueR32; }
			else { file->error = Result_FloatParseFailure; return defaultValue; }
		}
	}
	return defaultValue;
}

r64 XmlGetAttributeR64(XmlFile* file, XmlElement* element, Str8 attributeName)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName))
		{
			r64 valueR64 = 0.0;
			Result error = Result_None;
			if (TryParseR64(attribute->value, &valueR64, &error)) { return valueR64; }
			else { file->error = Result_FloatParseFailure; return 0.0; }
		}
	}
	file->error = Result_NotFound;
	return 0.0;
}
r64 XmlGetAttributeR64OrDefault(XmlFile* file, XmlElement* element, Str8 attributeName, r64 defaultValue)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName))
		{
			r64 valueR64 = 0.0;
			Result error = Result_None;
			if (TryParseR64(attribute->value, &valueR64, &error)) { return valueR64; }
			else { file->error = Result_FloatParseFailure; return defaultValue; }
		}
	}
	return defaultValue;
}

u64 XmlGetAttributeU64(XmlFile* file, XmlElement* element, Str8 attributeName)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName))
		{
			u64 valueU64 = 0;
			Result error = Result_None;
			if (TryParseU64(attribute->value, &valueU64, &error)) { return valueU64; }
			else { file->error = Result_FloatParseFailure; return 0; }
		}
	}
	file->error = Result_NotFound;
	return 0;
}
u64 XmlGetAttributeU64OrDefault(XmlFile* file, XmlElement* element, Str8 attributeName, u64 defaultValue)
{
	NotNull(file);
	NotNull(element);
	VarArrayLoop(&element->attributes, aIndex)
	{
		VarArrayLoopGet(XmlAttribute, attribute, &element->attributes, aIndex);
		if (StrExactEquals(attribute->key, attributeName))
		{
			u64 valueU64 = 0;
			Result error = Result_None;
			if (TryParseU64(attribute->value, &valueU64, &error)) { return valueU64; }
			else { file->error = Result_FloatParseFailure; return defaultValue; }
		}
	}
	return defaultValue;
}

#define XmlGetOneChildOrBreak(file, parent, type) XmlGetOneChild((file), (parent), (type)); if ((file)->error != Result_None) { break; }
#define XmlGetChildOrBreak(file, parent, type, index) XmlGetChild((file), (parent), (type), (index)); if ((file)->error != Result_None) { break; }
#define XmlGetAttributeOrBreak(file, element, attributeName) XmlGetAttribute((file), (element), (attributeName)); if ((file)->error != Result_None) { break; }
#define XmlGetAttributeR32OrBreak(file, element, attributeName) XmlGetAttributeR32((file), (element), (attributeName)); if ((file)->error != Result_None) { break; }
#define XmlGetAttributeR64OrBreak(file, element, attributeName) XmlGetAttributeR64((file), (element), (attributeName)); if ((file)->error != Result_None) { break; }
#define XmlGetAttributeU64OrBreak(file, element, attributeName) XmlGetAttributeU64((file), (element), (attributeName)); if ((file)->error != Result_None) { break; }
