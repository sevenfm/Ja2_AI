#include "Feature/ContainerLoot/ContainerLootScanner.h"
#include "Feature/ContainerLoot/ContainerLoot.h"
#include "Feature/ContainerLoot/ContainerLootXml.h"

#include "worlddef.h"
#include "tiledef.h"
#include "worldman.h"
#include "structure.h"
#include "Handle Items.h"
#include "GameSettings.h"
#include "Quests.h"
#include "strategicmap.h"
#include "Map Information.h"
#include "Sys Globals.h"
#include "vobject.h"
#include "imgfmt.h"
#include "debug.h"

#include <ctype.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <windows.h>

// travnik: аудит пишет CSV только в debug-сборке. В release весь блок
// вырезается препроцессором, чтобы не создавать файлы в игре.
#if CONTAINER_LOOT_AUDIT_ENABLED

// travnik: одна строка каталога структур/объектов для последующего маппинга
// tileset + usIndex -> kind/subtype в Lua.
static void DumpStructureLine(
	std::ofstream& csv,
	const char* sectorKey,
	INT32 gridNo,
	const char* layer,
	UINT16 usIndex,
	UINT32 flags,
	INT8 height,
	BOOLEAN hasPool)
{
	// travnik: имя STI-файла помогает отличать шкафы/кассы/бочки/растения
	// без угадывания по диапазонам usIndex.
	const char* image = "";
	if (usIndex < gTileDatabaseSize)
	{
		HVOBJECT hTileSurface = gTileDatabase[usIndex].hTileSurface;
		if (hTileSurface)
		{
			image = hTileSurface->ImageFile;
		}
	}

	csv << sectorKey << ","
		<< gridNo << ","
		<< layer << ","
		<< giCurrentTilesetID << ","
		<< usIndex << ","
		<< flags << ","
		<< (INT32)height << ","
		<< ((flags & STRUCTURE_OPENABLE) ? 1 : 0) << ","
		<< ((flags & STRUCTURE_VEHICLE) ? 1 : 0) << ","
		<< ((flags & STRUCTURE_TREE) ? 1 : 0) << ","
		<< ((flags & STRUCTURE_GENERIC) ? 1 : 0) << ","
		<< ((flags & STRUCTURE_MULTI) ? 1 : 0) << ","
		<< ((flags & STRUCTURE_HASITEMONTOP) ? 1 : 0) << ","
		<< (hasPool ? 1 : 0) << ","
		<< image
		<< "\n";
}

void ContainerLootScanner::DumpSectorAudit(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ)
{
	CHAR8 sectorKey[16];
	sprintf_s(sectorKey, "%c%d", (CHAR8)(sSectorY + 'A' - 1), sSectorX);
	(void)bSectorZ;

	INT32 scanned = 0;
	INT32 openable = 0;
	INT32 vehicles = 0;
	INT32 trees = 0;
	INT32 generic = 0;
	INT32 pools = 0;

	std::ofstream csv("ContainerLoot_audit.csv", std::ios::out | std::ios::app);
	if (csv)
	{
		csv << "sector,gridNo,layer,tileset,usIndex,flags,height,"
			<< "openable,vehicle,tree,generic,multi,itemOnTop,hasPool,image\n";
	}

	for (INT32 gridNo = 0; gridNo < WORLD_MAX; ++gridNo)
	{
		for (STRUCTURE* s = gpWorldLevelData[gridNo].pStructureHead; s; s = s->pNext)
		{
			if (!(s->fFlags & STRUCTURE_BASE_TILE))
				continue;

			if (s->sGridNo < 0 || s->sGridNo >= WORLD_MAX)
				continue;

			LEVELNODE* pNode = FindLevelNodeBasedOnStructure(s->sGridNo, s);
			if (!pNode)
				continue;

			const UINT16 usIndex = pNode->usIndex;
			const UINT32 flags = s->fFlags;
			const INT8 height = (INT8)StructureHeight(s);

			ITEM_POOL* pPool = NULL;
			const BOOLEAN hasPool = GetItemPoolFromGround(s->sGridNo, &pPool);

			++scanned;
			if (flags & STRUCTURE_OPENABLE) ++openable;
			if (flags & STRUCTURE_VEHICLE) ++vehicles;
			if (flags & STRUCTURE_TREE) ++trees;
			if (flags & STRUCTURE_GENERIC) ++generic;
			if (hasPool) ++pools;

			DumpStructureLine(csv, sectorKey, s->sGridNo, "struct", usIndex, flags, height, hasPool);
		}
	}

	if (csv)
		csv.close();

	CHAR8 message[192];
	sprintf_s(message, "DumpSectorAudit: %s scanned=%d openable=%d vehicles=%d trees=%d generic=%d pools=%d",
		sectorKey, scanned, openable, vehicles, trees, generic, pools);
	ContainerLoot::Trace(message);
}

#else

void ContainerLootScanner::DumpSectorAudit(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ)
{
	(void)sSectorX;
	(void)sSectorY;
	(void)bSectorZ;
}

#endif

namespace
{
	std::string GetTileImageName(UINT16 usIndex)
	{
		if (usIndex >= gTileDatabaseSize)
			return std::string();

		HVOBJECT hTileSurface = gTileDatabase[usIndex].hTileSurface;
		if (!hTileSurface)
			return std::string();

		std::string image = hTileSurface->ImageFile;
		const size_t slash = image.find_last_of("\\/");
		if (slash != std::string::npos)
			image.erase(0, slash + 1);

		const size_t dot = image.find_last_of('.');
		if (dot != std::string::npos)
			image.erase(dot);

		for (size_t i = 0; i < image.size(); ++i)
			image[i] = (char)tolower((unsigned char)image[i]);

		return image;
	}

	BOOLEAN IsExcludedStructure(UINT32 flags)
	{
		return (flags & (STRUCTURE_ANYDOOR | STRUCTURE_ANYFENCE | STRUCTURE_WALLNWINDOW |
			STRUCTURE_SLIDINGDOOR | STRUCTURE_SWITCH | STRUCTURE_CAVEWALL |
			STRUCTURE_WALL | STRUCTURE_ROOF | STRUCTURE_PERSON | STRUCTURE_CORPSE)) != 0;
	}

