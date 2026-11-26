#include <algorithm>
#include <vector>

#include "DoorInterruptHandler.h"

#include "sgp.h"
#include "Overhead.h"
#include "Overhead Types.h"
#include "Soldier Control.h"
#include "Soldier Profile.h"
#include "Soldier macros.h"
#include "opplist.h"
#include "TeamTurns.h"
#include "Points.h"
#include "LOS.h"
#include "SkillCheck.h"
#include "Isometric Utils.h"
#include "lighting.h"
#include "Weapons.h"
#include "random.h"
#include "GameSettings.h"
#include "WorldDef.h"

extern UINT8 gubInterruptProvoker;
extern BOOLEAN InterruptsAllowed;
extern void HandleSystemNewAISituation( SOLDIERTYPE* pSoldier, BOOLEAN fResetABC );
extern void EndInterrupt( BOOLEAN fMarkInterruptOccurred );

namespace
{
	struct SightingDuelEntry
	{
		UINT8 ubSighterID;
		UINT8 ubTargetID;
		INT8 bDuelPts;
	};

	struct IISExposedEntry
	{
		UINT8 ubSighterID;
		UINT8 ubTargetID;
	};

	struct WinningInterrupter
	{
		UINT8 ubSoldierID;
		INT32 iMargin;
	};

	struct DoorInterruptState
	{
		bool fCollectingSightings;
		bool fCollectingIIS;
		UINT8 ubPendingDoorOpenerID;
		INT16 sPendingDoorAPCost;
		std::vector<SightingDuelEntry> sightingDuels;
		std::vector<IISExposedEntry> iisExposed;

		DoorInterruptState()
			: fCollectingSightings( false )
			, fCollectingIIS( false )
			, ubPendingDoorOpenerID( NOBODY )
			, sPendingDoorAPCost( 0 )
		{
		}
	};

	DoorInterruptState& GetState()
	{
		static DoorInterruptState s_state;
		return s_state;
	}

	struct ScopedDoorOpenGuard
	{
		ScopedDoorOpenGuard()
		{
			DoorInterruptState& state = GetState();
			state.iisExposed.clear();
			state.fCollectingIIS = ( gGameOptions.fImprovedInterruptSystem != FALSE );
		}

		~ScopedDoorOpenGuard()
		{
			DoorInterruptState& state = GetState();
			state.fCollectingIIS = false;
			state.iisExposed.clear();
			state.fCollectingSightings = false;
			state.sightingDuels.clear();
			state.ubPendingDoorOpenerID = NOBODY;
			state.sPendingDoorAPCost = 0;
		}
	};

	void ResetAllInterruptDuelPoints()
	{
		for ( UINT32 uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop )
		{
			if ( MercSlots[uiLoop] != nullptr )
			{
				MercSlots[uiLoop]->aiData.bInterruptDuelPts = NO_INTERRUPT;
			}
		}
	}

	SOLDIERTYPE* FindValidSoldier( UINT8 ubID )
	{
		if ( ubID >= TOTAL_SOLDIERS )
		{
			return nullptr;
		}
		SOLDIERTYPE* pSoldier = MercPtrs[ubID];
		if ( pSoldier == nullptr || !pSoldier->bActive || !pSoldier->bInSector )
		{
			return nullptr;
		}
		return pSoldier;
	}

	// Selects the active-team soldier who loses control when interrupted, matching ResolveInterruptsVs.
	SOLDIERTYPE* SelectInterruptedActiveSoldier( SOLDIERTYPE* pFallbackOpener )
	{
		const UINT8 ubCurrentTeam = gTacticalStatus.ubCurrentTeam;

		// For AI teams, take away SOLDIER_UNDERAICONTROL from whichever AI soldier currently holds it.
		if ( !gTacticalStatus.Team[ubCurrentTeam].bHuman )
		{
			for ( UINT8 ubLoop = gTacticalStatus.Team[ubCurrentTeam].bFirstID;
				  ubLoop <= gTacticalStatus.Team[ubCurrentTeam].bLastID;
				  ++ubLoop )
			{
				SOLDIERTYPE* pCandidate = MercPtrs[ubLoop];
				if ( pCandidate != nullptr && ( pCandidate->flags.uiStatusFlags & SOLDIER_UNDERAICONTROL ) )
				{
					pCandidate->flags.uiStatusFlags &= ( ~SOLDIER_UNDERAICONTROL );
					return pCandidate;
				}
			}
		}

		if ( pFallbackOpener != nullptr && pFallbackOpener->bTeam == ubCurrentTeam )
		{
			return pFallbackOpener;
		}

		SOLDIERTYPE* pProvoker = FindValidSoldier( gubInterruptProvoker );
		if ( pProvoker != nullptr && pProvoker->bTeam == ubCurrentTeam )
		{
			return pProvoker;
		}

		if ( gusSelectedSoldier < TOTAL_SOLDIERS )
		{
			SOLDIERTYPE* pSelected = FindValidSoldier( static_cast<UINT8>( gusSelectedSoldier ) );
			if ( pSelected != nullptr && pSelected->bTeam == ubCurrentTeam )
			{
				return pSelected;
			}
		}

		return nullptr;
	}

