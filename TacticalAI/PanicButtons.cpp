#ifdef PRECOMPILEDHEADERS
#include "AI All.h"
#else
#include "AIInternals.h"
#include "ai.h"
#include "pathai.h"
#include "items.h"
#include "World Items.h"
#include "strategicmap.h"
#include "Map Screen Interface Map.h"
#include "Soldier Profile.h"
#include "Quests.h"
// sevenfm
#include "message.h"
#include "Font Control.h"
#endif

#include "Queen Command.h"

void MakeClosestEnemyChosenOne()
{
	UINT32	cnt;
	INT16	sPathCost, sShortestPath = 1000;
	INT8	bOldKeys = -1;
	UINT8	ubClosestEnemy = NOBODY;
	SOLDIERTYPE *pSoldier;
	INT8	bPanicTrigger;
	INT32	sPanicTriggerGridNo;

	DebugMiscInfo(String("MakeClosestEnemyChosenOne"));

	if ( ! (gTacticalStatus.fPanicFlags & PANIC_TRIGGERS_HERE) )
	{
#ifdef BETAVERSION
		PopMessage("MakeClosestEnemyChosenOne: ERROR - Panic Trigger is NOWHERE");
#endif
		return;
	}

	if (!NeedToRadioAboutPanicTrigger() )
	{
		// no active panic triggers
		return;
	}

	// consider every enemy, looking for the closest capable, unbusy one
	for (cnt = 0; cnt < guiNumMercSlots; cnt++)
	{
		pSoldier = MercSlots[cnt];

		if (!pSoldier)	// if this merc is inactive, or not here
		{
			continue;
		}

		//DebugMiscInfo(String("check soldier [%d] gridno %d, level %d", pSoldier->ubID, pSoldier->sGridNo, pSoldier->pathing.bLevel));

		// check this soldier is alive and capable
		if (pSoldier->stats.bLife < OKLIFE ||
			pSoldier->bBreath < OKBREATH ||
			pSoldier->IsUnconscious() ||
			pSoldier->usSoldierFlagMask & SOLDIER_POW)
		{
			continue;
		}

		if ( gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y )
		{
			if ( pSoldier->ubProfile != WARDEN )
			{
				continue;
			}
		}
		else
		{
			// only consider for army guys
			if (pSoldier->bTeam != ENEMY_TEAM )
			{
				continue;
			}
		}

		// if this guy is in battle with opponent(s)
		if (pSoldier->aiData.bOppCnt > 0)
		{
			continue;	// next soldier
		}

		// if this guy is still in serious shock
		if (pSoldier->aiData.bWoundShock > 0 || pSoldier->HighShockLevel())
		{
			continue;	// next soldier
		}

		if ( pSoldier->pathing.bLevel != 0 )
		{
			// screw having guys on the roof go for panic triggers!
			continue;	// next soldier
		}

		bPanicTrigger = ClosestPanicTrigger( pSoldier );
		if (bPanicTrigger == -1)
		{
			continue; // next soldier
		}

		sPanicTriggerGridNo = gTacticalStatus.sPanicTriggerGridNo[ bPanicTrigger ];				
		if (TileIsOutOfBounds(sPanicTriggerGridNo))
		{
			// this should never happen!
			continue;
		}

		// give him keys to see if with them he can get to the panic trigger
		pSoldier->flags.bHasKeys = (pSoldier->flags.bHasKeys << 1) | 1;

		// ok, this enemy appears to be eligible

		// FindAdjacentGrid set HandGrid for us.	If we aren't at that spot already
		if (pSoldier->sGridNo != sPanicTriggerGridNo)
		{
			// get the AP cost for this enemy to go to target position
			sPathCost = PlotPath( pSoldier, sPanicTriggerGridNo, FALSE, FALSE, FALSE, WALKING, FALSE, FALSE, 0);
		}
		else
		{
			sPathCost = 0;
		}

		// set his keys value back to what it was before this hack
		pSoldier->flags.bHasKeys = (pSoldier->flags.bHasKeys >> 1 );

		// if he can get there (or is already there!)
		if (sPathCost || (pSoldier->sGridNo == sPanicTriggerGridNo))
		{
			if (sPathCost < sShortestPath)
			{
				sShortestPath = sPathCost;
				ubClosestEnemy = pSoldier->ubID;
			}
		}
		//else
		//NameMessage(pSoldier,"can't get there...");
	}

	// if we found have an eligible enemy, make him our "chosen one"
	if (ubClosestEnemy < TOTAL_SOLDIERS)
	{
		gTacticalStatus.ubTheChosenOne = ubClosestEnemy;		// flag him as the chosen one
		pSoldier = MercPtrs[gTacticalStatus.ubTheChosenOne];
		DebugMiscInfo(String("make [%d] %s chosen one", ubClosestEnemy, pSoldier->GetNameS().c_str()));
		if ( pSoldier->aiData.bAlertStatus < STATUS_RED )
		{
			pSoldier->aiData.bAlertStatus = STATUS_RED;
			CheckForChangingOrders( pSoldier );
		}
		SetNewSituation( pSoldier );	// set new situation for the chosen one
		pSoldier->flags.bHasKeys = (pSoldier->flags.bHasKeys << 1) | 1; // cheat and give him keys to every door
	}
	else
	{
		DebugMiscInfo(String("could not find chosen one"));
	}
}

