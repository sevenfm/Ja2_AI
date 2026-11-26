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

#include <fstream>
#include <stdio.h>
#include <set>

// travnik: временный диагностический лог. Пишется в рабочую директорию игры
// только в debug-сборке; в release Trace ничего не делает.
void ContainerLoot::Trace(const char* message)
{
#if CONTAINER_LOOT_AUDIT_ENABLED
	std::ofstream log("ContainerLoot_debug.log", std::ios::out | std::ios::app);
	if (log)
	{
		log << message << "\n";
	}
#else
	(void)message;
#endif
}

void ContainerLoot::ScanAllMapsOnce()
{
#if CONTAINER_LOOT_SCAN_ALL_MAPS
	static BOOLEAN sDone = FALSE;

	if (sDone)
		return;

	if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME)
		return;

	if (gfWorldLoaded)
		return;

	sDone = TRUE;
	Trace("ScanAllMapsOnce: begin");
	ContainerLootScanner::ScanAllMaps();
	Trace("ScanAllMapsOnce: end");
#endif
}

void ContainerLoot::ProcessSector(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ)
{
	CHAR8 trace[256];
	sprintf(trace, "ProcessSector: enter %c%d z=%d enabled=%d audit=%d allowVisited=%d forceRegen=%d",
		(CHAR8)(sSectorY + 'A' - 1), sSectorX, bSectorZ,
		(int)CONTAINER_LOOT_ENABLED, (int)CONTAINER_LOOT_AUDIT_ENABLED,
		(int)CONTAINER_LOOT_ALLOW_VISITED_SECTORS, (int)CONTAINER_LOOT_FORCE_REGENERATE);
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
	static std::set<INT32> sForcedRegeneratedSectors;
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

	sprintf(trace, "ProcessSector: context type=%s tags=%d progress=%d/%d coolness=%d difficulty=%d threat=%d wealth=%d traffic=%d enemy=%d day=%d",
		ctx.sectorType.c_str(), (INT32)ctx.sectorTags.size(), (INT32)ctx.progress, (INT32)ctx.currentProgress,
		(INT32)ctx.sectorCoolness, (INT32)ctx.difficulty, (INT32)ctx.threat, (INT32)ctx.wealth,
		(INT32)ctx.traffic, ctx.enemyControlled ? 1 : 0, (INT32)ctx.worldDay);
	Trace(trace);

	if (!ContainerLootXml::EnsureLoaded(ctx.profiles, ctx.lootTypes, ctx.securityProfiles, ctx.sectorSecurity))
	{
		Trace("ProcessSector: EnvironmentLoot.xml is not loaded — loot generation disabled");
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

	sprintf(trace, "ProcessSector: sites=%d open=%d hidden=%d plan=%d lua=%d spawn=%d",
		(INT32)ctx.sites.size(), openSites, hiddenSites,
		(INT32)plan.size(), luaOk ? 1 : 0, spawnOk ? 1 : 0);
	Trace(trace);
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

	// travnik: структура уже открыта — это закрытие, видимость не трогаем.
	if (pStructure->fFlags & STRUCTURE_OPEN)
		return TRUE;

	INT32 grids[3] = { sGridNo, pStructure->sGridNo, -1 };
	STRUCTURE* pBase = FindBaseStructure(pStructure);
	if (pBase != NULL)
		grids[2] = pBase->sGridNo;

	UINT8 levels[2] = { 0, 0 };
	if (pSoldier != NULL)
		levels[0] = pSoldier->pathing.bLevel;
	levels[1] = 0;

	std::set<INT32> visited;
	for (int gridIndex = 0; gridIndex < 3; ++gridIndex)
	{
		if (grids[gridIndex] < 0 || grids[gridIndex] >= WORLD_MAX)
			continue;

		for (int levelIndex = 0; levelIndex < 2; ++levelIndex)
		{
			const INT32 key = grids[gridIndex] * 4 + levels[levelIndex];
			if (!visited.insert(key).second)
				continue;

			ITEM_POOL* pItemPool = NULL;
			if (!GetItemPool(grids[gridIndex], &pItemPool, levels[levelIndex]) || pItemPool == NULL)
				continue;

			const BOOLEAN changed = SetItemPoolVisibilityOn(pItemPool, ANY_VISIBILITY_VALUE, FALSE);

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
			sprintf(trace, "OnContainerOpening: grid=%d level=%d changed=%d revealed=%d",
				grids[gridIndex], (INT32)levels[levelIndex], changed ? 1 : 0, revealed);
			Trace(trace);
		}
	}

	return TRUE;
#endif
}
