#include "Feature/ContainerLoot/ContainerLootSpawner.h"
#include "Feature/ContainerLoot/ContainerLoot.h"
#include "Feature/ContainerLoot/ContainerLootRandomItems.h"
#include "Feature/ContainerLoot/ContainerLootSecurity.h"

#include "Items.h"
#include "Item Types.h"
#include "Handle Items.h"
#include "World Items.h"
#include "Tactical Save.h"
#include "Campaign Types.h"
#include "GameSettings.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdio.h>

// travnik: этап 4. Спавнер только применяет уже готовый план Lua.
// Все шансы, выбор предметов и деньги приходят из containerloot.lua.

namespace
{
	BOOLEAN IsValidLootItem(UINT16 usItem)
	{
		if (usItem == 0 || usItem >= gMAXITEMS_READ)
			return FALSE;

		if (usItem == SWITCH || usItem == ACTION_ITEM || usItem == OWNERSHIP)
			return FALSE;

		if (Item[usItem].usItemClass == 0)
			return FALSE;

		if (!ItemIsLegal(usItem, TRUE))
			return FALSE;

		if (Item[usItem].defaultundroppable)
			return FALSE;

		if (Item[usItem].usItemClass & IC_KEY)
			return FALSE;

		return TRUE;
	}


	BOOLEAN IsPlacementAllowed(const ContainerLootData::Site& site, UINT8 placement)
	{
		switch (placement)
		{
		case ContainerLootData::PLACEMENT_OPEN:
			return site.canOpen && !site.isOpen;
		case ContainerLootData::PLACEMENT_HIDDEN:
			return site.canHide;
		case ContainerLootData::PLACEMENT_SURFACE:
			return site.canSurface;
		case ContainerLootData::PLACEMENT_CORPSE:
			return site.canCorpse;
		default:
			return FALSE;
		}
	}

	void AddObjectToWorld(const ContainerLootData::Site& site, OBJECTTYPE& object, UINT8 placement)
	{
		INT8 visible = VISIBLE;
		INT8 renderHeight = 0;

		if (placement == ContainerLootData::PLACEMENT_HIDDEN)
		{
#if CONTAINER_LOOT_DEBUG_VISIBLE_HIDDEN
			// travnik: debug — показать предмет на месте, чтобы проверить спавн.
			visible = VISIBLE;
			renderHeight = (INT8)site.height;
#else
			visible = HIDDEN_ITEM;
#endif
		}
		else if (placement == ContainerLootData::PLACEMENT_SURFACE)
		{
			visible = VISIBLE;
			renderHeight = (INT8)site.height;
		}

		// travnik: AddItemToPool сам скроет предмет в закрытом контейнере
		// через HIDDEN_IN_OBJECT и WORLD_ITEM_DONTRENDER. Для бочек/касс это
		// не подходит: они должны искаться «красной рукой», поэтому после
		// добавления принудительно возвращаем HIDDEN_ITEM.
		INT32 itemIndex = -1;
		AddItemToPoolAndGetIndex(site.lootGridNo, &object, visible, 0, 0, renderHeight, -1, &itemIndex);

		if (itemIndex >= 0 && itemIndex < (INT32)gWorldItems.size())
		{
			WORLDITEM& worldItem = gWorldItems[itemIndex];

#if CONTAINER_LOOT_DEBUG_VISIBLE_HIDDEN
			if (placement == ContainerLootData::PLACEMENT_HIDDEN)
			{
				worldItem.bVisible = VISIBLE;
				worldItem.usFlags &= ~WORLD_ITEM_DONTRENDER;
			}
#else
			if (placement == ContainerLootData::PLACEMENT_HIDDEN)
			{
				worldItem.bVisible = HIDDEN_ITEM;
				worldItem.usFlags &= ~WORLD_ITEM_DONTRENDER;
			}
#endif

			// travnik: AddItemToPool мог выставить WORLD_ITEM_DONTRENDER
			// и на самом пуле, а не только на world item.
			ITEM_POOL* pItemPool = NULL;
			if (GetItemPool(worldItem.sGridNo, &pItemPool, worldItem.ubLevel))
			{
				for (ITEM_POOL* p = pItemPool; p; p = p->pNext)
				{
					if (p->iItemIndex == itemIndex)
					{
						p->bVisible = worldItem.bVisible;
						p->usFlags &= ~WORLD_ITEM_DONTRENDER;
						break;
					}
				}
			}

			CHAR8 trace[192];
			sprintf(trace, "ContainerLootSpawner: add item=%d grid=%d visible=%d flags=0x%X",
				(INT32)worldItem.object.usItem, worldItem.sGridNo,
				(INT32)worldItem.bVisible, (UINT32)worldItem.usFlags);
			ContainerLoot::Trace(trace);
		}
	}

