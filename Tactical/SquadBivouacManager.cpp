#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include "Types.h"
	#include "SquadBivouacManager.h"
	#include "Squads.h"
	#include "Items.h"
	#include "Handle Items.h"			// AddItemToPool()
	#include "Tactical Save.h"			// AddItemsToUnLoadedSector()
	#include "Overhead.h"				// MercPtrs, Menptr, gTacticalStatus
	#include "Overhead Types.h"
	#include "Strategic Movement.h"		// GROUP, GetGroup()
	#include "Queen Command.h"			// NumHostilesInSector()
	#include "Strategicmap.h"			// gWorldSectorX/Y/Z
	#include "GameSettings.h"			// gGameExternalOptions (BIVOUAC_* values)
	#include "message.h"				// ScreenMsg
	#include "MessageBoxScreen.h"		// DoMessageBox
	#include "Map Screen Interface.h"	// DoMapMessageBox, MAP_SCREEN messagebox style
	#include "jascreens.h"				// screen ids (MAP_SCREEN)
	#include "screenids.h"
	#include "World Items.h"			// WORLD_ITEM_REACHABLE etc.
	#include "Isometric Utils.h"		// RandomGridNo(), TileIsOutOfBounds()
	#include "Debug.h"
	#include "Text.h"					// gzLateLocalizedString
	#include "Soldier macros.h"
	#include "Assignments.h"			// ON_DUTY, NUMBER_OF_SQUADS related enums
	#include "Game Clock.h"				// PauseGame(), LockPauseState()
	#include "Map Screen Interface Map.h"	// bSelectedInfoChar, gCharactersList
	#include "Map Screen Interface Map Inventory.h"	// fShowMapInventoryPool, pInventoryPoolList
	#include "strategic.h"				// INSERTION_CODE_GRIDNO

// SQUAD_BIVOUAC: bivouac panel stash lives in Map Screen Interface Map Inventory.cpp
// (extern declarations are placed at the dump site, near their only use)
	#include "Map Screen Interface.h"	// sSelMapX/sSelMapY, iCurrentMapSectorZ
	#include "SkillCheck.h"				// EffectiveStrength()
	#include "Soldier Control.h"		// HAS_SKILL_TRAIT, GetBackgroundValue
	#include "interface.h"				// BG_PERC_CARRYSTRENGTH
#endif

#include "SquadBivouacManager.h"

const STR BIVOUAC_SLOTS_FILE_PATH = "TableData\\Items\\BivouacSlots.xml";

// SQUAD_BIVOUAC: per-slot bivouac -> NIV pocket template references.
// Filled by LoadSlotRules(): INI default first, then BivouacSlots.xml overrides per slot.
BivouacSlotConfig	gBivouacSlotsConfig[ BIVOUAC_NUM_SLOTS ];

// SQUAD_BIVOUAC: sector inventory snapshot list (defined in Map Screen Interface Map Inventory.cpp)
extern std::vector<WORLDITEM> pUnSeenItems;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Squad Bivouac Manager - implementation
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / singleton
////////////////////////////////////////////////////////////////////////////////////////////////////

SquadBivouacManager::SquadBivouacManager()
	: m_fBivouacUIMode( FALSE )
	, m_iPendingDisbandSquad( -1 )
	, m_usPendingDisbandSoldierID( 0 )
	, m_iActiveUISquad( -1 )
	, m_iValidationSquad( -1 )
{
	m_PendingAssignChange.fValid = FALSE;
	m_PendingAssignChange.usSoldierID = 0;
	m_PendingAssignChange.bAssignment = -1;
	m_PendingAssignChange.iParam1 = 0;
	m_PendingAssignChange.iParam2 = 0;
	m_PendingAssignChange.iParam3 = 0;

	for ( INT32 i = 0; i < NUMBER_OF_SQUADS; ++i )
	{
		m_Bivouac[i].initialize();
	}

	// SQUAD_BIVOUAC: all slots unconfigured until LoadSlotRules() assigns pocket templates
	for ( INT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
	{
		gBivouacSlotsConfig[i].pPocketIndex = BIVOUAC_SLOT_POCKET_NONE;
	}
}

SquadBivouacManager::~SquadBivouacManager()
{
}

SquadBivouacManager& SquadBivouacManager::Instance()
{
	static SquadBivouacManager s_Instance;
	return s_Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Internal helpers
////////////////////////////////////////////////////////////////////////////////////////////////////

// validate squad id and return array index, or -1
INT32 SquadBivouacManager::SquadToDataIndex( INT32 iSquadID ) const
{
	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return( -1 );
	}
	return( iSquadID );
}

// count living mercs currently assigned to the squad
UINT16 SquadBivouacManager::CountLiveMercsInSquad( INT32 iSquadID ) const
{
	UINT16 usCount = 0;

	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return( 0 );
	}

	for ( INT32 iSlot = 0; iSlot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++iSlot )
	{
		SOLDIERTYPE* pSoldier = Squad[iSquadID][iSlot];
		if ( pSoldier != NULL && pSoldier->bActive && pSoldier->stats.bLife > 0 )
		{
			++usCount;
		}
	}

	return( usCount );
}

// TRUE if the squad's sector is ours and has no hostiles
BOOLEAN SquadBivouacManager::IsSectorPeacefulAndOurs( INT32 iSquadID ) const
{
	INT16 sX = -1, sY = -1;
	INT16 bZ = 0;

	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return( FALSE );
	}

	// squad location
	if ( !SectorSquadIsIn( (INT8)iSquadID, &sX, &sY, &bZ ) )
	{
		return( FALSE );
	}

	// no enemies / bloodcats / hostile civilians in that sector
	if ( NumHostilesInSector( sX, sY, bZ ) > 0 )
	{
		return( FALSE );
	}

	// if the loaded sector is this one and combat is going on, deny as well
	if ( ( gWorldSectorX == sX ) && ( gWorldSectorY == sY ) && ( gbWorldSectorZ == bZ ) )
	{
		if ( ( gTacticalStatus.uiFlags & INCOMBAT ) || gTacticalStatus.fEnemyInSector )
		{
			return( FALSE );
		}
	}

	return( TRUE );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Lifecycle
////////////////////////////////////////////////////////////////////////////////////////////////////

