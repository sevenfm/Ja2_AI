#ifndef __CONTAINER_LOOT_RANDOM_ITEMS_H
#define __CONTAINER_LOOT_RANDOM_ITEMS_H

#include "Types.h"

// travnik: выбор предмета из класса RandomItem.xml с фильтром по coolness.
class ContainerLootRandomItems
{
public:
	static BOOLEAN Pick(UINT16 usClass, UINT8 ubMaxCoolness, UINT16* pusItem);
};

#endif
