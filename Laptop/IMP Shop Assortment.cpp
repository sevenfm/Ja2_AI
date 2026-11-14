/**
* @file
* travnik: загрузка и применение внешнего ассортимента магазина снаряжения IMP.
*/

#include "IMP Shop Assortment.h"

#ifdef PRECOMPILEDHEADERS
	#include "Laptop All.h"
#else
	#include "builddefines.h"
	#include <stdio.h>
	#include <string.h>
	#include "sgp.h"
	#include "expat.h"
	#include "FileMan.h"
	#include "MemMan.h"
	#include "Debug.h"
	#include "Debug Control.h"
	#include "Item Types.h"
	#include "Items.h"
	#include "Campaign.h"
#endif

#include "XML.h"

// travnik: перечисление полей IMPShopItems.xml, чтобы не сравнивать строки дважды
enum IMP_SHOP_FIELD
{
	IMP_SHOP_FIELD_NONE = 0,
	IMP_SHOP_FIELD_RESPECT_COOLNESS,
	IMP_SHOP_FIELD_INDEX,
	IMP_SHOP_FIELD_ITEM,
	IMP_SHOP_FIELD_PRICE,
	IMP_SHOP_FIELD_COOLNESS,
	IMP_SHOP_FIELD_KIT,
	IMP_SHOP_FIELD_MAX_AMOUNT,
	IMP_SHOP_FIELD_FORCE_AVAILABLE,
	IMP_SHOP_FIELD_REMOVE,
	IMP_SHOP_FIELD_COMMENT
};

typedef struct
{
	PARSE_STAGE			curElement;
	IMP_SHOP_FIELD		curField;
	BOOLEAN				fFieldInList;		// travnik: TRUE, если читается свойство корневого уровня
	CHAR8				szCharData[MAX_CHAR_DATA_LENGTH+1];
	IMP_SHOP_ITEM		curItem;
	UINT32				uiIndex;
	BOOLEAN				fRespectCoolness;
	std::map<UINT16, IMP_SHOP_ITEM>*	pEntries;

	UINT32				currentDepth;
	UINT32				maxReadDepth;
} IMPShopParseData;


static IMP_SHOP_FIELD IMPShopGetFieldByName( const XML_Char* name )
{
	if ( strcmp( name, "fRespectCoolness" ) == 0 )	return IMP_SHOP_FIELD_RESPECT_COOLNESS;
	if ( strcmp( name, "uiIndex" ) == 0 )			return IMP_SHOP_FIELD_INDEX;
	if ( strcmp( name, "usItem" ) == 0 )			return IMP_SHOP_FIELD_ITEM;
	if ( strcmp( name, "usPrice" ) == 0 )			return IMP_SHOP_FIELD_PRICE;
	if ( strcmp( name, "ubCoolness" ) == 0 )		return IMP_SHOP_FIELD_COOLNESS;
	if ( strcmp( name, "ubKit" ) == 0 )				return IMP_SHOP_FIELD_KIT;
	if ( strcmp( name, "ubMaxAmount" ) == 0 )		return IMP_SHOP_FIELD_MAX_AMOUNT;
	if ( strcmp( name, "fForceAvailable" ) == 0 )	return IMP_SHOP_FIELD_FORCE_AVAILABLE;
	if ( strcmp( name, "fRemove" ) == 0 )			return IMP_SHOP_FIELD_REMOVE;
	if ( strcmp( name, "szComment" ) == 0 )			return IMP_SHOP_FIELD_COMMENT;

	return IMP_SHOP_FIELD_NONE;
}


static UINT8 IMPShopClampToByte( INT32 iValue )
{
	if ( iValue < 0 )
		return 0;

	if ( iValue > 255 )
		return 255;

	return (UINT8)iValue;
}