	bool CanActiveSoldierContestInterrupt( SOLDIERTYPE* pSoldier )
	{
		if ( pSoldier == nullptr || !pSoldier->bActive || !pSoldier->bInSector )
		{
			return false;
		}
		if ( !pSoldier->CanInterrupt() || pSoldier->aiData.bNeutral )
		{
			return false;
		}
		if ( AM_AN_EPC( pSoldier ) && !AM_A_ROBOT( pSoldier ) )
		{
			return false;
		}
		if ( pSoldier->bTeam == gbPlayerNum && pSoldier->bAssignment >= ON_DUTY )
		{
			return false;
		}
		if ( pSoldier->aiData.bPassedLastInterrupt )
		{
			return false;
		}

		const INT16 sMinPtsNeeded = ( gAnimControl[pSoldier->usAnimState].ubEndHeight != ANIM_CROUCH )
			? GetAPsCrouch( pSoldier, TRUE )
			: MinPtsToMove( pSoldier );

		return ( pSoldier->bActionPoints >= sMinPtsNeeded );
	}

	UINT16 CalcIISReactionTime( SOLDIERTYPE* pInterrupter, SOLDIERTYPE* pTarget )
	{
		UINT16 uiReactionTime = static_cast<UINT16>( gGameExternalOptions.ubBasicReactionTimeLengthIIS ) * 10;

		if ( pInterrupter->stats.bAgility >= 80 )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 - ( 2 * ( pInterrupter->stats.bAgility - 80 ) ) ) ) / 100 );
		}
		else if ( pInterrupter->stats.bAgility < 80 && pInterrupter->stats.bAgility > 50 )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 + ( 2 * ( 80 - pInterrupter->stats.bAgility ) ) ) ) / 100 );
		}
		else
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * 8 ) / 5 );
		}

		const INT16 sMaxAP = pInterrupter->CalcActionPoints();
		if ( sMaxAP > 0 )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 + ( 50 - ( 50 * pInterrupter->bActionPoints / sMaxAP ) ) ) ) / 100 );
		}

		if ( pInterrupter->stats.bLife < pInterrupter->stats.bLifeMax && pInterrupter->stats.bLifeMax > 0 )
		{
			const INT16 iInjuryPenalty = static_cast<INT16>(
				( 200 * ( pInterrupter->stats.bLifeMax - pInterrupter->stats.bLife
					+ ( ( pInterrupter->stats.bLifeMax - pInterrupter->stats.bLife - pInterrupter->bBleeding ) / 2 ) ) )
				/ pInterrupter->stats.bLifeMax );
			uiReactionTime = static_cast<UINT16>(
				( uiReactionTime * ( 100 + iInjuryPenalty * ( 100 - ( 3 * EffectiveExpLevel( pInterrupter, TRUE ) ) ) / 100 ) ) / 100 );
		}

		if ( pTarget->bBreath < 100 )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 + ( ( 100 - pTarget->bBreath ) / 2 ) ) ) / 100 );
		}

		if ( pInterrupter->IsGassed() )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 + AIM_PENALTY_GASSED ) ) / 100 );
		}
		if ( pInterrupter->ubServiceCount > 0 )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 + AIM_PENALTY_GETTINGAID ) ) / 100 );
		}
		if ( pInterrupter->aiData.bShock )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * ( 100 + ( pInterrupter->aiData.bShock * 20 ) ) ) / 100 );
		}

		if ( pTarget->ubProfile != NO_PROFILE &&
			 gMercProfiles[pTarget->ubProfile].bCharacterTrait == CHAR_TRAIT_PHLEGMATIC )
		{
			uiReactionTime = static_cast<UINT16>( ( uiReactionTime * 110 ) / 100 );
		}

		return static_cast<UINT16>( ( uiReactionTime + 5 ) / 10 );
	}

	UINT8 CalcIISPointsRegistered( SOLDIERTYPE* pInterrupter, SOLDIERTYPE* pTarget, SOLDIERTYPE* pOpener, INT16 sAPCost )
	{
		INT32 iPercent = static_cast<INT32>( gGameExternalOptions.ubBasicPercentRegisterValueIIS )
			+ ( static_cast<INT32>( gGameExternalOptions.ubPercentRegisterValuePerLevelIIS ) * EffectiveExpLevel( pInterrupter, TRUE ) );

		INT16 iDistVisible = pInterrupter->GetMaxDistanceVisible( pInterrupter->sGridNo, pInterrupter->bTargetLevel, CALC_FROM_ALL_DIRS ) * CELL_X_SIZE;
		iDistVisible = (std::max)( iDistVisible, static_cast<INT16>( CELL_X_SIZE ) );

		const INT32 iRange = GetRangeInCellCoordsFromGridNoDiff( pInterrupter->sGridNo, pTarget->sGridNo );
		iPercent -= (std::min)( 25, static_cast<INT32>( 25 * iRange / iDistVisible ) );

		if ( gGameOptions.fNewTraitSystem )
		{
			if ( !HAS_SKILL_TRAIT( pInterrupter, NIGHT_OPS_NT ) &&
				 pInterrupter->aiData.bOppList[pTarget->ubID] == SEEN_CURRENTLY )
			{
				const INT8 bLightLevel = static_cast<INT8>( LightTrueLevel( pTarget->sGridNo, pTarget->pathing.bLevel ) );
				if ( bLightLevel > 6 )
				{
					iPercent -= bLightLevel;
				}
			}

			if ( HAS_SKILL_TRAIT( pOpener, STEALTHY_NT ) )
			{
				iPercent -= gSkillTraitValues.ubSTReducedAPsRegistered;
			}
		}

		iPercent = (std::max)( 20, (std::min)( 100, iPercent ) );
		const FLOAT fRegistered = ( static_cast<FLOAT>( sAPCost ) * static_cast<FLOAT>( iPercent ) / 100.0f ) + 0.5f;
		return static_cast<UINT8>( (std::max)( 0, (std::min)( 255, static_cast<INT32>( fRegistered ) ) ) );
	}

	void HandleIISDoorOpenInterrupts( SOLDIERTYPE* pOpener, INT16 sDoorAPCost )
	{
		if ( !gGameOptions.fImprovedInterruptSystem || sDoorAPCost <= 0 || pOpener == nullptr )
		{
			return;
		}
		if ( !( gTacticalStatus.uiFlags & TURNBASED ) || !( gTacticalStatus.uiFlags & INCOMBAT ) )
		{
			return;
		}
		if ( gTacticalStatus.ubCurrentTeam != pOpener->bTeam )
		{
			return;
		}
		if ( !InterruptsAllowed || gTacticalStatus.ubAttackBusyCount > 0 || gTacticalStatus.bBoxingState != NOT_BOXING )
		{
			return;
		}

		const DoorInterruptState& state = GetState();
		std::vector<UINT8> interrupters;
		std::vector<UINT8> interrupterTargets;

		for ( UINT16 uCnt = 0; uCnt < MAX_NUM_SOLDIERS; ++uCnt )
		{
			SOLDIERTYPE* pOpponent = MercPtrs[uCnt];
			if ( pOpponent == nullptr || !pOpponent->bActive || !pOpponent->bInSector )
			{
				continue;
			}
			if ( !pOpponent->CanInterrupt() )
			{
				continue;
			}
			if ( pOpponent->bTeam == gbPlayerNum && pOpponent->bAssignment >= ON_DUTY )
			{
				continue;
			}
			if ( pOpener->bTeam == pOpponent->bTeam || pOpener->bSide == pOpponent->bSide )
			{
				continue;
			}
			if ( CONSIDERED_NEUTRAL( pOpener, pOpponent ) || CONSIDERED_NEUTRAL( pOpponent, pOpener ) )
			{
				continue;
			}

			// Prefer the door opener if visible; otherwise use the closest newly-exposed teammate of the opener.
			SOLDIERTYPE* pExposedTarget = nullptr;
			if ( pOpponent->aiData.bOppList[pOpener->ubID] == SEEN_CURRENTLY ||
				 ( pOpponent->aiData.bOppList[pOpener->ubID] == HEARD_THIS_TURN &&
				   SoldierToSoldierLineOfSightTest( pOpponent, pOpener, TRUE, CALC_FROM_ALL_DIRS ) ) )
			{
				pExposedTarget = pOpener;
			}
			else
			{
				INT32 iBestDist = 999999;
				for ( const IISExposedEntry& entry : state.iisExposed )
				{
					if ( entry.ubSighterID != pOpponent->ubID )
					{
						continue;
					}
					SOLDIERTYPE* pTeammate = FindValidSoldier( entry.ubTargetID );
					if ( pTeammate == nullptr || pTeammate->bTeam != pOpener->bTeam )
					{
						continue;
					}
					if ( CONSIDERED_NEUTRAL( pTeammate, pOpponent ) || CONSIDERED_NEUTRAL( pOpponent, pTeammate ) )
					{
						continue;
					}
					if ( pOpponent->aiData.bOppList[pTeammate->ubID] != SEEN_CURRENTLY )
					{
						continue;
					}
					const INT32 iDist = PythSpacesAway( pOpponent->sGridNo, pTeammate->sGridNo );
					if ( iDist < iBestDist )
					{
						iBestDist = iDist;
						pExposedTarget = pTeammate;
					}
				}
			}

			if ( pExposedTarget == nullptr )
			{
				continue;
			}

			const UINT8 ubPointsRegistered = CalcIISPointsRegistered( pOpponent, pExposedTarget, pOpener, sDoorAPCost );
			if ( ubPointsRegistered > 0 )
			{
				const INT32 iNewCounter = static_cast<INT32>( pOpponent->aiData.ubInterruptCounter[pExposedTarget->ubID] ) + ubPointsRegistered;
				pOpponent->aiData.ubInterruptCounter[pExposedTarget->ubID] = static_cast<UINT8>( (std::min)( 255, iNewCounter ) );
			}

			const UINT16 uiReactionTime = CalcIISReactionTime( pOpponent, pExposedTarget );
			if ( pOpponent->aiData.ubInterruptCounter[pExposedTarget->ubID] >= uiReactionTime )
			{
				interrupters.push_back( pOpponent->ubID );
				interrupterTargets.push_back( pExposedTarget->ubID );
			}
		}

		if ( interrupters.empty() )
		{
			return;
		}

		SOLDIERTYPE* pInterrupted = SelectInterruptedActiveSoldier( pOpener );
		if ( pInterrupted == nullptr )
		{
			return;
		}

		AddToIntList( pInterrupted->ubID, FALSE, TRUE );

		const size_t originalCount = interrupters.size();
		for ( size_t i = 0; i < originalCount; ++i )
		{
			const UINT8 ubInterrupterID = interrupters[i];
			const UINT8 ubTargetID = interrupterTargets[i];
			AddToIntList( ubInterrupterID, TRUE, TRUE );
			MercPtrs[ubInterrupterID]->aiData.ubInterruptCounter[ubTargetID] = 0;
		}

		if ( gGameExternalOptions.fAllowCollectiveInterrupts )
		{
			for ( size_t i = 0; i < originalCount; ++i )
			{
				SOLDIERTYPE* pInterrupter = MercPtrs[interrupters[i]];
				const UINT8 ubTargetID = interrupterTargets[i];
				if ( pInterrupter == nullptr )
				{
					continue;
				}

				for ( UINT16 uCnt2 = gTacticalStatus.Team[pInterrupter->bTeam].bFirstID;
					  uCnt2 <= gTacticalStatus.Team[pInterrupter->bTeam].bLastID;
					  ++uCnt2 )
				{
					SOLDIERTYPE* pTeammate = MercPtrs[uCnt2];
					if ( pTeammate == nullptr || pTeammate->bTeam != pInterrupter->bTeam )
					{
						continue;
					}
					if ( pTeammate->stats.bLife < OKLIFE || pTeammate->bCollapsed ||
						 !pTeammate->bActive || !pTeammate->bInSector || pTeammate->bActionPoints < 4 )
					{
						continue;
					}

					const bool fAlreadyIn = ( std::find( interrupters.begin(), interrupters.end(), pTeammate->ubID ) != interrupters.end() );
					if ( !fAlreadyIn && PythSpacesAway( pInterrupter->sGridNo, pTeammate->sGridNo ) <= 5 )
					{
						UINT16 usColIntChance = static_cast<UINT16>( 10 * ( ( ( pInterrupter->stats.bLeadership * 3 ) +
							( EffectiveExpLevel( pInterrupter, TRUE ) * 20 ) +
							( EffectiveExpLevel( pTeammate, TRUE ) * 20 ) +
							( pTeammate->stats.bAgility * 2 ) +
							( pTeammate->stats.bWisdom ) ) / 100 ) );

						if ( HAS_SKILL_TRAIT( pInterrupter, SQUADLEADER_NT ) && gGameOptions.fNewTraitSystem )
						{
							usColIntChance += gSkillTraitValues.ubSLCollectiveInterruptsBonus * NUM_SKILL_TRAITS( pInterrupter, SQUADLEADER_NT );
						}

						if ( PreChance( usColIntChance ) )
						{
							interrupters.push_back( pTeammate->ubID );
							AddToIntList( pTeammate->ubID, TRUE, TRUE );
							pTeammate->aiData.ubInterruptCounter[ubTargetID] = 0;
						}
					}
				}
			}
		}

		gTacticalStatus.ubInterruptPending = DISABLED_INTERRUPT;
		DoneAddingToIntList( pInterrupted, TRUE, SIGHTINTERRUPT );
	}
}

