#include "Feature/ContainerLoot/ContainerLootSectorContext.h"

#include "Campaign Types.h"
#include "Campaign.h"
#include "Game Clock.h"
#include "GameSettings.h"
#include "mapscreen.h"
#include "strategic.h"
#include "strategicmap.h"
#include "World Tileset Enums.h"
#include "environment.h"
#include "worlddef.h"

#include <algorithm>
#include <stdio.h>
#include <string>
#include <vector>

// travnik: gCoolnessBySector определена в Tactical/Soldier Create.cpp
extern UINT32 gCoolnessBySector[256];

namespace
{
	// travnik: CoolnessBySector.xml uses a 0..20 scale in r7609; clamp modded values.
	const FLOAT MAX_SECTOR_COOLNESS = 20.0f;

	void AddTag(std::vector<std::string>& tags, const char* tag)
	{
		if (!tag || !*tag)
			return;

		for (size_t i = 0; i < tags.size(); ++i)
		{
			if (tags[i] == tag)
				return;
		}

		tags.push_back(tag);
	}

	bool HasFlag(UINT32 flags, UINT32 flag)
	{
		return (flags & flag) != 0;
	}

	INT32 TownWealth(UINT8 townId)
	{
		switch (townId)
		{
		case BALIME:
		case MEDUNA:
			return 6;
		case CAMBRIA:
		case GRUMM:
		case ALMA:
			return 5;
		case DRASSEN:
		case ESTONI:
		case ORTA:
			return 4;
		case SAN_MONA:
		case TIXA:
		case CHITZENA:
			return 3;
		case OMERTA:
			return 2;
		default:
			return 2;
		}
	}

	bool IsTileset(INT32 tileset, INT32 first, INT32 last = -1)
	{
		if (tileset < 0)
			return false;

		if (last < 0)
			return tileset == first;

		return tileset >= first && tileset <= last;
	}

	bool IsSamTileset(INT32 tileset)
	{
		return IsTileset(tileset, TLS_SAM_SITES) ||
			IsTileset(tileset, TLS_TROPICAL_SAM) ||
			IsTileset(tileset, TLS_DESERT_SAM) ||
			IsTileset(tileset, TLS_QUEENS_SAM);
	}

	bool IsMilitaryTileset(INT32 tileset)
	{
		return IsTileset(tileset, TLS_MILITARY_BASE) ||
			IsTileset(tileset, TLS_MILITARY_JAIL) ||
			IsTileset(tileset, TLS_MILITARY_WAREHOUSE) ||
			IsTileset(tileset, TLS_MILITARY_TOWN) ||
			IsTileset(tileset, TLS_ORTA_WEAPONS);
	}

	bool IsPrisonTileset(INT32 tileset)
	{
		return IsTileset(tileset, TLS_PRISON) ||
			IsTileset(tileset, TLS_PRISON_DUNGEON) ||
			IsTileset(tileset, TLS_QUEENS_PRISON);
	}

	bool IsAirportTileset(INT32 tileset)
	{
		return IsTileset(tileset, TLS_AIRSTRIP) ||
			IsTileset(tileset, TLS_DEAD_AIRSTRIP) ||
			IsTileset(tileset, TLS_QUEENS_AIRPORT);
	}

	bool IsUndergroundTileset(INT32 tileset)
	{
		return IsTileset(tileset, TLS_BASEMENT) ||
			IsTileset(tileset, TLS_SEWERS) ||
			IsTileset(tileset, TLS_UNDERGROUND_COMPLEX) ||
			IsTileset(tileset, TLS_UPPER_COMPLEX) ||
			IsTileset(tileset, TLS_LOWEST_LEVEL_COMPLEX);
	}