	// travnik: gridNo is local to the current sector, so special item-pool
	// coordinates must be checked together with their sector.
	BOOLEAN IsSpecialContainerGrid(INT16 sectorX, INT16 sectorY, INT8 sectorZ, INT32 gridNo)
	{
		if (sectorX == OMERTA_LEAVE_EQUIP_SECTOR_X &&
			sectorY == OMERTA_LEAVE_EQUIP_SECTOR_Y &&
			sectorZ == OMERTA_LEAVE_EQUIP_SECTOR_Z &&
			gridNo == OMERTA_LEAVE_EQUIP_GRIDNO)
		{
			return TRUE;
		}

		if (sectorX == BOBBYR_SHIPPING_DEST_SECTOR_X &&
			sectorY == BOBBYR_SHIPPING_DEST_SECTOR_Y &&
			sectorZ == BOBBYR_SHIPPING_DEST_SECTOR_Z &&
			gridNo == BOBBYR_SHIPPING_DEST_GRIDNO)
		{
			return TRUE;
		}

		if (sectorX == (INT16)gModSettings.ubAirportX &&
			sectorY == (INT16)gModSettings.ubAirportY &&
			sectorZ == 0 &&
			gridNo == gModSettings.iAirportDropOff)
		{
			return TRUE;
		}

		return FALSE;
	}

	// travnik: hidden stashes are discovered from a tile the soldier can occupy.
	BOOLEAN IsLootTilePassable(INT32 gridNo)
	{
		if (gridNo < 0 || gridNo >= WORLD_MAX)
			return FALSE;

		STRUCTURE* pStructure = FindStructure(gridNo, STRUCTURE_BLOCKSMOVES);
		while (pStructure)
		{
			if (!(pStructure->fFlags & STRUCTURE_PASSABLE) &&
				pStructure->sCubeOffset == STRUCTURE_ON_GROUND)
			{
				return FALSE;
			}

			pStructure = FindNextStructure(pStructure, STRUCTURE_BLOCKSMOVES);
		}

		return TRUE;
	}

	BOOLEAN HasAccessibleTile(INT32 gridNo)
	{
		if (IsLootTilePassable(gridNo))
			return TRUE;

		const INT32 col = gridNo % WORLD_COLS;
		const INT32 row = gridNo / WORLD_COLS;

		for (INT32 dy = -1; dy <= 1; ++dy)
		{
			const INT32 y = row + dy;
			if (y < 0 || y >= WORLD_ROWS)
				continue;

			for (INT32 dx = -1; dx <= 1; ++dx)
			{
				if (dx == 0 && dy == 0)
					continue;

				const INT32 x = col + dx;
				if (x < 0 || x >= WORLD_COLS)
					continue;

				if (IsLootTilePassable(y * WORLD_COLS + x))
					return TRUE;
			}
		}

		return FALSE;
	}

	BOOLEAN HasAccessibleTileForStructure(STRUCTURE* pStructure)
	{
		if (!pStructure)
			return FALSE;

		if (HasAccessibleTile(pStructure->sGridNo))
			return TRUE;

		if (!pStructure->pDBStructureRef ||
			!pStructure->pDBStructureRef->pDBStructure ||
			!pStructure->pDBStructureRef->ppTile)
		{
			return FALSE;
		}

		const UINT16 tileCount = pStructure->pDBStructureRef->pDBStructure->ubNumberOfTiles;
		for (UINT16 i = 0; i < tileCount; ++i)
		{
			if (!pStructure->pDBStructureRef->ppTile[i])
				continue;

			const INT32 gridNo = AddPosRelToBase(pStructure->sGridNo,
				pStructure->pDBStructureRef->ppTile[i]);
			if (gridNo == pStructure->sGridNo || gridNo < 0 || gridNo >= WORLD_MAX)
				continue;

			if (HasAccessibleTile(gridNo))
				return TRUE;
		}

		return FALSE;
	}

	// travnik: multi-tile containers may store map-authored items on any occupied tile.
	BOOLEAN HasItemPoolForStructure(STRUCTURE* pStructure)
	{
		if (!pStructure)
			return FALSE;

		ITEM_POOL* pPool = NULL;
		if (GetItemPoolFromGround(pStructure->sGridNo, &pPool))
			return TRUE;

		if (!pStructure->pDBStructureRef ||
			!pStructure->pDBStructureRef->pDBStructure ||
			!pStructure->pDBStructureRef->ppTile)
			return FALSE;

		const UINT16 tileCount = pStructure->pDBStructureRef->pDBStructure->ubNumberOfTiles;
		for (UINT16 i = 0; i < tileCount; ++i)
		{
			if (!pStructure->pDBStructureRef->ppTile[i])
				continue;

			const INT32 gridNo = AddPosRelToBase(pStructure->sGridNo,
				pStructure->pDBStructureRef->ppTile[i]);
			if (gridNo == pStructure->sGridNo || gridNo < 0 || gridNo >= WORLD_MAX)
				continue;

			if (GetItemPoolFromGround(gridNo, &pPool))
				return TRUE;
		}

		return FALSE;
	}

	BOOLEAN IsHiddenContainerImage(const std::string& image)
	{
		return image == "drum_01" || image == "drum_02";
	}

	BOOLEAN IsHiddenContainerTile(UINT8 tileset, UINT16 usIndex)
	{
		static const UINT32 hiddenTiles[] =
		{
			((UINT32)16 << 16) | 2279,
			((UINT32)16 << 16) | 2280,
			((UINT32)18 << 16) | 2279,
			((UINT32)18 << 16) | 2280,
			((UINT32)19 << 16) | 2359,
			((UINT32)21 << 16) | 2279,
			((UINT32)21 << 16) | 2280,
			((UINT32)23 << 16) | 2301,
			((UINT32)23 << 16) | 2302,
			((UINT32)31 << 16) | 1814,
			((UINT32)31 << 16) | 1815,
			((UINT32)31 << 16) | 1830,
			((UINT32)31 << 16) | 1831,
			((UINT32)31 << 16) | 1833,
			((UINT32)31 << 16) | 2311,
			((UINT32)31 << 16) | 2315,
			((UINT32)31 << 16) | 2316,
			((UINT32)31 << 16) | 2349,
			((UINT32)31 << 16) | 2350,
			((UINT32)43 << 16) | 1830,
			((UINT32)43 << 16) | 1831,
			((UINT32)43 << 16) | 1833,
			((UINT32)49 << 16) | 1830,
			((UINT32)49 << 16) | 1831,
			((UINT32)49 << 16) | 1833,
		};

		const UINT32 key = ((UINT32)tileset << 16) | usIndex;
		for (size_t i = 0; i < sizeof(hiddenTiles) / sizeof(hiddenTiles[0]); ++i)
		{
			if (hiddenTiles[i] == key)
				return TRUE;
		}

		return FALSE;
	}