void CDoorInterruptHandler::SetPendingDoorAPCost( UINT8 ubOpenerID, INT16 sAPCost )
{
	DoorInterruptState& state = GetState();
	state.ubPendingDoorOpenerID = ubOpenerID;
	state.sPendingDoorAPCost = sAPCost;
}

void CDoorInterruptHandler::BeginSightingCollection( void )
{
	DoorInterruptState& state = GetState();
	state.sightingDuels.clear();
	state.fCollectingSightings = true;
}

void CDoorInterruptHandler::EndSightingCollection( void )
{
	DoorInterruptState& state = GetState();
	state.sightingDuels.clear();
	state.fCollectingSightings = false;
}

BOOLEAN CDoorInterruptHandler::IsCollectingSightings( void )
{
	return GetState().fCollectingSightings ? TRUE : FALSE;
}

void CDoorInterruptHandler::RecordSightingDuel( UINT8 ubSighterID, UINT8 ubTargetID, INT8 bDuelPts )
{
	DoorInterruptState& state = GetState();
	if ( !state.fCollectingSightings || bDuelPts == NO_INTERRUPT )
	{
		return;
	}
	if ( ubSighterID >= TOTAL_SOLDIERS || ubTargetID >= TOTAL_SOLDIERS )
	{
		return;
	}

	for ( SightingDuelEntry& existing : state.sightingDuels )
	{
		if ( existing.ubSighterID == ubSighterID && existing.ubTargetID == ubTargetID )
		{
			if ( bDuelPts > existing.bDuelPts )
			{
				existing.bDuelPts = bDuelPts;
			}
			return;
		}
	}

	SightingDuelEntry entry;
	entry.ubSighterID = ubSighterID;
	entry.ubTargetID = ubTargetID;
	entry.bDuelPts = bDuelPts;
	state.sightingDuels.push_back( entry );
}