void SquadBivouacManager::Initialize()
{
	// wipe all slots
	for ( INT32 i = 0; i < NUMBER_OF_SQUADS; ++i )
	{
		m_Bivouac[i].initialize();
	}

	// reset UI state
	m_fBivouacUIMode = FALSE;
	m_iPendingDisbandSquad = -1;
	m_usPendingDisbandSoldierID = 0;
	m_iActiveUISquad = -1;
	m_iValidationSquad = -1;
	m_PendingAssignChange.fValid = FALSE;

	// config values are NOT read here.
	// They are read once in LoadGameExternalOptions() (GameSettings.cpp) into
	// gGameExternalOptions (BIVOUAC_MIN_MERCS_REQUIRED / BIVOUAC_WEIGHT_CAPACITY /
	// BIVOUAC_MOVEMENT_PENALTY_MULTIPLIER), so this feature has no hardcoded numbers.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Inventory access
////////////////////////////////////////////////////////////////////////////////////////////////////

BOOLEAN SquadBivouacManager::IsEmpty( INT32 iSquadID )
{
	INT32 iIndex = SquadToDataIndex( iSquadID );
	if ( iIndex == -1 )
	{
		return( TRUE );
	}

	for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
	{
		if ( m_Bivouac[iIndex].Slots[i].exists() )
		{
			return( FALSE );
		}
	}

	return( TRUE );
}

BOOLEAN SquadBivouacManager::IsAccessAllowed( INT32 iSquadID )
{
	// valid squad?
	if ( SquadToDataIndex( iSquadID ) == -1 )
	{
		return( FALSE );
	}

	// squad must have enough LIVE mercs to carry the bivouac
	UINT16 usMinMercs = (UINT16)__max( 1, gGameExternalOptions.usBivouacMinMercsRequired );
	if ( CountLiveMercsInSquad( iSquadID ) < usMinMercs )
	{
		return( FALSE );
	}

	// squad must be stationary (not between sectors)
	if ( IsThisSquadOnTheMove( (INT8)iSquadID ) )
	{
		return( FALSE );
	}

	// sector must be ours and peaceful
	if ( !IsSectorPeacefulAndOurs( iSquadID ) )
	{
		return( FALSE );
	}

	return( TRUE );
}

OBJECTTYPE* SquadBivouacManager::GetInventoryPointer( INT32 iSquadID )
{
	INT32 iIndex = SquadToDataIndex( iSquadID );
	if ( iIndex == -1 )
	{
		return( NULL );
	}

	// pointer to the whole 30-slot array; UI and drag&drop operate on it natively
	return( &(m_Bivouac[iIndex].Slots[0]) );
}

void SquadBivouacManager::DumpToFloor( INT32 iSquadID, INT16 sMapX, INT16 sMapY, INT8 bMapZ, INT32 sGridNo )
{
	INT32 iIndex = SquadToDataIndex( iSquadID );
	if ( iIndex == -1 )
	{
		return;
	}

	// gridno for the drop. If invalid, use a random spot (loaded sector) or entry point flag (unloaded).
	INT32 sDropGridNo = sGridNo;
	if ( TileIsOutOfBounds( sDropGridNo ) )
	{
		// will be resolved per-item below
	}

	BOOLEAN fLoadedSector = ( ( gWorldSectorX == sMapX ) && ( gWorldSectorY == sMapY ) && ( gbWorldSectorZ == bMapZ ) );

	// SQUAD_BIVOUAC: while the fullscreen sector inventory is open, the sector storage is
	// shadowed by the pInventoryPoolList snapshot; closing the panel overwrites the storage
	// with that snapshot (SaveSeenAndUnseenItems / ReBuildWorldItemStashForLoadedSector).
	// If the open panel shows the dump sector, the items must go into the snapshot instead -
	// otherwise they are lost on panel close.
	// EXCEPTION: when the panel is open in BIVOUAC UI mode, the snapshot holds the bivouac
	// slots of a squad (not the sector storage). Appending to it would lose the items
	// (StoreBivouacStash reads only the first BIVOUAC_NUM_SLOTS entries), and closing the
	// panel would write the dumped items back into the bivouac. So in that mode the dump
	// always goes to the floor/temp-file, and the panel snapshot is invalidated instead.
	BOOLEAN fBivouacUIActive = m_fBivouacUIMode;

	// panel currently shows THIS squad's bivouac: it holds the working copy of the slots
	// (slots are only synced back on close), so sync it in first - otherwise items the
	// player just dropped into the panel would not be part of the dump.
	// NOTE: the panel stash is pBivouacPanelList (Map Screen Interface Map Inventory.cpp).
	// pInventoryPoolList belongs to the fullscreen SECTOR inventory and has nothing to do
	// with the bivouac - storing from it would clobber the squad slots with sector items.
	BOOLEAN fPanelShowsThisBivouac = ( fBivouacUIActive && m_iActiveUISquad == iSquadID );
	if ( fPanelShowsThisBivouac )
	{
		extern std::vector<WORLDITEM> pBivouacPanelList;
		StoreBivouacStash( pBivouacPanelList );
	}

	BOOLEAN fSectorInventoryShadowsDumpSector = FALSE;
	// NOTE: no !fBivouacUIActive here - the bivouac panel has its own stash
	// (pBivouacPanelList), so pInventoryPoolList still holds the real sector items even
	// while the bivouac panel is open. If the fullscreen sector inventory is open for the
	// dump sector, its snapshot shadows the world storage (closing it would overwrite the
	// storage WITHOUT the dumped items), so the dump must go into the snapshot.
	if ( fShowMapInventoryPool &&
		 sSelMapX == sMapX && sSelMapY == sMapY && iCurrentMapSectorZ == bMapZ )
	{
		fSectorInventoryShadowsDumpSector = TRUE;
	}

	// SQUAD_BIVOUAC: fill the FIRST FREE slot of the open sector-inventory snapshot instead
	// of appending to the end - items then appear on the page the player is looking at,
	// no need to close/reopen the panel
	auto BivouacAppendToSectorInventory = []( OBJECTTYPE* pObj, INT32 sGridNo, BOOLEAN fLoadedSector )
	{
		WORLDITEM wi;
		wi.initialize( );
		wi.fExists = TRUE;
		wi.bVisible = 1;
		wi.usFlags = WORLD_ITEM_REACHABLE;
		wi.ubLevel = 0;
		wi.bRenderZHeightAboveLevel = 0;
		wi.object = *pObj;

		if ( fLoadedSector )
		{
			// resolve now: AddItemToPool() would spam a beta warning for every -1 item
			INT32 sDropGridNo = sGridNo;
			if ( TileIsOutOfBounds( sDropGridNo ) )
			{
				sDropGridNo = RandomGridNo( );
			}

			// SQUAD_BIVOUAC: snap to the nearest reachable tile - items must not sink in
			// water or land inside a wall (see DumpToFloor)
			{
				INT32 sSafeGridNo = FindNearestAvailableGridNoForItem( sDropGridNo, 10 );
				if ( !TileIsOutOfBounds( sSafeGridNo ) )
				{
					sDropGridNo = sSafeGridNo;
				}
			}
			wi.sGridNo = sDropGridNo;
		}
		else
		{
			// unloaded sector: temp-file path understands this flag and resolves the
			// position when the sector is loaded
			wi.sGridNo = -1;
			wi.usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;
		}

		// find the first free slot in the snapshot (kept in place, empty slots within the
		// list are preserved by the panel code paths); fall back to appending at the end
		UINT32 uiSlot = pInventoryPoolList.size( );
		for ( UINT32 i = 0; i < pInventoryPoolList.size( ); ++i )
		{
			if ( !pInventoryPoolList[i].fExists && !pInventoryPoolList[i].object.exists() )
			{
				uiSlot = i;
				break;
			}
		}

		if ( uiSlot < pInventoryPoolList.size( ) )
		{
			pInventoryPoolList[uiSlot] = wi;
		}
		else
		{
			pInventoryPoolList.push_back( wi );
		}

		// the non-INVFIX_Moa code paths require pInventoryPoolList and pUnSeenItems
		// to stay the same size (RefreshSeenAndUnseenPools indexes both blindly)
		pUnSeenItems.resize( pInventoryPoolList.size( ) );
	};

	if ( fSectorInventoryShadowsDumpSector )
	{
		// sector inventory panel shadows this sector: append everything to its snapshot
		for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
		{
			OBJECTTYPE* pObj = &(m_Bivouac[iIndex].Slots[i]);
			if ( !pObj->exists() )
			{
				continue;
			}

			BivouacAppendToSectorInventory( pObj, sDropGridNo, fLoadedSector );

			// clear the slot
			DeleteObj( pObj );
		}

		// SQUAD_BIVOUAC: recalc page count and list size NOW. ResizeInventoryList() normally
		// runs every frame inside BlitInventoryPoolGraphic(), but the panel can be closed in
		// the same frame the items were appended (disband dialog YES -> character removed ->
		// panel force-closed) before any render happens. The non-INVFIX_Moa save path
		// (SaveSeenAndUnseenItems) only compacts/saves the first
		// MAP_INVENTORY_POOL_SLOT_COUNT * (iLastInventoryPoolPage + 1) entries - without this
		// call the appended items sit beyond that window and are silently lost.
		{
			extern void ResizeInventoryList( void );
			ResizeInventoryList( );
		}

		// SQUAD_BIVOUAC: force the sector inventory panel to repaint immediately so the
		// dumped items show up without closing/reopening the panel
		{
			extern BOOLEAN fMapPanelDirty;
			fMapPanelDirty = TRUE;
		}

		// SQUAD_BIVOUAC: the bivouac panel (if open for this squad) still holds the
		// pre-dump working copy in its stash - refresh it to the now-empty slots,
		// otherwise closing the bivouac panel would write the dumped items back in
		if ( fPanelShowsThisBivouac )
		{
			extern std::vector<WORLDITEM> pBivouacPanelList;
			BuildBivouacStash( pBivouacPanelList );
		}

		return;
	}

	if ( fLoadedSector )
	{
		// loaded sector: add each stack to the item pool individually
		if ( TileIsOutOfBounds( sDropGridNo ) )
		{
			sDropGridNo = RandomGridNo();
		}

		// SQUAD_BIVOUAC: RandomGridNo() (and a merc's gridno received from a caller) is NOT
		// passability-checked - an item dropped on deep water sinks (AddItemToPool deletes
		// items whose Item[].sinks is set) or lands inside a wall/roof where the player
		// cannot pick it up. Snap the anchor to the nearest reachable tile first; if the
		// snap fails (isolated spot), keep the original gridno so AddItemToPool's own
		// fallback logic applies.
		{
			INT32 sSafeGridNo = FindNearestAvailableGridNoForItem( sDropGridNo, 10 );
			if ( !TileIsOutOfBounds( sSafeGridNo ) )
			{
				sDropGridNo = sSafeGridNo;
			}
		}

		for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
		{
			OBJECTTYPE* pObj = &(m_Bivouac[iIndex].Slots[i]);
			if ( !pObj->exists() )
			{
				continue;
			}

			// world flags: reachable so the player can pick it up again
			AddItemToPool( sDropGridNo, pObj, 1, 0, WORLD_ITEM_REACHABLE, 0 );

			// clear the slot
			DeleteObj( pObj );
		}
	}
	else
	{
		// unloaded sector: bulk-add through the temp-file mechanism.
		// Copy stacks to a temporary array because AddItemsToUnLoadedSector consumes them.
		OBJECTTYPE pTempObjects[BIVOUAC_NUM_SLOTS];
		UINT32 uiCount = 0;

		for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
		{
			OBJECTTYPE* pObj = &(m_Bivouac[iIndex].Slots[i]);
			if ( !pObj->exists() )
			{
				continue;
			}

			pTempObjects[uiCount] = *pObj;
			++uiCount;

			// clear the slot after copying
			DeleteObj( pObj );
		}

		if ( uiCount > 0 )
		{
			// SQUAD_BIVOUAC: sector inventory panel is open for this sector - its snapshot
			// will overwrite the storage on close, so append the items there directly
			// (the shadowing branch above already handled that case, so this is unreachable
			// for the panel-open path; kept for safety)
			if ( fSectorInventoryShadowsDumpSector )
			{
				for ( UINT32 i = 0; i < uiCount; ++i )
				{
					BivouacAppendToSectorInventory( &pTempObjects[i], sDropGridNo, fLoadedSector );
				}

				return;
			}

			if ( TileIsOutOfBounds( sDropGridNo ) )
			{
				// no gridno: use the entry-point mechanism (resolved to the map's center
				// gridno when the sector is loaded). Do NOT use FIND_SWEETSPOT here - it
				// would run pathing from the invalid gridno -1 and misplace/lose the items.
				AddItemsToUnLoadedSector( sMapX, sMapY, bMapZ, -1, uiCount, &(pTempObjects[0]),
											0, WORLD_ITEM_REACHABLE | WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT, 0, 1, FALSE );
			}
			else
			{
				AddItemsToUnLoadedSector( sMapX, sMapY, bMapZ, sDropGridNo, uiCount, &(pTempObjects[0]),
											0, WORLD_ITEM_REACHABLE, 0, 1, FALSE );
			}
		}
	}

	// SQUAD_BIVOUAC: the panel is showing this squad's bivouac (working copy). The slots
	// are empty now; refresh the panel snapshot so closing the panel cannot write the
	// just-dumped items back into the bivouac.
	if ( fPanelShowsThisBivouac )
	{
		extern std::vector<WORLDITEM> pBivouacPanelList;
		BuildBivouacStash( pBivouacPanelList );
	}
}

// SQUAD_BIVOUAC: best-known drop anchor for a merc standing in a sector.
// While the tactical map is NOT loaded, soldier->sGridNo is NOWHERE (it is reset when
// the soldier is removed from the tactical sector), so the anchor must come from
// sInsertionGridNo - the position where the merc was last placed in this sector and
// where he will spawn on re-entry. Returns -1 if nothing valid is known.
INT32 SquadBivouacManager::GetMercAnchorGridNo( SOLDIERTYPE* pSoldier )
{
	if ( pSoldier == NULL || !pSoldier->bActive )
	{
		return( -1 );
	}

	// tactical map loaded: the real current position
	if ( !TileIsOutOfBounds( pSoldier->sGridNo ) )
	{
		return( pSoldier->sGridNo );
	}

	// map screen: use the last tactical placement position (where the merc will spawn
	// on re-entry - the sector entry code INSERTION_CODE_GRIDNO restores this position)
	if ( !TileIsOutOfBounds( pSoldier->sInsertionGridNo ) )
	{
		return( pSoldier->sInsertionGridNo );
	}

	// last resort: explicit gridno insertion data
	if ( pSoldier->ubStrategicInsertionCode == INSERTION_CODE_GRIDNO &&
		 pSoldier->usStrategicInsertionData < MAX_MAP_POS )
	{
		return( (INT32)pSoldier->usStrategicInsertionData );
	}

	return( -1 );
}

// SQUAD_BIVOUAC: automatic dump when a squad loses its last member (dismissal, contract
// expiry, death of the last merc). The items must stay in the sector - not vanish with
// the squad. Sector is taken from the squad's current location; a squad between sectors
// has no meaningful drop location, so the bivouac keeps its items in that edge case.
// pLeavingSoldier (optional): the merc leaving the party. If he stands in the dump
// sector, his position is used as the drop anchor (passability-resolved in DumpToFloor),
// so the items land where the player expects them instead of a random spot.
void SquadBivouacManager::AutoDumpBivouacOnSquadLoss( INT32 iSquadID, SOLDIERTYPE* pLeavingSoldier )
{
	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return;
	}

	// nothing to dump
	if ( IsEmpty( iSquadID ) )
	{
		return;
	}

	// squad in transit: no sector to drop into, items stay in the (now hidden) bivouac
	SOLDIERTYPE* pFirst = Squad[ iSquadID ][ 0 ];
	// squad may already be empty - find any remaining member for between-sector state
	if ( pFirst == NULL )
	{
		for ( INT32 iSlot = 1; iSlot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++iSlot )
		{
			if ( Squad[ iSquadID ][ iSlot ] != NULL )
			{
				pFirst = Squad[ iSquadID ][ iSlot ];
				break;
			}
		}
	}

	INT16 sX = -1, sY = -1;
	INT16 bZ = 0;

	// the squad pointer array may already be empty here (removal ran before this call) -
	// fall back to the leaving merc's own sector coordinates passed indirectly is not
	// possible, so use SectorSquadIsIn which works while the squad array still holds
	// the (dead) member. If it fails, keep the items.
	if ( !SectorSquadIsIn( (INT8)iSquadID, &sX, &sY, &bZ ) )
	{
		return;
	}

	if ( pFirst != NULL && pFirst->flags.fBetweenSectors )
	{
		return;
	}

	// SQUAD_BIVOUAC: anchor the drop at the leaving merc's position when he stands in the
	// dump sector - DumpToFloor resolves it to a passable tile nearby (loaded sector) or
	// uses it directly (unloaded sector temp-file + sweet-spot resolution on load).
	// NOTE: GetMercAnchorGridNo also works on the map screen, where sGridNo is NOWHERE.
	INT32 sAnchorGridNo = -1;
	if ( pLeavingSoldier != NULL &&
		 pLeavingSoldier->sSectorX == sX && pLeavingSoldier->sSectorY == sY && pLeavingSoldier->bSectorZ == bZ )
	{
		sAnchorGridNo = GetMercAnchorGridNo( pLeavingSoldier );
	}

	DumpToFloor( iSquadID, sX, sY, bZ, sAnchorGridNo );

	ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzLateLocalizedString[68] );	// "The squad's bivouac has been emptied into the sector."
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Balance / penalties
////////////////////////////////////////////////////////////////////////////////////////////////////