	UINT32 MakeSiteRandomKey(INT16 sectorX, INT16 sectorY, INT8 sectorZ,
		INT32 gridNo, const std::string& profile, UINT32 campaignSeed)
	{
		UINT32 hash = 2166136261u;
		// travnik: seed is stable within one campaign but differs between new games.
		hash ^= campaignSeed; hash *= 16777619u;
		hash ^= (UINT32)(UINT16)sectorX; hash *= 16777619u;
		hash ^= (UINT32)(UINT16)sectorY; hash *= 16777619u;
		hash ^= (UINT32)(UINT8)sectorZ; hash *= 16777619u;
		hash ^= (UINT32)gridNo; hash *= 16777619u;
		for (size_t i = 0; i < profile.size(); ++i)
		{
			hash ^= (UINT8)profile[i];
			hash *= 16777619u;
		}
		hash ^= 0x43A5F00Du;
		hash *= 16777619u;
		return hash & 0x7FFFFFFFu;
	}

	void TryAddSite(ContainerLootData::Context& ctx, const ContainerLootData::Site& candidate)
	{
		// travnik: two structures on the same tile must not create two loot sites.
		for (std::vector<ContainerLootData::Site>::const_iterator it = ctx.sites.begin();
			it != ctx.sites.end(); ++it)
		{
			if (it->gridNo == candidate.gridNo)
				return;
		}

		ContainerLootData::Site site = candidate;
		site.site = (INT32)ctx.sites.size() + 1;
		ctx.sites.push_back(site);
	}
}

void ContainerLootScanner::CollectSites(ContainerLootData::Context& ctx)
{
	ctx.sites.clear();

	INT32 containers = 0;
	INT32 vehicles = 0;
	INT32 surfaces = 0;
	INT32 nature = 0;
	INT32 props = 0;
	INT32 hiddenContainers = 0;
	INT32 hiddenNoAccessSkipped = 0;
	INT32 nonEmptySkipped = 0;
	INT32 openContainersSkipped = 0;

	for (INT32 gridNo = 0; gridNo < WORLD_MAX; ++gridNo)
	{
		for (STRUCTURE* s = gpWorldLevelData[gridNo].pStructureHead; s; s = s->pNext)
		{
			if (!(s->fFlags & STRUCTURE_BASE_TILE))
				continue;

			// travnik: roof structures have no ground-level item pool.
			if (s->sCubeOffset != STRUCTURE_ON_GROUND)
				continue;

			if (s->sGridNo < 0 || s->sGridNo >= WORLD_MAX)
				continue;

			if (IsSpecialContainerGrid(ctx.sectorX, ctx.sectorY, ctx.sectorZ, s->sGridNo))
				continue;

			LEVELNODE* pNode = FindLevelNodeBasedOnStructure(s->sGridNo, s);
			if (!pNode)
				continue;

			const UINT32 flags = s->fFlags;
			const UINT16 usIndex = pNode->usIndex;
			const UINT8 tileset = (UINT8)giCurrentTilesetID;
			const std::string image = GetTileImageName(usIndex);
			const BOOLEAN isHiddenContainer = IsHiddenContainerImage(image) ||
				IsHiddenContainerTile(tileset, usIndex);

			// travnik: пустые контейнеры — безусловное правило, debug-обход отключён.
			if (HasItemPoolForStructure(s))
			{
				++nonEmptySkipped;
				continue;
			}

			ContainerLootData::Site site;
			site.gridNo = s->sGridNo;
			site.lootGridNo = s->sGridNo;
			site.usIndex = usIndex;
			site.tileset = tileset;
			site.layer = ContainerLootData::LAYER_STRUCT;
			site.height = (UINT8)StructureHeight(s);
			site.subtype = image;
			if (site.subtype.empty())
				site.subtype = "unknown";
			site.frame = (usIndex < gTileDatabaseSize) ? gTileDatabase[usIndex].usRegionIndex : 0;

			UINT8 profileKind = ContainerLootData::KIND_PROP;
			if (flags & STRUCTURE_VEHICLE)
				profileKind = ContainerLootData::KIND_VEHICLE;
			else if (flags & STRUCTURE_TREE)
				profileKind = ContainerLootData::KIND_NATURE;
			else if (flags & STRUCTURE_OPENABLE)
				profileKind = ContainerLootData::KIND_CONTAINER;

			const ContainerLootData::LootProfile* profile =
				ContainerLootXml::FindProfileForSector(ctx.profiles, tileset, usIndex, site.subtype, site.frame,
					profileKind, ctx.sectorType, ctx.sectorTags);
			if (profile == NULL)
				continue;

			site.profile = profile->id;
			site.priority = profile->priority;
			site.maxSites = profile->maxSites;
			site.randomKey = MakeSiteRandomKey(ctx.sectorX, ctx.sectorY, ctx.sectorZ,
				site.gridNo, site.profile, ContainerLoot::GetCampaignSeed());

			if (isHiddenContainer)
			{
				if (flags & STRUCTURE_OPEN)
				{
					++openContainersSkipped;
					continue;
				}

				if (!HasAccessibleTileForStructure(s))
				{
					++hiddenNoAccessSkipped;
					continue;
				}

				const BOOLEAN canOpen = (flags & STRUCTURE_OPENABLE) ? TRUE : FALSE;
				site.kind = canOpen ? ContainerLootData::KIND_CONTAINER : ContainerLootData::KIND_PROP;
				site.canOpen = canOpen;
				// travnik: hidden loot is only for non-openable stashes, never for openable containers.
				site.canHide = canOpen ? FALSE : TRUE;
				site.canSurface = FALSE;
				site.isOpen = FALSE;
				++hiddenContainers;
				if (hiddenContainers <= 20)
				{
					CHAR8 hiddenTrace[160];
					sprintf_s(hiddenTrace, "CollectSites: hidden container tile=%d:%d subtype=%s profile=%s",
						(INT32)site.tileset, (INT32)usIndex, site.subtype.c_str(), site.profile.c_str());
					ContainerLoot::Trace(hiddenTrace);
				}
				TryAddSite(ctx, site);
				continue;
			}

			if (flags & STRUCTURE_VEHICLE)
			{
				site.kind = ContainerLootData::KIND_VEHICLE;
				site.canHide = TRUE;
				++vehicles;
				TryAddSite(ctx, site);
				continue;
			}

			if (flags & STRUCTURE_TREE)
			{
				site.kind = ContainerLootData::KIND_NATURE;
				site.canHide = TRUE;
				++nature;
				TryAddSite(ctx, site);
				continue;
			}

			if ((flags & STRUCTURE_OPENABLE) && !IsExcludedStructure(flags))
			{
				// travnik: открытые контейнеры не участвуют в генерации.
				if (flags & STRUCTURE_OPEN)
				{
					++openContainersSkipped;
					continue;
				}

				site.kind = ContainerLootData::KIND_CONTAINER;
				site.isOpen = FALSE;
				site.canOpen = TRUE;
				site.canSurface = (flags & STRUCTURE_HASITEMONTOP) ? TRUE : FALSE;
				++containers;
				TryAddSite(ctx, site);
				continue;
			}

			if (flags & STRUCTURE_GENERIC)
			{
				if (IsExcludedStructure(flags))
					continue;

				site.kind = ContainerLootData::KIND_PROP;
				site.canHide = TRUE;
				if (flags & STRUCTURE_HASITEMONTOP)
				{
					site.canSurface = TRUE;
					++surfaces;
					TryAddSite(ctx, site);
				}
				else
				{
					++props;
					TryAddSite(ctx, site);
				}
			}
		}
	}

	CHAR8 message[320];
	sprintf_s(message, "CollectSites: total=%d containers=%d vehicles=%d surfaces=%d nature=%d props=%d hidden=%d hiddenNoAccessSkipped=%d nonEmptySkipped=%d openSkipped=%d",
		(INT32)ctx.sites.size(), containers, vehicles, surfaces, nature, props, hiddenContainers,
		hiddenNoAccessSkipped, nonEmptySkipped, openContainersSkipped);
	ContainerLoot::Trace(message);
}
namespace
{
	struct CatalogEntry
	{
		CatalogEntry()
			: count(0), kind(ContainerLootData::KIND_PROP),
			  canOpen(FALSE), canHide(FALSE), canSurface(FALSE),
			  openable(FALSE), vehicle(FALSE), tree(FALSE), generic(FALSE),
			  multi(FALSE), itemOnTop(FALSE), hasPool(FALSE),
			  exampleGridNo(0)
		{
		}

