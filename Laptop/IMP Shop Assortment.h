#ifndef __IMP_SHOP_ASSORTMENT_H_
#define __IMP_SHOP_ASSORTMENT_H_

/**
* @file
* travnik: внешний ассортимент магазина снаряжения IMP (IMPShopItems.xml).
*
* Файл дополняет пул предметов, который собирается из IMPItemChoices.xml:
* позволяет добавить в магазин любой предмет, задать ему цену, крутость,
* привязку к набору и лимит количества, а также исключить предмет из ассортимента.
*
* При IMP_SHOP_COOLNESS = 0 работает прежняя логика: ассортимент собирается
* из наборов IMPItemChoices.xml и дополняется записями этого файла.
* При IMP_SHOP_COOLNESS > 0 пул из IMPItemChoices.xml дополняется предметами
* Items.xml, крутость которых не выше значения настройки.
* Доступность по крутости: крутость 1 — с начала кампании, каждая следующая
* ступень требует ещё IMP_SHOP_COOLNESS_STEP процентов прогресса.
*/

#include "Types.h"

#include <map>
#include <set>

// travnik: длина строкового комментария записи (в игре не используется)
#define IMP_SHOP_COMMENT_LENGTH			64
// travnik: значение ubKit - предмет не привязан к набору и доступен всегда
#define IMP_SHOP_KIT_ALWAYS				255
// travnik: шаг перевода крутости в процент прогресса кампании:
// крутость 1 = доступно с начала игры (0%), каждая следующая +10% (крутость 10 = 90%)
#define IMP_SHOP_COOLNESS_STEP			10

// travnik: одна запись IMPShopItems.xml
struct IMP_SHOP_ITEM
{
	UINT16	usItem;				// id предмета из Items.xml
	UINT32	usPrice;			// цена в магазине IMP, 0 - взять Item[].usPrice
	UINT8	ubCoolness;			// крутость в магазине IMP, 0 - взять Item[].ubCoolness
	UINT8	ubKit;				// привязка к набору IMPItemChoices, IMP_SHOP_KIT_ALWAYS - без привязки
	UINT8	ubMaxAmount;		// максимум копий в снаряжении, 0 - без ограничения
	BOOLEAN	fForceAvailable;	// 1 - добавлять предмет, игнорируя ItemIsLegal
	BOOLEAN	fRemove;			// 1 - исключить предмет из ассортимента
	CHAR8	szComment[IMP_SHOP_COMMENT_LENGTH];
};

// travnik: синглтон, хранящий внешние записи ассортимента магазина IMP
class IMPShopAssortment
{
public:
	static IMPShopAssortment& Instance();

	// travnik: загрузка XML; FALSE при ошибке разбора - ассортимент остаётся базовым
	BOOLEAN Load( STR fileName );
	void Reset();

	BOOLEAN IsLoaded() const { return m_fLoaded; }

	// travnik: дополнить пул предметов магазина записями из XML
	void ApplyToPool( std::set<UINT16>& pool, const std::set<UINT8>& activeKits ) const;

	// travnik: TRUE - пул дополняется предметами Items.xml (IMP_SHOP_COOLNESS > 0)
	BOOLEAN IsCatalogMode() const;
	// travnik: дополнить пул предметами Items.xml с крутостью не выше IMP_SHOP_COOLNESS
	void BuildCatalogPool( std::set<UINT16>& pool ) const;
	// travnik: убрать из пула случайные предметы (randomitem)
	void RemoveRandomItems( std::set<UINT16>& pool ) const;

	// travnik: цена предмета для магазина IMP (с учётом переопределения из XML)
	INT32 GetPrice( UINT16 usItem ) const;
	// travnik: крутость предмета для магазина IMP (с учётом переопределения из XML)
	UINT8 GetCoolness( UINT16 usItem ) const;
	// travnik: максимум копий предмета в снаряжении (0 - без ограничения)
	UINT8 GetMaxAmount( UINT16 usItem ) const;

private:
	IMPShopAssortment();

	BOOLEAN IsItemAllowed( const IMP_SHOP_ITEM& entry ) const;
	BOOLEAN IsCoolnessAllowed( const IMP_SHOP_ITEM& entry ) const;
	const IMP_SHOP_ITEM* Find( UINT16 usItem ) const;

	std::map<UINT16, IMP_SHOP_ITEM>	m_entries;
	BOOLEAN							m_fLoaded;
	BOOLEAN							m_fRespectCoolness;
};

#endif
