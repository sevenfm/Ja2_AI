#ifndef __CONTAINER_LOOT_SECTOR_CONTEXT_H
#define __CONTAINER_LOOT_SECTOR_CONTEXT_H

#include "Feature/ContainerLoot/ContainerLootTypes.h"

// travnik: собирает прогресс, coolness, тип и теги сектора для Lua/XML.
class ContainerLootSectorContext
{
public:
	static void Resolve(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ,
		ContainerLootData::Context& ctx);
};

#endif