		std::string image;
		std::string layers;
		std::string exampleSector;
		INT32 count;
		UINT8 kind;
		BOOLEAN canOpen;
		BOOLEAN canHide;
		BOOLEAN canSurface;
		BOOLEAN openable;
		BOOLEAN vehicle;
		BOOLEAN tree;
		BOOLEAN generic;
		BOOLEAN multi;
		BOOLEAN itemOnTop;
		BOOLEAN hasPool;
		INT32 exampleGridNo;
	};

	const char* CatalogKindName(UINT8 kind)
	{
		switch (kind)
		{
		case ContainerLootData::KIND_CONTAINER: return "container";
		case ContainerLootData::KIND_PROP: return "prop";
		case ContainerLootData::KIND_VEHICLE: return "vehicle";
		case ContainerLootData::KIND_NATURE: return "nature";
		case ContainerLootData::KIND_GROUND: return "ground";
		case ContainerLootData::KIND_BODY: return "body";
		default: return "unknown";
		}
	}

	BOOLEAN IsNatureObjectImage(const std::string& image)
	{
		return image == "f_tree" || image == "tree2_t" || image == "montbush" ||
			image == "nogobush" || image == "l_bush" || image == "bush" ||
			image == "sweeds" || image == "miniweed" || image == "f_weed" ||
			image == "l_weed2" || image == "weeds_1";
	}

	BOOLEAN IsStashObjectImage(const std::string& image)
	{
		return image == "drum_01" || image == "drum_02" || image == "workshp2" ||
			image == "dump" || image == "mdrock" || image == "damgdeb" ||
			image == "debrocks" || image == "rubble1" || image == "rubble2" ||
			image == "burnt2" || image == "burnt3" || image == "deb_02" ||
			image == "h-debri1" || image == "hos_deb1" || image == "hos_deb2" ||
			image == "hos_deb3" || image == "litter" || image == "lawless" ||
			image == "lawless2" || image == "lawless3" || image == "lawless4";
	}

	void AppendLayer(CatalogEntry& entry, const char* layer)
	{
		if (entry.layers.empty())
		{
			entry.layers = layer;
			return;
		}

		if (entry.layers.find(layer) == std::string::npos)
		{
			entry.layers += "|";
			entry.layers += layer;
		}
	}

	void WriteAllMapsRow(
		std::ofstream& csv,
		const char* sectorLabel,
		INT32 gridNo,
		const char* layer,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT32 flags,
		INT8 height,
		BOOLEAN hasPool,
		UINT8 kind,
		BOOLEAN canOpen,
		BOOLEAN canHide,
		BOOLEAN canSurface)
	{
		UINT16 tileType = 0;
		UINT16 frame = 0;
		if (usIndex < gTileDatabaseSize)
		{
			tileType = gTileDatabase[usIndex].fType;
			frame = gTileDatabase[usIndex].usRegionIndex;
		}

		csv << sectorLabel << ","
			<< gridNo << ","
			<< layer << ","
			<< (INT32)tileset << ","
			<< usIndex << ","
			<< image << ","
			<< (INT32)tileType << ","
			<< (INT32)frame << ","
			<< flags << ","
			<< (INT32)height << ","
			<< ((flags & STRUCTURE_OPENABLE) ? 1 : 0) << ","
			<< ((flags & STRUCTURE_VEHICLE) ? 1 : 0) << ","
			<< ((flags & STRUCTURE_TREE) ? 1 : 0) << ","
			<< ((flags & STRUCTURE_GENERIC) ? 1 : 0) << ","
			<< ((flags & STRUCTURE_MULTI) ? 1 : 0) << ","
			<< ((flags & STRUCTURE_HASITEMONTOP) ? 1 : 0) << ","
			<< (hasPool ? 1 : 0) << ","
			<< CatalogKindName(kind) << ","
			<< (canOpen ? 1 : 0) << ","
			<< (canHide ? 1 : 0) << ","
			<< (canSurface ? 1 : 0)
			<< "\n";
	}