	BOOLEAN SpawnItem(const ContainerLootData::Site& site, const ContainerLootData::PlanItem& planItem,
		UINT8 placement)
	{
		UINT16 usItem = planItem.item;

		if (planItem.randomItemClass > 0)
		{
			if (!ContainerLootRandomItems::Pick(planItem.randomItemClass, planItem.maxCoolness, &usItem))
			{
				CHAR8 message[192];
				sprintf(message, "ContainerLootSpawner: random item class %d produced no item",
					(INT32)planItem.randomItemClass);
				ContainerLoot::Trace(message);
				return FALSE;
			}
		}

		if (!IsValidLootItem(usItem))
		{
			CHAR8 message[256];
			sprintf(message, "ContainerLootSpawner: rejected item %d", usItem);
			ContainerLoot::Trace(message);
			return FALSE;
		}

		if (planItem.count < 1 || planItem.count > ContainerLootData::MAX_COUNT)
			return FALSE;

		if (planItem.condition < 1 || planItem.condition > 100)
			return FALSE;


		OBJECTTYPE object;
		if (!CreateItems(usItem, planItem.condition, planItem.count, &object))
			return FALSE;


		AddObjectToWorld(site, object, placement);
		return TRUE;
	}
}

void ContainerLootSpawner::Reset()
{
}