static void IMPShopApplyField( IMPShopParseData* pData )
{
	INT32 iValue = atol( pData->szCharData );

	switch ( pData->curField )
	{
	case IMP_SHOP_FIELD_RESPECT_COOLNESS:
		pData->fRespectCoolness = ( iValue != 0 );
		break;

	case IMP_SHOP_FIELD_INDEX:
		pData->uiIndex = ( iValue < 0 ) ? 0 : (UINT32)iValue;
		break;

	case IMP_SHOP_FIELD_ITEM:
		pData->curItem.usItem = ( iValue < 0 ) ? 0 : (UINT16)iValue;
		break;

	case IMP_SHOP_FIELD_PRICE:
		pData->curItem.usPrice = ( iValue < 0 ) ? 0 : (UINT32)iValue;
		break;

	case IMP_SHOP_FIELD_COOLNESS:
		pData->curItem.ubCoolness = IMPShopClampToByte( iValue );
		break;

	case IMP_SHOP_FIELD_KIT:
		pData->curItem.ubKit = IMPShopClampToByte( iValue );
		break;

	case IMP_SHOP_FIELD_MAX_AMOUNT:
		pData->curItem.ubMaxAmount = IMPShopClampToByte( iValue );
		break;

	case IMP_SHOP_FIELD_FORCE_AVAILABLE:
		pData->curItem.fForceAvailable = ( iValue != 0 );
		break;

	case IMP_SHOP_FIELD_REMOVE:
		pData->curItem.fRemove = ( iValue != 0 );
		break;

	case IMP_SHOP_FIELD_COMMENT:
		strncpy( pData->curItem.szComment, pData->szCharData, IMP_SHOP_COMMENT_LENGTH - 1 );
		pData->curItem.szComment[IMP_SHOP_COMMENT_LENGTH - 1] = '\0';
		break;

	default:
		break;
	}
}


static void IMPShopStoreEntry( IMPShopParseData* pData )
{
	// travnik: запись без предмета бесполезна - пропускаем её
	if ( pData->curItem.usItem == 0 )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "IMPShopItems.xml: entry %d skipped, usItem is 0", pData->uiIndex ) );
		return;
	}

	if ( pData->curItem.usItem >= MAXITEMS )
	{
		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "IMPShopItems.xml: entry %d skipped, usItem %d out of range", pData->uiIndex, pData->curItem.usItem ) );
		return;
	}

	// travnik: при повторе предмета в файле применяется последняя запись
	( *pData->pEntries )[pData->curItem.usItem] = pData->curItem;
}


static void XMLCALL IMPShopStartElementHandle( void* userData, const XML_Char* name, const XML_Char** atts )
{
	IMPShopParseData* pData = (IMPShopParseData*)userData;

	if ( pData->currentDepth <= pData->maxReadDepth )
	{
		if ( strcmp( name, "IMPSHOPITEMS" ) == 0 && pData->curElement == ELEMENT_NONE )
		{
			pData->curElement = ELEMENT_LIST;
			pData->maxReadDepth++;
		}
		else if ( strcmp( name, "IMPSHOPITEM" ) == 0 && pData->curElement == ELEMENT_LIST )
		{
			pData->curElement = ELEMENT;

			memset( &pData->curItem, 0, sizeof( pData->curItem ) );
			pData->curItem.ubKit = IMP_SHOP_KIT_ALWAYS;
			pData->uiIndex = 0;

			pData->maxReadDepth++;
		}
		else if ( pData->curElement == ELEMENT_LIST )
		{
			const IMP_SHOP_FIELD field = IMPShopGetFieldByName( name );

			// travnik: свойства корневого уровня читаются вне записи предмета
			if ( field != IMP_SHOP_FIELD_NONE )
			{
				pData->curElement = ELEMENT_PROPERTY;
				pData->curField = field;
				pData->fFieldInList = TRUE;
				pData->maxReadDepth++;
			}
		}
		else if ( pData->curElement == ELEMENT && pData->curField == IMP_SHOP_FIELD_NONE )
		{
			const IMP_SHOP_FIELD field = IMPShopGetFieldByName( name );

			if ( field != IMP_SHOP_FIELD_NONE )
			{
				pData->curElement = ELEMENT_PROPERTY;
				pData->curField = field;
				pData->fFieldInList = FALSE;
				pData->maxReadDepth++;
			}
		}

		pData->szCharData[0] = '\0';
	}

	pData->currentDepth++;
}