	void AddCatalogRow(
		std::map<UINT32, CatalogEntry>& catalog,
		UINT8 tileset,
		UINT16 usIndex,
		const std::string& image,
		UINT32 flags,
		BOOLEAN hasPool,
		UINT8 kind,
		BOOLEAN canOpen,
		BOOLEAN canHide,
		BOOLEAN canSurface,
		const char* layer,
		const char* sectorLabel,
		INT32 gridNo)
	{
		const UINT32 key = ((UINT32)tileset << 16) | usIndex;
		CatalogEntry& entry = catalog[key];

		if (entry.count == 0)
		{
			entry.image = image;
			entry.kind = kind;
			entry.exampleSector = sectorLabel;
			entry.exampleGridNo = gridNo;
		}

		++entry.count;
		entry.canOpen = entry.canOpen || canOpen;
		entry.canHide = entry.canHide || canHide;
		entry.canSurface = entry.canSurface || canSurface;
		entry.openable = entry.openable || ((flags & STRUCTURE_OPENABLE) != 0);
		entry.vehicle = entry.vehicle || ((flags & STRUCTURE_VEHICLE) != 0);
		entry.tree = entry.tree || ((flags & STRUCTURE_TREE) != 0);
		entry.generic = entry.generic || ((flags & STRUCTURE_GENERIC) != 0);
		entry.multi = entry.multi || ((flags & STRUCTURE_MULTI) != 0);
		entry.itemOnTop = entry.itemOnTop || ((flags & STRUCTURE_HASITEMONTOP) != 0);
		entry.hasPool = entry.hasPool || hasPool;
		AppendLayer(entry, layer);
	}

	bool IsContactSheetExcludedImage(const std::string& image)
	{
		// travnik: трава/сорняки — это фон, а не контейнер/пропс.
		return image == "weeds_1" || image == "sweeds" || image == "miniweed" ||
			image == "f_weed" || image == "l_weed2" || image == "weeds_2" ||
			image == "grass" || image == "grass1" || image == "grass2" ||
			image == "swampgr7" || image == "twigs";
	}

	bool IsContactSheetCandidate(const CatalogEntry& entry)
	{
		if (entry.count == 0)
			return FALSE;

		if (IsContactSheetExcludedImage(entry.image))
			return FALSE;

		// travnik: деревья и кусты нужны только как nature-сайты, отдельный
		// subtype им не назначается — в контактные листы их не берём.
		if (entry.openable || entry.vehicle)
			return TRUE;

		return entry.canHide && IsStashObjectImage(entry.image);
	}

	bool IsContactSheetOpen(const CatalogEntry& entry)
	{
		return entry.openable != FALSE;
	}

#pragma pack(push, 1)
	struct ContactBmpFileHeader
	{
		UINT16 type;
		UINT32 size;
		UINT16 reserved1;
		UINT16 reserved2;
		UINT32 offBits;
	};

	struct ContactBmpInfoHeader
	{
		UINT32 size;
		INT32 width;
		INT32 height;
		UINT16 planes;
		UINT16 bitCount;
		UINT32 compression;
		UINT32 sizeImage;
		INT32 xPelsPerMeter;
		INT32 yPelsPerMeter;
		UINT32 clrUsed;
		UINT32 clrImportant;
	};
#pragma pack(pop)

	// travnik: собственный декодер ETRLE. Штатный GetETRLEPixelValue неверно
	// переходит между строками (не пропускает нулевой терминатор), из-за чего
	// в BMP повторялась одна и та же вертикальная полоса.
	bool DecodeEtrleFrame(
		HVOBJECT hVObject,
		UINT16 subIndex,
		std::vector<UINT8>& pixels,
		INT32& width,
		INT32& height)
	{
		if (!hVObject || !hVObject->pETRLEObject || !hVObject->pPixData)
			return false;

		if (hVObject->ubBitDepth != 8)
			return false;

		if (subIndex >= hVObject->usNumberOfObjects)
			return false;

		const ETRLEObject& frame = hVObject->pETRLEObject[subIndex];
		if (!frame.usWidth || !frame.usHeight)
			return false;

		width = frame.usWidth;
		height = frame.usHeight;

		const UINT32 total = (UINT32)width * (UINT32)height;
		pixels.assign(total, 0);

		const UINT8* pCurrent = (const UINT8*)hVObject->pPixData + frame.uiDataOffset;
		const UINT8* pEnd = (const UINT8*)hVObject->pPixData + hVObject->uiSizePixData;

		for (INT32 y = 0; y < height; ++y)
		{
			UINT32 x = 0;

			while (pCurrent < pEnd)
			{
				const UINT8 run = *pCurrent++;
				const UINT8 runLength = run & COMPRESS_RUN_LIMIT;

				// travnik: нулевой байт — конец строки.
				if (runLength == 0)
					break;

				if (run & COMPRESS_TRANSPARENT)
				{
					x += runLength;
					continue;
				}

				for (UINT8 i = 0; i < runLength && pCurrent < pEnd; ++i)
				{
					if (x < (UINT32)width)
						pixels[(UINT32)y * (UINT32)width + x] = *pCurrent;
					++x;
					++pCurrent;
				}
			}
		}

		return true;
	}

