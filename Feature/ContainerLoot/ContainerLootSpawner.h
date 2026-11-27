#ifndef __CONTAINER_LOOT_SPAWNER_H
#define __CONTAINER_LOOT_SPAWNER_H

#include "Feature/ContainerLoot/ContainerLootTypes.h"

// travnik: спавнер фичи. Этап 4: CreateItems/CreateMoney, placement,
// HIDDEN_ITEM/VISIBLE, AddCorpseFromObject. Пока оставлен каркас.
class ContainerLootSpawner
{
public:
	static void Reset();

	// travnik: этап 4 — валидировать план Lua и разместить предметы.
	// Возвращает TRUE, если план обработан и сектор можно помечать
	// как SF_CONTAINER_LOOT_GENERATED.
	static BOOLEAN Apply(const ContainerLootData::Context& ctx,
		const std::vector<ContainerLootData::PlanEntry>& plan);
};

#endif
