#include "Feature/ContainerLoot/ContainerLoot.h"
#include "Feature/ContainerLoot/ContainerLootScanner.h"
#include "Feature/ContainerLoot/ContainerLootLua.h"
#include "Feature/ContainerLoot/ContainerLootSpawner.h"
#include "Feature/ContainerLoot/ContainerLootSectorContext.h"
#include "Feature/ContainerLoot/ContainerLootXml.h"
#include "Feature/ContainerLoot/ContainerLootSecurity.h"

#include "Campaign Types.h"
#include "Tactical Save.h"
#include "Overhead.h"
#include "Map Information.h"
#include "Sys Globals.h"
#include "Meanwhile.h"
#include "connect.h"
#include "GameSettings.h"
#include "strategic.h"
#include "strategicmap.h"
#include "mapscreen.h"
#include "Handle Items.h"
#include "World Items.h"
#include "Soldier Control.h"
#include "structure.h"
#include "Random.h"

#include <fstream>
#include <stdio.h>
#include <set>

namespace
{
#if CONTAINER_LOOT_AUDIT_ENABLED
	// travnik: keep the debug log open so Trace() does not reopen it per line.
	std::ofstream& TraceLog()
	{
		static std::ofstream log("ContainerLoot_debug.log", std::ios::out | std::ios::app);
		return log;
	}
#endif

	// travnik: stable per-campaign seed; persisted through GENERAL_SAVE_INFO::uiSeedNumber.
	UINT32 sCampaignSeed = 0;

#if CONTAINER_LOOT_SCAN_ALL_MAPS
	BOOLEAN sScanAllMapsDone = FALSE;
#endif

#if CONTAINER_LOOT_FORCE_REGENERATE
	std::set<INT32> sForcedRegeneratedSectors;
#endif
}

// travnik: временный диагностический лог. Пишется в рабочую директорию игры
// только в debug-сборке; в release Trace ничего не делает.
void ContainerLoot::Trace(const char* message)
{
#if CONTAINER_LOOT_AUDIT_ENABLED
	std::ofstream& log = TraceLog();
	if (log)
	{
		log << message << '\n';
	}
#else
	(void)message;
#endif
}

void ContainerLoot::FlushTrace()
{
#if CONTAINER_LOOT_AUDIT_ENABLED
	std::ofstream& log = TraceLog();
	if (log)
	{
		log.flush();
	}
#endif
}

void ContainerLoot::SetCampaignSeed(UINT32 seed)
{
	sCampaignSeed = seed;
}

UINT32 ContainerLoot::GetCampaignSeed()
{
	return sCampaignSeed;
}

UINT32 ContainerLoot::CreateCampaignSeed()
{
	// travnik: NewRandom() uses an int uniform distribution; splitting the seed
	// into 16-bit chunks avoids the invalid UINT32_MAX range.
	const UINT32 chunkSize = 0x10000;
	UINT32 seed = ((UINT32)Random(chunkSize) << 16) | (UINT32)Random(chunkSize);
	if (seed == 0)
		seed = 1;

	return seed;
}

void ContainerLoot::Reset()
{
	sCampaignSeed = 0;

#if CONTAINER_LOOT_SCAN_ALL_MAPS
	sScanAllMapsDone = FALSE;
#endif

#if CONTAINER_LOOT_FORCE_REGENERATE
	sForcedRegeneratedSectors.clear();
#endif

	// travnik: reset every module that may gain process-wide state.
	ContainerLootLua::Reset();
	ContainerLootSpawner::Reset();
	ContainerLootSecurity::Reset();
	FlushTrace();
}

void ContainerLoot::ScanAllMapsOnce()
{
#if CONTAINER_LOOT_SCAN_ALL_MAPS
	if (sScanAllMapsDone)
		return;

	if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME)
		return;

	if (gfWorldLoaded)
		return;

	sScanAllMapsDone = TRUE;
	Trace("ScanAllMapsOnce: begin");
	ContainerLootScanner::ScanAllMaps();
	Trace("ScanAllMapsOnce: end");
#endif
}