UINT32 SquadBivouacManager::GetTotalWeight( INT32 iSquadID )
{
	INT32 iIndex = SquadToDataIndex( iSquadID );
	if ( iIndex == -1 )
	{
		return( 0 );
	}

	UINT32 uiTotalWeight = 0;
	for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
	{
		// CalculateObjectWeight() accounts for stack size, attachments and dynamic ammo weight
		uiTotalWeight += CalculateObjectWeight( &(m_Bivouac[iIndex].Slots[i]) );
	}

	return( uiTotalWeight );
}

// strength-based max carry weight of one merc, in internal weight units (10ths of kg).
// Reproduces the inverse of CalculateCarriedWeight() at the 100% threshold:
//   100% <= (20 * iStrengthToLiftHalfKilo * weight) / strength
//     =>  maxWeight = (5 * strength) / iStrengthToLiftHalfKilo
// Includes the same modifiers: strength >80 bonus, Bodybuilding trait, carry background.
UINT32 SquadBivouacManager::GetMercMaxCarryWeight( SOLDIERTYPE* pSoldier ) const
{
	if ( pSoldier == NULL || !pSoldier->bActive || pSoldier->stats.bLife <= 0 )
	{
		return( 0 );
	}

	UINT32 ubStrengthForCarrying = EffectiveStrength( pSoldier, FALSE );
	if ( ubStrengthForCarrying > 80 )
	{
		ubStrengthForCarrying += (ubStrengthForCarrying - 80);
	}

	// SANDRO - STOMP traits - Bodybuilding carry weight bonus
	if ( gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT( pSoldier, BODYBUILDING_NT ) )
	{
		ubStrengthForCarrying = (ubStrengthForCarrying * (100 + gSkillTraitValues.ubBBCarryWeightBonus) / 100);
	}

	ubStrengthForCarrying = (ubStrengthForCarrying * (100 + pSoldier->GetBackgroundValue(BG_PERC_CARRYSTRENGTH)) / 100);

	FLOAT fKiloFactor = (FLOAT)gGameExternalOptions.iStrengthToLiftHalfKilo;
	if ( fKiloFactor < 0.1f )
	{
		fKiloFactor = 0.1f;
	}

	// max weight (in 10ths of kg) at which carried percent reaches 100
	return( (UINT32)( ( 5.0f * (FLOAT)ubStrengthForCarrying ) / fKiloFactor ) );
}