	void WriteContactBmp(const char* path, HVOBJECT hVObject, UINT16 subIndex)
	{
		std::vector<UINT8> pixels;
		INT32 width = 0;
		INT32 height = 0;

		if (!DecodeEtrleFrame(hVObject, subIndex, pixels, width, height))
			return;

		const INT32 rowSize = ((width * 3) + 3) & ~3;
		const UINT32 imageSize = (UINT32)rowSize * height;
		const UINT32 fileSize = sizeof(ContactBmpFileHeader) + sizeof(ContactBmpInfoHeader) + imageSize;

		ContactBmpFileHeader fileHeader;
		fileHeader.type = 0x4D42; // BM
		fileHeader.size = fileSize;
		fileHeader.reserved1 = 0;
		fileHeader.reserved2 = 0;
		fileHeader.offBits = sizeof(ContactBmpFileHeader) + sizeof(ContactBmpInfoHeader);

		ContactBmpInfoHeader infoHeader;
		infoHeader.size = sizeof(ContactBmpInfoHeader);
		infoHeader.width = width;
		infoHeader.height = height;
		infoHeader.planes = 1;
		infoHeader.bitCount = 24;
		infoHeader.compression = 0;
		infoHeader.sizeImage = imageSize;
		infoHeader.xPelsPerMeter = 0;
		infoHeader.yPelsPerMeter = 0;
		infoHeader.clrUsed = 0;
		infoHeader.clrImportant = 0;

		std::ofstream out(path, std::ios::out | std::ios::binary);
		if (!out)
			return;

		out.write((const char*)&fileHeader, sizeof(fileHeader));
		out.write((const char*)&infoHeader, sizeof(infoHeader));

		const SGPPaletteEntry* palette = hVObject->pPaletteEntry;
		std::vector<UINT8> row(rowSize, 0);

		for (INT32 y = height - 1; y >= 0; --y)
		{
			for (INT32 x = 0; x < width; ++x)
			{
				const UINT8 pixel = pixels[(UINT32)y * (UINT32)width + (UINT32)x];

				UINT8 r = 0;
				UINT8 g = 0;
				UINT8 b = 0;

				if (pixel == 0)
				{
					// travnik: прозрачный фон делаем белым, чтобы контактный лист
					// было проще читать.
					r = 255;
					g = 255;
					b = 255;
				}
				else if (palette)
				{
					r = palette[pixel].peRed;
					g = palette[pixel].peGreen;
					b = palette[pixel].peBlue;
				}
				else
				{
					r = pixel;
					g = pixel;
					b = pixel;
				}

				row[x * 3 + 0] = b;
				row[x * 3 + 1] = g;
				row[x * 3 + 2] = r;
			}

			out.write((const char*)&row[0], rowSize);
		}
	}

	void GenerateContactSheetsForTileset(
		const std::map<UINT32, CatalogEntry>& catalog,
		std::set<UINT32>& generated,
		UINT8 tileset)
	{
		CreateDirectoryA("ContainerLoot_contact_sheets", NULL);
		CreateDirectoryA("ContainerLoot_contact_sheets\\open", NULL);
		CreateDirectoryA("ContainerLoot_contact_sheets\\hidden", NULL);

		for (std::map<UINT32, CatalogEntry>::const_iterator it = catalog.begin(); it != catalog.end(); ++it)
		{
			const UINT32 key = it->first;
			if ((UINT8)(key >> 16) != tileset)
				continue;

			if (generated.find(key) != generated.end())
				continue;

			const CatalogEntry& entry = it->second;
			if (!IsContactSheetCandidate(entry))
				continue;

			const UINT16 usIndex = (UINT16)(key & 0xFFFF);
			if (usIndex >= gTileDatabaseSize)
				continue;

			HVOBJECT hVObject = gTileDatabase[usIndex].hTileSurface;
			if (!hVObject)
				continue;

			const UINT16 subIndex = gTileDatabase[usIndex].usRegionIndex;
			if (subIndex >= hVObject->usNumberOfObjects)
				continue;

			const char* folder = IsContactSheetOpen(entry) ? "open" : "hidden";
			CHAR8 path[256];
			sprintf_s(path, "ContainerLoot_contact_sheets\\%s\\%d_%d.bmp",
				folder, (INT32)tileset, (INT32)usIndex);

			WriteContactBmp(path, hVObject, subIndex);
			generated.insert(key);
		}
	}

	void WriteContactSheetHtml(
		const std::map<UINT32, CatalogEntry>& catalog,
		const std::set<UINT32>& generated)
	{
		std::ofstream html("ContainerLoot_contact_sheets/index.html", std::ios::out);
		if (!html)
			return;

		html << "<!doctype html><html><head><meta charset=\"utf-8\">"
			<< "<title>ContainerLoot contact sheets</title>"
			<< "<style>"
			<< "body{font-family:Segoe UI,Arial,sans-serif;background:#20242a;color:#eee;margin:20px}"
			<< "h1{font-size:22px}h2{font-size:18px;margin-top:32px;border-bottom:1px solid #555}"
			<< ".grid{display:flex;flex-wrap:wrap;gap:12px}"
			<< "figure{margin:0;padding:8px;background:#2d333b;border:1px solid #444;border-radius:6px;width:180px}"
			<< "img{image-rendering:pixelated;max-width:164px;max-height:164px;background:#fff;display:block;margin:0 auto 6px}"
			<< "figcaption{font-size:12px;line-height:1.35;color:#ccc}"
			<< "</style></head><body><h1>ContainerLoot contact sheets</h1>"
			<< "<p>Открой картинки и запиши subtype/profile в ContainerLoot_catalog_template.csv.</p>";

		std::map<std::string, std::vector<UINT32> > groups;
		for (std::set<UINT32>::const_iterator it = generated.begin(); it != generated.end(); ++it)
		{
			std::map<UINT32, CatalogEntry>::const_iterator entryIt = catalog.find(*it);
			if (entryIt == catalog.end())
				continue;

			const CatalogEntry& entry = entryIt->second;
			if (!IsContactSheetCandidate(entry))
				continue;

			const char* folder = IsContactSheetOpen(entry) ? "open" : "hidden";
			const std::string group = std::string(folder) + "/" + entry.image;
			groups[group].push_back(*it);
		}

		for (std::map<std::string, std::vector<UINT32> >::const_iterator groupIt = groups.begin();
			groupIt != groups.end(); ++groupIt)
		{
			html << "<h2>" << groupIt->first << "</h2><div class=\"grid\">";

			for (std::vector<UINT32>::const_iterator it = groupIt->second.begin();
				it != groupIt->second.end(); ++it)
			{
				const UINT32 key = *it;
				const CatalogEntry& entry = catalog.find(key)->second;
				const char* folder = IsContactSheetOpen(entry) ? "open" : "hidden";
				const UINT8 tileset = (UINT8)(key >> 16);
				const UINT16 usIndex = (UINT16)(key & 0xFFFF);

				CHAR8 imagePath[256];
				sprintf_s(imagePath, "%s/%d_%d.bmp", folder, (INT32)tileset, (INT32)usIndex);

				html << "<figure><img src=\"" << imagePath << "\">"
					<< "<figcaption><b>" << (INT32)tileset << ":" << (INT32)usIndex << "</b><br>"
					<< entry.image << "<br>"
					<< entry.exampleSector << " / " << entry.exampleGridNo << "<br>"
					<< "count=" << entry.count
					<< "</figcaption></figure>";
			}

			html << "</div>";
		}

		html << "</body></html>";
	}