void ContainerLoot::ProcessSector(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ)
{
	CHAR8 trace[256];
	sprintf_s(trace, "ProcessSector: enter %c%d z=%d enabled=%d audit=%d allowVisited=%d forceRegen=%d seed=%u",
		(CHAR8)(sSectorY + 'A' - 1), sSectorX, bSectorZ,
		(int)CONTAINER_LOOT_ENABLED, (int)CONTAINER_LOOT_AUDIT_ENABLED,
		(int)CONTAINER_LOOT_ALLOW_VISITED_SECTORS, (int)CONTAINER_LOOT_FORCE_REGENERATE,
		GetCampaignSeed());
	Trace(trace);

#if !CONTAINER_LOOT_ENABLED
	// travnik: контрольная сборка — фича полностью выключена
	(void)sSectorX;
	(void)sSectorY;
	(void)bSectorZ;
	Trace("ProcessSector: skipped — CONTAINER_LOOT_ENABLED is FALSE");
	return;
#else
	// travnik: защита от невалидных координат. GetSectorFlagStatus использует
	// макрос SECTOR() с AssertGE/AssertLE, поэтому сюда нельзя передавать 0,
	// -1 или координаты за пределами карты.
	if (sSectorX < MINIMUM_VALID_X_COORDINATE || sSectorX > MAXIMUM_VALID_X_COORDINATE ||
		sSectorY < MINIMUM_VALID_Y_COORDINATE || sSectorY > MAXIMUM_VALID_Y_COORDINATE ||
		bSectorZ < MINIMUM_VALID_Z_COORDINATE || bSectorZ > MAXIMUM_VALID_Z_COORDINATE)
	{
		Trace("ProcessSector: skipped — invalid sector coordinates");
		return;
	}

	if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME)
	{
		Trace("ProcessSector: skipped — LOADING_SAVED_GAME");
		return;
	}

	if (!gfWorldLoaded)
	{
		Trace("ProcessSector: skipped — world is not loaded");
		return;
	}

	if (gfEditMode)
	{
		Trace("ProcessSector: skipped — editor mode");
		return;
	}

	if (is_networked)
	{
		Trace("ProcessSector: skipped — multiplayer");
		return;
	}

	if (AreInMeanwhile())
	{
		Trace("ProcessSector: skipped — meanwhile");
		return;
	}

	// travnik: уже посещённые сектора не трогаем — это защита старых сейвов
	// и повторного захода. В debug можно разрешить тестовую генерацию.
#if !CONTAINER_LOOT_ALLOW_VISITED_SECTORS
	if (GetSectorFlagStatus(sSectorX, sSectorY, bSectorZ, SF_ALREADY_LOADED))
	{
		Trace("ProcessSector: skipped — SF_ALREADY_LOADED");
		return;
	}
#endif

#if CONTAINER_LOOT_FORCE_REGENERATE
	// travnik: один раз за запуск игры сбрасываем флаг для каждого сектора,
	// чтобы можно было проверить лут в уже сгенерированном секторе.
	const INT32 sectorId = (INT32)SECTOR(sSectorX, sSectorY) + ((INT32)bSectorZ * 256);
	if (sForcedRegeneratedSectors.find(sectorId) == sForcedRegeneratedSectors.end())
	{
		ContainerLootSecurity::ClearGeneratedForSector(sSectorX, sSectorY, bSectorZ);
		ReSetSectorFlag(sSectorX, sSectorY, (UINT8)bSectorZ, SF_CONTAINER_LOOT_GENERATED);
		sForcedRegeneratedSectors.insert(sectorId);
		Trace("ProcessSector: debug force reset SF_CONTAINER_LOOT_GENERATED");
	}
#endif

	if (GetSectorFlagStatus(sSectorX, sSectorY, bSectorZ, SF_CONTAINER_LOOT_GENERATED))
	{
		Trace("ProcessSector: skipped — SF_CONTAINER_LOOT_GENERATED");
		return;
	}

	ContainerLootData::Context ctx;
	ContainerLootSectorContext::Resolve(sSectorX, sSectorY, bSectorZ, ctx);

	sprintf_s(trace, "ProcessSector: context type=%s tags=%d progress=%d/%d coolness=%d difficulty=%d threat=%d wealth=%d traffic=%d enemy=%d day=%d",
		ctx.sectorType.c_str(), (INT32)ctx.sectorTags.size(), (INT32)ctx.progress, (INT32)ctx.currentProgress,
		(INT32)ctx.sectorCoolness, (INT32)ctx.difficulty, (INT32)ctx.threat, (INT32)ctx.wealth,
		(INT32)ctx.traffic, ctx.enemyControlled ? 1 : 0, (INT32)ctx.worldDay);
	Trace(trace);

	if (!ContainerLootXml::EnsureLoaded(ctx.profiles, ctx.lootTypes, ctx.securityProfiles, ctx.sectorSecurity))
	{
		Trace("ProcessSector: EnvironmentLoot.xml is not loaded — loot generation disabled");
		FlushTrace();
		return;
	}

	ContainerLootScanner::CollectSites(ctx);

	INT32 openSites = 0;
	INT32 hiddenSites = 0;
	for (std::vector<ContainerLootData::Site>::const_iterator it = ctx.sites.begin(); it != ctx.sites.end(); ++it)
	{
		if (it->canOpen) ++openSites;
		if (it->canHide) ++hiddenSites;
	}

	std::vector<ContainerLootData::PlanEntry> plan;
	const BOOLEAN luaOk = ContainerLootLua::Generate(ctx, plan);
	BOOLEAN spawnOk = FALSE;
	if (luaOk)
	{
		spawnOk = ContainerLootSpawner::Apply(ctx, plan);
	}

	sprintf_s(trace, "ProcessSector: sites=%d open=%d hidden=%d plan=%d lua=%d spawn=%d",
		(INT32)ctx.sites.size(), openSites, hiddenSites,
		(INT32)plan.size(), luaOk ? 1 : 0, spawnOk ? 1 : 0);
	Trace(trace);
	FlushTrace();
