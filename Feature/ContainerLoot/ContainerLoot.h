#ifndef __CONTAINER_LOOT_H
#define __CONTAINER_LOOT_H

#include "Feature/ContainerLoot/ContainerLootTypes.h"

// travnik: forward declarations, чтобы не тянуть тяжёлые заголовки в стратегический код
class SOLDIERTYPE;
struct TAG_STRUCTURE;

// travnik: фасад фичи. Этап 0-1 — аудит карты и каркас вызовов.
// Генерация лута, Lua-мост и ловушки подключаются на следующих этапах.
class ContainerLoot
{
public:
	// travnik: вызывается из EnterSector() после загрузки карты и temp-предметов
	static void ProcessSector(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ);

	// travnik: временная диагностика для проверки, доходит ли hook до ProcessSector
	static void Trace(const char* message);

	// travnik: flush buffered trace output without closing the log stream.
	static void FlushTrace();

	// travnik: один раз за запуск сканирует все карты (debug-инструмент).
	static void ScanAllMapsOnce();

	// travnik: вызывается из HandleOpenableStruct() перед сменой не-дверной
	// openable-структуры. Принудительно показывает пул предметов, иначе лут
	// может остаться в HIDDEN_IN_OBJECT.
	static BOOLEAN OnContainerOpening(SOLDIERTYPE* pSoldier, INT32 sGridNo, TAG_STRUCTURE* pStructure);

	// travnik: сбрасывает process-wide состояние фичи при новой игре/загрузке.
	static void Reset();

	// travnik: seed текущей кампании для стабильного выбора сайтов под лимиты.
	static void SetCampaignSeed(UINT32 seed);
	static UINT32 GetCampaignSeed();

	// travnik: create a non-zero 32-bit seed without overflowing NewRandom().
	static UINT32 CreateCampaignSeed();
};

#endif