BOOLEAN ContainerLootSpawner::Apply(const ContainerLootData::Context& ctx,
	const std::vector<ContainerLootData::PlanEntry>& plan)
{
	std::set<INT32> usedSites;
	std::map<std::string, INT32> spawnedByProfile;
	INT32 totalSpawns = 0;
	const INT32 maxSpawns = (gGameExternalOptions.iContainerLootMaxSpawns > 0) ?
		gGameExternalOptions.iContainerLootMaxSpawns : ContainerLootData::DEFAULT_MAX_SPAWNS_PER_SECTOR;

	// travnik: ????????? ? randomKey ???????? ?? XML/C++-???????.
	// ????????? ?????? ??????? ????????, randomKey ? ???????????? ??????? ??????.
	std::vector<ContainerLootData::PlanEntry> ordered(plan);
	std::stable_sort(ordered.begin(), ordered.end(), [&ctx](const ContainerLootData::PlanEntry& left, const ContainerLootData::PlanEntry& right)
	{
		const ContainerLootData::Site* leftSite =
			(left.site >= 1 && left.site <= (INT32)ctx.sites.size()) ? &ctx.sites[left.site - 1] : NULL;
		const ContainerLootData::Site* rightSite =
			(right.site >= 1 && right.site <= (INT32)ctx.sites.size()) ? &ctx.sites[right.site - 1] : NULL;

		if (!leftSite)
			return false;
		if (!rightSite)
			return true;
		return ContainerLootData::SiteSortLess(*leftSite, *rightSite) != FALSE;
	});

	for (std::vector<ContainerLootData::PlanEntry>::const_iterator it = ordered.begin(); it != ordered.end(); ++it)
	{
		const ContainerLootData::PlanEntry& entry = *it;

		if (entry.site < 1 || entry.site > (INT32)ctx.sites.size())
		{
			CHAR8 message[128];
			sprintf(message, "ContainerLootSpawner: invalid site %d", entry.site);
			ContainerLoot::Trace(message);
			continue;
		}

		if (totalSpawns >= maxSpawns)
			break;

		const ContainerLootData::Site& site = ctx.sites[entry.site - 1];
		if (site.site != entry.site || usedSites.find(entry.site) != usedSites.end())
			continue;

		if (!IsPlacementAllowed(site, entry.placement))
		{
			CHAR8 message[128];
			sprintf(message, "ContainerLootSpawner: placement rejected for site %d", entry.site);
			ContainerLoot::Trace(message);
			continue;
		}

		const std::string profileId = entry.profile.empty() ? site.profile : entry.profile;
		const INT32 profileMaxSites = (entry.maxSites >= 0) ? entry.maxSites : site.maxSites;
		const INT32 profileSpawned = spawnedByProfile[profileId];
		if (profileMaxSites > 0 && profileSpawned >= profileMaxSites)
		{
			CHAR8 message[192];
			sprintf(message, "ContainerLootSpawner: profile cap reached site=%d profile=%s maxSites=%d",
				entry.site, profileId.c_str(), profileMaxSites);
			ContainerLoot::Trace(message);
			continue;
		}

		if (entry.placement == ContainerLootData::PLACEMENT_CORPSE)
		{
			// travnik: трупы подключим отдельным этапом
			ContainerLoot::Trace("ContainerLootSpawner: corpse placement not implemented yet");
			continue;
		}

		if (entry.money > ContainerLootData::MAX_MONEY)
			continue;

		if (entry.items.size() > (size_t)ContainerLootData::MAX_ITEMS_PER_SITE)
			continue;

		{
			CHAR8 trace[256];
			sprintf(trace, "ContainerLootSpawner: plan site=%d subtype=%s profile=%s placement=%d grid=%d lootGrid=%d money=%u items=%d",
				entry.site, site.subtype.c_str(), profileId.c_str(), (INT32)entry.placement,
				site.gridNo, site.lootGridNo, entry.money, (INT32)entry.items.size());
			ContainerLoot::Trace(trace);
		}

		BOOLEAN spawnedAny = FALSE;

		if (entry.money > 0)
		{
			OBJECTTYPE money;
			if (CreateMoney(entry.money, &money))
			{
				AddObjectToWorld(site, money, entry.placement);
				spawnedAny = TRUE;
				++totalSpawns;
			}
		}

		for (std::vector<ContainerLootData::PlanItem>::const_iterator item = entry.items.begin();
			item != entry.items.end(); ++item)
		{
			if (totalSpawns >= maxSpawns)
				break;

			if (SpawnItem(site, *item, entry.placement))
			{
				spawnedAny = TRUE;
				++totalSpawns;
			}
		}

		// travnik: ???? ????????? ???????????? ?????? ???? ???????? ???? ?? ???? ??????.
		if (spawnedAny)
		{
			usedSites.insert(entry.site);
			++spawnedByProfile[profileId];
			ContainerLootSecurity::Apply(ctx, site, entry);
		}

		if (totalSpawns >= maxSpawns)
			break;
	}

	// travnik: даже пустой/частично отклонённый план считается обработанным,
	// чтобы не пытаться генерировать лут при каждом входе в сектор.
	SetSectorFlag(ctx.sectorX, ctx.sectorY, (UINT8)ctx.sectorZ, SF_CONTAINER_LOOT_GENERATED);

	CHAR8 message[200];
	sprintf(message, "ContainerLootSpawner: spawned=%d sites=%d maxSpawns=%d",
		totalSpawns, (INT32)usedSites.size(), maxSpawns);
	ContainerLoot::Trace(message);
	return TRUE;
}