	void ScanMapFile(
		const char* filename,
		const char* sectorLabel,
		INT16 sectorX,
		INT16 sectorY,
		INT8 sectorZ,
		std::ofstream& allMaps,
		std::map<UINT32, CatalogEntry>& catalog)
	{
		if (gfWorldLoaded)
			TrashWorld();

		gWorldSectorX = sectorX;
		gWorldSectorY = sectorY;
		gbWorldSectorZ = sectorZ;

		if (!LoadWorld((STR8)filename))
		{
			CHAR8 message[256];
			sprintf_s(message, "ScanAllMaps: cannot load %s", filename);
			ContainerLoot::Trace(message);
			return;
		}

		for (INT32 gridNo = 0; gridNo < WORLD_MAX; ++gridNo)
		{
			for (STRUCTURE* s = gpWorldLevelData[gridNo].pStructureHead; s; s = s->pNext)
			{
				if (!(s->fFlags & STRUCTURE_BASE_TILE))
					continue;

				// travnik: roof structures have no ground-level item pool.
				if (s->sCubeOffset != STRUCTURE_ON_GROUND)
					continue;

				if (s->sGridNo < 0 || s->sGridNo >= WORLD_MAX)
					continue;

				if (IsExcludedStructure(s->fFlags))
					continue;

				if (!(s->fFlags & (STRUCTURE_OPENABLE | STRUCTURE_VEHICLE |
					STRUCTURE_TREE | STRUCTURE_GENERIC | STRUCTURE_HASITEMONTOP)))
					continue;

				LEVELNODE* pNode = FindLevelNodeBasedOnStructure(s->sGridNo, s);
				if (!pNode)
					continue;

				const UINT16 usIndex = pNode->usIndex;
				const std::string image = GetTileImageName(usIndex);

				ITEM_POOL* pPool = NULL;
				const BOOLEAN hasPool = GetItemPoolFromGround(s->sGridNo, &pPool);

				UINT8 kind = ContainerLootData::KIND_PROP;
				BOOLEAN canOpen = FALSE;
				BOOLEAN canHide = FALSE;
				BOOLEAN canSurface = FALSE;

				if (s->fFlags & STRUCTURE_VEHICLE)
				{
					kind = ContainerLootData::KIND_VEHICLE;
					canHide = TRUE;
				}
				else if (s->fFlags & STRUCTURE_TREE)
				{
					kind = ContainerLootData::KIND_NATURE;
					canHide = TRUE;
				}
				else if (s->fFlags & STRUCTURE_OPENABLE)
				{
					kind = ContainerLootData::KIND_CONTAINER;
					canOpen = TRUE;
					canSurface = ((s->fFlags & STRUCTURE_HASITEMONTOP) != 0);
				}
				else if (s->fFlags & STRUCTURE_GENERIC)
				{
					kind = ContainerLootData::KIND_PROP;
					canHide = TRUE;
					canSurface = ((s->fFlags & STRUCTURE_HASITEMONTOP) != 0);
				}

				// travnik: синхронно с CollectSites — бочки/касса/автомат
				// считаются скрытыми тайниками.
				if (!(s->fFlags & (STRUCTURE_VEHICLE | STRUCTURE_TREE)) &&
					(IsHiddenContainerImage(image) ||
					 IsHiddenContainerTile((UINT8)giCurrentTilesetID, usIndex)))
				{
					kind = ContainerLootData::KIND_PROP;
					canOpen = FALSE;
					canHide = TRUE;
					canSurface = FALSE;
				}

				WriteAllMapsRow(allMaps, sectorLabel, s->sGridNo, "struct",
					(UINT8)giCurrentTilesetID, usIndex, image, s->fFlags,
					StructureHeight(s), hasPool, kind, canOpen, canHide, canSurface);

				AddCatalogRow(catalog, (UINT8)giCurrentTilesetID, usIndex, image,
					s->fFlags, hasPool, kind, canOpen, canHide, canSurface,
					"struct", sectorLabel, s->sGridNo);
			}

			for (LEVELNODE* n = gpWorldLevelData[gridNo].pObjectHead; n; n = n->pNext)
			{
				if (!n->usIndex)
					continue;

				const std::string image = GetTileImageName(n->usIndex);
				UINT8 kind = ContainerLootData::KIND_PROP;
				BOOLEAN canHide = IsStashObjectImage(image);

				if (IsNatureObjectImage(image))
					kind = ContainerLootData::KIND_NATURE;

				WriteAllMapsRow(allMaps, sectorLabel, gridNo, "object",
					(UINT8)giCurrentTilesetID, n->usIndex, image, 0, 0, FALSE,
					kind, FALSE, canHide, FALSE);

				AddCatalogRow(catalog, (UINT8)giCurrentTilesetID, n->usIndex, image,
					0, FALSE, kind, FALSE, canHide, FALSE,
					"object", sectorLabel, gridNo);
			}
		}

		TrashWorld();
	}

	std::string AlternateMapName(const char* filename, const char* suffix)
	{
		std::string name = filename;
		const size_t dot = name.find_last_of('.');
		if (dot == std::string::npos)
			return name + suffix;

		return name.substr(0, dot) + suffix + name.substr(dot);
	}

	void WriteCatalogTemplate(const std::map<UINT32, CatalogEntry>& catalog)
	{
		std::ofstream csv("ContainerLoot_catalog_template.csv", std::ios::out);
		if (!csv)
			return;

		csv << "tileset,usIndex,image,count,kind,canOpen,canHide,canSurface,"
			<< "openable,vehicle,tree,generic,multi,itemOnTop,hasPool,layers,"
			<< "exampleSector,exampleGridNo,subtype,profile,notes\n";

		for (std::map<UINT32, CatalogEntry>::const_iterator it = catalog.begin(); it != catalog.end(); ++it)
		{
			const CatalogEntry& e = it->second;
			csv << (INT32)(it->first >> 16) << ","
				<< (UINT16)(it->first & 0xFFFF) << ","
				<< e.image << ","
				<< e.count << ","
				<< CatalogKindName(e.kind) << ","
				<< (e.canOpen ? 1 : 0) << ","
				<< (e.canHide ? 1 : 0) << ","
				<< (e.canSurface ? 1 : 0) << ","
				<< (e.openable ? 1 : 0) << ","
				<< (e.vehicle ? 1 : 0) << ","
				<< (e.tree ? 1 : 0) << ","
				<< (e.generic ? 1 : 0) << ","
				<< (e.multi ? 1 : 0) << ","
				<< (e.itemOnTop ? 1 : 0) << ","
				<< (e.hasPool ? 1 : 0) << ","
				<< e.layers << ","
				<< e.exampleSector << ","
				<< e.exampleGridNo << ","
				<< ","
				<< ","
				<< "\n";
		}
	}