UINT32 SquadBivouacManager::GetMercsMaxCarryWeight( INT32 iSquadID )
{
	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return( 0 );
	}

	UINT32 uiTotal = 0;
	for ( INT32 iSlot = 0; iSlot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++iSlot )
	{
		uiTotal += GetMercMaxCarryWeight( Squad[iSquadID][iSlot] );
	}

	return( uiTotal );
}

UINT32 SquadBivouacManager::GetMercsCarriedWeight( INT32 iSquadID )
{
	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return( 0 );
	}

	UINT32 uiTotal = 0;
	for ( INT32 iSlot = 0; iSlot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++iSlot )
	{
		SOLDIERTYPE* pSoldier = Squad[iSquadID][iSlot];
		if ( pSoldier != NULL && pSoldier->bActive && pSoldier->stats.bLife > 0 )
		{
			// plain inventory weight (no dragging): what the merc carries right now
			// (::GetTotalWeight - the global function from Items.cpp, not the manager's method)
			uiTotal += ::GetTotalWeight( pSoldier );
		}
	}

	return( uiTotal );
}

UINT32 SquadBivouacManager::GetMercsFreeWeight( INT32 iSquadID )
{
	if ( iSquadID < 0 || iSquadID >= NUMBER_OF_SQUADS )
	{
		return( 0 );
	}

	UINT32 uiFree = 0;
	for ( INT32 iSlot = 0; iSlot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++iSlot )
	{
		SOLDIERTYPE* pSoldier = Squad[iSquadID][iSlot];
		if ( pSoldier == NULL || !pSoldier->bActive || pSoldier->stats.bLife <= 0 )
		{
			continue;
		}

		UINT32 uiMax = GetMercMaxCarryWeight( pSoldier );
		UINT32 uiCarried = ::GetTotalWeight( pSoldier );

		// an overloaded merc simply contributes nothing, never a negative amount
		uiFree += ( uiMax > uiCarried ) ? ( uiMax - uiCarried ) : 0;
	}

	return( uiFree );
}

// Bivouac weight capacity: config base + free carrying capacity of the squad's mercs.
// Calculated on demand -> automatically reflects squad composition / equipment changes.
UINT32 SquadBivouacManager::GetWeightCapacity( INT32 iSquadID )
{
	UINT32 uiBase = (UINT32)__max( 1, gGameExternalOptions.uiBivouacWeightCapacity );

	return( uiBase + GetMercsFreeWeight( iSquadID ) );
}

