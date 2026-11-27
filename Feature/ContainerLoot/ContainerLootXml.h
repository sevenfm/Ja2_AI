#ifndef __CONTAINER_LOOT_XML_H
#define __CONTAINER_LOOT_XML_H

#include "Feature/ContainerLoot/ContainerLootTypes.h"

#include <string>
#include <vector>

// travnik: загрузчик TableData\Loot\EnvironmentLoot.xml.
// C++ читает статические профили и отдаёт их в ctx.profiles для Lua.
namespace ContainerLootXml
{
	// travnik: загружает XML один раз и кэширует результат.
	BOOLEAN EnsureLoaded(std::vector<ContainerLootData::LootProfile>& profiles,
		std::vector<ContainerLootData::LootTypeDef>& lootTypes,
		std::vector<ContainerLootData::ContainerSecurityProfile>& securityProfiles,
		std::vector<ContainerLootData::SectorSecurityModifier>& sectorSecurity,
		BOOLEAN forceReload = FALSE);

	// travnik: принудительная перезагрузка (debug / hot reload).
	BOOLEAN Reload(std::vector<ContainerLootData::LootProfile>& profiles,
		std::vector<ContainerLootData::LootTypeDef>& lootTypes,
		std::vector<ContainerLootData::ContainerSecurityProfile>& securityProfiles,
		std::vector<ContainerLootData::SectorSecurityModifier>& sectorSecurity);

	// travnik: ищет профиль по tileset+usIndex, затем по image+frame.
	const ContainerLootData::LootProfile* FindProfile(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT16 frame,
		UINT8 kind);

	// travnik: поиск с учётом типа/тегов сектора и fallbackProfile.
	const ContainerLootData::LootProfile* FindProfileForSector(
		const std::vector<ContainerLootData::LootProfile>& profiles,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT16 frame,
		UINT8 kind,
		const std::string& sectorType,
		const std::vector<std::string>& sectorTags);
}

#endif