void PossiblyMakeThisEnemyChosenOne( SOLDIERTYPE * pSoldier )
{
	INT32		iAPCost, iPathCost;
	//INT8		bOldKeys;
	INT8		bPanicTrigger;
	INT32		sPanicTriggerGridNo;
	UINT32	uiPercentEnemiesKilled = 0;

	if ( ! (gTacticalStatus.fPanicFlags & PANIC_TRIGGERS_HERE) )
	{
		return;
	}

	if ( pSoldier->pathing.bLevel != 0 )
	{
		// screw having guys on the roof go for panic triggers!
		return;
	}

	// blind soldier cannot become chosen one
	if(pSoldier->bBlindedCounter > 0)
	{
		return;
	}

	bPanicTrigger = ClosestPanicTrigger( pSoldier );
	if (bPanicTrigger == -1)
	{
		return;
	}

	sPanicTriggerGridNo = gTacticalStatus.sPanicTriggerGridNo[ bPanicTrigger ];

	// sevenfm: r8133 fix
	//if ( gTacticalStatus.Team[ENEMY_TEAM].bMenInSector + gTacticalStatus.ubArmyGuysKilled > 0 )
		//uiPercentEnemiesKilled = (UINT32)( 100 * (UINT32)(gTacticalStatus.ubArmyGuysKilled) / (UINT32)( gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector + gTacticalStatus.ubArmyGuysKilled ) );

	uiPercentEnemiesKilled = ArmyPercentKilled();

	if ( gTacticalStatus.ubPanicTolerance[ bPanicTrigger ] > uiPercentEnemiesKilled )
	{
		// not yet... not yet
		return;
	}

	//bOldKeys = pSoldier->flags.bHasKeys;
	pSoldier->flags.bHasKeys = (pSoldier->flags.bHasKeys << 1) | 1;

	// if he can't get to a spot where he could get at the panic trigger
	iAPCost = APBPConstants[AP_PULL_TRIGGER];
	if (pSoldier->sGridNo != sPanicTriggerGridNo)
	{
		iPathCost = PlotPath( pSoldier, sPanicTriggerGridNo, FALSE, FALSE, FALSE, RUNNING, FALSE, FALSE, 0);
		if (iPathCost == 0)
		{
			//pSoldier->flags.bHasKeys = bOldKeys;
			pSoldier->flags.bHasKeys = (pSoldier->flags.bHasKeys >> 1);
			return;
		}
		iAPCost += iPathCost;

	}

	if ( iAPCost <= pSoldier->CalcActionPoints( ) * 2)
	{
		// go!!!
		gTacticalStatus.ubTheChosenOne = pSoldier->ubID;
		return;
	}
	// else return keys to normal
	//pSoldier->flags.bHasKeys = bOldKeys;
	pSoldier->flags.bHasKeys = (pSoldier->flags.bHasKeys >> 1);
}


