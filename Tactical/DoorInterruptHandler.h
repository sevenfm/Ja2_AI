#ifndef DOOR_INTERRUPT_HANDLER_H
#define DOOR_INTERRUPT_HANDLER_H

#include "Types.h"

class SOLDIERTYPE;

class CDoorInterruptHandler
{
public:
	// Full post-door-open sight, noise, interrupt, and AI update sequence.
	static void HandlePostDoorOpen( SOLDIERTYPE* pOpener );

	// Store door-opening AP cost for deferred IIS registration after LOS updates.
	static void SetPendingDoorAPCost( UINT8 ubOpenerID, INT16 sAPCost );

	// Scoped collection of turn-based sighting pairs during AllTeamsLookForAll(TRUE) / door opening.
	static void BeginSightingCollection( void );
	static void EndSightingCollection( void );
	static BOOLEAN IsCollectingSightings( void );

	// Record a valid turn-based sighting duel candidate (keeps highest points per pair).
	static void RecordSightingDuel( UINT8 ubSighterID, UINT8 ubTargetID, INT8 bDuelPts );

	// Record a newly-exposed active-team soldier for IIS door-open AP registration.
	static void RecordIISExposedSighting( UINT8 ubSighterID, UINT8 ubTargetID );

	// Resolve all collected turn-based sighting interrupts cleanly (replaces legacy 6-slot TB handler).
	static void ResolveTurnbasedSightingInterrupts( void );
};

#endif // DOOR_INTERRUPT_HANDLER_H
