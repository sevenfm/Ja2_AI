#ifndef __AUTO_RESOLVE_H
#define __AUTO_RESOLVE_H

#include "types.h"

void EnterAutoResolveMode( UINT8 ubSectorX, UINT8 ubSectorY );

// is the autoresolve active?
BOOLEAN IsAutoResolveActive( void );

void EliminateAllEnemies( UINT8 ubSectorX, UINT8 ubSectorY );

void ConvertTacticalBattleIntoStrategicAutoResolveBattle();

UINT8 GetAutoResolveSectorID();

extern BOOLEAN gfTransferTacticalOppositionToAutoResolve;

//Returns TRUE if autoresolve is active or a sector is loaded.
BOOLEAN GetCurrentBattleSectorXYZ( INT16 *psSectorX, INT16 *psSectorY, INT16 *psSectorZ );

void CheckForSoldiersWhoRetreatedIntoMilitiaHeldSectors();

//generic face images
enum
{
	ADMIN_FACE,
	TROOP_FACE,
	ELITE_FACE,
	MILITIA1_FACE,
	MILITIA2_FACE,
	MILITIA3_FACE,
	YM_CREATURE_FACE,
	AM_CREATURE_FACE,
	YF_CREATURE_FACE,
	AF_CREATURE_FACE,
	HUMAN_SKULL,
	CREATURE_SKULL,
	ELITEF_FACE,
	MILITIA1F_FACE,
	MILITIA2F_FACE,
	MILITIA3F_FACE,
	MAX_65_FACES,
};

#endif