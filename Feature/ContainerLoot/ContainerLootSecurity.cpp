#include "Feature/ContainerLoot/ContainerLootSecurity.h"
#include "Feature/ContainerLoot/ContainerLoot.h"

#include "Campaign Types.h"
#include "Keys.h"
#include "structure.h"

#include <map>
#include <set>
#include <stdio.h>
#include <string.h>

// travnik: Generated container locks are stored in the normal DoorTable.
// This class only decides whether a generated lock may be applied and
// remembers which grids belong to the feature for debug regeneration.
namespace
{
	std::map<INT32, std::set<INT32> > sGeneratedLocks;

	INT32 GetSectorId(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ)
	{
		return (INT32)SECTOR(sSectorX, sSectorY) + ((INT32)bSectorZ * 256);
	}

	BOOLEAN IsGeneratedLock(INT32 sectorId, INT32 gridNo)
	{
		std::map<INT32, std::set<INT32> >::const_iterator it = sGeneratedLocks.find(sectorId);
		if (it == sGeneratedLocks.end())
			return FALSE;

		return it->second.find(gridNo) != it->second.end() ? TRUE : FALSE;
	}

	BOOLEAN IsContainerSite(const ContainerLootData::Site& site)
	{
		return (site.kind == ContainerLootData::KIND_CONTAINER &&
			site.canOpen && !site.isOpen) ? TRUE : FALSE;
	}
}

BOOLEAN ContainerLootSecurity::Apply(const ContainerLootData::Context& ctx,
	const ContainerLootData::Site& site,
	const ContainerLootData::PlanEntry& entry)
{
	if (!entry.hasLock)
	{
		if (IsContainerSite(site))
		{
			CHAR8 message[192];
			sprintf_s(message, "ContainerLootSecurity: no lock site=%d profile=%s grid=%d",
				entry.site, entry.profile.c_str(), site.gridNo);
			ContainerLoot::Trace(message);
		}
		return TRUE;
	}

	if (!IsContainerSite(site))
	{
		CHAR8 message[192];
		sprintf_s(message, "ContainerLootSecurity: skip non-container site=%d grid=%d kind=%d",
			entry.site, site.gridNo, (INT32)site.kind);
		ContainerLoot::Trace(message);
		return FALSE;
	}

	// travnik: lock id 0 means "no lock" and must not create a fake locked door.
	if (entry.lockId == 0 || entry.lockId > ContainerLootData::MAX_LOCK_ID)
	{
		CHAR8 message[192];
		sprintf_s(message, "ContainerLootSecurity: invalid lock id=%d site=%d grid=%d",
			(INT32)entry.lockId, entry.site, site.gridNo);
		ContainerLoot::Trace(message);
		return FALSE;
	}

	BOOLEAN hasTrap = entry.hasTrap;
	UINT8 trapId = entry.trapId;
	UINT8 trapLevel = entry.trapLevel;

	if (hasTrap)
	{
		if (trapId == NO_TRAP || trapId >= NUM_DOOR_TRAPS ||
			trapLevel < 1 || trapLevel > ContainerLootData::MAX_TRAP_LEVEL)
		{
			CHAR8 message[192];
			sprintf_s(message, "ContainerLootSecurity: invalid trap id=%d level=%d site=%d grid=%d, trap disabled",
				(INT32)trapId, (INT32)trapLevel, entry.site, site.gridNo);
			ContainerLoot::Trace(message);
			hasTrap = FALSE;
			trapId = NO_TRAP;
			trapLevel = 0;
		}
	}

	const INT32 sectorId = GetSectorId(ctx.sectorX, ctx.sectorY, ctx.sectorZ);
	DOOR* existing = FindDoorInfoAtGridNo(site.gridNo);

	// travnik: Never overwrite a map-authored door or container lock.
	if (existing && !IsGeneratedLock(sectorId, site.gridNo))
	{
		CHAR8 message[192];
		sprintf_s(message, "ContainerLootSecurity: existing non-generated lock at grid=%d, skip",
			site.gridNo);
		ContainerLoot::Trace(message);
		return FALSE;
	}

	DOOR door;
	if (existing)
	{
		door = *existing;
	}
	else
	{
		memset(&door, 0, sizeof(door));
	}

	door.sGridNo = site.gridNo;
	door.fLocked = TRUE;
	door.ubLockID = entry.lockId;
	door.ubTrapID = hasTrap ? trapId : NO_TRAP;
	door.ubTrapLevel = hasTrap ? trapLevel : 0;
	door.bPerceivedLocked = DOOR_PERCEIVED_UNKNOWN;
	door.bPerceivedTrapped = DOOR_PERCEIVED_UNKNOWN;
	door.bLockDamage = 0;

	AddDoorInfoToTable(&door);

	if (!FindDoorInfoAtGridNo(site.gridNo))
	{
		CHAR8 message[192];
		sprintf_s(message, "ContainerLootSecurity: failed to add lock at grid=%d", site.gridNo);
		ContainerLoot::Trace(message);
		return FALSE;
	}

	sGeneratedLocks[sectorId].insert(site.gridNo);

	CHAR8 message[256];
	sprintf_s(message, "ContainerLootSecurity: applied grid=%d profile=%s lock=%d trap=%d level=%d",
		site.gridNo, entry.profile.c_str(), (INT32)door.ubLockID,
		(INT32)door.ubTrapID, (INT32)door.ubTrapLevel);
	ContainerLoot::Trace(message);

	return TRUE;
}

void ContainerLootSecurity::ClearGeneratedForSector(INT16 sSectorX, INT16 sSectorY, INT8 bSectorZ)
{
	const INT32 sectorId = GetSectorId(sSectorX, sSectorY, bSectorZ);
	std::map<INT32, std::set<INT32> >::iterator it = sGeneratedLocks.find(sectorId);
	if (it == sGeneratedLocks.end())
		return;

	for (std::set<INT32>::const_iterator grid = it->second.begin(); grid != it->second.end(); ++grid)
	{
		if (FindDoorInfoAtGridNo(*grid))
			RemoveDoorInfoFromTable(*grid);
	}

	CHAR8 message[192];
	sprintf_s(message, "ContainerLootSecurity: cleared generated locks sector=%d count=%d",
		sectorId, (INT32)it->second.size());
	ContainerLoot::Trace(message);

	sGeneratedLocks.erase(it);
}

void ContainerLootSecurity::Reset()
{
	sGeneratedLocks.clear();
}