static void XMLCALL IMPShopCharacterDataHandle( void* userData, const XML_Char* str, int len )
{
	IMPShopParseData* pData = (IMPShopParseData*)userData;

	if ( pData->currentDepth <= pData->maxReadDepth && strlen( pData->szCharData ) < MAX_CHAR_DATA_LENGTH )
	{
		strncat( pData->szCharData, str, __min( (unsigned int)len, MAX_CHAR_DATA_LENGTH - strlen( pData->szCharData ) ) );
	}
}


static void XMLCALL IMPShopEndElementHandle( void* userData, const XML_Char* name )
{
	IMPShopParseData* pData = (IMPShopParseData*)userData;

	if ( pData->currentDepth <= pData->maxReadDepth )
	{
		if ( strcmp( name, "IMPSHOPITEMS" ) == 0 && pData->curElement == ELEMENT_LIST )
		{
			pData->curElement = ELEMENT_NONE;
		}
		else if ( strcmp( name, "IMPSHOPITEM" ) == 0 && pData->curElement == ELEMENT )
		{
			IMPShopStoreEntry( pData );
			pData->curElement = ELEMENT_LIST;
		}
		else if ( pData->curElement == ELEMENT_PROPERTY )
		{
			IMPShopApplyField( pData );

			pData->curField = IMP_SHOP_FIELD_NONE;
			pData->curElement = pData->fFieldInList ? ELEMENT_LIST : ELEMENT;
		}

		pData->maxReadDepth--;
	}

	pData->currentDepth--;
}


IMPShopAssortment& IMPShopAssortment::Instance()
{
	// travnik: локальная статика - безопасная инициализация при первом обращении
	static IMPShopAssortment instance;
	return instance;
}


IMPShopAssortment::IMPShopAssortment()
{
	m_fLoaded = FALSE;
	m_fRespectCoolness = FALSE;
}


void IMPShopAssortment::Reset()
{
	m_entries.clear();
	m_fLoaded = FALSE;
	m_fRespectCoolness = FALSE;
}


BOOLEAN IMPShopAssortment::Load( STR fileName )
{
	Reset();

	HWFILE hFile = FileOpen( fileName, FILE_ACCESS_READ, FALSE );
	if ( !hFile )
		return FALSE;

	const UINT32 uiFSize = FileGetSize( hFile );
	CHAR8* lpcBuffer = (CHAR8*)MemAlloc( uiFSize + 1 );
	UINT32 uiBytesRead = 0;

	if ( !FileRead( hFile, lpcBuffer, uiFSize, &uiBytesRead ) )
	{
		MemFree( lpcBuffer );
		FileClose( hFile );
		return FALSE;
	}

	lpcBuffer[uiFSize] = 0;
	FileClose( hFile );

	IMPShopParseData pData;
	memset( &pData, 0, sizeof( pData ) );
	pData.pEntries = &m_entries;

	XML_Parser parser = XML_ParserCreate( NULL );

	XML_SetElementHandler( parser, IMPShopStartElementHandle, IMPShopEndElementHandle );
	XML_SetCharacterDataHandler( parser, IMPShopCharacterDataHandle );
	XML_SetUserData( parser, &pData );

	const BOOLEAN fParsed = ( XML_Parse( parser, lpcBuffer, uiFSize, TRUE ) != 0 );

	if ( !fParsed )
	{
		CHAR8 errorBuf[511];
		sprintf( errorBuf, "XML Parser Error in %s: %s at line %d", fileName, XML_ErrorString( XML_GetErrorCode( parser ) ), XML_GetCurrentLineNumber( parser ) );
		LiveMessage( errorBuf );

		m_entries.clear();
	}

	XML_ParserFree( parser );
	MemFree( lpcBuffer );

	if ( fParsed )
	{
		m_fRespectCoolness = pData.fRespectCoolness;
		m_fLoaded = TRUE;

		DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "IMPShopAssortment: loaded %d entries from %s", (int)m_entries.size(), fileName ) );
	}

	return fParsed;
}


