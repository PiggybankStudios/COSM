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

void FreeXmlParser(XmlParser* parser)
{
	NotNull(parser);
	if (parser->arena != nullptr)
	{
		if (parser->infos != nullptr) { FreeArray(SrlInfo, parser->arena, parser->numInfos, parser->infos); }
		if (parser->stack != nullptr) { FreeArray(XmlStackElem, parser->arena, parser->maxStackSize, parser->stack); }
		if (xmlStrIsOwned) { FreeStr8(parser->arena, &parser->xmlStr); }
		if (parser->errorMessage.chars != nullptr) { FreeStr8(parser->arena, &parser->errorMessage); }
		if (parserOut->hoxmlBuffer != nullptr) { FreeMem(parser->arena, parser->hoxmlBuffer, parser->hoxmlBufferSize); }
	}
	ClearPointer(parser);
}

const SrlInfo* FindSerialInfo(u64 numSrlInfos, const SrlInfo* srlInfos, Str8 name)
{
	
}

XmlResult TryParseXml(Arena* arena, Str8 xmlData, u64 numSrlInfos, const SrlInfo* srlInfos, u64 maxStackSize)
{
	NotNull(arena);
	NotNullStr(xmlData);
	Assert(numSrlInfos > 0 && srlInfos != nullptr);
	Assert(maxStackSize > 0);
	XmlResult result = ZEROED;
	
	ScratchBegin1(scratch, arena);
	{
		u64 stackSize = 0;
		XmlStackElem* stack = AllocArray(XmlStackElem, scratch, maxStackSize);
		NotNull(stack);
		MyMemSet(stack, 0x00, sizeof(XmlStackElem) * maxStackSize);
		
		u64 hoxmlBufferSize = xmlData.length; //TODO: Hone this allocation better? Or investigate how to dynamically allocate?
		u8* hoxmlBuffer = (u8*)AllocMem(scratch, hoxmlBufferSize);
		NotNull(hoxmlBuffer);
		hoxml_context_t hoxmlContext;
		hoxml_init(&hoxmlContext, hoxmlBuffer, hoxmlBufferSize);
		
		hoxml_code_t hoxmlCode;
		while ((hoxmlCode = hoxml_parse(&hoxmlContext, xmlData.chars, xmlData.length)) != HOXML_END_OF_DOCUMENT)
		{
			if (hoxmlCode == HOXML_ELEMENT_BEGIN)
			{
				if (stackSize == 0)
				{
					
				}
			}
			else
			{
				PrintLine_W("Unhandled hoxml code %u %s", hoxmlCode, GetHoxmlCodeStr(hoxmlCode));
			}
		}
	}
	ScratchEnd(scratch);
	
	return result;
}

XmlParserResult XmlParseStep(XmlParser* parser)
{
	NotNull(parser);
	NotNull(parser->arena);
	XmlParserResult result = ZEROED;
	if (parser->isFinished) { result.isFinished = true; return result; }
	
	hoxml_code_t hoxmlCode;
	while ((hoxmlCode = hoxml_parse(&parser->hoxmlContext, parser->xmlStr.chars, parser->xmlStr.length)) != HOXML_END_OF_DOCUMENT)
	{
		switch (hoxmlCode)
		{
			case HOXML_ERROR_INSUFFICIENT_MEMORY:
			{
				result = XmlCreateError(parser, false,
					"Not enough xml parsing memory allocated! Allocated %llu bytes for %llu char xml string. Parsed %llu xml elements before running out of memory, stack is %llu elements deep",
					parser->hoxmlBufferSize,
					parser->xmlStr.length,
					parser->numXmlTokensParsed, parser->stackSize
				);
				return result;
			} break;
			
			default:
			{
				result = XmlCreateError(parser, true, "Unhandled hoxml code: %u %s", (u32)hoxmlCode, GetHoxmlCodeStr(hoxmlCode));
				return result;
			} break;
		}
	}
	
	parser->isFinished = true;
	result.isFinished = true;
	return result;
}
