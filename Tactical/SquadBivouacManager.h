#ifndef _SQUAD_BIVOUAC_MANAGER_H_
#define _SQUAD_BIVOUAC_MANAGER_H_

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Squad Bivouac Manager (SQUAD_BIVOUAC feature)
//
//	Encapsulated singleton managing a shared 30-slot "bivouac" inventory per squad,
//	accessible on the Map Screen through the Sector Inventory UI.
//
//	Features:
//	 - 30 OBJECTTYPE slots per squad, serializable (savegame-safe, fixed-size blocks)
//	 - Access control: squad not between sectors, sector peaceful, enough live mercs
//	 - Overload penalties: travel time multiplier + per-minute breath (bBreathMax) drain
//	 - Guard against disbanding a squad with items left inside (YES/NO messagebox -> dump to floor)
//	 - All balance values are read from Ja2_Options.ini via gGameExternalOptions (no hardcode)
//
//	Integration: only one-line hooks in procedural engine files (see SQBM() macro).
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Types.h"
#include "Item Types.h"
#include "FileMan.h"
#include "World Items.h"
#include "Squads.h"			// NUMBER_OF_SQUADS
#include <vector>

// number of slots in one squad's bivouac
#define BIVOUAC_NUM_SLOTS				30

// SQUAD_BIVOUAC: marker for an unconfigured bivouac slot
// (pPocketIndex value that matches no pocket template, rejects everything)
#define BIVOUAC_SLOT_POCKET_NONE		0xFF

// SQUAD_BIVOUAC: per-slot bivouac configuration.
// Every slot references a pocket template (pIndex) from Pockets.xml. The NIV pocket
// structure LBEPocketType[pPocketIndex] supplies the item capacity per item size,
// the class restriction mask and the silhouette graphic of the slot.
struct BivouacSlotConfig
{
	UINT8	pPocketIndex;
};

// SQUAD_BIVOUAC: slot validation failure reasons (CanItemFitInBivouacSlot)
enum BivouacFitReason
{
	BIVOUAC_FIT_OK = 0,
	BIVOUAC_FIT_NO_POCKET,		// slot not configured / pocket template missing
	BIVOUAC_FIT_NO_CAPACITY,	// ItemCapacityPerSize[size] == 0
	BIVOUAC_FIT_WRONG_CLASS,	// item class fails the pocket restriction mask
	BIVOUAC_FIT_STACK_OVERFLOW,	// stack does not fit into the remaining capacity
};

// SQUAD_BIVOUAC: sentinel for CanItemFitInBivouacSlot: read the slot state from the
// squad's storage (m_iValidationSquad / m_iActiveUISquad) instead of a caller-provided object
#define BIVOUAC_SLOT_SELF		( (OBJECTTYPE*)-1 )

// SQUAD_BIVOUAC: global per-slot configuration, filled by LoadSlotRules()
extern BivouacSlotConfig	gBivouacSlotsConfig[ BIVOUAC_NUM_SLOTS ];

// forward declarations to keep the header light
class	SOLDIERTYPE;
struct	GROUP;
class	SquadBivouacManager;

// hook helper: SquadBivouacManager::Instance().Method()  ->  SQBM().Method()
#define SQBM()	SquadBivouacManager::Instance()