	void AddTilesetTags(INT32 tileset, std::vector<std::string>& tags)
	{
		if (IsSamTileset(tileset))
			AddTag(tags, "sam");

		if (IsMilitaryTileset(tileset))
			AddTag(tags, "military");

		if (IsPrisonTileset(tileset))
			AddTag(tags, "prison");

		if (IsAirportTileset(tileset))
			AddTag(tags, "airport");

		if (IsTileset(tileset, TLS_FARM_1) || IsTileset(tileset, TLS_FARM_2))
		{
			AddTag(tags, "farm");
			AddTag(tags, "agricultural");
			AddTag(tags, "rural");
		}

		if (IsTileset(tileset, TLS_HOSPITAL))
			AddTag(tags, "hospital");

		if (IsTileset(tileset, TLS_PALACE) || IsTileset(tileset, TLS_MEDUNA_INNER_TOWN))
			AddTag(tags, "palace");

		if (IsTileset(tileset, TLS_LAWLESS_1) ||
			IsTileset(tileset, TLS_LAWLESS_2) ||
			IsTileset(tileset, TLS_LAWLESS_3))
			AddTag(tags, "lawless");

		if (IsTileset(tileset, TLS_MINING_TOWN))
		{
			AddTag(tags, "mine");
			AddTag(tags, "town");
			AddTag(tags, "industrial");
		}

		if (IsTileset(tileset, TLS_MILITARY_WAREHOUSE) ||
			IsTileset(tileset, TLS_ORTA_WEAPONS))
			AddTag(tags, "industrial");

		if (IsUndergroundTileset(tileset))
			AddTag(tags, "underground");

		// travnik: подземные комплексы обычно военные
		if (IsTileset(tileset, TLS_UNDERGROUND_COMPLEX) ||
			IsTileset(tileset, TLS_UPPER_COMPLEX) ||
			IsTileset(tileset, TLS_LOWEST_LEVEL_COMPLEX))
			AddTag(tags, "military");

		if (IsTileset(tileset, TLS_OMERTA) ||
			IsTileset(tileset, TLS_ACTIVE_DRASSEN) ||
			IsTileset(tileset, TLS_CAMBRIA_STRIP) ||
			IsTileset(tileset, TLS_CAMBRIA_HOMES) ||
			IsTileset(tileset, TLS_GRUMM_1) ||
			IsTileset(tileset, TLS_GRUMM_2) ||
			IsTileset(tileset, TLS_BALIME) ||
			IsTileset(tileset, TLS_BALIME_MUSEUM) ||
			IsTileset(tileset, TLS_ORTA) ||
			IsTileset(tileset, TLS_ESTONI) ||
			IsTileset(tileset, TLS_QUEENS_TROPICAL) ||
			IsTileset(tileset, TLS_FALL_TOWN))
			AddTag(tags, "town");
	}

	std::string ClassifyTileset(INT32 tileset, bool underground)
	{
		if (underground || IsUndergroundTileset(tileset))
			return "underground";
		if (IsSamTileset(tileset))
			return "sam";
		if (IsTileset(tileset, TLS_PALACE) || IsTileset(tileset, TLS_MEDUNA_INNER_TOWN))
			return "palace";
		if (IsMilitaryTileset(tileset))
			return "military";
		if (IsPrisonTileset(tileset))
			return "prison";
		if (IsTileset(tileset, TLS_HOSPITAL))
			return "hospital";
		if (IsAirportTileset(tileset))
			return "airport";
		if (IsTileset(tileset, TLS_FARM_1) || IsTileset(tileset, TLS_FARM_2))
			return "farm";
		if (IsTileset(tileset, TLS_LAWLESS_1) ||
			IsTileset(tileset, TLS_LAWLESS_2) ||
			IsTileset(tileset, TLS_LAWLESS_3))
			return "lawless";
		if (IsTileset(tileset, TLS_MINING_TOWN))
			return "mine";

		return "wilderness";
	}
}