FLOAT SquadBivouacManager::GetOverloadRatio( INT32 iSquadID )
{
	UINT32 uiCapacity = GetWeightCapacity( iSquadID );

	UINT32 uiWeight = GetTotalWeight( iSquadID );

	// ratio >= 1.0: below capacity it stays at 1.0 (no penalty), above capacity it grows
	FLOAT fRatio = (FLOAT)uiWeight / (FLOAT)uiCapacity;
	if ( fRatio < 1.0f )
	{
		fRatio = 1.0f;
	}

	return( fRatio );
}

BOOLEAN SquadBivouacManager::IsOverloaded( INT32 iSquadID )
{
	return( GetTotalWeight( iSquadID ) > GetWeightCapacity( iSquadID ) );
}

INT32 SquadBivouacManager::GetOverloadBreathDrain( INT32 iSquadID )
{
	// no drain when not overloaded
	if ( !IsOverloaded( iSquadID ) )
	{
		return( 0 );
	}

	UINT32 uiCapacity = GetWeightCapacity( iSquadID );
	UINT32 uiWeight = GetTotalWeight( iSquadID );

	// overload percent above capacity: 100% = exactly at capacity, 200% = twice the capacity
	INT32 iOverloadPercent = (INT32)( ( 100 * (INT64)uiWeight ) / (INT64)uiCapacity );	// 100..N

	// dynamic drain: overload% * breath multiplier / 100.
	// E.g. 200% overload with multiplier 15 => 30 breath points per minute (scaled by caller).
	INT32 iDrain = ( iOverloadPercent * gGameExternalOptions.usBivouacBreathPenaltyMultiplier ) / 100;

	return( __max( 0, iDrain ) );
}