void CDoorInterruptHandler::RecordIISExposedSighting( UINT8 ubSighterID, UINT8 ubTargetID )
{
	DoorInterruptState& state = GetState();
	if ( !state.fCollectingIIS )
	{
		return;
	}
	if ( ubSighterID >= TOTAL_SOLDIERS || ubTargetID >= TOTAL_SOLDIERS )
	{
		return;
	}

	for ( const IISExposedEntry& existing : state.iisExposed )
	{
		if ( existing.ubSighterID == ubSighterID && existing.ubTargetID == ubTargetID )
		{
			return;
		}
	}

	IISExposedEntry entry;
	entry.ubSighterID = ubSighterID;
	entry.ubTargetID = ubTargetID;
	state.iisExposed.push_back( entry );
}

void CDoorInterruptHandler::HandlePostDoorOpen( SOLDIERTYPE* pOpener )
{
	if ( pOpener == nullptr )
	{
		return;
	}

	// Capture any deferred door AP cost set by HandleOpenableStruct before the guard resets it on exit.
	DoorInterruptState& state = GetState();
	const INT16 sDoorAPCost = ( state.ubPendingDoorOpenerID == pOpener->ubID ) ? state.sPendingDoorAPCost : 0;

	ScopedDoorOpenGuard guard;

	// Initialize door-opening opplist state BEFORE noise so door creaking and AllTeamsLookForAll are evaluated together.
	InitOpplistForDoorOpening();

	if ( pOpener->ubDoorOpeningNoise > 0 )
	{
		MakeNoise(
			pOpener->ubID,
			pOpener->aiData.sPendingActionData2,
			pOpener->pathing.bLevel,
			gpWorldLevelData[pOpener->sGridNo].ubTerrainID,
			pOpener->ubDoorOpeningNoise,
			NOISE_CREAKING );
	}

	gubInterruptProvoker = pOpener->ubID;
	AllTeamsLookForAll( TRUE );

	// Under IIS, register door-opening APs after LOS updates (handles both silent frontal opens and SIDE_OPEN_DOOR).
	if ( gGameOptions.fImprovedInterruptSystem && sDoorAPCost > 0 )
	{
		HandleIISDoorOpenInterrupts( pOpener, sDoorAPCost );
	}

	// Check AI guy to cancel what he was doing if situation changed.
	HandleSystemNewAISituation( pOpener, TRUE );
}

