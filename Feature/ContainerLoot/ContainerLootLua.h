#ifndef __CONTAINER_LOOT_LUA_H
#define __CONTAINER_LOOT_LUA_H

#include "Feature/ContainerLoot/ContainerLootTypes.h"

// travnik: Lua-мост фичи. Этап 3: сборка ctx, вызов containerloot.lua,
// разбор и валидация плана. Пока оставлен каркас.
class ContainerLootLua
{
public:
	static void Reset();

	// travnik: загрузить scripts\\containerloot.lua, передать ctx и разобрать план.
	// Возвращает FALSE при ошибке Lua или неверном формате результата.
	static BOOLEAN Generate(const ContainerLootData::Context& ctx,
		std::vector<ContainerLootData::PlanEntry>& plan);
};

#endif