UINT32 SquadBivouacManager::ApplyMovementTimePenalty( UINT32 uiBaseTime, INT32 iSquadID )
{
	// safety: never touch illegal travel times
	if ( uiBaseTime == 0 || uiBaseTime == 0xffffffff )
	{
		return( uiBaseTime );
	}

	if ( !IsOverloaded( iSquadID ) )
	{
		return( uiBaseTime );
	}

	FLOAT fRatio = GetOverloadRatio( iSquadID );	// > 1.0 here

	// penalty factor: (ratio - 1) * multiplier / 100, e.g.
	//   ratio 1.5, multiplier 15 => +7.5% travel time
	//   ratio 2.0, multiplier 15 => +15% travel time
	FLOAT fPenalty = ( fRatio - 1.0f ) * ( (FLOAT)gGameExternalOptions.usBivouacMovementPenaltyMultiplier / 100.0f );

	FLOAT fResult = (FLOAT)uiBaseTime * ( 1.0f + fPenalty );

	// saturate instead of overflowing
	if ( fResult >= (FLOAT)0xfffffffe )
	{
		return( 0xfffffffe );
	}

	return( (UINT32)( fResult + 0.5f ) );	// round up half a minute
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Disband interception
////////////////////////////////////////////////////////////////////////////////////////////////////

void SquadBivouacManager::SetPendingDisbandSquad( INT32 iSquadID, UINT16 usSoldierID )
{
	m_iPendingDisbandSquad = iSquadID;
	m_usPendingDisbandSoldierID = usSoldierID;
}

// SQUAD_BIVOUAC: remember an assignment change to replay after the dump is confirmed.
// The actual SetSoldierAssignment() call is performed via the extern declared below
// (defined in Assignments.cpp) to avoid a circular include.
extern void BivouacReplayPendingAssignmentChange( void );

void SquadBivouacManager::SetPendingAssignmentChange( SOLDIERTYPE* pSoldier, INT8 bAssignment, INT32 iParam1, INT32 iParam2, INT32 iParam3 )
{
	if ( pSoldier == NULL )
	{
		m_PendingAssignChange.fValid = FALSE;
		return;
	}

	m_PendingAssignChange.usSoldierID = pSoldier->ubID;
	m_PendingAssignChange.bAssignment = bAssignment;
	m_PendingAssignChange.iParam1 = iParam1;
	m_PendingAssignChange.iParam2 = iParam2;
	m_PendingAssignChange.iParam3 = iParam3;
	m_PendingAssignChange.fValid = TRUE;
}

BOOLEAN SquadBivouacManager::TakePendingAssignmentChange( UINT16& usSoldierID, INT8& bAssignment, INT32& iParam1, INT32& iParam2, INT32& iParam3 )
{
	if ( !m_PendingAssignChange.fValid )
	{
		return( FALSE );
	}

	// consume the request (guards against re-entrancy)
	m_PendingAssignChange.fValid = FALSE;

	usSoldierID	= m_PendingAssignChange.usSoldierID;
	bAssignment	= m_PendingAssignChange.bAssignment;
	iParam1		= m_PendingAssignChange.iParam1;
	iParam2		= m_PendingAssignChange.iParam2;
	iParam3		= m_PendingAssignChange.iParam3;

	return( TRUE );
}

BOOLEAN SquadBivouacManager::CanSquadMoveWithBivouac( INT32 iSquadID )
{
	// empty bivouac: no restriction beyond normal game rules
	if ( IsEmpty( iSquadID ) )
	{
		return( TRUE );
	}

	// non-empty bivouac needs enough live mercs to carry it
	UINT16 usMinMercs = (UINT16)__max( 1, gGameExternalOptions.usBivouacMoveMinMercs );

	return( CountLiveMercsInSquad( iSquadID ) >= usMinMercs );
}

BOOLEAN SquadBivouacManager::HandleLastMercLeavingSquad( INT32 iSquadID, SOLDIERTYPE* pLastSoldier )
{
	// empty bivouac -> nothing to protect, allow the removal
	if ( IsEmpty( iSquadID ) )
	{
		return( TRUE );
	}

	// remember who/what we are deciding about
	SetPendingDisbandSquad( iSquadID, ( pLastSoldier != NULL ? pLastSoldier->ubID : 0 ) );

	// pause the game while the player decides (mirrors dismissal flow in Assignments.cpp)
	PauseGame();
	LockPauseState( 9 );

	// ask the player: dump the loot to the floor, or cancel?
	// gzLateLocalizedString[67] = "There are items left in the squad's bivouac. Drop them in the sector before disbanding the squad?"
	DoMapMessageBox( MSG_BOX_BASIC_STYLE, gzLateLocalizedString[67], MAP_SCREEN, MSG_BOX_FLAG_YESNO,
						SquadBivouacManager::BivouacDisbandMessageBoxCallBack );

	// blocked until the callback decides
	return( FALSE );
}

void SquadBivouacManager::BivouacDisbandMessageBoxCallBack( UINT8 bExitValue )
{
	SquadBivouacManager& m = SQBM();

	INT32 iSquadID = m.GetPendingDisbandSquad();

	// resume time regardless of the answer (we paused it when showing the box)
	UnLockPauseState();
	UnPauseGame();

	if ( iSquadID == -1 )
	{
		return;
	}

	if ( bExitValue == MSG_BOX_RETURN_YES )
	{
		// find out where the squad is and dump everything onto the ground there
		INT16 sX = -1, sY = -1;
		INT16 bZ = 0;

		if ( SectorSquadIsIn( (INT8)iSquadID, &sX, &sY, &bZ ) )
		{
			// SQUAD_BIVOUAC: anchor the drop at the leaving merc's position (the player
			// expects the items at his feet) - fallback: random spot / entry point.
			INT32 sAnchorGridNo = -1;

			SOLDIERTYPE* pAnchor = MercPtrs[ m.m_usPendingDisbandSoldierID ];
			if ( pAnchor != NULL && pAnchor->bActive &&
				 pAnchor->sSectorX == sX && pAnchor->sSectorY == sY && pAnchor->bSectorZ == bZ )
			{
				sAnchorGridNo = m.GetMercAnchorGridNo( pAnchor );
			}

			m.DumpToFloor( iSquadID, sX, sY, bZ, sAnchorGridNo );

			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzLateLocalizedString[68] );	// "The squad's bivouac has been emptied into the sector."
		}

		// clear the pending marker: the caller of HandleLastMercLeavingSquad() may now retry the removal
		m.SetPendingDisbandSquad( -1, 0 );

		// SQUAD_BIVOUAC: if a deferred assignment change is waiting (last merc of the squad
		// was being reassigned when the guard triggered), grant a one-shot removal bypass so
		// the assignment code's RemoveCharacterFromSquads() succeeds now that the bivouac is
		// empty, then replay the deferred change (it re-runs the removal itself).
		if ( m.m_PendingAssignChange.fValid )
		{
			extern BOOLEAN gBivouacAllowRemovalOnce;
			gBivouacAllowRemovalOnce = TRUE;

			BivouacReplayPendingAssignmentChange( );
		}	}
	else // MSG_BOX_RETURN_NO (or ESC)
	{
		// cancel: nothing is dumped, the deferred assignment change (if any) is discarded -
		// the merc keeps his current squad and assignment untouched
		m.m_PendingAssignChange.fValid = FALSE;
		m.SetPendingDisbandSquad( -1, 0 );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Map screen UI state
////////////////////////////////////////////////////////////////////////////////////////////////////

INT32 SquadBivouacManager::GetBivouacUISquadID()
{
	if ( !m_fBivouacUIMode )
	{
		return( -1 );
	}

	SOLDIERTYPE* pSoldier = NULL;

	if ( bSelectedInfoChar == -1 || !gCharactersList[bSelectedInfoChar].fValid )
	{
		return( -1 );
	}

	pSoldier = MercPtrs[ gCharactersList[bSelectedInfoChar].usSolID ];

	if ( pSoldier == NULL || !pSoldier->bActive )
	{
		return( -1 );
	}

	// only mercs assigned TO a squad expose its bivouac
	if ( pSoldier->bAssignment < ON_DUTY )
	{
		return( (INT32)pSoldier->bAssignment );
	}

	return( -1 );
}

BOOLEAN SquadBivouacManager::UpdateBivouacUIModeForSelectedChar( INT8 bSelectedInfoChar )
{
	// leaving inventory or no character -> leave bivouac mode too
	if ( bSelectedInfoChar == -1 || !gCharactersList[bSelectedInfoChar].fValid )
	{
		m_fBivouacUIMode = FALSE;
		return( FALSE );
	}

	SOLDIERTYPE* pSoldier = MercPtrs[ gCharactersList[bSelectedInfoChar].usSolID ];

	// character not on a squad -> no bivouac
	if ( pSoldier == NULL || pSoldier->bAssignment < 0 || pSoldier->bAssignment >= ON_DUTY )
	{
		m_fBivouacUIMode = FALSE;
		return( FALSE );
	}

	// access rules (min mercs, peaceful, not moving)
	if ( !IsAccessAllowed( (INT32)pSoldier->bAssignment ) )
	{
		m_fBivouacUIMode = FALSE;
		return( FALSE );
	}

	return( TRUE );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Map screen sector-inventory <-> bivouac bridge (SQUAD_BIVOUAC UI mode)
////////////////////////////////////////////////////////////////////////////////////////////////////

void SquadBivouacManager::BuildBivouacStash( std::vector< WORLDITEM >& vec )
{
	// one WORLDITEM per bivouac slot; the UI renders and drag&drops against these
	vec.resize( BIVOUAC_NUM_SLOTS );

	INT32 iIndex = SquadToDataIndex( m_iActiveUISquad );

	for( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
	{
		WORLDITEM& wi = vec[i];
		wi.initialize();

		if( iIndex == -1 )
		{
			continue;
		}

		OBJECTTYPE* pObj = &(m_Bivouac[iIndex].Slots[i]);
		if( pObj->exists() )
		{
			wi.fExists = TRUE;
			wi.bVisible = 1;
			wi.sGridNo = -1;		// no real position, UI-only
			wi.usFlags = WORLD_ITEM_REACHABLE;
			wi.object = *pObj;
		}
	}
}

void SquadBivouacManager::StoreBivouacStash( std::vector< WORLDITEM >& vec )
{
	INT32 iIndex = SquadToDataIndex( m_iActiveUISquad );
	if( iIndex == -1 )
	{
		return;
	}

	// wipe squad slots first
	m_Bivouac[iIndex].initialize();

	UINT32 uiCount = (UINT32)min( (INT32)vec.size(), (INT32)BIVOUAC_NUM_SLOTS );
	for( UINT32 i = 0; i < uiCount; ++i )
	{
		if( vec[i].fExists && vec[i].object.exists() )
		{
			m_Bivouac[iIndex].Slots[i] = vec[i].object;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Per-slot placement rules (SQUAD_BIVOUAC)
////////////////////////////////////////////////////////////////////////////////////////////////////

// SQUAD_BIVOUAC: TRUE if the slot references a valid NIV pocket template (LBEPocketType)
BOOLEAN SquadBivouacManager::IsSlotConfigured( UINT32 uiSlot ) const
{
	if ( uiSlot >= BIVOUAC_NUM_SLOTS )
	{
		return( FALSE );
	}

	// an unconfigured slot (0xFF) matches no pocket template
	if ( gBivouacSlotsConfig[uiSlot].pPocketIndex == BIVOUAC_SLOT_POCKET_NONE )
	{
		return( FALSE );
	}

	// the referenced pocket template must exist in the loaded Pockets.xml data
	return( gBivouacSlotsConfig[uiSlot].pPocketIndex < LBEPocketType.size( ) );
}

// SQUAD_BIVOUAC: NIV capacity of the slot's pocket template for one item size (0 = does not fit)
UINT8 SquadBivouacManager::GetSlotCapacityForSize( UINT32 uiSlot, UINT8 ubItemSize ) const
{
	if ( IsSlotConfigured( uiSlot ) == FALSE )
	{
		return( 0 );
	}

	UINT8 ubPocketType = gBivouacSlotsConfig[uiSlot].pPocketIndex;

	// item sizes above guiMaxItemSize cannot occur (Items.xml validation) but clamp anyway,
	// exactly like the NIV code does when indexing ItemCapacityPerSize
	UINT8 ubSize = (UINT8)__min( ubItemSize, gGameExternalOptions.guiMaxItemSize );

	// Safety boundary check to prevent out-of-bounds array access in case of corrupted item data
	if (ubSize >= 136)
	{
		return(0);
	}

	return(LBEPocketType[ubPocketType].ItemCapacityPerSize[ubSize]);
}

BivouacSlotConfig SquadBivouacManager::GetSlotConfig( UINT32 uiSlot ) const
{
	BivouacSlotConfig empty;

	empty.pPocketIndex = BIVOUAC_SLOT_POCKET_NONE;

	if ( uiSlot >= BIVOUAC_NUM_SLOTS )
	{
		return( empty );
	}

	return( gBivouacSlotsConfig[uiSlot] );
}

void SquadBivouacManager::SetSlotConfig( UINT32 uiSlot, const BivouacSlotConfig& config )
{
	if ( uiSlot < BIVOUAC_NUM_SLOTS )
	{
		gBivouacSlotsConfig[uiSlot] = config;
	}
}

void SquadBivouacManager::LoadSlotRules( void )
{
	// INI default pocket index for every slot (BivouacSlots.xml overrides per slot).
	// The value 0xFF keeps a slot unconfigured (rejects everything).
	BivouacSlotConfig def;

	def.pPocketIndex = (UINT8)gGameExternalOptions.usBivouacSlotDefaultPocket;

	for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
	{
		gBivouacSlotsConfig[i] = def;
	}

	// SQUAD_BIVOUAC: per-slot overrides from BivouacSlots.xml (optional file).
	// ReadInBivouacSlotRules() fills gBivouacSlotsConfig entries whose uiIndex exists in the
	// file; slots not present in the file keep the INI default pocket index.
	extern BOOLEAN ReadInBivouacSlotRules( STR fileName );
	STR pFileName = BIVOUAC_SLOTS_FILE_PATH;
	if ( FileExists(pFileName) )
	{
		if ( !ReadInBivouacSlotRules( pFileName ) )
		{
			ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"Error: failed to parse TableData\\Items\\BivouacSlots.xml - using default slot rules." );
		}
	}
}

// SQUAD_BIVOUAC: full NIV placement validation chain for one bivouac slot
int SquadBivouacManager::CanItemFitInBivouacSlot( UINT8 ubSlotIndex, OBJECTTYPE* pObject, UINT8 ubCountToAdd, OBJECTTYPE* pCurrentSlotItem )
{
	// invalid slot or object -> reject
	if ( ubSlotIndex >= BIVOUAC_NUM_SLOTS ||
		 pObject == NULL ||
		 pObject->exists() == false )
	{
		return( BIVOUAC_FIT_NO_POCKET );
	}

	// the slot must reference a valid NIV pocket template
	if ( IsSlotConfigured( ubSlotIndex ) == FALSE )
	{
		return( BIVOUAC_FIT_NO_POCKET );
	}

	UINT8 ubPocketType = gBivouacSlotsConfig[ubSlotIndex].pPocketIndex;

	// real item size from the item table (NIV sizes are clamped to guiMaxItemSize)
	UINT8 bItemSize = (UINT8)__min( Item[ pObject->usItem ].ItemSize, gGameExternalOptions.guiMaxItemSize );

	// capacity of the pocket template for this size; 0 = item does not fit by dimensions
	UINT8 ubMaxAllowed = GetSlotCapacityForSize( ubSlotIndex, bItemSize );

	if ( ubMaxAllowed == 0 )
	{
		return( BIVOUAC_FIT_NO_CAPACITY );
	}

	// class restriction mask of the pocket template (0 = any class)
	if ( LBEPocketType[ubPocketType].pRestriction != 0 &&
		 ( Item[ pObject->usItem ].usItemClass & LBEPocketType[ubPocketType].pRestriction ) == 0 )
	{
		return( BIVOUAC_FIT_WRONG_CLASS );
	}

	// resolve the current slot content:
	// BIVOUAC_SLOT_SELF = read from the squad storage, NULL = empty slot
	OBJECTTYPE* pSlotObj = NULL;

	if ( pCurrentSlotItem == BIVOUAC_SLOT_SELF )
	{
		INT32 iValidationSquad = ( m_iValidationSquad != -1 ) ? m_iValidationSquad : m_iActiveUISquad;

		INT32 iDataIndex = SquadToDataIndex( iValidationSquad );

		if ( iDataIndex == -1 )
		{
			return( BIVOUAC_FIT_NO_POCKET );
		}

		pSlotObj = &( m_Bivouac[iDataIndex].Slots[ubSlotIndex] );
	}
	else
	{
		pSlotObj = pCurrentSlotItem;
	}

	// empty slot: accept up to the template capacity
	if ( pSlotObj == NULL || pSlotObj->exists( ) == false )
	{
		return( ubCountToAdd <= ubMaxAllowed ? BIVOUAC_FIT_OK : BIVOUAC_FIT_STACK_OVERFLOW );
	}

	// money always stacks (two separate piles are merged by the stash code)
	if ( Item[ pObject->usItem ].usItemClass == IC_MONEY )
	{
		return( BIVOUAC_FIT_OK );
	}

	// occupied slot with the SAME item: stack up to the remaining capacity
	if ( pSlotObj->usItem == pObject->usItem )
	{
		UINT8 ubCurrentCount = pSlotObj->ubNumberOfObjects;

		return( (UINT16)( ubCurrentCount + ubCountToAdd ) <= (UINT16)ubMaxAllowed ? BIVOUAC_FIT_OK : BIVOUAC_FIT_STACK_OVERFLOW );
	}

	// occupied slot with a DIFFERENT item: allowed (swap, like the vehicle trunk).
	// The swapped-out item keeps its own legality; the slot just changes owners.
	return( BIVOUAC_FIT_OK );
}

BOOLEAN SquadBivouacManager::CanSlotAcceptItem( INT32 iSquadID, UINT32 uiSlot, OBJECTTYPE* pObj ) const
{
	// invalid squad/slot/object -> reject
	if ( SquadToDataIndex( iSquadID ) == -1 ||
		 uiSlot >= BIVOUAC_NUM_SLOTS ||
		 pObj == NULL ||
		 pObj->exists() == false )
	{
		return( FALSE );
	}

	// delegate to the NIV validation chain; the whole cursor stack must fit into the slot.
	// Slot state is validated against iSquadID (the bivouac this check is for).
	SquadBivouacManager* pThis = (SquadBivouacManager*)this;

	// const cast is safe here: CanItemFitInBivouacSlot only reads the squad's slots
	pThis->SetValidationSquad( iSquadID );

	int iReason = pThis->CanItemFitInBivouacSlot( (UINT8)uiSlot, pObj, pObj->ubNumberOfObjects, BIVOUAC_SLOT_SELF );

	// consume the validation squad so later calls fall back to the active UI squad
	pThis->SetValidationSquad( -1 );

	return( iReason == BIVOUAC_FIT_OK ? TRUE : FALSE );
}

BOOLEAN SquadBivouacManager::CanAnySlotAcceptItem( INT32 iSquadID, OBJECTTYPE* pObj ) const
{
	for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
	{
		if ( CanSlotAcceptItem( iSquadID, i, pObj ) )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Save / Load
//
// Layout (fixed-size, forward compatible):
//   UINT32  uiVersionMarker	- format marker (BIVOUAC_SAVE_FORMAT_VERSION)
//   format v2+:
//     UINT32  uiBlockSize	- total byte size of the block AFTER this field; unknown
//                               future formats FileSeek over it instead of failing
//   UINT32  uiNumSquads		- NUMBER_OF_SQUADS, sanity checked on load
//   for each squad:
//     UINT32 uiSlotCount		- BIVOUAC_NUM_SLOTS
//     BIVOUAC_NUM_SLOTS x OBJECTTYPE::Save() (self-describing size per stack)
//
// v1 (savegame 152) has NO size field - those blocks cannot be seeked over; a mismatched
// v1 marker still fails the load (v1 was only ever written by this dev branch).
////////////////////////////////////////////////////////////////////////////////////////////////////

#define BIVOUAC_SAVE_FORMAT_VERSION		2

BOOLEAN SquadBivouacManager::Save( HWFILE hFile )
{
	UINT32	uiNumBytesWritten = 0;
	UINT32	uiVersionMarker = BIVOUAC_SAVE_FORMAT_VERSION;
	UINT32	uiNumSquads = (UINT32)NUMBER_OF_SQUADS;
	UINT32	uiSlotCount = (UINT32)BIVOUAC_NUM_SLOTS;
	INT32	sBlockSizePos = -1;

	// format marker
	FileWrite( hFile, &uiVersionMarker, sizeof( UINT32 ), &uiNumBytesWritten );
	if ( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		return( FALSE );
	}

	// SQUAD_BIVOUAC: reserve the block-size field so an UNKNOWN future format can be
	// skipped safely (FileSeek) instead of desyncing the rest of the savegame.
	// We write the real size afterwards (seek back, write, seek to end).
	sBlockSizePos = FileGetPos( hFile );
	uiNumBytesWritten = 0;
	FileWrite( hFile, &uiNumBytesWritten, sizeof( UINT32 ), &uiNumBytesWritten );	// placeholder
	if ( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		return( FALSE );
	}

	INT32 sBlockStartPos = FileGetPos( hFile );

	// squad count (sanity check on load)
	FileWrite( hFile, &uiNumSquads, sizeof( UINT32 ), &uiNumBytesWritten );
	if ( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		return( FALSE );
	}

	for ( UINT32 iSquad = 0; iSquad < uiNumSquads; ++iSquad )
	{
		// slot count per squad
		FileWrite( hFile, &uiSlotCount, sizeof( UINT32 ), &uiNumBytesWritten );
		if ( uiNumBytesWritten != sizeof( UINT32 ) )
		{
			return( FALSE );
		}

		// each slot saves itself (handles stacks, attachments, LBE internally)
		for ( UINT32 iSlot = 0; iSlot < uiSlotCount; ++iSlot )
		{
			if ( !m_Bivouac[iSquad].Slots[iSlot].Save( hFile, false ) )
			{
				return( FALSE );
			}
		}
	}

	// patch the real block size (everything after the size field)
	INT32 sBlockEndPos = FileGetPos( hFile );
	UINT32 uiBlockSize = (UINT32)( sBlockEndPos - sBlockStartPos );

	if ( !FileSeek( hFile, sBlockSizePos, FILE_SEEK_FROM_START ) )
	{
		return( FALSE );
	}

	FileWrite( hFile, &uiBlockSize, sizeof( UINT32 ), &uiNumBytesWritten );
	if ( uiNumBytesWritten != sizeof( UINT32 ) )
	{
		return( FALSE );
	}

	// return to the end of the block so the caller's stream position stays correct
	if ( !FileSeek( hFile, sBlockEndPos, FILE_SEEK_FROM_START ) )
	{
		return( FALSE );
	}

	return( TRUE );
}

BOOLEAN SquadBivouacManager::Load( HWFILE hFile )
{
	UINT32	uiNumBytesRead = 0;
	UINT32	uiVersionMarker = 0;
	UINT32	uiBlockSize = 0;
	UINT32	uiNumSquads = 0;
	UINT32	uiSlotCount = 0;

	// wipe first, so a partially failed load cannot leave stale items behind
	for ( INT32 i = 0; i < NUMBER_OF_SQUADS; ++i )
	{
		m_Bivouac[i].initialize();
	}

	// format marker
	FileRead( hFile, &uiVersionMarker, sizeof( UINT32 ), &uiNumBytesRead );
	if ( uiNumBytesRead != sizeof( UINT32 ) )
	{
		return( FALSE );
	}

	// SQUAD_BIVOUAC: a future format (higher marker) MUST NOT desync the savegame read -
	// the caller reads more blocks after this one. v2+ blocks carry their total size, so
	// an unknown future format is skipped with FileSeek (bivouacs stay empty).
	// v1 (savegame 152) is a KNOWN format without the size field - read it fully.
	if ( uiVersionMarker != 1 && uiVersionMarker != BIVOUAC_SAVE_FORMAT_VERSION )
	{
		// future format: read the size field and seek over the whole block
		FileRead( hFile, &uiBlockSize, sizeof( UINT32 ), &uiNumBytesRead );
		if ( uiNumBytesRead != sizeof( UINT32 ) )
		{
			return( FALSE );
		}

		// sanity: reject absurd sizes (corrupted file) instead of seeking into nowhere
		if ( uiBlockSize == 0 || uiBlockSize > 64 * 1024 * 1024 )
		{
			return( FALSE );
		}

		if ( !FileSeek( hFile, uiBlockSize, FILE_SEEK_FROM_CURRENT ) )
		{
			return( FALSE );
		}

		// bivouacs stay empty (already wiped) - but the savegame itself loads fine
		return( TRUE );
	}

	// v1 has no size field; v2 carries it (read for layout uniformity)
	if ( uiVersionMarker >= 2 )
	{
		FileRead( hFile, &uiBlockSize, sizeof( UINT32 ), &uiNumBytesRead );
		if ( uiNumBytesRead != sizeof( UINT32 ) )
		{
			return( FALSE );
		}
	}

	// squad count
	FileRead( hFile, &uiNumSquads, sizeof( UINT32 ), &uiNumBytesRead );
	if ( uiNumBytesRead != sizeof( UINT32 ) )
	{
		return( FALSE );
	}
	if ( uiNumSquads != (UINT32)NUMBER_OF_SQUADS )
	{
		// savegame from a build with a different squad count: cannot map data safely
		return( FALSE );
	}

	for ( UINT32 iSquad = 0; iSquad < uiNumSquads; ++iSquad )
	{
		// slot count per squad
		FileRead( hFile, &uiSlotCount, sizeof( UINT32 ), &uiNumBytesRead );
		if ( uiNumBytesRead != sizeof( UINT32 ) )
		{
			return( FALSE );
		}
		if ( uiSlotCount != (UINT32)BIVOUAC_NUM_SLOTS )
		{
			return( FALSE );
		}

		for ( UINT32 iSlot = 0; iSlot < uiSlotCount; ++iSlot )
		{
			OBJECTTYPE& obj = m_Bivouac[iSquad].Slots[iSlot];
			obj.initialize();

			if ( !obj.Load( hFile ) )
			{
				return( FALSE );
			}
		}
	}

	return( TRUE );
}