INT8 PanicAI(SOLDIERTYPE *pSoldier, UINT8 ubCanMove)
{
	BOOLEAN		fFoundRoute = FALSE;
	INT8		bSlot;
	INT32		iPathCost;
	INT8		bPanicTrigger;
	INT32		sPanicTriggerGridNo;
	UINT16		usMovementMode;
#ifdef DEBUGDECISIONS
	STR16		tempstr;
#endif	

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Panic AI]"));

	// if there are panic bombs here
	if (gTacticalStatus.fPanicFlags & PANIC_BOMBS_HERE)
	{
		DebugAI(AI_MSG_TOPIC, pSoldier, String("found panic bomb"));

		// if enemy is holding a portable panic bomb detonator, he tries to use it
		bSlot = FindTrigger( pSoldier );
		if (bSlot != NO_SLOT)
		{
			DebugAI(AI_MSG_TOPIC, pSoldier, String("found trigger, slot %d", bSlot));
			//////////////////////////////////////////////////////////////////////
			// ACTIVATE DETONATOR: blow up sector's panic bombs
			//////////////////////////////////////////////////////////////////////

			// if we have enough APs to activate it now
			if (pSoldier->bActionPoints >= APBPConstants[AP_USE_REMOTE])
			{
#ifdef TESTVERSION
				sprintf(tempstr,"TEST MSG: %s - ACTIVATING his DETONATOR!",pSoldier->name);
				PopMessage(tempstr);
#endif
				DebugAI(AI_MSG_TOPIC, pSoldier, String("blow up panic bomb"));
				// blow up all the PANIC bombs!
				return(AI_ACTION_USE_DETONATOR);
			}
			else	 // otherwise, wait a turn
			{
				DebugAI(AI_MSG_TOPIC, pSoldier, String("wait a turn"));
				pSoldier->aiData.usActionData = NOWHERE;
				return(AI_ACTION_NONE);
			}
		}
	}

	// sevenfm: blind soldier cannot do panic AI except activating detonator
	if (pSoldier->bBlindedCounter > 0)
	{
		return(-1);
	}

	// no panic bombs, or no portable detonator

	// if there's a panic trigger here (DOESN'T MATTER IF ANY PANIC BOMBS EXIST!)
	if ( gTacticalStatus.fPanicFlags & PANIC_TRIGGERS_HERE )
	{
		DebugAI(AI_MSG_TOPIC, pSoldier, String("found panic trigger"));
		usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_GET_CLOSER);
		DebugAI(AI_MSG_TOPIC, pSoldier, String("movement mode %d", usMovementMode));

		// Have WE been chosen to go after the trigger?
		if (pSoldier->ubID == gTacticalStatus.ubTheChosenOne)
		{
			bPanicTrigger = ClosestPanicTrigger( pSoldier );

			if (bPanicTrigger == -1)
			{
				// augh!
				return( -1 );
			}

			sPanicTriggerGridNo = gTacticalStatus.sPanicTriggerGridNo[ bPanicTrigger ];
			DebugAI(AI_MSG_TOPIC, pSoldier, String("closest panic trigger %d, spot %d", bPanicTrigger, sPanicTriggerGridNo));

			// if not standing on the panic trigger
			if (pSoldier->sGridNo != sPanicTriggerGridNo)
			{
				// determine whether we can still get there
				iPathCost = PlotPath( pSoldier, sPanicTriggerGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
				if (iPathCost != 0)
				{
					DebugAI(AI_MSG_TOPIC, pSoldier, String("found route to panic trigger, path cost %d", iPathCost));
					fFoundRoute = TRUE;
				}
			}
			else
			{
				DebugAI(AI_MSG_TOPIC, pSoldier, String("standing at panic trigger spot"));
				fFoundRoute = TRUE;
			}

			// sevenfm: when in black state, ignore panic trigger if we don't have enough AP to reach and activate it in one turn
			if( fFoundRoute &&
				(pSoldier->aiData.bAlertStatus == STATUS_BLACK || pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier)) &&
				pSoldier->bActionPoints < iPathCost + APBPConstants[AP_CHANGE_FACING] + APBPConstants[AP_PULL_TRIGGER] )
			{
				fFoundRoute = FALSE;
			}

			// if red alert is raised, also check path for fresh corpses
			if( fFoundRoute && pSoldier->aiData.bAlertStatus >= STATUS_RED )
			{
				DebugAI(AI_MSG_TOPIC, pSoldier, String("check path to panic trigger for fresh corpses"));
				//INT16 sLoop;
				INT32 sCheckGridNo = pSoldier->sGridNo;
				INT32 sMoveSpot = InternalGoAsFarAsPossibleTowards(pSoldier, sPanicTriggerGridNo, 0, AI_ACTION_GET_CLOSER, 0);
				DebugAI(AI_MSG_TOPIC, pSoldier, String("found move spot %d", sMoveSpot));

				if( !TileIsOutOfBounds(sMoveSpot) )
				{
					INT32 sDangerousSpot, sLastSafeSpot;
					if (AbortFinalSpot(pSoldier, sMoveSpot, AI_ACTION_GET_CLOSER, NOWHERE, 0, sDangerousSpot))
					{
						fFoundRoute = FALSE;
					}
					else if (AbortPath(pSoldier, AI_ACTION_GET_CLOSER, NOWHERE, 0, sDangerousSpot, sLastSafeSpot))
					{
						fFoundRoute = FALSE;
					}
					
					/*for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
					{
						sCheckGridNo = NewGridNo( sCheckGridNo,DirectionInc( (UINT8) (pSoldier->pathing.usPathingData[sLoop]) ) );
						// stop if found fresh corpse near spot and enemy can attack me at spot
						if( CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, TRUE) &&
							EnemyCanAttackSpot(pSoldier, sCheckGridNo,  pSoldier->pathing.bLevel) )
						{
							DebugAI(AI_MSG_TOPIC, pSoldier, String("found fresh corpse at %d, enemy can attack spot", sCheckGridNo));
							fFoundRoute = FALSE;
							break;
						}
					}*/
				}
			}

			// if we managed to find an adjacent spot
			if (fFoundRoute)
			{
				DebugAI(AI_MSG_TOPIC, pSoldier, String("have good route to panic triger"));
				/*
				*** COMMENTED OUT BECAUSE WE DON'T HAVE SUPPORT ROUTINES YET

				// make sure it's not in water (those triggers can't be pulled)
				if (Water(Terrain(gTacticalStatus.sHandGrid),Structure(gTacticalStatus.sHandGrid)))
				{
				#ifdef BETAVERSION
				PopMessage("BAD SCENARIO DESIGN: Enemies can't use this panic trigger!");
				#endif
				gTacticalStatus.ubTheChosenOne = NOBODY;	// strip him of his Chosen One status
				// don't bother replacing him either, the next won't have more luck!
				return(-1);
				}

				*/
				// if we are at that spot now
				if (pSoldier->sGridNo == sPanicTriggerGridNo)
				{
					DebugAI(AI_MSG_TOPIC, pSoldier, String("we are at the spot, activate the trigger!"));
					////////////////////////////////////////////////////////////////
					// PULL THE PANIC TRIGGER!
					////////////////////////////////////////////////////////////////

					// and we have enough APs left to pull the trigger
					if (pSoldier->bActionPoints >= APBPConstants[AP_PULL_TRIGGER])
					{
						// blow up the all the PANIC bombs (or just the journal)
						pSoldier->aiData.usActionData = sPanicTriggerGridNo;

#ifdef TESTVERSION
						sprintf(tempstr,"TEST MSG: %s - PULLS PANIC TRIGGER at grid %d",
							pSoldier->name,pSoldier->aiData.usActionData);
						PopMessage(tempstr);
#endif
						DebugAI(AI_MSG_TOPIC, pSoldier, String("enough AP, activate trigger!"));
						return(AI_ACTION_PULL_TRIGGER);
					}
					else		// otherwise, wait a turn
					{
						DebugAI(AI_MSG_TOPIC, pSoldier, String("wait a turn, not enough AP"));
						pSoldier->aiData.usActionData = NOWHERE;
						return(AI_ACTION_NONE);
					}
				}
				else			// we are NOT at the HandGrid spot
				{
					// if we can move at least 1 square's worth
					if (ubCanMove)
					{
						// if we can get to the HandGrid spot to yank the trigger
						// animations don't allow trigger-pulling from water, so we won't!
						if (LegalNPCDestination(pSoldier,sPanicTriggerGridNo,ENSURE_PATH,NOWATER,0))
						{
							pSoldier->aiData.usActionData = sPanicTriggerGridNo;
							pSoldier->pathing.bPathStored = TRUE;

#ifdef DEBUGDECISIONS
							sprintf(tempstr,"%s - GETTING CLOSER to PANIC TRIGGER at grid %d (Trigger at %d)", pSoldier->name,pSoldier->aiData.usActionData,sPanicTriggerGridNo);
							AIPopMessage(tempstr);
#endif
							DebugAI(AI_MSG_TOPIC, pSoldier, String("move closet to panic trigger %d", sPanicTriggerGridNo));
							return(AI_ACTION_GET_CLOSER);
						}
						else		// Oh oh, the chosen one can't get to the trigger!
						{
#ifdef TESTVERSION
							PopMessage("TEST MSG: Oh oh!	!legalDest - ChosenOne can't get to the trigger!");
#endif
							DebugAI(AI_MSG_TOPIC, pSoldier, String("cannot move to the trigger, find another chosen one"));
							gTacticalStatus.ubTheChosenOne = NOBODY;	// strip him of his Chosen One status
							MakeClosestEnemyChosenOne();	 // and replace him!
						}
					}
					else		 // can't move, wait 1 turn
					{
						DebugAI(AI_MSG_TOPIC, pSoldier, String("cannot move, wait one turn"));
						pSoldier->aiData.usActionData = NOWHERE;
						return(AI_ACTION_NONE);
					}
				}
			}
			else	 // Oh oh, the chosen one can't get to the trigger!
			{
#ifdef TESTVERSION
				PopMessage("TEST MSG: Oh oh!	!adjacentFound - ChosenOne can't get to the trigger!");
#endif
				DebugAI(AI_MSG_TOPIC, pSoldier, String("cannot find route to the trigger, find another chosen one"));
				gTacticalStatus.ubTheChosenOne = NOBODY; // strip him of his Chosen One status
				MakeClosestEnemyChosenOne();	// and replace him!
			}
		}
	}

	// no action decided
	return(-1);
}