////////////////////////////////////////////////////////////////////////////////////////////////////
// BivouacData
//
// Plain, serializable POD-ish storage block for ONE squad.
// Kept deliberately simple so Save()/Load() can stream it with fixed sizes.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct BivouacData
{
	// 30 fixed item slots. OBJECTTYPE supports Load()/Save() and initialize().
	OBJECTTYPE	Slots[ BIVOUAC_NUM_SLOTS ];

	void initialize()
	{
		for ( UINT32 i = 0; i < BIVOUAC_NUM_SLOTS; ++i )
		{
			Slots[i].initialize();
		}
	}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// SquadBivouacManager
////////////////////////////////////////////////////////////////////////////////////////////////////
class SquadBivouacManager
{
public:
	// singleton access
	static SquadBivouacManager& Instance();

	////////////////////////////////////////////////////////////////////////////////////////////
	// Lifecycle
	////////////////////////////////////////////////////////////////////////////////////////////

	// Clear all bivouac arrays (New Game start) and read BIVOUAC_* config values.
	// Hooked from InitSquads() in Squads.cpp.
	void Initialize();

	////////////////////////////////////////////////////////////////////////////////////////////
	// Inventory access
	////////////////////////////////////////////////////////////////////////////////////////////

	// TRUE if all 30 slots of this squad are empty.
	BOOLEAN	IsEmpty( INT32 iSquadID );

	// TRUE if the player may open/manipulate this squad's bivouac right now:
	//  - squad id valid
	//  - squad not between sectors (not moving)
	//  - no hostiles in the squad's sector (peaceful & ours)
	//  - number of live mercs in squad >= BIVOUAC_MIN_MERCS_REQUIRED
	//  - no item currently on the map screen cursor
	BOOLEAN	IsAccessAllowed( INT32 iSquadID );

	// Returns pointer to the 30-slot array so the map screen UI can render
	// and drag&drop natively against this storage.
	// Returns NULL on invalid squad id.
	OBJECTTYPE*	GetInventoryPointer( INT32 iSquadID );

	// Dump every item of the squad's bivouac onto the ground at sGridNo.
	// For the LOADED sector: items go through AddItemToPool() and slots are cleared with DeleteObj().
	// For an UNLOADED sector: items go through AddItemsToUnLoadedSector() (single bulk call).
	// sGridNo == -1 means "use random gridno / entry point" (unloaded sector case).
	void	DumpToFloor( INT32 iSquadID, INT16 sMapX, INT16 sMapY, INT8 bMapZ, INT32 sGridNo );

	// SQUAD_BIVOUAC: automatic silent dump used when a squad loses its last member
	// (dismissal / contract expiry / death). Dumps the bivouac into the squad's sector
	// (loaded or unloaded) and shows the standard [68] message. Does nothing for an
	// empty bivouac or a squad that is between sectors (no meaningful drop location).
	void	AutoDumpBivouacOnSquadLoss( INT32 iSquadID, SOLDIERTYPE* pLeavingSoldier = NULL );

	// SQUAD_BIVOUAC: best-known drop anchor for a merc standing in a sector.
	// While the tactical map is NOT loaded, soldier->sGridNo is NOWHERE (it is reset when
	// the soldier is removed from the tactical sector), so the anchor must come from
	// sInsertionGridNo - the position where the merc was last placed in this sector and
	// where he will spawn on re-entry. Returns -1 if nothing valid is known.
	INT32	GetMercAnchorGridNo( SOLDIERTYPE* pSoldier );

	////////////////////////////////////////////////////////////////////////////////////////////
	// Balance / penalties
	////////////////////////////////////////////////////////////////////////////////////////////

	// Total weight of all items inside the squad's bivouac (internal JA2 weight units).
	UINT32	GetTotalWeight( INT32 iSquadID );

	// Total weight capacity of this squad's bivouac:
	//   BIVOUAC_WEIGHT_CAPACITY (config base) + sum of the free (spare) carrying
	//   capacity of every live merc in the squad.
	// Free capacity of one merc = strength-based max carry weight - currently carried weight.
	// Calculated on demand, so it always reflects the current squad composition and equipment.
	UINT32	GetWeightCapacity( INT32 iSquadID );

	// Sum of the strength-based maximum carry weight over all live mercs of the squad.
	// (Informational: free capacity = this - currently carried weight.)
	UINT32	GetMercsMaxCarryWeight( INT32 iSquadID );

	// Sum of the weight currently carried by all live mercs of the squad.
	UINT32	GetMercsCarriedWeight( INT32 iSquadID );

	// Sum of the free (spare) carrying capacity of all live mercs of the squad
	// (never negative for a single merc: an overloaded merc just contributes 0).
	UINT32	GetMercsFreeWeight( INT32 iSquadID );

	// Overload ratio: GetTotalWeight / GetWeightCapacity, clamped to >= 1.0.
	// 1.0 = within capacity, >1.0 = overloaded by that factor.
	FLOAT	GetOverloadRatio( INT32 iSquadID );

	// TRUE if the bivouac is overloaded (ratio > 1.0).
	BOOLEAN	IsOverloaded( INT32 iSquadID );

	// Additional per-minute breath drain for a marching merc of this squad.
	// Scales dynamically with the overload percentage:
	//   drain = overloadPercent * BIVOUAC_BREATH_PENALTY_MULTIPLIER / 100
	// Returns 0 for non-overloaded squads / invalid params.
	INT32	GetOverloadBreathDrain( INT32 iSquadID );

	// Movement time multiplier: returns travel time scaled by overload penalty.
	//   uiTime * (1 + (ratio-1) * BIVOUAC_MOVEMENT_PENALTY_MULTIPLIER / 100)
	// Used as a one-line hook inside GetSectorMovementTime()/GetSectorMvtTimeForGroup().
	UINT32	ApplyMovementTimePenalty( UINT32 uiBaseTime, INT32 iSquadID );

	// TRUE if the squad may move between sectors with its current bivouac:
	// an empty bivouac is always allowed; a non-empty one requires
	// gGameExternalOptions.usBivouacMoveMinMercs live mercs in the squad.
	BOOLEAN	CanSquadMoveWithBivouac( INT32 iSquadID );

	////////////////////////////////////////////////////////////////////////////////////////////
	// Disband interception
	////////////////////////////////////////////////////////////////////////////////////////////

	// Called when the LAST merc is about to leave squad iSquadID.
	// If bivouac is not empty: shows DoMessageBox(YES/NO) and returns FALSE (block removal).
	//   YES -> dump items to sector floor, allow removal.
	//   NO  -> abort, merc stays in the squad.
	// If bivouac is empty: returns TRUE immediately (allow removal).
	BOOLEAN	HandleLastMercLeavingSquad( INT32 iSquadID, SOLDIERTYPE* pLastSoldier );

	// Messagebox callback. Installed as extern "C"-style free function is NOT needed:
	// the static member below is registered directly with DoMessageBox/DoMapMessageBox.
	static void	BivouacDisbandMessageBoxCallBack( UINT8 bExitValue );

	// SQUAD_BIVOUAC: pending assignment change deferred to the disband YES-callback.
	// When the last merc of a squad with a non-empty bivouac is being reassigned, the
	// caller stores the requested change here INSTEAD of performing it; if the player
	// answers YES, items are dumped and the change is replayed; on NO it is discarded.
	struct PendingAssignChange
	{
		UINT16	usSoldierID;
		INT8	bAssignment;
		INT32	iParam1;
		INT32	iParam2;
		INT32	iParam3;
		BOOLEAN	fValid;
	};

	// Queue a SetSoldierAssignment()-call to be executed after the player confirms the dump.
	void	SetPendingAssignmentChange( SOLDIERTYPE* pSoldier, INT8 bAssignment, INT32 iParam1, INT32 iParam2, INT32 iParam3 );

	// Fetch the deferred assignment change (if any) and consume it. Returns FALSE if none pending.
	BOOLEAN	TakePendingAssignmentChange( UINT16& usSoldierID, INT8& bAssignment, INT32& iParam1, INT32& iParam2, INT32& iParam3 );

	////////////////////////////////////////////////////////////////////////////////////////////
	// Per-slot placement rules (SQUAD_BIVOUAC)
	////////////////////////////////////////////////////////////////////////////////////////////

	// Load slot configuration. Priority: BivouacSlots.xml entry (<pPocketIndex>) > INI default
	// (BIVOUAC_SLOT_DEFAULT_POCKET). Called once during game init, after the NIV pocket
	// data (LBEPocketType) is loaded. An unconfigured slot keeps pPocketIndex == 0xFF and
	// rejects everything.
	void	LoadSlotRules( void );

	// TRUE if the slot is configured (references a valid LBEPocketType pocket template).
	BOOLEAN	IsSlotConfigured( UINT32 uiSlot ) const;

	// NIV capacity of this slot for the given item size (0 = item of that size does not fit).
	// Returns 0 for unconfigured/invalid slots.
	UINT8	GetSlotCapacityForSize( UINT32 uiSlot, UINT8 ubItemSize ) const;

	// TRUE if slot uiSlot of this squad may hold pObj right now (pocket template capacity +
	// class restriction + current stack state). An empty slot accepts up to the template
	// capacity; a stack of the same item only up to the free capacity remaining in the slot.
	BOOLEAN	CanSlotAcceptItem( INT32 iSquadID, UINT32 uiSlot, OBJECTTYPE* pObj ) const;

	// SQUAD_BIVOUAC: full NIV placement validation chain for one bivouac slot.
	// Checks item size capacity, class restriction and the current slot stack state against
	// the NIV pocket template. ubCountToAdd is the number of objects being added.
	// pCurrentSlotItem: current content of the slot to validate against. The UI passes the
	// object from the panel stash (the live UI state); NULL = slot is empty.
	// Returns a BivouacFitReason code, BIVOUAC_FIT_OK = placement allowed.
	// Placing a DIFFERENT item into an occupied slot is allowed (swap, like the vehicle
	// trunk) - the swapped-out item leaves the slot with its own legality untouched.
	int		CanItemFitInBivouacSlot( UINT8 ubSlotIndex, OBJECTTYPE* pObject, UINT8 ubCountToAdd, OBJECTTYPE* pCurrentSlotItem );

	// SQUAD_BIVOUAC: squad id whose slot state is checked by CanItemFitInBivouacSlot
	// (used when pCurrentSlotItem == BIVOUAC_SLOT_SELF)
	void	SetValidationSquad( INT32 iSquadID )	{ m_iValidationSquad = iSquadID; }

	// TRUE if at least one slot of this squad's bivouac can hold pObj (used to give a
	// meaningful "item not accepted" message instead of a generic "bivouac full").
	BOOLEAN	CanAnySlotAcceptItem( INT32 iSquadID, OBJECTTYPE* pObj ) const;

	// GetSlotConfig / SetSlotConfig: XML loader uses Set; the UI can read for tooltips.
	BivouacSlotConfig	GetSlotConfig( UINT32 uiSlot ) const;
	void				SetSlotConfig( UINT32 uiSlot, const BivouacSlotConfig& config );


	// Writes all squad bivouac data as a fixed-size block. Returns FALSE on file error.
	BOOLEAN	Save( HWFILE hFile );
	// Reads all squad bivouac data. Returns FALSE on file error or corrupted size fields.
	BOOLEAN	Load( HWFILE hFile );

	////////////////////////////////////////////////////////////////////////////////////////////
	// Map screen UI state
	////////////////////////////////////////////////////////////////////////////////////////////

	// Bivouac overlay mode: when TRUE, the map screen "team inventory" panel shows
	// the bivouac slots of the selected character's squad instead of the vehicle trunk.
	BOOLEAN	IsBivouacUIModeActive()				{ return m_fBivouacUIMode; }
	void	SetBivouacUIMode( BOOLEAN fActive )	{ m_fBivouacUIMode = fActive; }
	void	ToggleBivouacUIMode()				{ m_fBivouacUIMode = !m_fBivouacUIMode; }

	// Squad id currently shown in bivouac UI mode (-1 = none). Derived from selected char.
	INT32	GetBivouacUISquadID();
	// Enable/disable UI mode based on the currently selected map screen character.
	// Returns FALSE (and disables mode) if access is not allowed for that squad.
	BOOLEAN	UpdateBivouacUIModeForSelectedChar( INT8 bSelectedInfoChar );

	// Used by the messagebox callback: squad id pending disband decision.
	void	SetPendingDisbandSquad( INT32 iSquadID, UINT16 usSoldierID );
	INT32	GetPendingDisbandSquad()			{ return m_iPendingDisbandSquad; }

	////////////////////////////////////////////////////////////////////////////////////////////
	// Map screen sector-inventory <-> bivouac bridge (SQUAD_BIVOUAC UI mode)
	////////////////////////////////////////////////////////////////////////////////////////////

	// Build pInventoryPoolList-compatible WORLDITEM array from the squad's bivouac slots.
	// Output vector is resized to BIVOUAC_NUM_SLOTS entries (fExists = slot used).
	void	BuildBivouacStash( std::vector< WORLDITEM >& vec );

	// Read items back from the WORLDITEM array into the squad's bivouac slots (only first
	// BIVOUAC_NUM_SLOTS entries), clearing the squad slots first. Used on closing the UI.
	void	StoreBivouacStash( std::vector< WORLDITEM >& vec );

	// squad whose bivouac is currently displayed in the sector inventory UI, -1 = sector inventory
	INT32	GetActiveUISquadID()				{ return m_iActiveUISquad; }
	void	SetActiveUISquadID( INT32 iSquadID ){ m_iActiveUISquad = iSquadID; }

private:
	SquadBivouacManager();
	~SquadBivouacManager();
	SquadBivouacManager( const SquadBivouacManager& );				// no copy
	SquadBivouacManager& operator=( const SquadBivouacManager& );	// no assign

	// helpers
	INT32	SquadToDataIndex( INT32 iSquadID ) const;		// validate & map id -> array index
	UINT16	CountLiveMercsInSquad( INT32 iSquadID ) const;
	BOOLEAN	IsSectorPeacefulAndOurs( INT32 iSquadID ) const;
	UINT32	GetMercMaxCarryWeight( SOLDIERTYPE* pSoldier ) const;	// strength-based max carry weight of one merc

	// storage: one data block per squad (NUMBER_OF_SQUADS defined in Squads.h)
	BivouacData		m_Bivouac[ NUMBER_OF_SQUADS ];

	// UI state
	BOOLEAN			m_fBivouacUIMode;

	// disband interception state (squad whose bivouac triggered the YES/NO box)
	INT32			m_iPendingDisbandSquad;
	UINT16			m_usPendingDisbandSoldierID;

	// deferred assignment change executed when the player confirms the dump
	PendingAssignChange	m_PendingAssignChange;

	// squad whose bivouac is currently displayed in the map screen sector inventory UI (-1 = none)
	INT32			m_iActiveUISquad;

	// SQUAD_BIVOUAC: squad id used by CanItemFitInBivouacSlot for slot state validation
	// (set by CanSlotAcceptItem from its iSquadID argument, -1 = use m_iActiveUISquad)
	INT32			m_iValidationSquad;

	// SQUAD_BIVOUAC: per-slot pocket template references (loaded from ini default + BivouacSlots.xml)
	// SQUAD_BIVOUAC: static global config shared by all squads (see gBivouacSlotsConfig)
};

#endif // _SQUAD_BIVOUAC_MANAGER_H_
