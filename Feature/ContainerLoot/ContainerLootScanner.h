#ifndef __CONTAINER_LOOT_SCANNER_H
#define __CONTAINER_LOOT_SCANNER_H

#include "Feature/ContainerLoot/ContainerLootTypes.h"

// travnik: сканер карты. На этапе 0-1 он только собирает debug-аудит структур,
// чтобы определить, какие usIndex/tileset соответствуют контейнерам, пропсам,
// машинам и природным объектам. Полный сбор Site'ов появится на этапе 2.
class ContainerLootScanner
{
public:
	static void DumpSectorAudit(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ);

	// travnik: этап 2 — собрать кандидатов на лут в ctx.sites.
	// Шансы и выбор предметов здесь не считаются — это делает Lua.
	static void CollectSites(ContainerLootData::Context& ctx);

	// travnik: debug-проход по всем картам для каталога tileset + usIndex.
	static void ScanAllMaps();
};

#endif