#endif
}

// travnik: при открытии не-дверного openable-контейнера принудительно
// возвращаем пул в VISIBLE. В r7609 для таких структур нет полной поддержки
// замков, поэтому штатная смена структуры не всегда вызывает
// SetItemPoolVisibilityOn. Пул ищем и на сетке клика, и на базовой сетке.
BOOLEAN ContainerLoot::OnContainerOpening(SOLDIERTYPE* pSoldier, INT32 sGridNo, TAG_STRUCTURE* pStructure)
{
#if !CONTAINER_LOOT_ENABLED
	(void)pSoldier;
	(void)sGridNo;
	(void)pStructure;
	return TRUE;
#else
	if (pStructure == NULL)
		return TRUE;

	if (!(pStructure->fFlags & STRUCTURE_OPENABLE))
		return TRUE;

	// travnik: only the player opens loot containers; switches are not loot sites.
	if (pSoldier == NULL || pSoldier->bTeam != gbPlayerNum)
		return TRUE;

	if (pStructure->fFlags & STRUCTURE_SWITCH)
		return TRUE;

	// travnik: структура уже открыта — это закрытие, видимость не трогаем.
	if (pStructure->fFlags & STRUCTURE_OPEN)
		return TRUE;

	INT32 grids[3] = { sGridNo, pStructure->sGridNo, -1 };
	STRUCTURE* pBase = FindBaseStructure(pStructure);
	if (pBase != NULL)
		grids[2] = pBase->sGridNo;

	// travnik: the structure's cube offset is authoritative for its item pool level.
	const UINT8 ubLevel = (pStructure->sCubeOffset == STRUCTURE_ON_ROOF) ? 1 : 0;
	UINT8 levels[1] = { ubLevel };

	std::set<INT32> visited;
	for (int gridIndex = 0; gridIndex < 3; ++gridIndex)
	{
		if (grids[gridIndex] < 0 || grids[gridIndex] >= WORLD_MAX)
			continue;

		// travnik: only the structure's own level is valid; levels has exactly one entry.
		for (int levelIndex = 0; levelIndex < 1; ++levelIndex)
		{
			const INT32 key = grids[gridIndex] * 4 + levels[levelIndex];
			if (!visited.insert(key).second)
				continue;

			ITEM_POOL* pItemPool = NULL;
			if (!GetItemPool(grids[gridIndex], &pItemPool, levels[levelIndex]) || pItemPool == NULL)
				continue;

			// travnik: reveal only items hidden inside the opened object; never
			// HIDDEN_ITEM/BURIED_ITEM that may exist in a map-authored pool.
			const BOOLEAN changed = SetItemPoolVisibilityOn(pItemPool, HIDDEN_IN_OBJECT, TRUE);

			INT32 revealed = 0;
			for (ITEM_POOL* p = pItemPool; p != NULL; p = p->pNext)
			{
				if (p->iItemIndex < 0 || p->iItemIndex >= (INT32)gWorldItems.size())
					continue;

				WORLDITEM& worldItem = gWorldItems[p->iItemIndex];
				if (worldItem.bVisible != VISIBLE)
					continue;

				// travnik: снимаем флаг, из-за которого открытый лут не рисуется.
				worldItem.usFlags &= ~WORLD_ITEM_DONTRENDER;
				p->usFlags &= ~WORLD_ITEM_DONTRENDER;
				++revealed;
			}

			CHAR8 trace[192];
			sprintf_s(trace, "OnContainerOpening: grid=%d level=%d changed=%d revealed=%d",
				grids[gridIndex], (INT32)levels[levelIndex], changed ? 1 : 0, revealed);
			Trace(trace);
		}
	}

	return TRUE;
#endif
}