void InitPanicSystem( void )
{
	// start by assuming there is no panic bombs or triggers here
	gTacticalStatus.ubTheChosenOne				= NOBODY;
	FindPanicBombsAndTriggers();
}

BOOLEAN PanicTriggerExistsAtSpot(INT32 sSpot)
{
	if (FindStructure(sSpot + DirectionInc(NORTH), STRUCTURE_SWITCH))
	{
		return TRUE;
	}
	if (FindStructure(sSpot + DirectionInc(WEST), STRUCTURE_SWITCH))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN PanicTriggerAtSpot(INT32 sSpot)
{
	for (INT8 bLoop = 0; bLoop < NUM_PANIC_TRIGGERS; bLoop++)
	{
		if (!TileIsOutOfBounds(gTacticalStatus.sPanicTriggerGridNo[bLoop]))
		{
			if (gTacticalStatus.sPanicTriggerGridNo[bLoop] == sSpot)
				return TRUE;
		}
	}

	return FALSE;
}

INT8 ClosestPanicTrigger( SOLDIERTYPE * pSoldier )
{
	INT8		bLoop;
	INT16		sDistance;
	INT16		sClosestDistance = 1000;
	INT8		bClosestTrigger = -1;
	UINT32		uiPercentEnemiesKilled = 0;

	if ( gTacticalStatus.Team[ENEMY_TEAM].bMenInSector + gTacticalStatus.ubArmyGuysKilled > 0 )
		uiPercentEnemiesKilled = (UINT32)(100 * (UINT32)(gTacticalStatus.ubArmyGuysKilled) / (UINT32)(gTacticalStatus.Team[ENEMY_TEAM].bMenInSector + gTacticalStatus.ubArmyGuysKilled));

	for ( bLoop = 0; bLoop < NUM_PANIC_TRIGGERS; bLoop++ )
	{		
		if (!TileIsOutOfBounds(gTacticalStatus.sPanicTriggerGridNo[ bLoop ]))
		{

			if ( gTacticalStatus.ubPanicTolerance[ bLoop ] > uiPercentEnemiesKilled )
			{
				// not yet... not yet...
				continue; // next trigger
			}

			// in Tixa
			if ( gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y )
			{
				// screen out everyone but the warden
				if ( pSoldier->ubProfile != WARDEN )
				{
					break;
				}

				// screen out the second/later panic trigger if the first one hasn't been triggered				
				if ( bLoop > 0 && !TileIsOutOfBounds(gTacticalStatus.sPanicTriggerGridNo[ bLoop - 1 ]) )
				{
					break;
				}
			}

			// sevenfm: check if panic trigger was destroyed
			if (!PanicTriggerExistsAtSpot(gTacticalStatus.sPanicTriggerGridNo[bLoop]))
			{
				continue;
			}

			sDistance = PythSpacesAway( pSoldier->sGridNo, gTacticalStatus.sPanicTriggerGridNo[ bLoop ] );
			if (sDistance < sClosestDistance)
			{
				sClosestDistance = sDistance;
				bClosestTrigger = bLoop;
			}
		}
	}

	return( bClosestTrigger );
}

BOOLEAN NeedToRadioAboutPanicTrigger( void )
{
	UINT32		uiPercentEnemiesKilled;
	INT8		bLoop;

	// if sector is jammed - no panic triggers
	// sevenfm: allow panic triggers since local communication is allowed now in jammed sector
	/*if( SectorJammed() )
	{
		return( FALSE );
	}*/

	if ( !(gTacticalStatus.fPanicFlags & PANIC_TRIGGERS_HERE) || gTacticalStatus.ubTheChosenOne != NOBODY )
	{
		// already done!
		return( FALSE );
	}

	if ( gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector == 0 )
	{
		return( FALSE );
	}

	if ( gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y )
	{
		SOLDIERTYPE * pSoldier;
		pSoldier = FindSoldierByProfileID( WARDEN, FALSE );
		if ( !pSoldier || pSoldier->ubID == gTacticalStatus.ubTheChosenOne )
		{
			return( FALSE );
		}
	}

	//uiPercentEnemiesKilled = (UINT32)( 100 * (UINT32)(gTacticalStatus.ubArmyGuysKilled) / (UINT32)( gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector + gTacticalStatus.ubArmyGuysKilled ) );
	uiPercentEnemiesKilled = ArmyPercentKilled();

	for ( bLoop = 0; bLoop < NUM_PANIC_TRIGGERS; bLoop++ )
	{
		// if the bomb exists and its tolerance has been exceeded		
		if( !TileIsOutOfBounds(gTacticalStatus.sPanicTriggerGridNo[ bLoop ]) && 
			uiPercentEnemiesKilled >= gTacticalStatus.ubPanicTolerance[ bLoop ] )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}


#define STAIRCASE_GRIDNO 12067
#define STAIRCASE_DIRECTION 0

INT8 HeadForTheStairCase( SOLDIERTYPE * pSoldier )
{
	UNDERGROUND_SECTORINFO * pBasementInfo;

	pBasementInfo = FindUnderGroundSector( 3, MAP_ROW_P, 1 );
	if ( pBasementInfo && pBasementInfo->uiTimeCurrentSectorWasLastLoaded != 0 && ( pBasementInfo->ubNumElites + pBasementInfo->ubNumTroops + pBasementInfo->ubNumAdmins ) < 5 )
	{
		return( AI_ACTION_NONE );
	}

	if ( PythSpacesAway( pSoldier->sGridNo, STAIRCASE_GRIDNO ) < 2 )
	{
		return( AI_ACTION_TRAVERSE_DOWN );
	}
	else
	{
		if ( LegalNPCDestination( pSoldier, STAIRCASE_GRIDNO, ENSURE_PATH, WATEROK, 0 ) )
		{
			pSoldier->aiData.usActionData = STAIRCASE_GRIDNO;
			return( AI_ACTION_GET_CLOSER );
		}
	}
	return( AI_ACTION_NONE );
}

#define WARDEN_ALARM_GRIDNO 9376
#define WARDEN_GAS_GRIDNO 9216
// in both cases, direction 6