	// travnik: отчёт покрытия XML-профилями по уникальным тайлам каталога.
	// assigned — профиль найден; generic — сработал KIND container;
	// skip — объект не является лут-кандидатом; missing — кандидат без профиля.
	void WriteCoverageReport(const std::map<UINT32, CatalogEntry>& catalog)
	{
		std::vector<ContainerLootData::LootProfile> profiles;
		std::vector<ContainerLootData::LootTypeDef> lootTypes;
		std::vector<ContainerLootData::ContainerSecurityProfile> securityProfiles;
		std::vector<ContainerLootData::SectorSecurityModifier> sectorSecurity;
		ContainerLootXml::EnsureLoaded(profiles, lootTypes, securityProfiles, sectorSecurity);

		std::ofstream csv("ContainerLoot_coverage.csv", std::ios::out);
		if (!csv)
			return;

		csv << "tileset,usIndex,image,kind,openable,vehicle,tree,canHide,canSurface,"
			<< "count,profile,status,exampleSector,exampleGridNo\n";

		INT32 assigned = 0;
		INT32 generic = 0;
		INT32 skipped = 0;
		INT32 missing = 0;

		for (std::map<UINT32, CatalogEntry>::const_iterator it = catalog.begin(); it != catalog.end(); ++it)
		{
			const CatalogEntry& e = it->second;
			const UINT8 tileset = (UINT8)(it->first >> 16);
			const UINT16 usIndex = (UINT16)(it->first & 0xFFFF);
			const ContainerLootData::LootProfile* profile =
				ContainerLootXml::FindProfile(profiles, tileset, usIndex, e.image, 0, e.kind);

			const BOOLEAN lootCandidate = (e.openable || e.vehicle || e.tree ||
				e.canHide || e.itemOnTop) ? TRUE : FALSE;

			const char* status = "assigned";
			if (profile == NULL)
			{
				if (!lootCandidate)
				{
					status = "skip";
					++skipped;
				}
				else
				{
					status = "missing";
					++missing;
				}
			}
			else if (profile->id == "generic")
			{
				status = "generic";
				++generic;
			}
			else
			{
				++assigned;
			}

			csv << (INT32)tileset << ","
				<< usIndex << ","
				<< e.image << ","
				<< CatalogKindName(e.kind) << ","
				<< (e.openable ? 1 : 0) << ","
				<< (e.vehicle ? 1 : 0) << ","
				<< (e.tree ? 1 : 0) << ","
				<< (e.canHide ? 1 : 0) << ","
				<< (e.canSurface ? 1 : 0) << ","
				<< e.count << ","
				<< (profile ? profile->id : "") << ","
				<< status << ","
				<< e.exampleSector << ","
				<< e.exampleGridNo << "\n";
		}

		CHAR8 message[256];
		sprintf_s(message, "Coverage: assigned=%d generic=%d skip=%d missing=%d",
			assigned, generic, skipped, missing);
		ContainerLoot::Trace(message);
	}
}

void ContainerLootScanner::ScanAllMaps()
{
	std::ofstream allMaps("ContainerLoot_allmaps.csv", std::ios::out);
	if (!allMaps)
	{
		ContainerLoot::Trace("ScanAllMaps: cannot open ContainerLoot_allmaps.csv");
		return;
	}

	allMaps << "sector,gridNo,layer,tileset,usIndex,image,tileType,frame,flags,height,"
		<< "openable,vehicle,tree,generic,multi,itemOnTop,hasPool,kind,canOpen,canHide,canSurface\n";

	std::map<UINT32, CatalogEntry> catalog;
	std::set<UINT32> contactGenerated;
	INT32 mapsScanned = 0;

	const INT16 oldX = gWorldSectorX;
	const INT16 oldY = gWorldSectorY;
	const INT8 oldZ = gbWorldSectorZ;
	const BOOLEAN oldEditMode = gfEditMode;
	const BOOLEAN oldAlternate = gfUseAlternateMap;

	gfEditMode = TRUE;
	gfUseAlternateMap = FALSE;

	for (INT8 z = 0; z <= 3; ++z)
	{
		for (INT16 y = 1; y <= 16; ++y)
		{
			for (INT16 x = 1; x <= 16; ++x)
			{
				CHAR8 baseName[128];
				GetMapFileName(x, y, z, baseName, FALSE, FALSE);

				if (MapExists((UINT8*)baseName))
				{
					ScanMapFile(baseName, baseName, x, y, z, allMaps, catalog);
					++mapsScanned;
#if CONTAINER_LOOT_GENERATE_CONTACT_SHEETS
					GenerateContactSheetsForTileset(catalog, contactGenerated, (UINT8)giCurrentTilesetID);
#endif
				}

				const std::string alternate = AlternateMapName(baseName, "_a");
				if (MapExists((UINT8*)alternate.c_str()))
				{
					ScanMapFile(alternate.c_str(), alternate.c_str(), x, y, z, allMaps, catalog);
					++mapsScanned;
#if CONTAINER_LOOT_GENERATE_CONTACT_SHEETS
					GenerateContactSheetsForTileset(catalog, contactGenerated, (UINT8)giCurrentTilesetID);
#endif
				}
			}
		}
	}

	if (gfWorldLoaded)
		TrashWorld();

	gfEditMode = oldEditMode;
	gfUseAlternateMap = oldAlternate;
	gWorldSectorX = oldX;
	gWorldSectorY = oldY;
	gbWorldSectorZ = oldZ;

	allMaps.close();
	WriteCatalogTemplate(catalog);
	WriteCoverageReport(catalog);

#if CONTAINER_LOOT_GENERATE_CONTACT_SHEETS
	WriteContactSheetHtml(catalog, contactGenerated);
#endif

	CHAR8 message[256];
	sprintf_s(message, "ScanAllMaps: maps=%d catalog=%d contact=%d",
		mapsScanned, (INT32)catalog.size(), (INT32)contactGenerated.size());
	ContainerLoot::Trace(message);
}

