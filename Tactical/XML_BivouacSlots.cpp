#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include "sgp.h"
	#include "Debug Control.h"
	#include "expat.h"
	#include "XML.h"
	#include "SquadBivouacManager.h"
	#include "GameSettings.h"
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// SQUAD_BIVOUAC: Items\BivouacSlots.xml
//
// Per-slot bivouac configuration. One <BIVOUACSLOT> entry per slot:
//   uiIndex        slot number 0..BIVOUAC_NUM_SLOTS-1 (required)
//   pPocketIndex   pocket template index (pIndex) from Pockets.xml; the NIV pocket
//                  structure supplies item capacity per size, class restrictions
//                  and the silhouette graphic of the slot
//
// Slots missing from the file keep the INI default pocket index
// (BIVOUAC_SLOT_DEFAULT_POCKET from Ja2_Options.ini).
////////////////////////////////////////////////////////////////////////////////////////////////////

struct
{
	PARSE_STAGE	curElement;

	CHAR8		szCharData[MAX_CHAR_DATA_LENGTH+1];

	BivouacSlotConfig	curConfig;
	UINT32			curIndex;
	BOOLEAN			fHaveIndex;

	UINT32			currentDepth;
	UINT32			maxReadDepth;
}
typedef bivouacslotParseData;

static void XMLCALL
bivouacslotStartElementHandle(void *userData, const XML_Char *name, const XML_Char **atts)
{
	bivouacslotParseData * pData = (bivouacslotParseData *)userData;

	if(pData->currentDepth <= pData->maxReadDepth)
	{
		if(strcmp(name, "BIVOUACSLOTLIST") == 0 && pData->curElement == ELEMENT_NONE)
		{
			pData->curElement = ELEMENT_LIST;

			pData->maxReadDepth++;
		}
		else if(strcmp(name, "BIVOUACSLOT") == 0 && pData->curElement == ELEMENT_LIST)
		{
			pData->curElement = ELEMENT;

			pData->curConfig.pPocketIndex = BIVOUAC_SLOT_POCKET_NONE;
			pData->curIndex = 0;
			pData->fHaveIndex = FALSE;

			pData->maxReadDepth++;
		}
		else if(pData->curElement == ELEMENT &&
			(strcmp(name, "uiIndex") == 0 ||
			strcmp(name, "pPocketIndex") == 0))
		{
			pData->curElement = ELEMENT_PROPERTY;

			pData->maxReadDepth++;
		}

		pData->szCharData[0] = '\0';
	}

	pData->currentDepth++;

}

static void XMLCALL
bivouacslotCharacterDataHandle(void *userData, const XML_Char *str, int len)
{
	bivouacslotParseData * pData = (bivouacslotParseData *)userData;

	if( (pData->currentDepth <= pData->maxReadDepth) &&
		(strlen(pData->szCharData) < MAX_CHAR_DATA_LENGTH)
	){
		strncat(pData->szCharData,str,__min((unsigned int)len,MAX_CHAR_DATA_LENGTH-strlen(pData->szCharData)));
	}
}

static void XMLCALL
bivouacslotEndElementHandle(void *userData, const XML_Char *name)
{
	bivouacslotParseData * pData = (bivouacslotParseData *)userData;

	if(pData->currentDepth <= pData->maxReadDepth)
	{
		if(strcmp(name, "BIVOUACSLOTLIST") == 0)
		{
			pData->curElement = ELEMENT_NONE;
		}
		else if(strcmp(name, "BIVOUACSLOT") == 0)
		{
			pData->curElement = ELEMENT_LIST;

			// write the config into the global slot array (only slots with a valid uiIndex)
			if( pData->fHaveIndex )
			{
				SQBM().SetSlotConfig( pData->curIndex, pData->curConfig );
			}
		}
		else if(strcmp(name, "uiIndex") == 0)
		{
			pData->curElement = ELEMENT;
			pData->curIndex = (UINT32) strtoul(pData->szCharData, NULL, 0);
			pData->fHaveIndex = TRUE;
		}
		else if(strcmp(name, "pPocketIndex") == 0)
		{
			pData->curElement = ELEMENT;
			pData->curConfig.pPocketIndex = (UINT8) strtoul(pData->szCharData, NULL, 0);
		}

		pData->maxReadDepth--;
	}

	pData->currentDepth--;
}

BOOLEAN ReadInBivouacSlotRules(STR fileName)
{
	HWFILE		hFile;
	UINT32		uiBytesRead;
	UINT32		uiFSize;
	CHAR8 *		lpcBuffer;
	XML_Parser	parser = XML_ParserCreate(NULL);

	bivouacslotParseData pData;

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Loading BivouacSlots.xml" );

	// Open file
	hFile = FileOpen( fileName, FILE_ACCESS_READ, FALSE );
	if ( !hFile )
		return( FALSE );

	uiFSize = FileGetSize(hFile);
	lpcBuffer = (CHAR8 *) MemAlloc(uiFSize+1);

	//Read in block
	if ( !FileRead( hFile, lpcBuffer, uiFSize, &uiBytesRead ) )
	{
		MemFree(lpcBuffer);
		return( FALSE );
	}

	lpcBuffer[uiFSize] = 0;

	FileClose( hFile );

	XML_SetElementHandler(parser, bivouacslotStartElementHandle, bivouacslotEndElementHandle);
	XML_SetCharacterDataHandler(parser, bivouacslotCharacterDataHandle);

	memset(&pData,0,sizeof(pData));

	XML_SetUserData(parser, &pData);

	if(!XML_Parse(parser, lpcBuffer, uiFSize, TRUE))
	{
		CHAR8 errorBuf[511];

		sprintf(errorBuf, "XML Parser Error in BivouacSlots.xml: %s at line %d", XML_ErrorString(XML_GetErrorCode(parser)), XML_GetCurrentLineNumber(parser));
		LiveMessage(errorBuf);

		MemFree(lpcBuffer);
		return FALSE;
	}

	MemFree(lpcBuffer);

	XML_ParserFree(parser);

	return( TRUE );
}