void CDoorInterruptHandler::ResolveTurnbasedSightingInterrupts( void )
{
	DoorInterruptState& state = GetState();
	const std::vector<SightingDuelEntry> duels = state.sightingDuels;
	EndSightingCollection();

	if ( !( gTacticalStatus.uiFlags & TURNBASED ) || !( gTacticalStatus.uiFlags & INCOMBAT ) )
	{
		ResetAllInterruptDuelPoints();
		return;
	}

	if ( gGameOptions.fImprovedInterruptSystem && !gGameExternalOptions.fAllowInstantInterruptsOnSight )
	{
		ResetAllInterruptDuelPoints();
		return;
	}

	if ( duels.empty() )
	{
		ResetAllInterruptDuelPoints();
		return;
	}

	const UINT8 ubCurrentTeam = gTacticalStatus.ubCurrentTeam;

	// Collect unique opposing-team sighters who spotted at least one active-team soldier.
	std::vector<UINT8> opposingSighters;
	for ( const SightingDuelEntry& entry : duels )
	{
		SOLDIERTYPE* pSighter = FindValidSoldier( entry.ubSighterID );
		SOLDIERTYPE* pTarget = FindValidSoldier( entry.ubTargetID );
		if ( pSighter == nullptr || pTarget == nullptr )
		{
			continue;
		}
		if ( pSighter->bTeam == ubCurrentTeam || pTarget->bTeam != ubCurrentTeam )
		{
			continue;
		}
		if ( std::find( opposingSighters.begin(), opposingSighters.end(), entry.ubSighterID ) == opposingSighters.end() )
		{
			opposingSighters.push_back( entry.ubSighterID );
		}
	}

	std::vector<WinningInterrupter> winners;

	for ( UINT8 ubOppID : opposingSighters )
	{
		SOLDIERTYPE* pOpp = FindValidSoldier( ubOppID );
		if ( pOpp == nullptr )
		{
			continue;
		}

		// Best duel points this opposing sighter achieved against any newly-spotted active-team soldier.
		INT8 bOppBestPts = -127;
		bool fHasOppPts = false;
		std::vector<UINT8> spottedActiveTargets;

		for ( const SightingDuelEntry& entry : duels )
		{
			if ( entry.ubSighterID != ubOppID )
			{
				continue;
			}
			SOLDIERTYPE* pTarget = FindValidSoldier( entry.ubTargetID );
			if ( pTarget == nullptr || pTarget->bTeam != ubCurrentTeam )
			{
				continue;
			}
			if ( !fHasOppPts || entry.bDuelPts > bOppBestPts )
			{
				bOppBestPts = entry.bDuelPts;
				fHasOppPts = true;
			}
			if ( std::find( spottedActiveTargets.begin(), spottedActiveTargets.end(), entry.ubTargetID ) == spottedActiveTargets.end() )
			{
				spottedActiveTargets.push_back( entry.ubTargetID );
			}
		}

		if ( !fHasOppPts )
		{
			continue;
		}

		// Find the best active-team soldier who newly spotted this opposing sighter.
		SOLDIERTYPE* pBestActive = nullptr;
		INT8 bBestActivePts = -127;

		for ( const SightingDuelEntry& entry : duels )
		{
			if ( entry.ubTargetID != ubOppID )
			{
				continue;
			}
			SOLDIERTYPE* pActive = FindValidSoldier( entry.ubSighterID );
			if ( pActive == nullptr || pActive->bTeam != ubCurrentTeam )
			{
				continue;
			}
			if ( pBestActive == nullptr || entry.bDuelPts > bBestActivePts )
			{
				pBestActive = pActive;
				bBestActivePts = entry.bDuelPts;
			}
		}

		// If nobody on the active team newly spotted pOpp, check if any of pOpp's newly-spotted targets
		// ALREADY saw pOpp before the door opened (so pOpp doesn't falsely get an unseen-ambush auto-interrupt).
		if ( pBestActive == nullptr )
		{
			for ( UINT8 ubActiveID : spottedActiveTargets )
			{
				SOLDIERTYPE* pActive = FindValidSoldier( ubActiveID );
				if ( pActive == nullptr || pActive->aiData.bOppList[ubOppID] != SEEN_CURRENTLY )
				{
					continue;
				}
				if ( !CanActiveSoldierContestInterrupt( pActive ) )
				{
					continue;
				}
				const INT8 bPts = CalcInterruptDuelPts( pActive, ubOppID, TRUE );
				if ( bPts != NO_INTERRUPT && ( pBestActive == nullptr || bPts > bBestActivePts ) )
				{
					pBestActive = pActive;
					bBestActivePts = bPts;
				}
			}
		}

		bool fWonInterrupt = false;
		INT32 iMargin = 0;

		if ( pBestActive == nullptr )
		{
			// Unseen ambush: nobody on the active team can see/contest this interrupter.
			fWonInterrupt = true;
			iMargin = static_cast<INT32>( AUTOMATIC_INTERRUPT ) + static_cast<INT32>( bOppBestPts );
		}
		else
		{
			pOpp->aiData.bInterruptDuelPts = bOppBestPts;
			pBestActive->aiData.bInterruptDuelPts = bBestActivePts;

			fWonInterrupt = ( InterruptDuel( pOpp, pBestActive ) != FALSE );
			iMargin = static_cast<INT32>( pOpp->aiData.bInterruptDuelPts ) - static_cast<INT32>( pBestActive->aiData.bInterruptDuelPts );

			pOpp->aiData.bInterruptDuelPts = NO_INTERRUPT;
			pBestActive->aiData.bInterruptDuelPts = NO_INTERRUPT;
		}

		if ( fWonInterrupt )
		{
			WinningInterrupter winner;
			winner.ubSoldierID = ubOppID;
			winner.iMargin = iMargin;
			winners.push_back( winner );
		}
	}

	if ( !winners.empty() )
	{
		SOLDIERTYPE* pInterrupted = SelectInterruptedActiveSoldier( nullptr );
		if ( pInterrupted != nullptr )
		{
			// Sort ascending by margin so the highest-margin interrupter is pushed LAST onto the LIFO stack and acts FIRST.
			std::sort( winners.begin(), winners.end(),
				[]( const WinningInterrupter& a, const WinningInterrupter& b )
				{
					return a.iMargin < b.iMargin;
				} );

			AddToIntList( pInterrupted->ubID, FALSE, TRUE );

			bool fEndedEarly = false;
			for ( const WinningInterrupter& winner : winners )
			{
				AddToIntList( winner.ubSoldierID, TRUE, TRUE );

				if ( gGameOptions.fImprovedInterruptSystem )
				{
					SOLDIERTYPE* pWinner = MercPtrs[winner.ubSoldierID];
					if ( pWinner != nullptr )
					{
						for ( const SightingDuelEntry& entry : duels )
						{
							if ( entry.ubSighterID == winner.ubSoldierID && entry.ubTargetID < TOTAL_SOLDIERS )
							{
								pWinner->aiData.ubInterruptCounter[entry.ubTargetID] = 0;
							}
						}
					}
				}

				if ( gubOutOfTurnPersons == 1 )
				{
					EndInterrupt( TRUE );
					fEndedEarly = true;
					break;
				}
			}

			if ( !fEndedEarly )
			{
				DoneAddingToIntList( pInterrupted, TRUE, SIGHTINTERRUPT );
			}
		}
	}

	ResetAllInterruptDuelPoints();
}