const IMP_SHOP_ITEM* IMPShopAssortment::Find( UINT16 usItem ) const
{
	std::map<UINT16, IMP_SHOP_ITEM>::const_iterator it = m_entries.find( usItem );

	if ( it == m_entries.end() )
		return NULL;

	return &( it->second );
}


INT32 IMPShopAssortment::GetPrice( UINT16 usItem ) const
{
	if ( usItem >= MAXITEMS )
		return 0;

	const IMP_SHOP_ITEM* entry = Find( usItem );

	if ( entry && entry->usPrice > 0 )
		return (INT32)entry->usPrice;

	return (INT32)Item[usItem].usPrice;
}


UINT8 IMPShopAssortment::GetCoolness( UINT16 usItem ) const
{
	if ( usItem >= MAXITEMS )
		return 0;

	const IMP_SHOP_ITEM* entry = Find( usItem );

	if ( entry && entry->ubCoolness > 0 )
		return entry->ubCoolness;

	return Item[usItem].ubCoolness;
}


UINT8 IMPShopAssortment::GetMaxAmount( UINT16 usItem ) const
{
	const IMP_SHOP_ITEM* entry = Find( usItem );

	if ( entry )
		return entry->ubMaxAmount;

	return 0;
}


BOOLEAN IMPShopAssortment::IsItemAllowed( const IMP_SHOP_ITEM& entry ) const
{
	if ( entry.usItem >= gMAXITEMS_READ )
		return FALSE;

	if ( Item[entry.usItem].usItemClass == 0 )
		return FALSE;

	// travnik: заданная в XML крутость перекрывает проверку крутости внутри ItemIsLegal
	const BOOLEAN fIgnoreCoolness = ( entry.ubCoolness > 0 ) ? TRUE : FALSE;

	if ( ItemIsLegal( entry.usItem, fIgnoreCoolness ) )
		return TRUE;

	// travnik: оружие разрешаем всегда, чтобы IMP не остался без него
	if ( Item[entry.usItem].usItemClass == IC_GUN )
		return TRUE;

	return entry.fForceAvailable;
}


BOOLEAN IMPShopAssortment::IsCoolnessAllowed( const IMP_SHOP_ITEM& entry ) const
{
	if ( !m_fRespectCoolness || entry.ubCoolness == 0 )
		return TRUE;

	// travnik: крутость из XML переводится в процент прогресса кампании
	return ( entry.ubCoolness * IMP_SHOP_COOLNESS_STEP <= CurrentPlayerProgressPercentage() );
}


void IMPShopAssortment::ApplyToPool( std::set<UINT16>& pool, const std::set<UINT8>& activeKits ) const
{
	if ( m_entries.empty() )
		return;

	std::map<UINT16, IMP_SHOP_ITEM>::const_iterator itend = m_entries.end();

	for ( std::map<UINT16, IMP_SHOP_ITEM>::const_iterator it = m_entries.begin(); it != itend; ++it )
	{
		const IMP_SHOP_ITEM& entry = it->second;

		// travnik: fRemove действует всегда и перекрывает добавление из IMPItemChoices
		if ( entry.fRemove )
		{
			pool.erase( entry.usItem );
			continue;
		}

		// travnik: предмет, привязанный к набору, доступен только с этим набором
		if ( entry.ubKit != IMP_SHOP_KIT_ALWAYS && activeKits.find( entry.ubKit ) == activeKits.end() )
			continue;

		if ( !IsItemAllowed( entry ) || !IsCoolnessAllowed( entry ) )
			continue;

		pool.insert( entry.usItem );
	}
}