void ContainerLootSectorContext::Resolve(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ,
	ContainerLootData::Context& ctx)
{
	ctx.sectorX = sSectorX;
	ctx.sectorY = sSectorY;
	ctx.sectorZ = bSectorZ;
	ctx.isUnderground = (bSectorZ > 0) ? TRUE : FALSE;
	ctx.difficulty = gGameOptions.ubDifficultyLevel;
	ctx.foodSystem = gGameOptions.fFoodSystem;
	ctx.progress = HighestPlayerProgressPercentage();
	ctx.currentProgress = CurrentPlayerProgressPercentage();
	ctx.sectorCoolness = gCoolnessBySector[SECTOR(sSectorX, sSectorY)];
	ctx.worldDay = GetWorldDay();
	ctx.weather = guiEnvWeather;

	CHAR8 sectorKey[16];
	sprintf_s(sectorKey, "%c%d", (CHAR8)(sSectorY + 'A' - 1), sSectorX);
	ctx.sectorKey = sectorKey;

	const INT32 sectorIndex = SECTOR(sSectorX, sSectorY);
	const INT32 strategicIndex = CALCULATE_STRATEGIC_INDEX(sSectorX, sSectorY);
	const StrategicMapElement& strategic = StrategicMap[strategicIndex];
	const SECTORINFO* pSector = (sectorIndex >= 0 && sectorIndex < (INT32)SectorInfo.size()) ?
		&SectorInfo[sectorIndex] : NULL;

	const UINT32 sectorFlags = pSector ? pSector->uiFlags : 0;
	const INT32 tileset = giCurrentTilesetID;

	ctx.townId = (strategic.bNameId > BLANK_SECTOR) ? (UINT8)strategic.bNameId : BLANK_SECTOR;
	ctx.enemyControlled = strategic.fEnemyControlled ? TRUE : FALSE;

	ctx.traffic = 0;
	if (pSector)
	{
		ctx.traffic = (UINT8)std::min<INT32>(10, (INT32)pSector->ubTravelRating / 10);
	}

	ctx.threat = 0;
	if (pSector)
	{
		const INT32 enemyScore =
			(INT32)pSector->ubNumElites * 3 +
			(INT32)pSector->ubNumTroops * 2 +
			(INT32)pSector->ubNumAdmins;
		ctx.threat = (UINT8)std::min<INT32>(10, (enemyScore + 4) / 5);
	}

	INT32 wealth = TownWealth(ctx.townId);
	if (IsAirportTileset(tileset) || IsTileset(tileset, TLS_AIRSTRIP))
		++wealth;
	if (IsMilitaryTileset(tileset))
		++wealth;
	if (HasFlag(sectorFlags, SF_MINING_SITE))
		wealth += 2;
	wealth += ctx.traffic / 2;
	ctx.wealth = (UINT8)std::max<INT32>(0, std::min<INT32>(10, wealth));

	std::vector<std::string> tags;
	if (ctx.isUnderground)
		AddTag(tags, "underground");
	if (ctx.townId != BLANK_SECTOR)
		AddTag(tags, "town");
	if (HasFlag(sectorFlags, SF_SAM_SITE))
		AddTag(tags, "sam");
	if (HasFlag(sectorFlags, SF_MINING_SITE))
		AddTag(tags, "mine");
	if (IsMilitaryTileset(tileset))
		AddTag(tags, "military");
	if (IsPrisonTileset(tileset))
		AddTag(tags, "prison");
	if (IsTileset(tileset, TLS_HOSPITAL))
		AddTag(tags, "hospital");
	if (IsAirportTileset(tileset))
		AddTag(tags, "airport");
	if (ctx.townId == SAN_MONA)
		AddTag(tags, "lawless");
	if (ctx.wealth >= 7)
		AddTag(tags, "wealthy");
	if (ctx.traffic >= 5)
		AddTag(tags, "traffic");

	AddTilesetTags(tileset, tags);

	std::string type;
	if (ctx.isUnderground)
		type = "underground";
	else if (HasFlag(sectorFlags, SF_SAM_SITE) || IsSamTileset(tileset))
		type = "sam";
	else if (HasFlag(sectorFlags, SF_MINING_SITE))
		type = "mine";
	else if (ctx.townId != BLANK_SECTOR)
		type = "town";
	else
		type = ClassifyTileset(tileset, false);

	// travnik: военный/тюремный/госпитальный/аэропортный tileset важнее town
	if (!ctx.isUnderground && type == "town")
	{
		const std::string tilesetType = ClassifyTileset(tileset, false);
		if (tilesetType != "wilderness")
			type = tilesetType;
	}

	AddTag(tags, type.c_str());
	if (type == "wilderness")
		AddTag(tags, "rural");
	ctx.sectorType = type;
	ctx.sectorTags = tags;

	ctx.progressWeight = gGameExternalOptions.fContainerLootProgressWeight;
	ctx.sectorCoolnessWeight = gGameExternalOptions.fContainerLootSectorCoolnessWeight;
	ctx.militaryTypeBonus = gGameExternalOptions.fContainerLootMilitaryTypeBonus;
	ctx.difficultyBonus = gGameExternalOptions.fContainerLootDifficultyBonus;
	ctx.progressTierCapBonus = gGameExternalOptions.fContainerLootProgressTierCapBonus;
	ctx.maxItemCoolnessBonus = gGameExternalOptions.fContainerLootMaxItemCoolnessBonus;
	ctx.moneyProgressScale = gGameExternalOptions.fContainerLootMoneyProgressScale;
	ctx.emptyChanceScale = gGameExternalOptions.fContainerLootEmptyChanceScale;
	ctx.coolnessNorm = std::min<FLOAT>(1.0f, (FLOAT)ctx.sectorCoolness / MAX_SECTOR_COOLNESS);
}
