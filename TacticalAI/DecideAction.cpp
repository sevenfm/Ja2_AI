#ifdef PRECOMPILEDHEADERS
#include "AI All.h"
#else
#include "ai.h"
#include "AIInternals.h"
#include "Isometric utils.h"
#include "Points.h"
#include "overhead.h"
#include "opplist.h"
#include "items.h"
#include "Weapons.h"
#include "NPC.h"
#include "Soldier Functions.h"
#include "worldman.h"
#include "Scheduling.h"
#include "Message.h"
#include "Structure Wrap.h"
#include "Keys.h"
#include "pathai.h"
#include "Render Fun.h"
#include "Boxing.h"
//	#include "Air Raid.h"
#include "Soldier Profile.h"
#include "soldier profile type.h"
#include "Soldier macros.h"
#include "los.h"
#include "Buildings.h"
#include "strategicmap.h"
#include "Quests.h"
#include "Map Screen Interface Map.h"
#include "soldier ani.h"
#include "rotting corpses.h"
#include "GameSettings.h"
#include "Dialogue Control.h"
// sevenfm
#include "Text.h"
#include "Game Clock.h"		
#include "Interface Panels.h"
#include "SmokeEffects.h"
#include "Drugs And Alcohol.h"
#include "SkillCheck.h"
#include "Explosion Control.h"
#include "Structure Wrap.h"
#include "Campaign.h"
#endif

#include "connect.h"

//////////////////////////////////////////////////////////////////////////////
// SANDRO - In this file, all APBPConstants[AP_CROUCH] and APBPConstants[AP_PRONE] were changed to GetAPsCrouch() and GetAPsProne()
//			On the bottom here, there are these functions made
//////////////////////////////////////////////////////////////////////

extern BOOLEAN gfHiddenInterrupt;
extern BOOLEAN gfUseAlternateQueenPosition;
extern UINT16 PickSoldierReadyAnimation( SOLDIERTYPE *pSoldier, BOOLEAN fEndReady, BOOLEAN fHipStance );
extern void IncrementWatchedLoc( UINT8 ubID, INT32 sGridNo, INT8 bLevel, BOOLEAN fCommunicate );
//extern void BeginMultiPurposeLocator( INT32 sGridNo, INT8 bLevel, BOOLEAN fSlideTo );

STR8 gStr8AlertStatus[] = {"Green", "Yellow", "Red", "Black"};
STR8 gStr8Attitude[] = {"DEFENSIVE", "BRAVESOLO", "BRAVEAID", "CUNNINGSOLO", "CUNNINGAID", "AGGRESSIVE", "MAXATTITUDES", "ATTACKSLAYONLY"};
STR8 gStr8Orders[] = {"STATIONARY", "ONGUARD", "CLOSEPATROL", "FARPATROL", "POINTPATROL", "ONCALL", "SEEKENEMY", "RNDPTPATROL", "SNIPER"};
STR8 gStr8Team[] = {"OUR_TEAM", "ENEMY_TEAM", "CREATURE_TEAM", "MILITIA_TEAM", "CIV_TEAM", "LAST_TEAM", "PLAYER_PLAN", "LAN_TEAM_ONE", "LAN_TEAM_TWO", "LAN_TEAM_THREE", "LAN_TEAM_FOUR"};
STR8 gStr8Class[] = {"SOLDIER_CLASS_NONE", "SOLDIER_CLASS_ADMINISTRATOR", "SOLDIER_CLASS_ELITE", "SOLDIER_CLASS_ARMY", "SOLDIER_CLASS_GREEN_MILITIA", "SOLDIER_CLASS_REG_MILITIA", "SOLDIER_CLASS_ELITE_MILITIA", "SOLDIER_CLASS_CREATURE", "SOLDIER_CLASS_MINER", "SOLDIER_CLASS_ZOMBIE"};
STR8 gStr8Knowledge[] = {"HEARD_3_TURNS_AGO", "HEARD_2_TURNS_AGO", "HEARD_LAST_TURN", "HEARD_THIS_TURN", "NOT_HEARD_OR_SEEN", "SEEN_CURRENTLY", "SEEN_THIS_TURN", "SEEN_LAST_TURN", "SEEN_2_TURNS_AGO", "SEEN_3_TURNS_AGO"};

extern STR szAction[];

// global status time counters to determine what takes the most time

#define CENTER_OF_RING 11237//dnl!!!

void DoneScheduleAction( SOLDIERTYPE * pSoldier )
{
	pSoldier->aiData.ubAIFlags &= (~AI_CHECK_SCHEDULE);
	pSoldier->bAIScheduleProgress = 0;
	PostNextSchedule( pSoldier );
}

// Flugente: dummies if we do not want to check for any conditions or taboos
BOOLEAN SoldierCondTrue(SOLDIERTYPE *pSoldier)			{ return TRUE; }
BOOLEAN SoldierCondFalse(SOLDIERTYPE *pSoldier)			{ return FALSE; }

INT8 DecideActionSchedule( SOLDIERTYPE * pSoldier )
{
	SCHEDULENODE *		pSchedule;
	INT32							iScheduleIndex;
	UINT8							ubScheduleAction;
	INT32 usGridNo1, usGridNo2;
	INT16							sX, sY;
	INT8							bDirection;
	STRUCTURE *				pStructure;
	BOOLEAN						fDoUseDoor;
	DOOR_STATUS	*			pDoorStatus;

	pSchedule = GetSchedule( pSoldier->ubScheduleID );
	if (!pSchedule)
	{
		return( AI_ACTION_NONE );
	}

	if (pSchedule->usFlags & SCHEDULE_FLAGS_ACTIVE1)
	{
		iScheduleIndex = 0;
	}
	else if (pSchedule->usFlags & SCHEDULE_FLAGS_ACTIVE2)
	{
		iScheduleIndex = 1;
	}
	else if (pSchedule->usFlags & SCHEDULE_FLAGS_ACTIVE3)
	{
		iScheduleIndex = 2;
	}
	else if (pSchedule->usFlags & SCHEDULE_FLAGS_ACTIVE4)
	{
		iScheduleIndex = 3;
	}
	else
	{
		// error!
		return( AI_ACTION_NONE );
	}

	ubScheduleAction = pSchedule->ubAction[ iScheduleIndex ];
	usGridNo1 = pSchedule->usData1[ iScheduleIndex ];
	usGridNo2 = pSchedule->usData2[ iScheduleIndex ];

	// assume soldier is awake unless the action is a sleep
	pSoldier->aiData.ubAIFlags &= ~(AI_ASLEEP);

	switch( ubScheduleAction )
	{
	case SCHEDULE_ACTION_LOCKDOOR:
		//Uses first gridno for locking door, then second to move to after door is locked.
		//It is possible that the second gridno will border the edge of the map, meaning that
		//the individual will walk off of the map.
		//If this is a "merchant", make sure that nobody occupies the building/room.

		switch( pSoldier->bAIScheduleProgress )
		{
		case 0: // move to gridno specified
			if (pSoldier->sGridNo == usGridNo1)
			{
				pSoldier->bAIScheduleProgress++;
				// fall through
			}
			else
			{
				pSoldier->aiData.usActionData = usGridNo1;
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}
			// fall through
		case 1:
			// start the door open: find the door...
			usGridNo1 = FindDoorAtGridNoOrAdjacent( usGridNo1 );

			if (TileIsOutOfBounds(usGridNo1))
			{
				// do nothing right now!
				return( AI_ACTION_NONE );
			}

			pDoorStatus = GetDoorStatus( usGridNo1 );
			if (pDoorStatus && pDoorStatus->ubFlags & DOOR_BUSY)
			{
				// do nothing right now!
				return( AI_ACTION_NONE );
			}

			pStructure = FindStructure( usGridNo1, STRUCTURE_ANYDOOR );
			if (pStructure == NULL)
			{
				fDoUseDoor = FALSE;
			}
			else
			{
				// action-specific tests to not handle the door
				fDoUseDoor = TRUE;

				if (pStructure->fFlags & STRUCTURE_OPEN)
				{
					// not only do we have to lock the door but
					// close it too!
					pSoldier->aiData.ubAIFlags |= AI_LOCK_DOOR_INCLUDES_CLOSE;
				}
				else
				{
					DOOR * pDoor;

					pDoor = FindDoorInfoAtGridNo( usGridNo1 );
					if (pDoor)
					{
						if (pDoor->fLocked)
						{
							// door already locked!
							fDoUseDoor = FALSE;
						}
						else
						{
							pDoor->fLocked = TRUE;
						}
					}
					else
					{
						ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Schedule involved locked door at %d but there's no lock there!", usGridNo1 );
						fDoUseDoor = FALSE;
					}
				}
			}

			if (fDoUseDoor)
			{
				pSoldier->aiData.usActionData = usGridNo1;
				return( AI_ACTION_LOCK_DOOR );
			}

			// the door is already in the desired state, or it doesn't exist!
			pSoldier->bAIScheduleProgress++;
			// fall through

		case 2:			
			if (pSoldier->sGridNo == usGridNo2 || TileIsOutOfBounds(pSoldier->sGridNo))
			{
				// NOWHERE indicates we were supposed to go off map and have done so
				DoneScheduleAction( pSoldier );

				if (!TileIsOutOfBounds(pSoldier->sGridNo))
				{
					pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
				}
			}
			else
			{
				if ( GridNoOnEdgeOfMap( usGridNo2, &bDirection ) )
				{
					// told to go to edge of map, so go off at that point!
					pSoldier->ubQuoteActionID = GetTraversalQuoteActionID( bDirection );
				}
				pSoldier->aiData.usActionData = usGridNo2;
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}
			break;
		}
		break;


	case SCHEDULE_ACTION_UNLOCKDOOR:
	case SCHEDULE_ACTION_OPENDOOR:
	case SCHEDULE_ACTION_CLOSEDOOR:
		//Uses first gridno for opening/closing/unlocking door, then second to move to after door is opened.
		//It is possible that the second gridno will border the edge of the map, meaning that
		//the individual will walk off of the map.
		switch( pSoldier->bAIScheduleProgress )
		{
		case 0: // move to gridno specified
			if (pSoldier->sGridNo == usGridNo1)
			{
				pSoldier->bAIScheduleProgress++;
				// fall through
			}
			else
			{
				pSoldier->aiData.usActionData = usGridNo1;
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}
			// fall through
		case 1:
			// start the door open: find the door...
			usGridNo1 = FindDoorAtGridNoOrAdjacent( usGridNo1 );

			if (TileIsOutOfBounds(usGridNo1))
			{
				// do nothing right now!
				return( AI_ACTION_NONE );
			}

			pDoorStatus = GetDoorStatus( usGridNo1 );
			if (pDoorStatus && pDoorStatus->ubFlags & DOOR_BUSY)
			{
				// do nothing right now!
				return( AI_ACTION_NONE );
			}

			pStructure = FindStructure( usGridNo1, STRUCTURE_ANYDOOR );
			if (pStructure == NULL)
			{
				fDoUseDoor = FALSE;
			}
			else
			{
				fDoUseDoor = TRUE;

				// action-specific tests to not handle the door
				switch( ubScheduleAction )
				{
				case SCHEDULE_ACTION_UNLOCKDOOR:
					if (pStructure->fFlags & STRUCTURE_OPEN)
					{
						// door is already open!
						fDoUseDoor = FALSE;
					}
					else
					{
						// set the door to unlocked
						DOOR * pDoor;

						pDoor = FindDoorInfoAtGridNo( usGridNo1 );
						if (pDoor)
						{
							if (pDoor->fLocked)
							{
								pDoor->fLocked = FALSE;
							}
							else
							{
								// door already unlocked!
								fDoUseDoor = FALSE;
							}
						}
						else
						{
							// WTF?  Warning time!
							ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Schedule involved locked door at %d but there's no lock there!", usGridNo1 );
							fDoUseDoor = FALSE;
						}
					}
					break;
				case SCHEDULE_ACTION_OPENDOOR:
					if (pStructure->fFlags & STRUCTURE_OPEN)
					{
						// door is already open!
						fDoUseDoor = FALSE;
					}
					break;
				case SCHEDULE_ACTION_CLOSEDOOR:
					if ( !(pStructure->fFlags & STRUCTURE_OPEN) )
					{
						// door is already closed!
						fDoUseDoor = FALSE;
					}
					break;
				default:
					break;
				}
			}

			if (fDoUseDoor)
			{
				pSoldier->aiData.usActionData = usGridNo1;
				if (ubScheduleAction == SCHEDULE_ACTION_UNLOCKDOOR)
				{
					return( AI_ACTION_UNLOCK_DOOR );
				}
				else
				{
					return( AI_ACTION_OPEN_OR_CLOSE_DOOR );
				}
			}

			// the door is already in the desired state, or it doesn't exist!
			pSoldier->bAIScheduleProgress++;
			// fall through

		case 2:			
			if (pSoldier->sGridNo == usGridNo2 || TileIsOutOfBounds(pSoldier->sGridNo))
			{
				// NOWHERE indicates we were supposed to go off map and have done so
				DoneScheduleAction( pSoldier );				
				if (!TileIsOutOfBounds(pSoldier->sGridNo))
				{
					pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
				}
			}
			else
			{
				if ( GridNoOnEdgeOfMap( usGridNo2, &bDirection ) )
				{
					// told to go to edge of map, so go off at that point!
					pSoldier->ubQuoteActionID = GetTraversalQuoteActionID( bDirection );
				}
				pSoldier->aiData.usActionData = usGridNo2;
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}
			break;
		}
		break;

	case SCHEDULE_ACTION_GRIDNO:
		// Only uses the first gridno
		if ( pSoldier->sGridNo == usGridNo1 )
		{
			// done!
			DoneScheduleAction( pSoldier );			
			if (!TileIsOutOfBounds(pSoldier->sGridNo))
			{
				pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
			}
		}
		else
		{
			// move!
			pSoldier->aiData.usActionData = usGridNo1;
			pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
			return( AI_ACTION_SCHEDULE_MOVE );
		}
		break;
	case SCHEDULE_ACTION_LEAVESECTOR:
		//Doesn't use any gridno data
		switch( pSoldier->bAIScheduleProgress )
		{
		case 0: // start the action

			pSoldier->aiData.usActionData = FindNearestEdgePoint( pSoldier->sGridNo );

			if (TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
#ifdef JA2BETAVERSION
				ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Civilian could not find path to map edge!" );
#endif
				DoneScheduleAction( pSoldier );
				return( AI_ACTION_NONE );
			}

			if ( pSoldier->sGridNo == pSoldier->aiData.usActionData )
			{
				// time to go off the map
				pSoldier->bAIScheduleProgress++;
			}
			else
			{
				// move!
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}

			// fall through

		case 1: // near edge

			pSoldier->aiData.usActionData = FindNearbyPointOnEdgeOfMap( pSoldier, &bDirection );

			if (TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				// what the heck??
				// ABORT!
				DoneScheduleAction( pSoldier );
			}
			else
			{
				pSoldier->ubQuoteActionID = GetTraversalQuoteActionID( bDirection );
				pSoldier->bAIScheduleProgress++;
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}
			break;

		case 2: // should now be done!
			DoneScheduleAction( pSoldier );
			break;

		default:
			break;
		}
		break;

	case SCHEDULE_ACTION_ENTERSECTOR:
		if ( pSoldier->ubProfile != NO_PROFILE && gMercProfiles[ pSoldier->ubProfile ].ubMiscFlags2 & PROFILE_MISC_FLAG2_DONT_ADD_TO_SECTOR )//Moa: changed 'ubMiscFlags' to 'ubMiscFlags2'
		{
			// ignore.
			DoneScheduleAction( pSoldier );
			break;
		}
		switch( pSoldier->bAIScheduleProgress )
		{
		case 0:
			sX = CenterX( pSoldier->sOffWorldGridNo );
			sY = CenterY( pSoldier->sOffWorldGridNo );
			pSoldier->EVENT_SetSoldierPosition( sX, sY );
			pSoldier->bInSector = TRUE;
			MoveSoldierFromAwayToMercSlot( pSoldier );
			pSoldier->aiData.usActionData = usGridNo1;
			pSoldier->bAIScheduleProgress++;
			pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
			return( AI_ACTION_SCHEDULE_MOVE );
		case 1:
			if (pSoldier->sGridNo == usGridNo1)
			{
				DoneScheduleAction( pSoldier );

				if (!TileIsOutOfBounds(pSoldier->sGridNo))
				{
					pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
				}
			}
			else
			{
				pSoldier->aiData.usActionData = usGridNo1;
				pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
				return( AI_ACTION_SCHEDULE_MOVE );
			}
			break;
		}
		break;

	case SCHEDULE_ACTION_WAKE:
		// Go to this position
		if (pSoldier->sGridNo == pSoldier->sInitialGridNo)
		{
			// th-th-th-that's it!
			DoneScheduleAction( pSoldier );
			pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
		}
		else
		{
			pSoldier->aiData.usActionData = pSoldier->sInitialGridNo;
			pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
			return( AI_ACTION_SCHEDULE_MOVE );
		}
		break;

	case SCHEDULE_ACTION_SLEEP:
		// Go to this position
		if (pSoldier->sGridNo == usGridNo1)
		{
			// Sleep
			pSoldier->aiData.ubAIFlags |= AI_ASLEEP;
			DoneScheduleAction( pSoldier );

			if (!TileIsOutOfBounds(pSoldier->sGridNo))
			{
				pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
			}
		}
		else
		{
			pSoldier->aiData.usActionData = usGridNo1;
			pSoldier->sAbsoluteFinalDestination = pSoldier->aiData.usActionData;
			return( AI_ACTION_SCHEDULE_MOVE );
		}
		break;
	}


	return( AI_ACTION_NONE );
}

INT8 DecideActionBoxerEnteringRing(SOLDIERTYPE *pSoldier)
{
	//DBrot: More Rooms
	//UINT8 ubRoom;
	UINT16 usRoom;
	INT32	sDesiredMercLoc;
	UINT8 ubDesiredMercDir;

	// boxer, should move into ring!
	if ( InARoom( pSoldier->sGridNo, &usRoom ))
	{
		if (usRoom == BOXING_RING)
		{
			// look towards nearest player
			sDesiredMercLoc = ClosestPC( pSoldier, NULL );

			if (!TileIsOutOfBounds(sDesiredMercLoc))
			{
				// see if we are facing this person
				ubDesiredMercDir = AIDirection(pSoldier->sGridNo, sDesiredMercLoc);

				// if not already facing in that direction,
				if ( pSoldier->ubDirection != ubDesiredMercDir && pSoldier->InternalIsValidStance( ubDesiredMercDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
				{
					pSoldier->aiData.usActionData = ubDesiredMercDir;
					return( AI_ACTION_CHANGE_FACING );
				}
			}
			return( AI_ACTION_ABSOLUTELY_NONE );
		}
		else
		{
			// move to starting spot
			pSoldier->aiData.usActionData = FindClosestBoxingRingSpot( pSoldier, TRUE );
			return( AI_ACTION_GET_CLOSER );
		}
	}

	return( AI_ACTION_ABSOLUTELY_NONE );
}

INT8 DecideActionNamedNPC( SOLDIERTYPE * pSoldier )
{
	INT32 sDesiredMercLoc;
	UINT8	ubDesiredMercDir;
	UINT8	ubDesiredMerc;
	INT32	sDesiredMercDist;

	// if a quote record has been set and we're not doing movement, then
	// it means we have to wait until someone is nearby and then see
	// to do...

	// is this person close enough to trigger event?
	if (pSoldier->ubQuoteRecord && pSoldier->ubQuoteActionID == QUOTE_ACTION_ID_TURNTOWARDSPLAYER )
	{
		sDesiredMercLoc = ClosestPC( pSoldier, &sDesiredMercDist );

		if (!TileIsOutOfBounds(sDesiredMercLoc))
		{
			if ( sDesiredMercDist <= NPC_TALK_RADIUS * 2)
			{
				pSoldier->ubQuoteRecord = 0;
				// see if this triggers a conversation/NPC record
				PCsNearNPC( pSoldier->ubProfile );
				// clear "handle every frame" flag
				pSoldier->aiData.ubAIFlags &= (~AI_HANDLE_EVERY_FRAME);
				return( AI_ACTION_ABSOLUTELY_NONE );
			}

			// see if we are facing this person
			ubDesiredMercDir = AIDirection(pSoldier->sGridNo, sDesiredMercLoc);

			// if not already facing in that direction,
			if (pSoldier->ubDirection != ubDesiredMercDir && pSoldier->InternalIsValidStance( ubDesiredMercDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
			{
				pSoldier->aiData.usActionData = ubDesiredMercDir;
				return( AI_ACTION_CHANGE_FACING );
			}
		}

		// do nothing; we're looking at the PC or the NPC is far away
		return( AI_ACTION_ABSOLUTELY_NONE );

	}
	else
	{
		///////////////
		// CHECK TO SEE IF WE WANT TO GO UP TO PERSON AND SAY SOMETHING
		///////////////
		pSoldier->aiData.usActionData = NPCConsiderInitiatingConv( pSoldier, &ubDesiredMerc );

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return( AI_ACTION_APPROACH_MERC );
		}
	}

	if ( pSoldier->IsAssassin() )
	{
		sDesiredMercLoc = ClosestPC( pSoldier, &sDesiredMercDist );

		if (!TileIsOutOfBounds(sDesiredMercLoc))
		{
			if ( sDesiredMercDist <= NPC_TALK_RADIUS * 2 )
			{
				AddToShouldBecomeHostileOrSayQuoteList( pSoldier->ubID );
				// now wait a bit!
				pSoldier->aiData.usActionData = 5000;
				return( AI_ACTION_WAIT );
			}
			else
			{
				pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards( pSoldier, sDesiredMercLoc, AI_ACTION_APPROACH_MERC );

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					return( AI_ACTION_APPROACH_MERC );
				}
			}
		}
	}

	return( AI_ACTION_NONE );
}


INT8 DecideActionGreen(SOLDIERTYPE *pSoldier)
{
	INT32 iChance, iSneaky = 10;
	INT8  bInWater, bInDeepWater, bInGas;

	// sevenfm
	BOOLEAN fClimb;
	INT32	sClosestFriend;
	INT32	sSmokeSpot = NOWHERE;
	INT8	bSmokeLevel = 0;

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DecideActionGreen, orders = %d",pSoldier->aiData.bOrders));
	
	DebugAI( AI_MSG_START, pSoldier, String("[Green]"));
	LogDecideInfo(pSoldier);

	// sevenfm: disable stealth mode
	pSoldier->bStealthMode = FALSE;
	// disable reverse movement mode
	pSoldier->bReverse = FALSE;
	// sevenfm: initialize data
	pSoldier->bWeaponMode = WM_NORMAL;
	// reset stop seek counter
	pSoldier->StopSeekReset();

	// sevenfm: stop right here if can't do anything
	if (pSoldier->bActionPoints <= 0 || pSoldier->IsUnconscious()) //Action points can be negative
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("No action points - nothing to do!"));
		pSoldier->aiData.usActionData = NOWHERE;
		return(AI_ACTION_NONE);
	}

	BOOLEAN fCivilian = pSoldier->IsCivilian();
	BOOLEAN fCivilianOrMilitia = PTR_CIV_OR_MILITIA;

	// sevenfm: stop dragging
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop dragging]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->IsDragging())
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop dragging"));
		return AI_ACTION_CANCEL_DRAG;
	}

	// sevenfm: chance to stop focusing
	if (pSoldier->CheckInitialAP() &&
		pSoldier->IsFocusing() &&
		pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] > 1 &&
		Chance(pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] * 10))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop focusing"));
		return AI_ACTION_STOP_FOCUS;
	}

	gubNPCPathCount = 0;

	if ( gTacticalStatus.bBoxingState != NOT_BOXING )
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("boxing is active"));

		if(pSoldier->IsBoxer())
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("soldier is boxer"));

			if ( gTacticalStatus.bBoxingState == PRE_BOXING )
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("pre boxing"));
				return( DecideActionBoxerEnteringRing( pSoldier ) );
			}
			else
			{
				UINT16 usRoom;
				UINT8 ubLoop;

				// boxer... but since in status green, it's time to leave the ring!
				if ( InARoom( pSoldier->sGridNo, &usRoom ))
				{
					if (usRoom == BOXING_RING)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("in a ring"));
						for ( ubLoop = 0; ubLoop < NUM_BOXERS; ++ubLoop )
						{
							if (pSoldier->ubID == gubBoxerID[ ubLoop ])
							{
								// we should go back where we started
								pSoldier->aiData.usActionData = gsBoxerGridNo[ ubLoop ];
								DebugAI(AI_MSG_INFO, pSoldier, String("return AI boxer to original location %d", gsBoxerGridNo[ubLoop]));
								return( AI_ACTION_GET_CLOSER );
							}
						}
						// find exit spot for player merc
						pSoldier->aiData.usActionData = FindClosestBoxingRingSpot( pSoldier, FALSE );
						DebugAI(AI_MSG_INFO, pSoldier, String("player boxer, found exit spot %d", pSoldier->aiData.usActionData));
						//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"exit spot %d", pSoldier->aiData.usActionData);
						//BeginMultiPurposeLocator(pSoldier->aiData.usActionData, 0, FALSE);

						return( AI_ACTION_GET_CLOSER );
					}
					else
					{
						// done!

						if (pSoldier->bTeam == gbPlayerNum || CountPeopleInBoxingRing() == 0)
						{
							TriggerEndOfBoxingRecord( pSoldier );
						}
					}
				}

				DebugAI(AI_MSG_INFO, pSoldier, String("boxer: nothing to do"));
				return( AI_ACTION_ABSOLUTELY_NONE );
			}
		}
		else if ( PythSpacesAway( pSoldier->sGridNo, CENTER_OF_RING ) <= MaxNormalDistanceVisible() )
		{
			UINT8 ubRingDir;
			// face ring!

			ubRingDir = AIDirection(pSoldier->sGridNo, CENTER_OF_RING);

			if (pSoldier->ubDirection != ubRingDir &&
				GetAPsToLook(pSoldier) <= pSoldier->bActionPoints &&
				pSoldier->InternalIsValidStance(ubRingDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
			{
				pSoldier->aiData.usActionData = ubRingDir;
				DebugAI(AI_MSG_INFO, pSoldier, String("not boxer: face the ring"));
				return( AI_ACTION_CHANGE_FACING );
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("not boxer: nothing to do"));
			return( AI_ACTION_NONE );
		}
	}
	else	// gTacticalStatus.bBoxingState == NOT_BOXING
	{
		if (!pSoldier->IsBoxer())
		{
			// sevenfm: sometimes boxers stay at ring even when boxing has ended, try to move them away
			UINT16 usRoom;
			UINT8 ubLoop;

			if (InARoom(pSoldier->sGridNo, &usRoom) && usRoom == BOXING_RING)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("found not boxer in a ring, move him out"));
				for (ubLoop = 0; ubLoop < NUM_BOXERS; ++ubLoop)
				{
					if (pSoldier->ubID == gubBoxerID[ubLoop])
					{
						// we should go back where we started
						pSoldier->aiData.usActionData = gsBoxerGridNo[ubLoop];
						DebugAI(AI_MSG_INFO, pSoldier, String("return AI boxer to original location %d", gsBoxerGridNo[ubLoop]));
						return(AI_ACTION_GET_CLOSER);
					}
				}
				// find exit spot for other soldiers
				pSoldier->aiData.usActionData = FindClosestBoxingRingSpot(pSoldier, FALSE);
				DebugAI(AI_MSG_INFO, pSoldier, String("not boxer, found exit spot %d", pSoldier->aiData.usActionData));
				//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"exit spot %d", pSoldier->aiData.usActionData);
				//BeginMultiPurposeLocator(pSoldier->aiData.usActionData, 0, FALSE);

				return(AI_ACTION_GET_CLOSER);
			}

			// WANNE: This should fix the bug if any merc are still under PC control. This could happen after boxing in SAN MONA.
			/*SOLDIERTYPE	*pTeamSoldier;
			for (INT8 bLoop = gTacticalStatus.Team[gbPlayerNum].bFirstID; bLoop <= gTacticalStatus.Team[gbPlayerNum].bLastID; bLoop++)
			{
				pTeamSoldier = MercPtrs[bLoop];

				if (pTeamSoldier->flags.uiStatusFlags & SOLDIER_PCUNDERAICONTROL)
					pTeamSoldier->flags.uiStatusFlags &= (~SOLDIER_PCUNDERAICONTROL);

				pTeamSoldier->DeleteBoxingFlag();
			}*/
		}		
	}

	bInDeepWater = DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel);
	bInWater = Water(pSoldier->sGridNo, pSoldier->pathing.bLevel);

	// check if standing in tear gas without a gas mask on, or in smoke
	bInGas = InGas( pSoldier, pSoldier->sGridNo );

	// sevenfm: chance to wear gas mask if soldier sees gas nearby
	if( SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		!pSoldier->WearsGasMask() && 
		FindClosestVisibleSmoke( pSoldier, sSmokeSpot, bSmokeLevel, TRUE, TACTICAL_RANGE / 2 ) &&
		Chance( 10 + SoldierDifficultyLevel(pSoldier) * 10 ) )
	{
		WearGasMaskIfAvailable(pSoldier);
	}

	// sevenfm: possibly remove gas mask
	if (SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		gbWorldSectorZ == 0 &&
		pSoldier->WearsGasMask() &&
		!InGas(pSoldier, pSoldier->sGridNo) &&
		!pSoldier->aiData.bUnderFire &&
		!FindClosestGas(pSoldier, sSmokeSpot, bSmokeLevel, TACTICAL_RANGE / 2) &&
		Chance(10 + SoldierDifficultyLevel(pSoldier) * 10))
	{
		RemoveGasMask(pSoldier);
	}

	// turn off flashlight if alert is raised
	if( //pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT &&
		IsActionAffordable(pSoldier, AI_ACTION_SWITCH_FLASHLIGHT) &&
		(!gfTurnBasedAI || pSoldier->bActionPoints == pSoldier->bInitialActionPoints) &&
		(!AIUseFlashlightCondition(pSoldier, pSoldier->ubDirection) || pSoldier->aiData.bAlertStatus >= STATUS_RED) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("turn off flashlight"));
		return AI_ACTION_SWITCH_FLASHLIGHT;
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN NOT IN WATER, DROP SCUBA FINS
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[when not in water, drop scuba fins]"));

	// when not in deep water, drop scuba fins
	if (pSoldier->CheckInitialAP() &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		pSoldier->inv[LEGPOS].exists() &&
		HasItemFlag(pSoldier->inv[LEGPOS].usItem, SCUBA_FINS) &&
		IsActionAffordable(pSoldier, AI_ACTION_DROP_ITEM))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found scuba fins in LEGPOS, drop to the ground"));
		//if (!AutoPlaceObject(pSoldier, &(pSoldier->inv[LEGPOS]), FALSE, LEGPOS))
		//RearrangePocket(pSoldier, HANDPOS, LEGPOS, FOREVER);

		pSoldier->inv[LEGPOS].fFlags |= OBJECT_AI_UNUSABLE;
		pSoldier->aiData.usActionData = LEGPOS;

		return AI_ACTION_DROP_ITEM;
	}

	// if real-time, and not in the way, do nothing 90% of the time (for GUARDS!)
	// unless in water (could've started there), then we better swim to shore!

	if (fCivilian || (gGameExternalOptions.fAllNamedNpcsDecideAction && pSoldier->ubProfile != NO_PROFILE))
	{
		// special stuff for civs

		if (pSoldier->flags.uiStatusFlags & SOLDIER_COWERING)
		{
			// everything's peaceful again, stop cowering!!
			pSoldier->aiData.usActionData = ANIM_STAND;
			DebugAI(AI_MSG_INFO, pSoldier, String("civilian: stop cowering"));
			return( AI_ACTION_STOP_COWERING );
		}

		if (!gfTurnBasedAI)
		{
			// ******************
			// REAL TIME NPC CODE
			// ******************
			if (pSoldier->aiData.ubAIFlags & AI_CHECK_SCHEDULE)
			{
				pSoldier->aiData.bAction = DecideActionSchedule( pSoldier );
				if (pSoldier->aiData.bAction != AI_ACTION_NONE)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("civilian: decide schedule"));
					return( pSoldier->aiData.bAction );
				}
			}

			if ( pSoldier->ubProfile != NO_PROFILE || pSoldier->IsAssassin() )
			{
				if (pSoldier->ubProfile != NO_PROFILE)
				{
					pSoldier->aiData.bAction = DecideActionNamedNPC(pSoldier);
				}
				else
				{
					INT32 sDesiredMercDist;
					INT32 sDesiredMercLoc = ClosestUnDisguisedPC( pSoldier, &sDesiredMercDist );

					if (!TileIsOutOfBounds(sDesiredMercLoc))
					{
						if ( sDesiredMercDist <= NPC_TALK_RADIUS * 2 )
						{
							AddToShouldBecomeHostileOrSayQuoteList( pSoldier->ubID );
							// now wait a bit!
							pSoldier->aiData.usActionData = 5000;
							pSoldier->aiData.bAction = AI_ACTION_WAIT;
						}
						else
						{
							pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards( pSoldier, sDesiredMercLoc, AI_ACTION_APPROACH_MERC );

							if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
							{
								pSoldier->aiData.bAction = AI_ACTION_APPROACH_MERC;
							}
						}
					}
				}

				if ( pSoldier->aiData.bAction != AI_ACTION_NONE )
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("civilian: decided action"));
					return( pSoldier->aiData.bAction );
				}
				// can we act again? not for a minute since we were last spoken to/triggered a record
				if ( pSoldier->uiTimeSinceLastSpoke && (GetJA2Clock() < pSoldier->uiTimeSinceLastSpoke + 60000) )
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("civilian: wait since last spoke"));
					return( AI_ACTION_NONE );
				}
				// turn off counter so we don't check it again
				pSoldier->uiTimeSinceLastSpoke = 0;
			}
		}

		// if not in the way, do nothing most of the time
		// unless in water (could've started there), then we better swim to shore!

		if (!(bInDeepWater) && PreRandom( 5 ) )
		{
			// don't do nuttin!
			DebugAI(AI_MSG_INFO, pSoldier, String("civilian: not in deep water, nothing to do"));
			return( AI_ACTION_NONE );
		}
	}

	////////////////////////////////////////////////////////////////////////
	// DRINK IF LOW IN BREATH
	////////////////////////////////////////////////////////////////////////

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[drink if low on breath]"));
	if (pSoldier->CheckInitialAP() &&
		!bInWater &&
		pSoldier->IsMercBodyType() &&
		(pSoldier->bBreath < pSoldier->bBreathMax * 3 / 4 || pSoldier->bBreath < OKBREATH) &&
		(Chance((100 - 100 * pSoldier->bBreath / pSoldier->bBreathMax) * (100 - 100 * pSoldier->bBreath / pSoldier->bBreathMax) / 100) || pSoldier->bBreath < OKBREATH) &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_USED_CANTEEN) &&
		IsActionAffordable(pSoldier, AI_ACTION_DRINK_CANTEEN) &&
		FindCanteen(pSoldier) != NO_SLOT)
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("drink from canteen"));
		return AI_ACTION_DRINK_CANTEEN;
	}

	////////////////////////////////////////////////////////////////////////
	// WAIT IF BEING BANDAGED
	////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Being bandaged]"));
	if( AICheckIsBandaged( pSoldier ) &&
		!bInWater &&
		!bInGas )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("ubServiceCount %d, wait", pSoldier->ubServiceCount));
		return AI_ACTION_NONE;
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Skills/Corpses/Bombs/Medics]"));

	if( !fCivilian &&
		pSoldier->bTeam == ENEMY_TEAM &&		
		SoldierAI(pSoldier) )
	{
		////////////////////////////////////////////////////////////////////////////
		// TURN TO CLOSEST COVERT OPPONENT
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[turn to covert]"));

		if (!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_COVERT) && 
			(gAnimControl[pSoldier->usAnimState].ubHeight != ANIM_PRONE || pSoldier->aiData.bAlertStatus >= STATUS_RED))
		{
			UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, FALSE );

			if( ubPerson != NOBODY &&
				CountTeamSeeSoldier(pSoldier->bTeam, MercPtrs[ubPerson]) == 0 &&
				Chance(25 + 75 * MercPtrs[ubPerson]->SuspicionPercent() / 100) &&
				LOS_Soldier(pSoldier, MercPtrs[ubPerson], LOS_NORMAL, CALC_FROM_ALL_DIRS) )
			{
				UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

				// and the opponent is close enough that he could possibly be seen
				if( pSoldier->ubDirection != ubOpponentDir &&
					pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
					pSoldier->bActionPoints >= GetAPsToLook(pSoldier)  )
				{
					// turn
					pSoldier->aiData.usActionData = ubOpponentDir;					

					// possibly raise weapon
					if( pSoldier->aiData.bAlertStatus >= STATUS_RED &&
						!WeaponReady(pSoldier) &&
						Chance(MercPtrs[ubPerson]->SuspicionPercent() / 2) &&
						PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
						(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding( pSoldier, TRUE ) < 30) &&
						GetAPsToLook(pSoldier) + GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) <= pSoldier->bActionPoints )
					{
						pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
						pSoldier->aiData.usNextActionData = 0;
						DebugAI( AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
					}
					
					// don't turn to covert again this turn
					pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_COVERT;

					DebugAI( AI_MSG_INFO, pSoldier, String("turn to closest covert opponent"));
					return(AI_ACTION_CHANGE_FACING);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF SECTOR IS JAMMED
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_INFO, pSoldier, String("check: raise alert if sector is jammed"));
		if (pSoldier->CheckInitialAP() &&
			pSoldier->CanUseRadio(TRUE) &&
			!pSoldier->IsJamming() &&
			SectorJammed() &&			
			!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_JAMMER]);
			DebugAI(AI_MSG_INFO, pSoldier, String("radio operator found jammer, raise alert"));
			return AI_ACTION_RED_ALERT;
		}

		////////////////////////////////////////////////////////////////////////////
		// TURN OFF RADIO IF ACTIVE
		////////////////////////////////////////////////////////////////////////////
		DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio if active"));
		if (pSoldier->CheckInitialAP() &&
			pSoldier->CanUseRadio(TRUE) &&
			pSoldier->IsUsingRadio() &&
			pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, TRUE))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio"));
			pSoldier->ubAISkillUse = SKILLS_RADIO_TURNOFF;
			pSoldier->aiData.usActionData = NOWHERE;
			return AI_ACTION_USE_SKILL;
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF PERCENT OF SOLDIERS KILLED
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_INFO, pSoldier, String("check: raise alert if certain percent of soldiers killed"));

		if( AIRoleOfficer(pSoldier) &&
			ArmyPercentKilled() > ArmyPercentKilledTolerance() &&
			!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition )
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_MANY_KILLED]);
			DebugAI(AI_MSG_INFO, pSoldier, String("officer found too many killed, raise alert"));
			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF FOUND CORPSE
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_INFO, pSoldier, String("check: raise alert if found fresh corpse"));

		if (!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
		{
			INT32				cnt;
			ROTTING_CORPSE *	pCorpse;

			for ( cnt = 0; cnt < giNumRottingCorpse; ++cnt )
			{
				pCorpse = &(gRottingCorpse[ cnt ] );

				if( pCorpse &&
					pCorpse->fActivated &&
					pCorpse->def.ubAIWarningValue > 0 &&
					pCorpse->def.ubType <= FMERC_FALLF &&
					!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
					CorpseEnemyTeam(pCorpse) &&
					LOS_Virtual(pSoldier, pCorpse->def.sGridNo, pCorpse->def.bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_WANTED_DIR))
				{
					ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_FOUND_DEAD_BODY]);
					// lower corpse warning value
					//pCorpse->def.ubAIWarningValue = 0;
					// also check locations nearby for hidden enemy (add noise at random spot)
					AddNoiseAtRandomSightCoverSpot( pSoldier, pCorpse->def.sGridNo, pCorpse->def.bLevel, TRUE, TRUE );
					DebugAI(AI_MSG_INFO, pSoldier, String("found rotting corpse, raise alert"));
					return( AI_ACTION_RED_ALERT );
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF FOUND BOMB
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[raise alert if found bomb]"));

		if (!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
			FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE))
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_BOMB], pSoldier->GetName());
			DebugAI(AI_MSG_INFO, pSoldier, String("found bomb, raise alert"));
			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF FOUND SMOKE
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[raise alert if found smoke]"));

		if (!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
			FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, FALSE) )
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_SMOKE]);

			// also check locations nearby for hidden enemy (add noise at random spot)
			AddNoiseAtRandomSightCoverSpot( pSoldier, sSmokeSpot, bSmokeLevel, TRUE, TRUE );
			DebugAI(AI_MSG_INFO, pSoldier, String("found smoke, raise alert"));
			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// IF YOU SEE CAPTURED FRIENDS, FREE THEM!
		////////////////////////////////////////////////////////////////////////////

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[captured friends]"));
		// Flugente: if we see one of our buddies in handcuffs, its a clear sign of enemy activity!
		if ( gGameExternalOptions.fAllowPrisonerSystem )
		{
			UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, VISION_RANGE, ENEMY_TEAM, SOLDIER_POW, TRUE );

			if( ubPerson != NOBODY &&
				!pSoldier->RaisedAlertThisTurn() &&
				!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition )
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("found handcuffed soldier %d at spot %d, raise alert", ubPerson, MercPtrs[ubPerson]->sGridNo));

				ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_HANDCUFF], MercPtrs[ubPerson]->GetName());
				// raise alarm!
				DebugAI(AI_MSG_INFO, pSoldier, String("found captured friends, raise alert"));
				return( AI_ACTION_RED_ALERT );
			}
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[drugged friends]"));

		UINT8 ubPerson = GetClosestSeenDruggedFriendID(pSoldier);

		if (ubPerson != NOBODY &&
			!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found drugged soldier %d at spot %d, raise alert", ubPerson, MercPtrs[ubPerson]->sGridNo));

			// raise alarm!
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_HANDCUFF], MercPtrs[ubPerson]->GetName());
			DebugAI(AI_MSG_INFO, pSoldier, String("found drugged friend, raise alert"));
			return(AI_ACTION_RED_ALERT);
		}

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[medic]"));
		if (DecideMedic(pSoldier))
			return pSoldier->aiData.bAction;		

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[inspect covert]"));
		// sevenfm: officer can come to inspect suspicious soldier
		if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SQUADLEADER_NT))
		{
			UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, TRUE );

			if( ubPerson != NOBODY &&
				pSoldier->pathing.bLevel == MercPtrs[ubPerson]->pathing.bLevel &&
				pSoldier->CanInspect( MercPtrs[ubPerson] ) &&
				pSoldier->RadioCallValue() == 0 &&
				Random(100) < MercPtrs[ubPerson]->SuspicionPercent() )
			{
				UINT8 ubFriendsNearby = CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4);
				UINT8 ubSoldierDifficulty = SoldierDifficultyLevel(pSoldier);				

				// come to investigate
				pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_NOISE, 0);

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					// sevenfm: call friends
					if (IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT) &&
						gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1 &&
						ubFriendsNearby < 1 + gGameOptions.fNewTraitSystem ? NUM_SKILL_TRAITS(pSoldier, SQUADLEADER_NT) : 0 &&
						Chance(10 * ubSoldierDifficulty))
					{
						// sevenfm: fake alert by showing radio animation
						pSoldier->aiData.bNextAction = AI_ACTION_SEEK_NOISE;
						pSoldier->aiData.usNextActionData = pSoldier->aiData.usActionData;
						DebugAI(AI_MSG_INFO, pSoldier, String("officer: found suspicious soldier, call for help"));
						return( AI_ACTION_FAKE_ALERT );
					}

					DebugAI(AI_MSG_INFO, pSoldier, String("officer: move to inspect suspicious soldier"));
					return(AI_ACTION_SEEK_NOISE);
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// USE FLASHLIGHT
	////////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Flashlight]"));

	// possibly turn on flashlight in peaceful situation
	if (pSoldier->CheckInitialAP() &&
		pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		pSoldier->ubProfile == NO_PROFILE &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT) &&
		IsActionAffordable(pSoldier, AI_ACTION_SWITCH_FLASHLIGHT) &&
		!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		pSoldier->aiData.bAlertStatus <= STATUS_YELLOW &&
		!AICheckNVG(pSoldier) &&
		AIUseFlashlightCondition(pSoldier, pSoldier->ubDirection) &&
		Chance(100 / (2 + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4))) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("use flashlight"));
		return AI_ACTION_SWITCH_FLASHLIGHT;
	}	

	////////////////////////////////////////////////////////////////////////////
	// POINT PATROL: move towards next point unless getting a bit winded
	////////////////////////////////////////////////////////////////////////////

	// this takes priority over water/gas checks, so that point patrol WILL work
	// from island to island, and through gas covered areas, too
	if (pSoldier->CheckInitialAP() &&
		pSoldier->aiData.bOrders == POINTPATROL && 
		pSoldier->bBreath >= 75)
	{
		if (PointPatrolAI(pSoldier))
		{
			if (!gfTurnBasedAI)
			{
				// wait after this...
				pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
				pSoldier->aiData.usNextActionData = RealtimeDelay( pSoldier );
			}
			DebugAI(AI_MSG_INFO, pSoldier, String("POINTPATROL: start point patrol"));
			return(AI_ACTION_POINT_PATROL);
		}
		else
		{
			// Reset path count to avoid deadlock
			gubNPCPathCount = 0;
		}
	}

	if (pSoldier->CheckInitialAP() &&
		pSoldier->aiData.bOrders == RNDPTPATROL && 
		pSoldier->bBreath >= 75)
	{
		if (RandomPointPatrolAI(pSoldier))
		{
			if (!gfTurnBasedAI)
			{
				// wait after this...
				pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
				pSoldier->aiData.usNextActionData = RealtimeDelay( pSoldier );
			}
			DebugAI(AI_MSG_INFO, pSoldier, String("RNDPTPATROL: start point patrol"));
			return(AI_ACTION_POINT_PATROL);
		}
		else
		{
			// Reset path count to avoid deadlock
			gubNPCPathCount = 0;
		}

	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN LEFT IN WATER OR GAS, GO TO NEAREST REACHABLE SPOT OF UNGASSED LAND
	////////////////////////////////////////////////////////////////////////////

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DecideActionGreen: get out of water and gas"));
	if (bInDeepWater || bInGas || FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE) || RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel))
	{
		pSoldier->aiData.usActionData = FindNearestUngassedLand(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("get out of dangerous place (water, gas, bomb, red smoke)"));
			return(AI_ACTION_LEAVE_WATER_GAS);
		}
	}

	////////////////////////////////////////////////////////////////////////
	// REST IF RUNNING OUT OF BREATH
	////////////////////////////////////////////////////////////////////////

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DecideActionGreen: rest if running out of breath"));
	// if our breath is running a bit low, and we're not in the way or in water
	if (pSoldier->CheckInitialAP() &&
		pSoldier->bBreath < 75 && 
		!bInDeepWater &&
		!bInGas &&
		(pSoldier->bBreath < OKBREATH * 2 || Chance(100 - pSoldier->bBreath)))
	{
		// take a breather for gods sake!
		// for realtime, AI will use a standard wait set outside of here
		pSoldier->aiData.usActionData = NOWHERE;
		DebugAI(AI_MSG_INFO, pSoldier, String("low breath, rest"));
		return(AI_ACTION_NONE);
	}

	////////////////////////////////////////////////////////////////////////
	// FAKE RADIO REPORTING
	////////////////////////////////////////////////////////////////////////
	
	if( pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->IsMercBodyType() &&
		pSoldier->RadioCallValue() == 0 &&
		!pSoldier->RaisedAlertThisTurn() &&
		(pSoldier->aiData.bOrders == STATIONARY || pSoldier->aiData.bOrders == POINTPATROL || pSoldier->aiData.bOrders == RNDPTPATROL) &&
		IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT) &&
		Chance(5) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("use fake alert to simulate radio communications"));

		if (fCivilianOrMilitia && !gfTurnBasedAI)
		{
			// pause after the action
			pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
			pSoldier->aiData.usNextActionData = (UINT16) REALTIME_CIV_AI_DELAY;
		}
		return AI_ACTION_FAKE_ALERT;
	}

	////////////////////////////////////////////////////////////////////////////
	// CLIMB A BUILDING
	////////////////////////////////////////////////////////////////////////////

	// sevenfm: soldier with long range scoped weapon may decide to climb on roof
	if( !fCivilian && 
		pSoldier->CheckInitialAP() &&
		pSoldier->aiData.bLastAction != AI_ACTION_CLIMB_ROOF && 
		!is_networked &&
		AIGunScoped(pSoldier) &&
		AIGunRange(pSoldier) >= DAY_VISION_RANGE &&
		pSoldier->pathing.bLevel == 0 &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->aiData.bOrders != SEEKENEMY &&
		pSoldier->aiData.bOrders != POINTPATROL &&
		pSoldier->aiData.bOrders != RNDPTPATROL &&
		!NightTime() &&
		CountNearbyFriendsOnRoof(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2) <= 1 )
	{
		iChance = 10 + pSoldier->aiData.bBypassToGreen;

		// reduce chance for any injury, less likely to wander around when hurt
		if( pSoldier->stats.bLifeMax > 0 )
			iChance = iChance * pSoldier->stats.bLife / pSoldier->stats.bLifeMax;

		// sevenfm: limit chance if low on breath
		iChance = iChance * pSoldier->bBreath / 100;

		// if there are some friends on roof already, lower chance
		UINT8 ubFriendsOnRoof = CountNearbyFriendsOnRoof(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE);
		if( ubFriendsOnRoof > 0 )
		{
			iChance = iChance / (ubFriendsOnRoof + 1);
		}

		// This is the chance that we want to be on the roof.  If already there, invert the chance to see if we want back down
		if (pSoldier->pathing.bLevel > 0)
		{
			iChance = 100 - iChance;
		}

		if ((INT16) PreRandom(100) < iChance)
		{
			if ( CanClimbFromHere ( pSoldier ) )
			{
				if (!gfTurnBasedAI)
				{
					// wait after this...
					pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
					pSoldier->aiData.usNextActionData = RealtimeDelay( pSoldier );
				}
				DebugAI(AI_MSG_INFO, pSoldier, String("scoped weapon, climb roof"));
				return( AI_ACTION_CLIMB_ROOF );
			}
			else
			{
				pSoldier->aiData.usActionData = FindClosestClimbPoint(pSoldier, TRUE);

				// Added the check here because sniper militia who are locked inside of a building without keys
				// will still have a >100% chance to want to climb, which means an infinite loop.  In fact, any
				// time a move is desired, there probably also will be a need to check for a path.				

				// sevenfm: removed path check as FindClosestClimbPoint now checks it
				if ( !TileIsOutOfBounds(pSoldier->aiData.usActionData) )
					//LegalNPCDestination(pSoldier,pSoldier->aiData.usActionData,ENSURE_PATH,WATEROK, 0 ))
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("scoped weapon, decided to climb up at %d", pSoldier->aiData.usActionData));	
					return( AI_ACTION_MOVE_TO_CLIMB  );
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// RANDOM PATROL:  determine % chance to start a new patrol route
	////////////////////////////////////////////////////////////////////////////
	if (!gubNPCPathCount && pSoldier->CheckInitialAP()) // try to limit pathing in Green AI
	{
		iChance = 25 + pSoldier->aiData.bBypassToGreen;

		// set base chance according to orders
		switch (pSoldier->aiData.bOrders)
		{
		case STATIONARY:     iChance += -20;  break;
		case ONGUARD:        iChance += -15;  break;
		case ONCALL:                          break;
		case CLOSEPATROL:    iChance += +15;  break;
		case RNDPTPATROL:
		case POINTPATROL:		iChance = 0; break;
		case FARPATROL:      iChance += +25;  break;
		case SEEKENEMY:      iChance += -10;  break;
		}

		// modify chance of patrol (and whether it's a sneaky one) by attitude
		switch (pSoldier->aiData.bAttitude)
		{
		case DEFENSIVE:      iChance += -10;                 break;
		case BRAVESOLO:      iChance +=   5;                 break;
		case BRAVEAID:                                       break;
		case CUNNINGSOLO:    iChance +=   5;  iSneaky += 10; break;
		case CUNNINGAID:                      iSneaky +=  5; break;
		case AGGRESSIVE:     iChance +=  10;  iSneaky += -5; break;
		case ATTACKSLAYONLY: iChance +=  10;  iSneaky += -5; break;
		}

		// reduce chance for any injury, less likely to wander around when hurt
		if( pSoldier->stats.bLifeMax > 0 )
			iChance = iChance * pSoldier->stats.bLife / pSoldier->stats.bLifeMax;

		// sevenfm: limit chance if low on breath
		iChance = iChance * pSoldier->bBreath / 100;

		// sevenfm: stationary should not patrol
		if (pSoldier->aiData.bOrders == STATIONARY)
		{
			iChance = 0;
		}

		// if we're in water with land miles (> 25 tiles) away,
		// OR if we roll under the chance calculated
		if (bInDeepWater || ((INT16) PreRandom(100) < iChance))
		{
			pSoldier->aiData.usActionData = RandDestWithinRange(pSoldier);

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards( pSoldier, pSoldier->aiData.usActionData, AI_ACTION_RANDOM_PATROL );
			}

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				if (!gfTurnBasedAI)
				{
					// wait after this...
					pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
					pSoldier->aiData.usNextActionData = RealtimeDelay( pSoldier );
				}
				DebugAI(AI_MSG_INFO, pSoldier, String("soldier decided to start random patrol"));
				return(AI_ACTION_RANDOM_PATROL);
			}
		}
	}

	// sevenfm: help friends
	if (!gubNPCPathCount && pSoldier->CheckInitialAP()) // try to limit pathing in Green AI
	{
		////////////////////////////////////////////////////////////////////////////
		// SEEK FRIEND WHO LAST RADIOED IN TO REPORT NOISE
		////////////////////////////////////////////////////////////////////////////

		sClosestFriend = ClosestReachableFriendInTrouble(pSoldier, &fClimb);

		DebugAI( AI_MSG_INFO, pSoldier, String("sClosestFriend %d", sClosestFriend));

		// if there is a friend alive & reachable who last radioed in		
		if (!TileIsOutOfBounds(sClosestFriend))
		{
			// there a chance enemy soldier choose to go "help" his friend
			iChance = 80 - PythSpacesAway(pSoldier->sGridNo, sClosestFriend);
			iSneaky = 10;

			// set base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY:     iChance += -20;  break;
			case ONGUARD:        iChance += -15;  break;
			case ONCALL:         iChance +=  20;  break;
			case CLOSEPATROL:    iChance += -10;  break;
			case RNDPTPATROL:
			case POINTPATROL:    iChance += -10;  break;
			case FARPATROL:                       break;
			case SEEKENEMY:      iChance +=  10;  break;
			}

			// modify chance of patrol (and whether it's a sneaky one) by attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:      iChance += -10;  iSneaky +=  15;        break;
			case BRAVESOLO:                                              break;
			case BRAVEAID:       iChance +=  20;  iSneaky += -10;        break;
			case CUNNINGSOLO:					   iSneaky +=  30;		  break;
			case CUNNINGAID:     iChance +=  20;  iSneaky +=  20;        break;
			case AGGRESSIVE:     iChance += -20;  iSneaky += -20;        break;
			case ATTACKSLAYONLY: iChance += -20;  iSneaky += -20;        break;
			}

			// reduce chance if breath is down, less likely to wander around when tired
			iChance -= (100 - pSoldier->bBreath);

			// sevenfm: stationary/snipers should not help
			if (pSoldier->aiData.bOrders == STATIONARY)
			{
				iChance = 0;
			}

			DebugAI( AI_MSG_INFO, pSoldier, String("chance to move to closest friend %d", iChance));

			if ((INT16)PreRandom(100) < iChance)
			{
				pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards(pSoldier,sClosestFriend,AI_ACTION_SEEK_FRIEND);

				DebugAI( AI_MSG_INFO, pSoldier, String("found move spot to closest friend %d", pSoldier->aiData.usActionData));

				// sevenfm: avoid going too close to known bombs
				if (FindBombNearby(pSoldier, pSoldier->aiData.usActionData, BOMB_DETECTION_RANGE))
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("found bomb nearby! abort!"));
					pSoldier->aiData.usActionData = NOWHERE;
				}

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					if ( fClimb )
					{
						// need to climb AND have enough APs to get there this turn
						DebugAI( AI_MSG_INFO, pSoldier, String("need to climb"));

						// 0verhaul:  Closest Friend call also returns the climb point if climbing is necessary.  So don't
						// climb the wrong building and don't search again
						if (pSoldier->sGridNo == sClosestFriend)
						{
							if (IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF) )
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("climb here"));
								return( AI_ACTION_CLIMB_ROOF );
							}
						}
						else
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("move to climb spot %d", sClosestFriend));
							pSoldier->aiData.usActionData = sClosestFriend;
							return( AI_ACTION_MOVE_TO_CLIMB  );
						}
					}
					else
					{
						DebugAI( AI_MSG_INFO, pSoldier, String("move to friend"));
						//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] moves to help at %d", pSoldier->ubID, sClosestFriend);
						//BeginMultiPurposeLocator(sClosestFriend, 0, FALSE);

						if (fCivilianOrMilitia && !gfTurnBasedAI)
						{
							// pause at the end of the walk!
							pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
							pSoldier->aiData.usNextActionData = (UINT16) REALTIME_CIV_AI_DELAY;
						}

						return(AI_ACTION_SEEK_FRIEND);
					}					
				}
			}
		}
	}

	if (!gubNPCPathCount && pSoldier->CheckInitialAP()) // try to limit pathing in Green AI
	{
		////////////////////////////////////////////////////////////////////////////
		// SEEK FRIEND: determine %chance for man to pay a friendly visit
		////////////////////////////////////////////////////////////////////////////

		iChance = 25 + pSoldier->aiData.bBypassToGreen;

		// set base chance and maximum seeking distance according to orders
		switch (pSoldier->aiData.bOrders)
		{
		case STATIONARY:     iChance += -20; break;
		case ONGUARD:        iChance += -15; break;
		case ONCALL:                         break;
		case CLOSEPATROL:    iChance += +10; break;
		case RNDPTPATROL:
		case POINTPATROL:    iChance  = -10; break;
		case FARPATROL:      iChance += +20; break;
		case SEEKENEMY:      iChance += -10; break;
		}

		// modify for attitude
		switch (pSoldier->aiData.bAttitude)
		{
		case DEFENSIVE:                       break;
		case BRAVESOLO:      iChance /= 2;    break;  // loners
		case BRAVEAID:       iChance += 10;   break;  // friendly
		case CUNNINGSOLO:    iChance /= 2;    break;  // loners
		case CUNNINGAID:     iChance += 10;   break;  // friendly
		case AGGRESSIVE:                      break;
		case ATTACKSLAYONLY:									 break;
		}

		// reduce chance for any injury, less likely to wander around when hurt
		if( pSoldier->stats.bLifeMax > 0 )
			iChance = iChance * pSoldier->stats.bLife / pSoldier->stats.bLifeMax;

		// sevenfm: limit chance if low on breath
		iChance = iChance * pSoldier->bBreath / 100;

		// sevenfm: stationary/snipers should not visit friends
		if (pSoldier->aiData.bOrders == STATIONARY)
		{
			iChance = 0;
		}

		if ((INT16) PreRandom(100) < iChance)
		{
			if (RandomFriendWithin(pSoldier))
			{
				if ( pSoldier->aiData.usActionData == GoAsFarAsPossibleTowards( pSoldier, pSoldier->aiData.usActionData, AI_ACTION_SEEK_FRIEND ) )
				{
					if (fCivilianOrMilitia && !gfTurnBasedAI)
					{
						// pause at the end of the walk!
						pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
						pSoldier->aiData.usNextActionData = (UINT16) REALTIME_CIV_AI_DELAY;
					}
					DebugAI(AI_MSG_INFO, pSoldier, String("soldier decided to visit friend"));
					return(AI_ACTION_SEEK_FRIEND);
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// SNIPERS LIKE TO CROUCH (on roofs)
	////////////////////////////////////////////////////////////////////////////

	// if not in water and not already crouched, try to crouch down first
	if (pSoldier->aiData.bOrders == STATIONARY &&
		(AIRoleSniper(pSoldier) || pSoldier->aiData.bOrders == STATIONARY && (pSoldier->pathing.bLevel > 0 || InARoom(pSoldier->sGridNo, NULL))) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->ubProfile == NO_PROFILE &&
		!PTR_CROUCHED &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		IsValidStance( pSoldier, ANIM_CROUCH ) &&
		//pSoldier->pathing.bLevel > 0 &&
		GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
	{
		pSoldier->aiData.usActionData = ANIM_CROUCH;
		DebugAI(AI_MSG_INFO, pSoldier, String("sniper decided to crouch"));
		return(AI_ACTION_CHANGE_STANCE);
	}

	// if not in water and not already prone, try to go prone first
	if (pSoldier->CheckInitialAP() &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->ubProfile == NO_PROFILE &&
		AIRoleSniper(pSoldier) &&
		!PTR_PRONE &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		IsValidStance(pSoldier, ANIM_PRONE) &&
		pSoldier->pathing.bLevel > 0 &&
		GetAPsToChangeStance(pSoldier, ANIM_PRONE) <= pSoldier->bActionPoints)
	{
		pSoldier->aiData.usActionData = ANIM_PRONE;
		DebugAI(AI_MSG_INFO, pSoldier, String("sniper decided to go prone"));
		return(AI_ACTION_CHANGE_STANCE);
	}

	////////////////////////////////////////////////////////////////////////////
	// SNIPER - RAISE WEAPON TO SCAN AREA
	////////////////////////////////////////////////////////////////////////////

	if (pSoldier->CheckInitialAP() &&
		SoldierAI(pSoldier) &&
		!WeaponReady(pSoldier) &&
		AIGunScoped(pSoldier) &&
		PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
		(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30 ) &&
		GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)) <= pSoldier->bActionPoints)
	{
		iChance = 25;

		if (pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE)
			iChance += 15;
		if (pSoldier->ubSoldierClass == SOLDIER_CLASS_GREEN_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR)
			iChance -= 15;
		if (AIRoleSniper(pSoldier))
			iChance += 25;
		if (pSoldier->pathing.bLevel > 0)
			iChance += 25;

		if(Chance(iChance))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("sniper decided to raise weapon"));
			return(AI_ACTION_RAISE_GUN);
		}
	}

	// sevenfm: possibly use focus to scan area
	if ((gfTurnBasedAI || pSoldier->CheckInitialAP()) &&
		SoldierAI(pSoldier) &&
		WeaponReady(pSoldier) &&
		AIGunInHandScoped(pSoldier) &&
		pSoldier->bActionPoints >= APBPConstants[AP_SPOTTER] &&
		!pSoldier->IsFocusing() &&
		pSoldier->CanUseSkill(SKILLS_FOCUS, FALSE) &&
		(AIRoleSniper(pSoldier) || Chance(10 * SoldierDifficultyLevel(pSoldier))))
	{
		INT32 sFocusSpot;
		INT8 bFocusLevel;

		if (FindRandomFocusSpot(pSoldier, sFocusSpot, bFocusLevel, Chance(75)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start focusing at location: spot %d level %d", sFocusSpot, bFocusLevel));

			pSoldier->ubAISkillUse = SKILLS_FOCUS;
			pSoldier->aiData.usActionData = sFocusSpot;
			pSoldier->bTargetLevel = bFocusLevel;

			if (!gfTurnBasedAI)
			{
				pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
				pSoldier->aiData.usNextActionData = RealtimeDelay(pSoldier);
			}

			return AI_ACTION_USE_SKILL;
		}
	}

	if (pSoldier->CheckInitialAP() &&
		WeaponReady(pSoldier) &&
		PickSoldierReadyAnimation(pSoldier, TRUE, FALSE) != INVALID_ANIMATION)
	{
		iChance = (100 - pSoldier->bBreath) / 2;
		// is it a heavy gun? And we have energy cost for shooting enabled? 
		iChance += GetBPCostPer10APsForGunHolding(pSoldier); // don't overexaggerate yourself
		if (pSoldier->pathing.bLevel > 0)
			iChance /= 2;
		if (AIGunScoped(pSoldier))
			iChance /= 2;
		if (AIRoleSniper(pSoldier))
			iChance /= 2;

		if (Chance(iChance))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("decided to lower weapon"));
			return(AI_ACTION_LOWER_GUN);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// LOOK AROUND: determine %chance for man to turn in place
	////////////////////////////////////////////////////////////////////////////

	DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DecideActionGreen: Soldier deciding to turn"));
	if( pSoldier->CheckInitialAP() &&
		GetAPsToLook( pSoldier ) <= pSoldier->bActionPoints )
	{
		// avoid 2 consecutive random turns in a row
		if (pSoldier->aiData.bLastAction != AI_ACTION_CHANGE_FACING)
		{
			iChance = 25 + pSoldier->aiData.bBypassToGreen;

			// set base chance according to orders
			if (pSoldier->aiData.bOrders == STATIONARY)
				iChance += 25;

			if (pSoldier->aiData.bOrders == ONGUARD)
				iChance += 20;

			if (pSoldier->aiData.bAttitude == DEFENSIVE)
				iChance += 25;

			if ( pSoldier->pathing.bLevel > 0)
				iChance += 25;

			// SANDRO - if readied weapon, make him more likely to turn around
			if ( WeaponReady(pSoldier) && !PTR_PRONE)
				iChance += 25;

			if (PTR_PRONE)
				iChance /= 2;

			if (Chance(iChance))
			{
				// roll random directions (stored in actionData) until different from current
				do
				{
					// if man has a LEGAL dominant facing, and isn't facing it, he will turn
					// back towards that facing 50% of the time here (normally just enemies)
					if ((pSoldier->aiData.bDominantDir >= 0) && (pSoldier->aiData.bDominantDir <= 8) &&
						(pSoldier->ubDirection != pSoldier->aiData.bDominantDir) && PreRandom(2))
					{
						pSoldier->aiData.usActionData = pSoldier->aiData.bDominantDir;
					}
					else
					{
						INT32 iNoiseValue;
						BOOLEAN fClimb;
						BOOLEAN fReachable;
						INT32 sNoiseGridNo = MostImportantNoiseHeard(pSoldier,&iNoiseValue, &fClimb, &fReachable);

						if (TileIsOutOfBounds(sNoiseGridNo) || pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, sNoiseGridNo))
						{
							pSoldier->aiData.usActionData = PreRandom(8);
						}
						else
						{
							pSoldier->aiData.usActionData = AIDirection(pSoldier->sGridNo, sNoiseGridNo);
						}
					}
				} while (pSoldier->aiData.usActionData == pSoldier->ubDirection);

				DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DecideActionGreen: Trying to turn - checking stance validity"));
				if ( pSoldier->InternalIsValidStance( (INT8) pSoldier->aiData.usActionData, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
				{

					if ( !gfTurnBasedAI )
					{
						// wait after this...
						pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
						pSoldier->aiData.usNextActionData = RealtimeDelay( pSoldier );
					}

					DebugAI(AI_MSG_INFO, pSoldier, String("decided to turn"));
					return(AI_ACTION_CHANGE_FACING);
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// NONE:
	////////////////////////////////////////////////////////////////////////////

	// by default, if everything else fails, just stands in place without turning
	// for realtime, regular AI guys will use a standard wait set outside of here
	pSoldier->aiData.usActionData = NOWHERE;
	DebugAI(AI_MSG_INFO, pSoldier, String("end of Green AI: absolutely nothing to do"));
	return(AI_ACTION_NONE);
}

INT8 DecideActionYellow(SOLDIERTYPE *pSoldier)
{
	INT32 iDummy;
	UINT8 ubNoiseDir;
	INT32 sNoiseGridNo;
	INT32 iNoiseValue;
	INT32 iChance, iSneaky;
	INT32 sClosestFriend;
	BOOLEAN fCivilian = pSoldier->IsCivilian();
	BOOLEAN fClimb;
	BOOLEAN fReachable;

	INT8  bInWater, bInDeepWater, bInGas;

	// sevenfm
	INT32	sSmokeSpot = NOWHERE;
	INT8	bSmokeLevel = 0;

	DebugAI( AI_MSG_START, pSoldier, String("[Yellow]"));
	LogDecideInfo(pSoldier);

	// sevenfm: disable stealth mode
	pSoldier->bStealthMode = FALSE;
	// disable reverse movement mode
	pSoldier->bReverse = FALSE;
	// sevenfm: initialize data
	pSoldier->bWeaponMode = WM_NORMAL;
	// reset stop seek counter
	pSoldier->StopSeekReset();

	// sevenfm: stop right here if can't do anything
	if (pSoldier->bActionPoints <= 0 || pSoldier->IsUnconscious()) //Action points can be negative
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("No action points - nothing to do!"));
		pSoldier->aiData.usActionData = NOWHERE;
		return(AI_ACTION_NONE);
	}

	// prepare data
	bInWater = Water(pSoldier->sGridNo, pSoldier->pathing.bLevel);
	bInDeepWater = DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel);
	bInGas = InGas( pSoldier, pSoldier->sGridNo );	
	
	// sevenfm: chance to wear gas mask if soldier sees gas nearby
	if( SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		pSoldier->HasGasMask() &&
		!pSoldier->WearsGasMask() && 
		FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, TRUE, TACTICAL_RANGE / 2) &&
		Chance( 10 + SoldierDifficultyLevel(pSoldier) * 10 ) )
	{
		WearGasMaskIfAvailable(pSoldier);
	}

	// sevenfm: possibly remove gas mask
	if (SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		gbWorldSectorZ == 0 &&
		pSoldier->WearsGasMask() &&
		!InGas(pSoldier, pSoldier->sGridNo) &&
		!pSoldier->aiData.bUnderFire &&
		!FindClosestGas(pSoldier, sSmokeSpot, bSmokeLevel, TACTICAL_RANGE / 2) &&
		Chance(10 + SoldierDifficultyLevel(pSoldier) * 10))
	{
		RemoveGasMask(pSoldier);
	}

	// turn off flashlight if alert is raised
	if( //pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT &&
		IsActionAffordable(pSoldier, AI_ACTION_SWITCH_FLASHLIGHT) &&
		(!gfTurnBasedAI || pSoldier->bActionPoints == pSoldier->bInitialActionPoints) &&
		(!AIUseFlashlightCondition(pSoldier, pSoldier->ubDirection) || pSoldier->aiData.bAlertStatus >= STATUS_RED) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("turn off flashlight"));
		return AI_ACTION_SWITCH_FLASHLIGHT;
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN NOT IN WATER, DROP SCUBA FINS
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[when not in water, drop scuba fins]"));

	// when not in deep water, drop scuba fins
	if (pSoldier->CheckInitialAP() &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		pSoldier->inv[LEGPOS].exists() &&
		HasItemFlag(pSoldier->inv[LEGPOS].usItem, SCUBA_FINS) &&
		IsActionAffordable(pSoldier, AI_ACTION_DROP_ITEM))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found scuba fins in LEGPOS, drop to the ground"));
		//if (!AutoPlaceObject(pSoldier, &(pSoldier->inv[LEGPOS]), FALSE, LEGPOS))
		//RearrangePocket(pSoldier, HANDPOS, LEGPOS, FOREVER);

		pSoldier->inv[LEGPOS].fFlags |= OBJECT_AI_UNUSABLE;
		pSoldier->aiData.usActionData = LEGPOS;

		return AI_ACTION_DROP_ITEM;
	}

	// sevenfm: stop dragging
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop dragging]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->IsDragging())
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop dragging"));
		return AI_ACTION_CANCEL_DRAG;
	}

	// sevenfm: chance to stop focusing
	if (pSoldier->CheckInitialAP() &&
		pSoldier->IsFocusing() &&
		pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] > 1 &&
		Chance(pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] * 10))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop focusing"));
		return AI_ACTION_STOP_FOCUS;
	}

	if (fCivilian || (gGameExternalOptions.fAllNamedNpcsDecideAction && pSoldier->ubProfile != NO_PROFILE))
	{
		if (pSoldier->flags.uiStatusFlags & SOLDIER_COWERING)
		{
			// everything's peaceful again, stop cowering!!
			pSoldier->aiData.usActionData = ANIM_STAND;
			return( AI_ACTION_STOP_COWERING );
		}
		if (!gfTurnBasedAI)
		{
			// ******************
			// REAL TIME NPC CODE
			// ******************
			if (pSoldier->ubProfile != NO_PROFILE || pSoldier->IsAssassin() )
			{
				if ( pSoldier->ubProfile != NO_PROFILE )
					pSoldier->aiData.bAction = DecideActionNamedNPC( pSoldier );
				else
				{
					INT32 sDesiredMercDist;
					INT32 sDesiredMercLoc = ClosestUnDisguisedPC( pSoldier, &sDesiredMercDist );

					// Flugente: if this guy is disguised, do not consider him

					if (!TileIsOutOfBounds(sDesiredMercLoc))
					{
						if ( sDesiredMercDist <= NPC_TALK_RADIUS * 2 )
						{
							AddToShouldBecomeHostileOrSayQuoteList( pSoldier->ubID );
							// now wait a bit!
							pSoldier->aiData.usActionData = 5000;
							pSoldier->aiData.bAction = AI_ACTION_WAIT;
						}
						else
						{
							pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards( pSoldier, sDesiredMercLoc, AI_ACTION_APPROACH_MERC );

							if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
							{
								pSoldier->aiData.bAction = AI_ACTION_APPROACH_MERC;
							}
						}
					}
				}

				if ( pSoldier->aiData.bAction != AI_ACTION_NONE )
				{
					return( pSoldier->aiData.bAction );
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN LEFT IN WATER OR GAS, GO TO NEAREST REACHABLE SPOT OF UNGASSED LAND
	////////////////////////////////////////////////////////////////////////////

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("DecideActionYellow: get out of water and gas"));

	if (bInDeepWater || bInGas || FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE) || RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel))
	{
		pSoldier->aiData.usActionData = FindNearestUngassedLand(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_LEAVE_WATER_GAS);
		}
	}

	////////////////////////////////////////////////////////////////////////
	// DRINK IF LOW IN BREATH
	////////////////////////////////////////////////////////////////////////

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[drink if low on breath]"));
	if (pSoldier->CheckInitialAP() &&
		!bInWater &&
		pSoldier->IsMercBodyType() &&
		(pSoldier->bBreath < pSoldier->bBreathMax * 3 / 4 || pSoldier->bBreath < OKBREATH) &&
		(Chance((100 - 100 * pSoldier->bBreath / pSoldier->bBreathMax) * (100 - 100 * pSoldier->bBreath / pSoldier->bBreathMax) / 100) || pSoldier->bBreath < OKBREATH) &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_USED_CANTEEN) &&
		IsActionAffordable(pSoldier, AI_ACTION_DRINK_CANTEEN) &&
		FindCanteen(pSoldier) != NO_SLOT)
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("drink from canteen"));
		return AI_ACTION_DRINK_CANTEEN;
	}

	////////////////////////////////////////////////////////////////////////
	// REST IF RUNNING OUT OF BREATH
	////////////////////////////////////////////////////////////////////////

	// if our breath is running a bit low, and we're not in water
	if (pSoldier->CheckInitialAP() &&
		pSoldier->bBreath < OKBREATH * 2 &&
		!pSoldier->MercInWater())
	{
		// take a breather for gods sake!
		pSoldier->aiData.usActionData = NOWHERE;

		// is it a heavy gun? And we have energy cost for shooting enabled? 
		if (WeaponReady(pSoldier) &&
			GetBPCostPer10APsForGunHolding(pSoldier) > 0 &&
			PickSoldierReadyAnimation(pSoldier, TRUE, FALSE) != INVALID_ANIMATION)
		{
			// unready
			return(AI_ACTION_LOWER_GUN);
		}

		return(AI_ACTION_NONE);
	}

	////////////////////////////////////////////////////////////////////////
	// WAIT IF BEING BANDAGED
	////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Being bandaged]"));
	if( AICheckIsBandaged( pSoldier ) &&
		!bInWater &&
		!bInGas )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("ubServiceCount %d, wait", pSoldier->ubServiceCount));
		return AI_ACTION_NONE;
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Skills/Corpses/Bombs/Medics]"));

	if( !fCivilian &&
		pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) )
	{
		////////////////////////////////////////////////////////////////////////////
		// TURN TO CLOSEST COVERT OPPONENT
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[turn to covert]"));

		if (!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_COVERT) && 
			(gAnimControl[pSoldier->usAnimState].ubHeight != ANIM_PRONE || pSoldier->aiData.bAlertStatus >= STATUS_RED))
		{
			UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, FALSE );

			if( ubPerson != NOBODY &&
				CountTeamSeeSoldier(pSoldier->bTeam, MercPtrs[ubPerson]) == 0 &&
				Chance(25 + 75 * MercPtrs[ubPerson]->SuspicionPercent() / 100) &&
				LOS_Soldier(pSoldier, MercPtrs[ubPerson], LOS_NORMAL, CALC_FROM_ALL_DIRS) )
			{
				UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

				// and the opponent is close enough that he could possibly be seen
				if( pSoldier->ubDirection != ubOpponentDir &&
					pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
					pSoldier->bActionPoints >= GetAPsToLook(pSoldier)  )
				{
					// turn
					pSoldier->aiData.usActionData = ubOpponentDir;

					// possibly raise weapon
					if( pSoldier->aiData.bAlertStatus >= STATUS_RED &&
						!WeaponReady(pSoldier) &&
						Chance(MercPtrs[ubPerson]->SuspicionPercent() / 2) &&
						PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
						(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
						GetAPsToLook(pSoldier) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)) <= pSoldier->bActionPoints)
					{
						pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
						pSoldier->aiData.usNextActionData = 0;
						DebugAI( AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
					}

					// don't turn to covert again this turn
					pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_COVERT;

					DebugAI( AI_MSG_INFO, pSoldier, String("turn to closest covert opponent"));
					return(AI_ACTION_CHANGE_FACING);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF SECTOR IS JAMMED
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_INFO, pSoldier, String("check: raise alert if sector is jammed"));
		if (pSoldier->CanUseRadio( TRUE ) &&
			SectorJammed() &&
			!pSoldier->IsJamming() &&
			!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition )
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_JAMMER]);
			DebugAI(AI_MSG_INFO, pSoldier, String("radio operator found jammer, raise alert"));
			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// TURN OFF RADIO IF ACTIVE
		////////////////////////////////////////////////////////////////////////////
		DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio if active"));
		if (pSoldier->CheckInitialAP() &&
			pSoldier->CanUseRadio(TRUE) &&
			pSoldier->IsUsingRadio() &&
			pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, TRUE))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio"));
			pSoldier->ubAISkillUse = SKILLS_RADIO_TURNOFF;
			pSoldier->aiData.usActionData = NOWHERE;
			return AI_ACTION_USE_SKILL;
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF TOO MANY SOLDIERS KILLED
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_INFO, pSoldier, String("check: raise alert if certain percent of soldiers killed"));

		if (AIRoleOfficer(pSoldier) &&
			ArmyPercentKilled() > ArmyPercentKilledTolerance() &&
			!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_MANY_KILLED]);
			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF SEE FRESH CORPSE
		////////////////////////////////////////////////////////////////////////////

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[raise alert if found fresh corpse]"));
		// raise alert if found fresh corpse
		// sevenfm: only if not raised alert yet
		if (!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
		{
			INT32				cnt;
			ROTTING_CORPSE *	pCorpse;

			for ( cnt = 0; cnt < giNumRottingCorpse; ++cnt )
			{
				pCorpse = &(gRottingCorpse[ cnt ] );

				if( pCorpse &&
					pCorpse->fActivated &&
					pCorpse->def.ubType <= FMERC_FALLF &&
					pCorpse->def.ubAIWarningValue > 0 &&
					CorpseEnemyTeam(pCorpse) &&
					!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
					LOS_Virtual(pSoldier, pCorpse->def.sGridNo, pCorpse->def.bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_WANTED_DIR))
				{
					ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_FOUND_DEAD_BODY]);
					// lower corpse warning value
					//pCorpse->def.ubAIWarningValue = 0;
					// also check locations nearby for hidden enemy (add noise at random spot)
					AddNoiseAtRandomSightCoverSpot(pSoldier, pCorpse->def.sGridNo, pCorpse->def.bLevel, TRUE, TRUE);
					return(AI_ACTION_RED_ALERT);
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF FOUND BOMB
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[raise alert if found bomb]"));

		if (!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
			FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE))
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_BOMB], pSoldier->GetName());
			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// RAISE ALERT IF FOUND SMOKE
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[raise alert if found smoke]"));

		if (!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
			FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, FALSE) )
		{
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_SMOKE]);

			// also check locations nearby for hidden enemy (add noise at random spot)
			AddNoiseAtRandomSightCoverSpot( pSoldier, sSmokeSpot, bSmokeLevel, TRUE, TRUE );

			return( AI_ACTION_RED_ALERT );
		}

		////////////////////////////////////////////////////////////////////////////
		// IF YOU SEE CAPTURED FRIENDS, FREE THEM!
		////////////////////////////////////////////////////////////////////////////

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[free captured friends]"));
		// Flugente: if we see one of our buddies in handcuffs, its a clear sign of enemy activity!
		if ( gGameExternalOptions.fAllowPrisonerSystem )
		{
			UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, VISION_RANGE, ENEMY_TEAM, SOLDIER_POW, TRUE );

			if ( ubPerson != NOBODY )
			{
				// sevenfm: raise alert first
				if (!pSoldier->RaisedAlertThisTurn() &&
					!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("found handcuffed soldier %d at spot %d, raise alert", ubPerson, MercPtrs[ubPerson]->sGridNo));

					ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_HANDCUFF], MercPtrs[ubPerson]->GetName());
					// raise alarm!
					return( AI_ACTION_RED_ALERT );
				}

				// if we are close, we can release this guy
				// possible only if not handcuffed (binders can be opened, handcuffs not)
				if (!MercPtrs[ubPerson]->inv[HANDPOS].exists() ||
					!HasItemFlag( (&(MercPtrs[ubPerson]->inv[HANDPOS]))->usItem, HANDCUFFS ) ||
					!Item[(&(MercPtrs[ubPerson]->inv[HANDPOS]))->usItem].metal ||
					FindToolkit( MercPtrs[ubPerson] ) != NO_SLOT ||
					FindLocksmithKit( MercPtrs[ubPerson] ) != NO_SLOT)
					//HAS_SKILL_TRAIT( MercPtrs[ubPerson], TECHNICIAN_NT ) )
				{
					if ( SpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) <= 1 )
					{
						// see if we are facing this person
						UINT8 ubDesiredMercDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

						// if not already facing in that direction,
						if (pSoldier->ubDirection != ubDesiredMercDir)
						{
							if (pSoldier->InternalIsValidStance(ubDesiredMercDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
							{
								pSoldier->aiData.usActionData = ubDesiredMercDir;
								return(AI_ACTION_CHANGE_FACING);
							}							
						}
						else
						{
							//ScreenMsg( FONT_LTGREEN, MSG_INTERFACE, L"[%d] found handcuffed %d! Free prisoner!", pSoldier->ubID, ubPerson);
							return(AI_ACTION_FREE_PRISONER);
						}						
					}
					else
					{
						pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_FRIEND, 0);

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
						{
							return(AI_ACTION_SEEK_FRIEND);
						}
					}
				}
			}
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[drugged friends]"));

		UINT8 ubPerson = GetClosestSeenDruggedFriendID(pSoldier);

		if (ubPerson != NOBODY &&
			!pSoldier->RaisedAlertThisTurn() &&
			!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found drugged soldier %d at spot %d, raise alert", ubPerson, MercPtrs[ubPerson]->sGridNo));

			// raise alarm!
			ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT_DRUGGED], MercPtrs[ubPerson]->GetName());
			return(AI_ACTION_RED_ALERT);
		}

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[medic]"));
		if (DecideMedic(pSoldier))
			return pSoldier->aiData.bAction;

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[inspect covert soldier]"));
		// sevenfm: officer can come to inspect suspicious soldier
		if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SQUADLEADER_NT))
		{
			UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, TRUE );

			if( ubPerson != NOBODY &&
				pSoldier->pathing.bLevel == MercPtrs[ubPerson]->pathing.bLevel &&
				pSoldier->CanInspect( MercPtrs[ubPerson] ) &&
				pSoldier->RadioCallValue() == 0 &&
				Random(100) < MercPtrs[ubPerson]->SuspicionPercent() )
			{
				UINT8 ubFriendsNearby = CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4);
				UINT8 ubSoldierDifficulty = SoldierDifficultyLevel(pSoldier);				

				// come to investigate
				pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_NOISE, 0);

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					// sevenfm: raise alert first if this soldier is not the person who raised alert last
					// chance to raise alert 30%
					if (IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT) &&
						gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1 &&
						ubFriendsNearby < 1 + gGameOptions.fNewTraitSystem ? NUM_SKILL_TRAITS(pSoldier, SQUADLEADER_NT) : 0 &&
						Chance(10 * ubSoldierDifficulty))
					{
						// sevenfm: use fake alert to show radio animation
						pSoldier->aiData.bNextAction = AI_ACTION_SEEK_NOISE;
						pSoldier->aiData.usNextActionData = pSoldier->aiData.usActionData;
						return( AI_ACTION_FAKE_ALERT );
					}

					return(AI_ACTION_SEEK_NOISE);
				}
			}
		}
	}	

	// determine the most important noise heard, and its relative value
	sNoiseGridNo = MostImportantNoiseHeard(pSoldier, &iNoiseValue, &fClimb, &fReachable);
	DebugAI(AI_MSG_INFO, pSoldier, String("MostImportantNoiseHeard gridno %d iNoiseValue %d, fClimb %d fReachable %d", sNoiseGridNo, iNoiseValue, fClimb, fReachable));

	if (TileIsOutOfBounds(sNoiseGridNo))
	{
		// then we have no business being under YELLOW status any more!
#ifdef BETAVERSION
		NumMessage("DecideActionYellow: ERROR - No important noise known by guynum ",pSoldier->ubID);
#endif

		return(AI_ACTION_NONE);
	}

	////////////////////////////////////////////////////////////////////////////
	// LOOK AROUND TOWARD NOISE: determine %chance for man to turn towards noise
	////////////////////////////////////////////////////////////////////////////

	// determine direction from this soldier in which the noise lies
	ubNoiseDir = AIDirection(pSoldier->sGridNo, sNoiseGridNo);

	// if soldier is not already facing in that direction,
	// and the noise source is close enough that it could possibly be seen
	if ( GetAPsToLook( pSoldier ) <= pSoldier->bActionPoints )
	{
		if ((pSoldier->ubDirection != ubNoiseDir) && PythSpacesAway(pSoldier->sGridNo,sNoiseGridNo) <= pSoldier->GetMaxDistanceVisible(sNoiseGridNo) )
		{
			// set base chance according to orders
			if ((pSoldier->aiData.bOrders == STATIONARY) || (pSoldier->aiData.bOrders == ONGUARD) )
				iChance = 50;
			else           // all other orders
				iChance = 25;

			if (pSoldier->aiData.bAttitude == DEFENSIVE)
				iChance += 15;


			if ((INT16)PreRandom(100) < iChance && pSoldier->InternalIsValidStance( ubNoiseDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
			{
				pSoldier->aiData.usActionData = ubNoiseDir;

				// sevenfm: check that weapon can be raised
				if( PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
					AIRoleSniper(pSoldier) &&
					(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30))
				{
					if( GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) <= pSoldier->bActionPoints )
					{
						pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
						pSoldier->aiData.usNextActionData = 0;
					}
				}
				////////////////////////////////////////////////////////////////////////////
				// SANDRO - allow regular soldiers to raise scoped weapons to see farther away too
				if ( AIGunInHandScoped(pSoldier) && PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION )
				{
					if (!(WeaponReady(pSoldier)) && (pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30))
					{
						if (GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) <= pSoldier->bActionPoints)
						{
							if ( Random(100) < 35 ) 
							{
								pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
								pSoldier->aiData.usNextActionData = 0;
							}
						}
					}
				}
				////////////////////////////////////////////////////////////////////////////

				return(AI_ACTION_CHANGE_FACING);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// RADIO YELLOW ALERT: determine %chance to call others and report noise
	////////////////////////////////////////////////////////////////////////////

	// if we have the action points remaining to RADIO
	// (we never want NPCs to choose to radio if they would have to wait a turn)
	if (!fCivilian && 
		pSoldier->aiData.bAlertStatus < STATUS_RED &&
		pSoldier->RadioCallValue() == 0 &&
		IsActionAffordable(pSoldier, AI_ACTION_YELLOW_ALERT) &&
		gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1)
	{
		// base chance depends on how much new info we have to radio to the others
		iChance = 5 * WhatIKnowThatPublicDont(pSoldier,FALSE);   // use 5 * for YELLOW alert

		// if I actually know something they don't and I ain't swimming (deep water)
		if (iChance && !DeepWater( pSoldier->sGridNo, pSoldier->pathing.bLevel))
		{
			// CJC: this addition allows for varying difficulty levels for soldier types
			iChance += gbDiff[ DIFF_RADIO_RED_ALERT ][ SoldierDifficultyLevel( pSoldier ) ] / 2;

			// Alex: this addition replaces the sectorValue/2 in original JA
			//iChance += gsDiff[DIFF_RADIO_RED_ALERT][GameOption[ENEMYDIFFICULTY]] / 2;

			// modify base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY: iChance +=  20;  break;
			case ONGUARD:    iChance +=  15;  break;
			case ONCALL:     iChance +=  10;  break;
			case CLOSEPATROL:                 break;
			case RNDPTPATROL:
			case POINTPATROL:                 break;
			case FARPATROL:  iChance += -10;  break;
			case SEEKENEMY:  iChance += -20;  break;
			}

			// modify base chance according to attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:  iChance +=  20;  break;
			case BRAVESOLO:  iChance += -10;  break;
			case BRAVEAID:                    break;
			case CUNNINGSOLO:iChance +=  -5;  break;
			case CUNNINGAID:                  break;
			case AGGRESSIVE: iChance += -20;  break;
			case ATTACKSLAYONLY: iChance = 0; break;
			}

			// sevenfm: lower chance to radio if some nearby friend radioed this turn
			//iChance = iChance / (1 + CountFriendsUsedRadio(pSoldier, DAY_VISION_RANGE));

			if ((INT16)PreRandom(100) < iChance)
			{
				return(AI_ACTION_YELLOW_ALERT);
			}
		}
	}

	// Hmmm, I don't think this check is doing what is intended.  But then I see no comment about what is intended.
	// However, civilians with no profile (and likely no weapons) do not need to be seeking out noises.  Most don't
	// even have the body type for it (can't climb or jump).
	//if ( !( pSoldier->bTeam == CIV_TEAM && pSoldier->ubProfile != NO_PROFILE && pSoldier->ubProfile != ELDIN ) )
	//if ( pSoldier->bTeam != CIV_TEAM || ( !pSoldier->aiData.bNeutral && pSoldier->ubProfile != ELDIN ) )
	// ADB: Eldin is the only neutral civilian who should be seeking out noises.  As the museum curator, he can be
	// available to talk to.  As the night watchman, he needs to look for thieves.
	bool onCivTeam = (pSoldier->bTeam == CIV_TEAM);
	bool isNamedCiv = (pSoldier->ubProfile != NO_PROFILE);
	bool isEldin = (pSoldier->ubProfile == ELDIN);//logically flipped from the original, isNotEldin == false is confusing
	// For purpose of seeking noise, cowardly civs are neutral, even if attacked by your thugs
	bool isNeutral = pSoldier->aiData.bNeutral || pSoldier->flags.uiStatusFlags & SOLDIER_COWERING; 
	if ((onCivTeam == false) ||
		(onCivTeam == true && isNamedCiv == true && isNeutral == false) ||
		(onCivTeam == true && isEldin == true))
	{
		// IF WE ARE MILITIA/CIV IN REALTIME, CLOSE TO NOISE, AND CAN SEE THE SPOT WHERE THE NOISE CAME FROM, FORGET IT
		if ( fReachable && !fClimb && !gfTurnBasedAI && (pSoldier->bTeam == MILITIA_TEAM || pSoldier->bTeam == CIV_TEAM )&& PythSpacesAway( pSoldier->sGridNo, sNoiseGridNo ) < 5 )
		{
			if ( LOS_Virtual( pSoldier, sNoiseGridNo, pSoldier->pathing.bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS )	)
			{
				// set reachable to false so we don't investigate
				fReachable = FALSE;
				// forget about noise
				pSoldier->aiData.sNoiseGridno = NOWHERE;
				pSoldier->aiData.ubNoiseVolume = 0;
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// SEEK NOISE
		////////////////////////////////////////////////////////////////////////////

		if ( fReachable )
		{
			// remember that noise value is negative, and closer to 0 => more important!
			iChance = 95 + (iNoiseValue / 3);
			iSneaky = 30;

			// increase

			// set base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY:     iChance += -20;  break;
			case ONGUARD:        iChance += -15;  break;
			case ONCALL:                          break;
			case CLOSEPATROL:    iChance += -10;  break;
			case RNDPTPATROL:
			case POINTPATROL:                     break;
			case FARPATROL:      iChance +=  10;  break;
			case SEEKENEMY:      iChance +=  25;  break;
			}

			// modify chance of patrol (and whether it's a sneaky one) by attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:      iChance += -10;  iSneaky +=  15;  break;
			case BRAVESOLO:      iChance +=  10;                   break;
			case BRAVEAID:       iChance +=   5;                   break;
			case CUNNINGSOLO:    iChance +=   5;  iSneaky +=  30;  break;
			case CUNNINGAID:                      iSneaky +=  30;  break;
			case AGGRESSIVE:     iChance +=  20;  iSneaky += -10;  break;
			case ATTACKSLAYONLY:	iChance +=  20;  iSneaky += -10;  break;
			}

			// reduce chance if breath is down, less likely to wander around when tired
			iChance -= (100 - pSoldier->bBreath);

			//Madd: make militia less likely to go running headlong into trouble
			if ( pSoldier->bTeam == MILITIA_TEAM )
				iChance -= 30;

			// sevenfm: stationary should not seek
			if ( pSoldier->aiData.bOrders == STATIONARY )
			{
				iChance = 0;
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("seek noise chance %d", iChance));

			if ((INT16) PreRandom(100) < iChance  )
			{

				pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards(pSoldier,sNoiseGridNo,AI_ACTION_SEEK_NOISE);
				DebugAI(AI_MSG_INFO, pSoldier, String("found move spot %d", pSoldier->aiData.usActionData));

				// sevenfm: avoid going too close to known bombs
				if (FindBombNearby(pSoldier, pSoldier->aiData.usActionData, BOMB_DETECTION_RANGE))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("found bomb near %d, abort movement", pSoldier->aiData.usActionData));
					pSoldier->aiData.usActionData = NOWHERE;
				}

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					if ( fClimb )
					{
						if ( pSoldier->sGridNo == sNoiseGridNo)
						{
							if (IsActionAffordable(pSoldier, pSoldier->aiData.bAction))
							{
								return( AI_ACTION_CLIMB_ROOF );
							}
							else if ( gfTurnBasedAI && pSoldier->bActionPoints < pSoldier->bInitialActionPoints )
							{
								return AI_ACTION_NONE;
							}
						}
						else
						{
							pSoldier->aiData.usActionData = sNoiseGridNo;							
							return( AI_ACTION_MOVE_TO_CLIMB  );
						}
					}
					else
					{
						return(AI_ACTION_SEEK_NOISE);
					}					
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// SEEK FRIEND WHO LAST RADIOED IN TO REPORT NOISE
		////////////////////////////////////////////////////////////////////////////

		sClosestFriend = ClosestReachableFriendInTrouble(pSoldier, &fClimb);

		// if there is a friend alive & reachable who last radioed in		
		if (!TileIsOutOfBounds(sClosestFriend))
		{
			// there a chance enemy soldier choose to go "help" his friend
			iChance = 80 - SpacesAway(pSoldier->sGridNo,sClosestFriend);
			iSneaky = 10;

			// set base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY:     iChance += -20;  break;
			case ONGUARD:        iChance += -15;  break;
			case ONCALL:         iChance +=  20;  break;
			case CLOSEPATROL:    iChance += -10;  break;
			case RNDPTPATROL:
			case POINTPATROL:    iChance += -10;  break;
			case FARPATROL:                       break;
			case SEEKENEMY:      iChance +=  10;  break;
			}

			// modify chance of patrol (and whether it's a sneaky one) by attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:      iChance += -10;  iSneaky +=  15;        break;
			case BRAVESOLO:                                              break;
			case BRAVEAID:       iChance +=  20;  iSneaky += -10;        break;
			case CUNNINGSOLO:					   iSneaky +=  30;		  break;
			case CUNNINGAID:     iChance +=  20;  iSneaky +=  20;        break;
			case AGGRESSIVE:     iChance += -20;  iSneaky += -20;        break;
			case ATTACKSLAYONLY: iChance += -20;  iSneaky += -20;        break;
			}

			// reduce chance if breath is down, less likely to wander around when tired
			iChance -= (100 - pSoldier->bBreath);

			// sevenfm: stationary should not help
			if ( pSoldier->aiData.bOrders == STATIONARY )
			{
				iChance = 0;
			}

			if ((INT16)PreRandom(100) < iChance)
			{
				pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards(pSoldier,sClosestFriend,AI_ACTION_SEEK_FRIEND);

				// sevenfm: avoid going too close to known bombs
				if (FindBombNearby(pSoldier, pSoldier->aiData.usActionData, BOMB_DETECTION_RANGE))
				{
					pSoldier->aiData.usActionData = NOWHERE;
				}

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					if ( fClimb )
					{
						// need to climb AND have enough APs to get there this turn
						DebugMsg ( TOPIC_JA2AI , DBG_LEVEL_3 , String("Soldier %d is climbing",pSoldier->ubID) );

						// 0verhaul:  Closest Friend call also returns the climb point if climbing is necessary.  So don't
						// climb the wrong building and don't search again
						if (pSoldier->sGridNo == sClosestFriend)
						{
							if (IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF) )
							{
								return( AI_ACTION_CLIMB_ROOF );
							}
						}
						else
						{							
							pSoldier->aiData.usActionData = sClosestFriend;							
							return( AI_ACTION_MOVE_TO_CLIMB  );
						}
					}
					else
					{
						return(AI_ACTION_SEEK_FRIEND);
					}					
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////
		// TAKE BEST NEARBY COVER FROM THE NOISE GENERATING GRIDNO
		////////////////////////////////////////////////////////////////////////////

		if( gfTurnBasedAI && 
			!pSoldier->SkipCoverCheck() && 			
			pSoldier->CheckInitialAP() ) // only do in turnbased
		{
			// remember that noise value is negative, and closer to 0 => more important!
			iChance = 25;
			iSneaky = 30;

			// set base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY:     iChance +=  20;  break;
			case ONGUARD:        iChance +=  15;  break;
			case ONCALL:                          break;
			case CLOSEPATROL:    iChance +=  10;  break;
			case RNDPTPATROL:
			case POINTPATROL:                     break;
			case FARPATROL:      iChance +=  -5;  break;
			case SEEKENEMY:      iChance += -20;  break;
			}

			// modify chance (and whether it's sneaky) by attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:      iChance +=  10;  iSneaky +=  15;  break;
			case BRAVESOLO:      iChance += -15;  iSneaky += -20;  break;
			case BRAVEAID:       iChance += -20;  iSneaky += -20;  break;
			case CUNNINGSOLO:    iChance +=  20;  iSneaky +=  30;  break;
			case CUNNINGAID:     iChance +=  15;  iSneaky +=  30;  break;
			case AGGRESSIVE:     iChance += -10;  iSneaky += -10;  break;
			case ATTACKSLAYONLY: iChance += -10;  iSneaky += -10;  break;
			}


			//Madd: make militia more likely to take cover
			if ( pSoldier->bTeam == MILITIA_TEAM )
				iChance += 20;

			// reduce chance if breath is down, less likely to wander around when tired
			iChance -= (100 - pSoldier->bBreath);

			if ((INT16)PreRandom(100) < iChance)
			{
				pSoldier->aiData.bAIMorale = CalcMorale( pSoldier );
				pSoldier->aiData.usActionData = FindBestNearbyCover(pSoldier, &iDummy);

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
				{
					return(AI_ACTION_TAKE_COVER);
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// SWITCH TO GREEN: determine if soldier acts as if nothing at all was wrong
	////////////////////////////////////////////////////////////////////////////
	if ((INT16)PreRandom(100) < 50 )
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("nothing to do, switch to green"));
		// Skip YELLOW until new situation, 15% extra chance to do GREEN actions
		pSoldier->aiData.bBypassToGreen = 15;
		return(DecideActionGreen(pSoldier));
	}

	////////////////////////////////////////////////////////////////////////////
	// CROUCH IF NOT CROUCHING ALREADY
	////////////////////////////////////////////////////////////////////////////

	// if not in water and not already crouched, try to crouch down first
	if (!fCivilian && 
		!pSoldier->IsBoxer() &&
		!PTR_CROUCHED && 
		IsValidStance( pSoldier, ANIM_CROUCH ) )
	{
		if (GetAPsToChangeStance( pSoldier, ANIM_CROUCH ) <= pSoldier->bActionPoints)
		{
			////////////////////////////////////////////////////////////////////////////
			// SANDRO - raise weapon maybe
			// if we are facing the direction of where the noise came from
			// sevenfm: check that weapon can be raised
			if( AIGunInHandScoped(pSoldier) &&
				PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
				!WeaponReady(pSoldier) &&
				pSoldier->ubDirection == ubNoiseDir &&
				(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
				(GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) + GetAPsToChangeStance( pSoldier, ANIM_CROUCH )) <= pSoldier->bActionPoints ) 
			{
				pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
				pSoldier->aiData.usNextActionData = 0;
			}
			////////////////////////////////////////////////////////////////////////////

			pSoldier->aiData.usActionData = ANIM_CROUCH;
			return(AI_ACTION_CHANGE_STANCE);
		}
	}
	else if (!fCivilian)
	{
		////////////////////////////////////////////////////////////////////////////
		// SANDRO - raise weapon maybe
		// if we are facing the direction of where the noise came from
		// sevenfm: check that weapon can be raised
		if( AIGunInHandScoped(pSoldier) &&
			PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
			!WeaponReady(pSoldier) &&
			pSoldier->ubDirection == ubNoiseDir &&
			(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
			GetAPsToReadyWeapon( pSoldier, pSoldier->usAnimState ) <= pSoldier->bActionPoints &&
			Random(100) < 35 ) 
		{
			return( AI_ACTION_RAISE_GUN );
		}
		////////////////////////////////////////////////////////////////////////////	
	}

	// sevenfm: possibly use focus to scan area
	if ((gfTurnBasedAI || pSoldier->CheckInitialAP()) &&
		SoldierAI(pSoldier) &&
		WeaponReady(pSoldier) &&
		AIGunInHandScoped(pSoldier) &&
		pSoldier->bActionPoints >= APBPConstants[AP_SPOTTER] &&
		!pSoldier->IsFocusing() &&
		pSoldier->CanUseSkill(SKILLS_FOCUS, FALSE) &&
		(AIRoleSniper(pSoldier) || Chance(10 * SoldierDifficultyLevel(pSoldier))))
	{
		INT32 sFocusSpot;
		INT8 bFocusLevel;

		if (FindRandomFocusSpot(pSoldier, sFocusSpot, bFocusLevel, Chance(75)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start focusing at location: spot %d level %d", sFocusSpot, bFocusLevel));

			pSoldier->ubAISkillUse = SKILLS_FOCUS;
			pSoldier->aiData.usActionData = sFocusSpot;
			pSoldier->bTargetLevel = bFocusLevel;

			if (!gfTurnBasedAI)
			{
				pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
				pSoldier->aiData.usNextActionData = RealtimeDelay(pSoldier);
			}

			return AI_ACTION_USE_SKILL;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// DO NOTHING: Not enough points left to move, so save them for next turn
	////////////////////////////////////////////////////////////////////////////

	// by default, if everything else fails, just stands in place without turning
	pSoldier->aiData.usActionData = NOWHERE;
	return(AI_ACTION_NONE);
}


INT8 DecideActionRed(SOLDIERTYPE *pSoldier)
{
	INT8	bActionReturned;
	INT32	iDummy;
	INT32	iChance;
	INT32	sClosestOpponent = NOWHERE, sClosestFriend = NOWHERE;
	INT32	sClosestDisturbance = NOWHERE, sCheckGridNo;
	INT8	bClosestDisturbanceLevel = 0;
	INT32	sClosestSeenThisTurnOpponent = NOWHERE, sClosestSeenThisTurnOpponentGridNo;	
	INT8	bClosestSeenThisTurnOpponentLevel;
	INT32	sClosestPossiblySeenOpponent = NOWHERE, sClosestPossiblySeenOpponentGridNo;
	INT8	bClosestPossiblySeenOpponentLevel = 0;
	INT32	sClosestSeenPathSpot = NOWHERE;
	UINT8	ubCanMove,ubOpponentDir;
	INT8	bInWater, bInDeepWater, bInGas, bInGasSpot;
	INT8	bSeekPts = 0, bHelpPts = 0, bHidePts = 0, bWatchPts = 0;
	INT8	bHighestWatchLoc;
	ATTACKTYPE	BestThrow, BestShot;
	BOOLEAN fSeekClimb, fHelpClimb;
	BOOLEAN fCivilian = pSoldier->IsCivilian();

	// sevenfm
	//INT32	iTotalEnemyThreat = 0;
	//INT32	iThreatTolerance = 0;
	BOOLEAN fDangerousSpot = FALSE;
	BOOLEAN fSafeSpot = FALSE;
	BOOLEAN fCanBeSeen = FALSE;
	BOOLEAN fProneSightCover = FALSE;
	BOOLEAN fAnyCover = FALSE;
	INT32	sOpponentGridNo;
	INT8	bOpponentLevel;
	INT32	sClosestNoise = NOWHERE;
	UINT8	ubThreatID = NOBODY;
	INT32	sThreatSpot = NOWHERE;
	INT8	bThreatLevel = 0;
	UINT8	ubSeekType = SEEK_NORMAL;

	INT32	sClosestThreat = NOWHERE;
	INT8	bClosestThreatLevel = 0;
	BOOLEAN fAllowProne = FALSE;

	INT32	sSmokeSpot = NOWHERE;
	INT8	bSmokeLevel = 0;

	INT32	sDangerousSpot;
	INT32	sLastSafeSpot;	

	// WANNE: Headrock informed me that I should remove that because it needs a lot of CPU!
	// HEADROCK HAM B2.7: Calculate the overall tactical situation
	//INT16 ubOverallTacticalSituation = AssessTacticalSituation(pSoldier->bSide);

	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,String("DecideActionRed: soldier orders = %d",pSoldier->aiData.bOrders));

	DebugAI( AI_MSG_START, pSoldier, String("[Red]"));
	LogDecideInfo(pSoldier);
	
	// sevenfm: disable stealth mode
	pSoldier->bStealthMode = FALSE;
	// disable reverse movement mode
	pSoldier->bReverse = FALSE;

	// sevenfm: initialize data
	pSoldier->bWeaponMode = WM_NORMAL;

	// if we have absolutely no action points, we can't do a thing under RED!
	if (pSoldier->bActionPoints <= 0 || pSoldier->IsUnconscious()) //Action points can be negative
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("No action points - nothing to do!"));
		pSoldier->aiData.usActionData = NOWHERE;
		return(AI_ACTION_NONE);
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Possibly update knowledge]"));

	if (pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		(gfTurnBasedAI || pSoldier->CheckInitialAP()))
	{
		PossiblyUpdateKnowledge(pSoldier);
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Prepare data]"));

	UINT8	ubSectorLeader = SectorLeader(pSoldier->bTeam);
	UINT8	ubSectorType = AISectorType();
	BOOLEAN fSectorAttack = SectorAttack(pSoldier->bTeam);

	sClosestNoise = MostImportantNoiseHeard(pSoldier, NULL, NULL, NULL);

	if( !fCivilian )
	{
		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Find opponents]"));
		// sevenfm: find closest opponent
		sClosestOpponent = ClosestKnownOpponent( pSoldier, &sOpponentGridNo, &bOpponentLevel );		
		sClosestSeenThisTurnOpponent = ClosestSeenThisTurnOpponent(pSoldier, &sClosestSeenThisTurnOpponentGridNo, &bClosestSeenThisTurnOpponentLevel);
		sClosestPossiblySeenOpponent = ClosestPossiblySeenOpponent(pSoldier, &sClosestPossiblySeenOpponentGridNo, &bClosestPossiblySeenOpponentLevel);

		DebugAI( AI_MSG_INFO, pSoldier, String("sClosestOpponent %d, sClosestNoise %d, sClosestSeenThisTurnOpponent %d, sClosestPossiblySeenOpponent %d", sClosestOpponent, sClosestNoise, sClosestSeenThisTurnOpponent, sClosestPossiblySeenOpponent));

		// first, set closest threat to closest recently seen opponent
		if( !TileIsOutOfBounds(sClosestSeenThisTurnOpponent) &&
			LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestSeenThisTurnOpponent, bClosestSeenThisTurnOpponentLevel, MAX_VISION_RANGE))
		{
			sClosestThreat = sClosestSeenThisTurnOpponent;
			bClosestThreatLevel = bClosestSeenThisTurnOpponentLevel;
		}
		// if not found, use closest known opponent
		if( !TileIsOutOfBounds(sClosestOpponent) && TileIsOutOfBounds(sClosestThreat)  )
		{
			sClosestThreat = sClosestOpponent;
			bClosestThreatLevel = bOpponentLevel;
		}
		// if not found, use closest noise
		if( !TileIsOutOfBounds(sClosestNoise) && TileIsOutOfBounds(sClosestThreat) )
		{
			sClosestThreat = sClosestNoise;
			bClosestThreatLevel = 0;
		}

		DebugAI( AI_MSG_INFO, pSoldier, String("sClosestThreat %d, bClosestThreatLevel %d", sClosestThreat, bClosestThreatLevel));

		// calculate personal/team threat at current gridno
		//iTotalEnemyThreat = AICalcTotalEnemyThreatAtSpot(pSoldier, pSoldier->sGridNo);
		//iThreatTolerance = AICalcThreatTolerance(pSoldier);

		if (!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE))
		{
			fCanBeSeen = TRUE;
			DebugAI(AI_MSG_INFO, pSoldier, String("can be seen"));
		}

		fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
		DebugAI(AI_MSG_INFO, pSoldier, String("prone sight cover %d", fProneSightCover));
		fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);
		DebugAI(AI_MSG_INFO, pSoldier, String("any cover %d", fAnyCover));
		fDangerousSpot = DangerousSpot(pSoldier);
		DebugAI(AI_MSG_INFO, pSoldier, String("dangerous spot %d", fDangerousSpot));
		fSafeSpot = SafeSpot(pSoldier);
		DebugAI(AI_MSG_INFO, pSoldier, String("safe spot %d", fSafeSpot));

		if( fCanBeSeen && (pSoldier->pathing.bLevel > 0 || !InARoom(pSoldier->sGridNo, NULL)) ||
			pSoldier->aiData.bUnderFire || 
			RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel) ||
			GuySawEnemy(pSoldier, SEEN_LAST_TURN))
		{
			fAllowProne = TRUE;
		}

		DebugAI( AI_MSG_INFO, pSoldier, String("fProneSightCover %d, fAnyCover %d fCanBeSeen %d, fDangerousSpot %d", fProneSightCover, fAnyCover, fCanBeSeen, fDangerousSpot));
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Decide stealth mode]"));
	// sevenfm: set stealth mode
	if (pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		!pSoldier->bBlindedCounter &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < TACTICAL_RANGE &&
		(pSoldier->aiData.bAttitude == DEFENSIVE ||	pSoldier->aiData.bAttitude == CUNNINGAID ||	pSoldier->aiData.bAttitude == CUNNINGSOLO ||
			(NightTime() || !WeAttack(ENEMY_TEAM) && !TeamSawEnemy(ENEMY_TEAM) && !AICheckWeOutnumberPublic(pSoldier, sClosestOpponent)) && TeamHasListeningRadioOperator(OUR_TEAM)) &&
		!pSoldier->aiData.bUnderFire &&
		!pSoldier->RushAttackAdvance() &&
		!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
		!GuySawEnemy(pSoldier) &&
		!InWaterOrGas(pSoldier, pSoldier->sGridNo) &&
		!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		(!gfTurnBasedAI ||
		NightTime() ||
		InARoom(pSoldier->sGridNo, NULL) ||
		pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_APPROACH ||
		//pSoldier->IsFlanking() || 
		TerrainDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, 2, TRUE) >= 20 ||
		pSoldier->HighWoodCamo() && TerrainJungleDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, DAY_VISION_RANGE / 4) > 30 ||
		pSoldier->HighDesertCamo() && TerrainDesertDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, DAY_VISION_RANGE / 4) > 30 ||
		pSoldier->HighUrbanCamo() && TerrainUrbanDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, DAY_VISION_RANGE / 4) > 30 ||
		pSoldier->HighSnowCamo() && TerrainSnowDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, DAY_VISION_RANGE / 4) > 30 ||
		pSoldier->HighWornStealth() && TerrainDarknessDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, DAY_VISION_RANGE / 4) > 30) &&
		CountFriendsBlack(pSoldier) == 0 &&
		!AICheckSuccessfulAttack(pSoldier, TRUE))
	{
		pSoldier->bStealthMode = TRUE;
	}

	DebugAI( AI_MSG_INFO, pSoldier, String("bStealthMode %d", pSoldier->bStealthMode));	

	// can this guy move to any of the neighbouring squares ? (sets TRUE/FALSE)
	ubCanMove = (pSoldier->bActionPoints >= MinPtsToMove(pSoldier));

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Turn off flashlight]"));
	// always turn off flashlight at the beginning of a new turn
	if (	//pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT &&
		IsActionAffordable(pSoldier, AI_ACTION_SWITCH_FLASHLIGHT) &&
		(!gfTurnBasedAI || pSoldier->bActionPoints == pSoldier->bInitialActionPoints))
		/* &&
		(	!AIUseFlashlightCondition(pSoldier, pSoldier->ubDirection) ||
		!GuySawEnemy(pSoldier, SEEN_2_TURNS_AGO) && !GuyHeardEnemy(pSoldier, HEARD_2_TURNS_AGO) ||
		Random(100) > 100 / (1 + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4))))*/
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("turn off flashlight"));
		return AI_ACTION_SWITCH_FLASHLIGHT;
	}

	// sevenfm: before deciding anything, stop cowering
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop cowering]"));
	if (!fCivilian &&
		SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->stats.bLife >= OKLIFE &&
		!pSoldier->bCollapsed &&
		!pSoldier->bBreathCollapsed &&
		pSoldier->IsCowering())
	{
		//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] stop cowering", pSoldier->ubID);
		return AI_ACTION_STOP_COWERING;
	}

	// sevenfm: stop giving aid
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop giving aid]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->stats.bLife >= OKLIFE &&
		!pSoldier->bCollapsed &&
		!pSoldier->bBreathCollapsed &&
		pSoldier->IsGivingAid())
	{
		//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] stop medic", pSoldier->ubID);
		return AI_ACTION_STOP_MEDIC;
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Panic AI]"));
	// if we're an alerted enemy, and there are panic bombs or a trigger around
	if ( (!PTR_CIVILIAN || pSoldier->ubProfile == WARDEN) && ( ( gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition || (pSoldier->ubID == gTacticalStatus.ubTheChosenOne) || (pSoldier->ubProfile == WARDEN) ) &&
		(gTacticalStatus.fPanicFlags & (PANIC_BOMBS_HERE | PANIC_TRIGGERS_HERE ) ) ) )
	{
		if ( pSoldier->ubProfile == WARDEN && gTacticalStatus.ubTheChosenOne == NOBODY )
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("possibly make Warden chosen one"));
			PossiblyMakeThisEnemyChosenOne( pSoldier );
		}

		// do some special panic AI decision making
		bActionReturned = PanicAI(pSoldier,ubCanMove);

		// if we decided on an action while in there, we're done
		if (bActionReturned != -1)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("Panic AI returned action %d", bActionReturned));
			return(bActionReturned);
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Queen/Joe]"));
	if ( pSoldier->ubProfile != NO_PROFILE )
	{
		if ( (pSoldier->ubProfile == QUEEN || pSoldier->ubProfile == JOE) && ubCanMove )
		{
			if ( gWorldSectorX == 3 && gWorldSectorY == MAP_ROW_P && gbWorldSectorZ == 0 && !gfUseAlternateQueenPosition )
			{
				bActionReturned = HeadForTheStairCase( pSoldier );
				if ( bActionReturned != AI_ACTION_NONE )
				{
					return( bActionReturned );
				}
			}
		}
	}

	// determine if we happen to be in water (in which case we're in BIG trouble!)
	bInWater = Water( pSoldier->sGridNo, pSoldier->pathing.bLevel );
	// sevenfm: changed Water() to DeepWater()
	bInDeepWater = WaterTooDeepForAttacks( pSoldier->sGridNo, pSoldier->pathing.bLevel );

	// check if standing in tear gas without a gas mask on
	bInGas = InGas( pSoldier, pSoldier->sGridNo );
	bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);

	////////////////////////////////////////////////////////////////////////////
	// WHEN LEFT IN GAS, WEAR GAS MASK IF AVAILABLE AND NOT WORN
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Wear gas mask]"));

	if ( !bInGas && (gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y) )
	{
		// only chance if we happen to be caught with our gas mask off
		if ( PreRandom( 10 ) == 0 && WearGasMaskIfAvailable( pSoldier ) )
		{
			// reevaluate
			bInGas = InGas( pSoldier, pSoldier->sGridNo );
			bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);
		}
	}

	//Only put mask on in gas
	if ((bInGas || InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE)) && WearGasMaskIfAvailable(pSoldier))//dnl ch40 200909
	{
		bInGas = InGas(pSoldier, pSoldier->sGridNo);
		bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);
	}

	// sevenfm: chance to wear gas mask if soldier sees gas nearby
	if( SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		pSoldier->HasGasMask() &&
		!pSoldier->WearsGasMask() && 
		FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, TRUE, TACTICAL_RANGE / 2) &&
		Chance(10 + SoldierDifficultyLevel(pSoldier) * 10))
	{
		WearGasMaskIfAvailable(pSoldier);
	}

	// sevenfm: possibly remove gas mask
	if (SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		gbWorldSectorZ == 0 &&
		pSoldier->WearsGasMask() &&
		!InGas(pSoldier, pSoldier->sGridNo) &&
		!pSoldier->aiData.bUnderFire &&
		!FindClosestGas(pSoldier, sSmokeSpot, bSmokeLevel, TACTICAL_RANGE / 2) &&
		Chance(10 + SoldierDifficultyLevel(pSoldier) * 10))
	{
		RemoveGasMask(pSoldier);
	}

	////////////////////////////////////////////////////////////////////////////
	// STOP/CONTINUE DRAGGING
	////////////////////////////////////////////////////////////////////////////

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop dragging]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->IsDragging() &&
		(pSoldier->aiData.bUnderFire ||
		SpotDangerLevel(pSoldier, pSoldier->sGridNo) > 2 ||
		pSoldier->ubDragPersonID != NOBODY && MercPtrs[pSoldier->ubDragPersonID]->stats.bLife < 5 ||
		pSoldier->ubDragPersonID != NOBODY && (MercPtrs[pSoldier->ubDragPersonID]->stats.bLife >= OKLIFE && !MercPtrs[pSoldier->ubDragPersonID]->IsUnconscious()) ||
		pSoldier->ubDragPersonID != NOBODY && !pSoldier->CanDragPerson(pSoldier->ubDragPersonID) ||
		SafeMedicSpot(pSoldier, pSoldier->sGridNo)))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop dragging"));
		return AI_ACTION_CANCEL_DRAG;
	}

	// sevenfm: chance to stop focusing
	if (pSoldier->CheckInitialAP() &&
		pSoldier->IsFocusing() &&
		pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] > 1 &&
		Chance(pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] * 10))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop focusing"));
		return AI_ACTION_STOP_FOCUS;
	}

	/*DebugAI(AI_MSG_TOPIC, pSoldier, String("[Continue dragging]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->IsDragging())
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("continue dragging"));
		
		// find closest safe medic spot
		INT32 sSafeMedicSpot = FindSafeMedicSpot(pSoldier);
		if(!TileIsOutOfBounds(sSafeMedicSpot))
		{
			// set reverse movement mode			
			DebugAI(AI_MSG_INFO, pSoldier, String("moving backwards to safe medic spot %d, use reverse movement mode", sSafeMedicSpot));
			pSoldier->bReverse = TRUE;
			pSoldier->aiData.usActionData = sSafeMedicSpot;
			return AI_ACTION_TAKE_COVER;
		}
		else
		{
			// cannot find safe medic spot, stop dragging
			DebugAI(AI_MSG_INFO, pSoldier, String("cannot find safe medic spot, stop dragging"));
			return AI_ACTION_CANCEL_DRAG;
		}
	}*/

	////////////////////////////////////////////////////////////////////////////
	// WHEN IN DEEP WATER, CREATE SCUBA FINS
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[when in deep water, create scuba fins]"));

	// sevenfm: when in deep water, create scuba fins
	if (ubCanMove &&
		bInDeepWater &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->IsFlanking() &&
		pSoldier->CheckInitialAP() &&
		Chance(25) &&
		20 * SoldierDifficultyLevel(pSoldier) >= 100 - HighestPlayerProgressPercentage() &&
		!FindNotDeepWaterNearby(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		FindUsableItemWithFlag(pSoldier, SCUBA_FINS) == NO_SLOT)
	{
		// create scuba fins and place in the inventory
		UINT16 usItem;
		if (GetFirstItemWithFlag(&usItem, SCUBA_FINS))
		{
			OBJECTTYPE newobj;
			CreateItem(usItem, 80 + Random(20), &newobj);
			newobj.fFlags |= OBJECT_UNDROPPABLE;

			// try to place item in inventory
			if (AutoPlaceObject(pSoldier, &newobj, FALSE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("successfully created and placed scuba fins in inventory"));
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN NOT IN WATER, DROP SCUBA FINS
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[when not in water, drop scuba fins]"));

	// when not in deep water, drop scuba fins
	if (pSoldier->CheckInitialAP() &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		pSoldier->inv[LEGPOS].exists() && 
		HasItemFlag(pSoldier->inv[LEGPOS].usItem, SCUBA_FINS) &&
		IsActionAffordable(pSoldier, AI_ACTION_DROP_ITEM))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found scuba fins in LEGPOS, drop to the ground"));
		//if (!AutoPlaceObject(pSoldier, &(pSoldier->inv[LEGPOS]), FALSE, LEGPOS))
		//RearrangePocket(pSoldier, HANDPOS, LEGPOS, FOREVER);

		pSoldier->inv[LEGPOS].fFlags |= OBJECT_AI_UNUSABLE;
		pSoldier->aiData.usActionData = LEGPOS;

		return AI_ACTION_DROP_ITEM;
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN IN GAS, GO TO NEAREST REACHABLE SPOT OF UNGASSED LAND
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[leave water/gas/bomb]"));

	// when in deep water, move to closest opponent
	if (ubCanMove && 
		bInDeepWater && 
		!pSoldier->aiData.bNeutral && 
		pSoldier->aiData.bOrders == SEEKENEMY &&
		!pSoldier->IsFlanking())
	{
		// find closest reachable opponent, excluding opponents in deep water
		pSoldier->aiData.usActionData = ClosestReachableDisturbance(pSoldier, &fSeekClimb, ubThreatID, sThreatSpot, bThreatLevel, ubSeekType);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_LEAVE_WATER_GAS);
		}
	}

	if (ubCanMove && (bInGas || bInDeepWater && !pSoldier->IsFlanking() || FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE) || RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel)))
	{
		pSoldier->aiData.usActionData = FindNearestUngassedLand(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_LEAVE_WATER_GAS);
		}
	}	

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[raise alert for others]"));
	if ((pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral && pSoldier->ubCivilianGroup != NON_CIV_GROUP) &&
		SoldierAI(pSoldier) &&
		!fCivilian &&
		!pSoldier->IsBoxer() &&
		!pSoldier->RaisedAlertThisTurn() &&
		!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
		Chance(SoldierDifficultyLevel(pSoldier) * 20) &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT))
	{
		ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT]);
		return(AI_ACTION_RED_ALERT);
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Civilian AI]"));

	if ( fCivilian && 
		pSoldier->ubBodyType != COW &&
		pSoldier->ubBodyType != CRIPPLECIV && 
		FindAIUsableObjClass( pSoldier, IC_WEAPON ) == NO_SLOT )
	{
		// cower in fear!!
		if ( pSoldier->flags.uiStatusFlags & SOLDIER_COWERING )
		{
			if (gfTurnBasedAI || gTacticalStatus.fEnemyInSector) // battle!
			{
				// in battle!
				if ( pSoldier->aiData.bLastAction == AI_ACTION_COWER )
				{
					// do nothing
					pSoldier->aiData.usActionData = NOWHERE;
					return( AI_ACTION_NONE );
				}
				else
				{
					// set up next action to run away
					pSoldier->aiData.usNextActionData = FindSpotMaxDistFromOpponents( pSoldier );

					if (!TileIsOutOfBounds(pSoldier->aiData.usNextActionData))
					{
						pSoldier->aiData.bNextAction = AI_ACTION_RUN_AWAY;

						pSoldier->aiData.usActionData = ANIM_STAND;
						return( AI_ACTION_STOP_COWERING );
					}
					else
					{
						return( AI_ACTION_NONE );
					}
				}
			}
			else
			{
				if ( pSoldier->aiData.bNewSituation == NOT_NEW_SITUATION )
				{
					// stop cowering, not in battle, timer expired
					// we have to turn off whatever is necessary to stop status red...
					pSoldier->aiData.bAlertStatus = STATUS_GREEN;
					return( AI_ACTION_STOP_COWERING );
				}
				else
				{
					return( AI_ACTION_NONE );
				}
			}
		}
		else
		{
			if ( gfTurnBasedAI || gTacticalStatus.fEnemyInSector )
			{
				// battle - cower!!!
				pSoldier->aiData.usActionData = ANIM_CROUCH;
				return( AI_ACTION_COWER );
			}
			else // not in battle, cower for a certain length of time
			{
				pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
				pSoldier->aiData.usNextActionData = (UINT16) REALTIME_CIV_AI_DELAY;
				pSoldier->aiData.usActionData = ANIM_CROUCH;
				return( AI_ACTION_COWER );
			}
		}
	}	

	////////////////////////////////////////////////////////////////////////////
	// TURN TO CLOSEST SUSPICIOUS COVERT OPPONENT
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[turn to suspicious covert]"));

	if (!fCivilian &&
		pSoldier->bTeam == ENEMY_TEAM &&
		!pSoldier->bBlindedCounter &&
		pSoldier->CheckInitialAP() &&
		SoldierAI(pSoldier) &&
		!bInWater &&
		!bInGas)
	{
		UINT8 ubPerson = GetClosestFlaggedSoldierID(pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, FALSE);

		// lower chance to turn if soldier is prone or recently seen enemy
		if (ubPerson != NOBODY &&
			CountTeamSeeSoldier(pSoldier->bTeam, MercPtrs[ubPerson]) == 0 &&
			(MercPtrs[ubPerson]->SuspicionPercent() >= 100 || MercPtrs[ubPerson]->usSoldierFlagMask2 & SOLDIER_COVERT_ACTION) &&
			(TileIsOutOfBounds(sClosestPossiblySeenOpponent) || PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) < PythSpacesAway(pSoldier->sGridNo, sClosestPossiblySeenOpponent)) &&
			LOS_Soldier(pSoldier, MercPtrs[ubPerson], LOS_NORMAL, CALC_FROM_ALL_DIRS))
		{
			UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

			// and the opponent is close enough that he could possibly be seen
			if (pSoldier->ubDirection != ubOpponentDir &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
				pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
			{
				// turn
				pSoldier->aiData.usActionData = ubOpponentDir;

				// possibly raise weapon
				if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					!WeaponReady(pSoldier) &&
					Chance(MercPtrs[ubPerson]->SuspicionPercent() / 2) &&
					PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
					(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
					GetAPsToLook(pSoldier) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)) <= pSoldier->bActionPoints)
				{
					pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
					pSoldier->aiData.usNextActionData = 0;
					DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
				}

				DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest suspicious covert opponent"));
				return(AI_ACTION_CHANGE_FACING);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	// WAIT IF BEING BANDAGED
	////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Being bandaged]"));
	if (SoldierAI(pSoldier) &&
		AICheckIsBandaged(pSoldier) &&
		!pSoldier->aiData.bUnderFire &&
		!bInWater &&
		!bInGas &&
		fSafeSpot)
	{
		// turn to closest threat
		if (!TileIsOutOfBounds(sClosestThreat) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestThreat) < DAY_VISION_RANGE * 2)
		{
			ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestThreat);

			// if soldier is not already facing in that direction,
			// and the opponent is close enough that he could possibly be seen
			if (pSoldier->ubDirection != ubOpponentDir &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
				pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
			{
				// turn
				pSoldier->aiData.usActionData = ubOpponentDir;

				DebugAI(AI_MSG_INFO, pSoldier, String("soldier is being bandaged, turn to closest threat"));
				return(AI_ACTION_CHANGE_FACING);
			}
		}

		// if not already crouched, try to crouch down first
		if (pSoldier->AnimEndHeight() > ANIM_CROUCH &&
			IsValidStance(pSoldier, ANIM_CROUCH) &&
			gAnimControl[pSoldier->usAnimState].ubEndHeight > ANIM_CROUCH)
		{
			if (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
			{
				pSoldier->aiData.usActionData = ANIM_CROUCH;
				DebugAI(AI_MSG_TOPIC, pSoldier, String("soldier is being bandaged, crouch"));
				return(AI_ACTION_CHANGE_STANCE);
			}
		}

		DebugAI( AI_MSG_INFO, pSoldier, String("Soldier is being bandaged, ubServiceCount %d, wait", pSoldier->ubServiceCount));
		return AI_ACTION_NONE;
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[pick up weapon/ammo]"));

	INT8 bGunSlot = FindAIUsableObjClass( pSoldier, IC_GUN );
	DebugAI( AI_MSG_INFO, pSoldier, String("found gun slot %d", bGunSlot));

	// sevenfm: if we don't have a gun, look around for a weapon!
	if( (pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM ) &&
		ubCanMove &&
		SoldierAI(pSoldier) &&
		!( pSoldier->usSoldierFlagMask & SOLDIER_SEARCH_ITEMS ) &&
		!InWaterOrGas(pSoldier, pSoldier->sGridNo) &&
		(bGunSlot == NO_SLOT || (pSoldier->inv[bGunSlot][0]->data.gun.ubGunShotsLeft == 0 && FindAmmoToReload( pSoldier, bGunSlot, NO_SLOT ) == NO_SLOT)))
	{
		// limit searching to 1 time per turn
		pSoldier->usSoldierFlagMask |= SOLDIER_SEARCH_ITEMS;

		// look around for a gun...
		DebugAI( AI_MSG_INFO, pSoldier, String("search weapon"));
		UINT16 usItem = (bGunSlot == NO_SLOT ? pSoldier->inv[HANDPOS].usItem : pSoldier->inv[bGunSlot].usItem);
		pSoldier->aiData.bAction = SearchForItems(pSoldier, SEARCH_WEAPONS, usItem);
		DebugAI( AI_MSG_INFO, pSoldier, String("SearchForItems returned %d", pSoldier->aiData.bAction));

		if (pSoldier->aiData.bAction == AI_ACTION_PICKUP_ITEM &&
			pSoldier->aiData.usActionData != pSoldier->sGridNo &&
			(CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || !SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("skip dangerous spot"));
			pSoldier->aiData.bAction = AI_ACTION_NONE;
		}

		if(pSoldier->aiData.bAction != AI_ACTION_NONE)
		{
			return( pSoldier->aiData.bAction );
		}
	}

	// sevenfm: also pick up ammo
	if( (pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM ) &&
		ubCanMove &&
		SoldierAI(pSoldier) &&
		!( pSoldier->usSoldierFlagMask & SOLDIER_SEARCH_ITEMS ) &&
		!InWaterOrGas(pSoldier, pSoldier->sGridNo) &&
		!pSoldier->aiData.bUnderFire &&
		bGunSlot != NO_SLOT &&
		FindAmmoToReload(pSoldier, bGunSlot, NO_SLOT) == NO_SLOT )
	{
		// limit searching to 1 time per turn
		pSoldier->usSoldierFlagMask |= SOLDIER_SEARCH_ITEMS;

		// look around for ammo
		DebugAI( AI_MSG_INFO, pSoldier, String("search ammo"));
		pSoldier->aiData.bAction = SearchForItems( pSoldier, SEARCH_AMMO, pSoldier->inv[bGunSlot].usItem );

		if (pSoldier->aiData.bAction == AI_ACTION_PICKUP_ITEM &&
			pSoldier->aiData.usActionData != pSoldier->sGridNo &&
			(CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || !SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("skip dangerous spot"));
			pSoldier->aiData.bAction = AI_ACTION_NONE;
		}

		if (pSoldier->aiData.bAction != AI_ACTION_NONE)
		{
			return( pSoldier->aiData.bAction );
		}
	}

	////////////////////////////////////////////////////////////////////////
	// Red Retreat
	////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[retreat]"));
	if( gfTurnBasedAI &&
		!fCivilian &&
		!bInWater &&
		ubCanMove &&
		SoldierAI(pSoldier) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->RetreatActive() &&
		(pSoldier->CheckInitialAP() || !fAnyCover || AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE)))
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("search for retreat spot"));
		INT32 sRetreatSpot = FindRetreatSpot(pSoldier);

		if( !TileIsOutOfBounds(sRetreatSpot) )
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("found retreat spot %d", sRetreatSpot));

			//BeginMultiPurposeLocator(sRetreatSpot, pSoldier->pathing.bLevel, FALSE);

			pSoldier->aiData.usActionData = sRetreatSpot;

			// alt movement mode for retreat
			if (DecideRetreatReverse(pSoldier, sClosestThreat, bClosestThreatLevel))
				return pSoldier->aiData.bAction;

			return AI_ACTION_TAKE_COVER;
		}
		else if (pSoldier->CheckInitialAP() && 
				(pSoldier->bTeam == MILITIA_TEAM || !fAnyCover && (pSoldier->InShock() || pSoldier->aiData.bWoundShock > 0)) && 
				pSoldier->bActionPoints > APBPConstants[AP_MAXIMUM] / 2)
			// (pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("couldn't find retreat spot, try to run away"));
			// look for best place to RUN AWAY to (farthest from the closest threat)
			pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				return AI_ACTION_RUN_AWAY;
			}
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[check to stop retreating]"));
	// stop retreating if current spot is safe
	if (SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		pSoldier->RetreatActive() &&
		pSoldier->RetreatValue() < RETREAT_DURATION)
	{
		if (!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE) &&
			SpotDangerLevel(pSoldier, pSoldier->sGridNo) < 2 &&			
			AnyCoverAtSpot(pSoldier, pSoldier->sGridNo) &&
			SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("current spot is safe, stop retreating"));
			pSoldier->StopTacticalOrder();
		}
	}

	////////////////////////////////////////////////////////////////////////
	// DRINK IF LOW IN BREATH
	////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[drink if low on breath]"));
	if (pSoldier->CheckInitialAP() &&
		!bInWater && 
		(fSafeSpot || pSoldier->bBreath < OKBREATH) &&
		pSoldier->IsMercBodyType() &&
		(pSoldier->bBreath < pSoldier->bBreathMax * 3 / 4 || pSoldier->bBreath < OKBREATH) &&
		(Chance((100 - 100 * pSoldier->bBreath / pSoldier->bBreathMax) * (100 - 100 * pSoldier->bBreath / pSoldier->bBreathMax) / 100) || pSoldier->bBreath < OKBREATH) &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_USED_CANTEEN) &&
		IsActionAffordable(pSoldier, AI_ACTION_DRINK_CANTEEN) &&
		FindCanteen(pSoldier) != NO_SLOT)
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("drink from canteen"));
		return AI_ACTION_DRINK_CANTEEN;
	}

	////////////////////////////////////////////////////////////////////////
	// CROUCH & REST IF RUNNING OUT OF BREATH
	////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[crouch and rest if low on breath]"));

	// if our breath is running a bit low, and we're not in water or under fire
	if (pSoldier->bBreath < OKBREATH * 2 && !bInWater && !pSoldier->IsBoxer())
	{
		DebugAI(AI_MSG_TOPIC, pSoldier, String("soldier is low on breath!"));
		if (fSafeSpot)
		{
			// rest here
			if (!TileIsOutOfBounds(sClosestThreat) &&
				PythSpacesAway(pSoldier->sGridNo, sClosestThreat) < DAY_VISION_RANGE)
			{
				ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestThreat);

				// if soldier is not already facing in that direction,
				// and the opponent is close enough that he could possibly be seen
				if (pSoldier->ubDirection != ubOpponentDir &&
					pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
					pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
				{
					// turn
					pSoldier->aiData.usActionData = ubOpponentDir;

					DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest threat before resting"));
					return(AI_ACTION_CHANGE_FACING);
				}
			}

			// if not already crouched, try to crouch down first
			if (!fCivilian &&
				!PTR_CROUCHED &&
				!pSoldier->IsBoxer() &&
				IsValidStance(pSoldier, ANIM_CROUCH) &&
				gAnimControl[pSoldier->usAnimState].ubEndHeight > ANIM_CROUCH)
			{
				if (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
				{
					pSoldier->aiData.usActionData = ANIM_CROUCH;
					DebugAI(AI_MSG_TOPIC, pSoldier, String("crouch first"));
					return(AI_ACTION_CHANGE_STANCE);
				}
			}

			pSoldier->aiData.usActionData = NOWHERE;

			// if under fire or in dangerous place - go prone
			if (!fCivilian &&
				!pSoldier->IsBoxer() &&
				IsValidStance(pSoldier, ANIM_PRONE) &&
				gAnimControl[pSoldier->usAnimState].ubEndHeight > ANIM_PRONE &&
				(pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier) || !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE)))
			{
				if (GetAPsToChangeStance(pSoldier, ANIM_PRONE) <= pSoldier->bActionPoints)
				{
					pSoldier->aiData.usActionData = ANIM_PRONE;
					DebugAI(AI_MSG_TOPIC, pSoldier, String("go prone first"));
					return(AI_ACTION_CHANGE_STANCE);
				}
			}

			// is it a heavy gun? And we have energy cost for shooting enabled? 
			if (WeaponReady(pSoldier) &&
				GetBPCostPer10APsForGunHolding(pSoldier) > 0 &&
				PickSoldierReadyAnimation(pSoldier, TRUE, FALSE) != INVALID_ANIMATION)
			{
				// unready
				DebugAI(AI_MSG_INFO, pSoldier, String("lower gun"));
				return(AI_ACTION_LOWER_GUN);
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("wait to restore breath"));
			return(AI_ACTION_NONE);
		}
		else
		{
			// try to retreat to safe position
			if (pSoldier->aiData.bOrders != STATIONARY && pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE && Chance(30))
			{
				pSoldier->RetreatStart(RETREAT_DURATION);
				DebugAI(AI_MSG_INFO, pSoldier, String("start retreat to rest"));
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	// USE DRUGS
	////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[use drugs]"));

	if (pSoldier->CheckInitialAP() &&
		!bInWater &&
		fSafeSpot &&
		pSoldier->IsMercBodyType() &&
		SoldierAI(pSoldier) &&
		!pSoldier->IsFlanking() &&
		IsActionAffordable(pSoldier, AI_ACTION_USE_DRUG))
	{
		// alcohol to reduce shock level
		if (pSoldier->ShockLevelPercent() > 25 &&
			!AIRoleSniper(pSoldier) &&
			!AIRoleMarksman(pSoldier) &&
			(!MercUnderTheInfluence(pSoldier) || MercUnderTheInfluence(pSoldier, DRUG_TYPE_ALCOHOL)) &&
			Chance(100 / (1 + GetDrunkLevel(pSoldier))) &&
			Chance(pSoldier->ShockLevelPercent() * pSoldier->ShockLevelPercent() / 100) &&
			FindDrug(pSoldier, DRUG_ALCOHOL))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("under suppression, use alcohol"));
			pSoldier->aiData.usActionData = DRUG_ALCOHOL;
			return AI_ACTION_USE_DRUG;
		}

		// combat drugs to prepare rush attack
		if (pSoldier->RushAttackPrepare() &&
			(!MercUnderTheInfluence(pSoldier) || MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE)))
		{
			if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_RESISTANCE) && FindDrug(pSoldier, DRUG_DAMAGERESISTANCE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use damage resistance drug"));
				pSoldier->aiData.usActionData = DRUG_DAMAGERESISTANCE;
				return AI_ACTION_USE_DRUG;
			}
			if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_AGILITY) && FindDrug(pSoldier, DRUG_AGILITY))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use agility drug"));
				pSoldier->aiData.usActionData = DRUG_AGILITY;
				return AI_ACTION_USE_DRUG;
			}
			if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_REGENERATION) && pSoldier->stats.bLife < pSoldier->stats.bLifeMax && FindDrug(pSoldier, DRUG_REGENERATION))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use regeneration drug"));
				pSoldier->aiData.usActionData = DRUG_REGENERATION;
				return AI_ACTION_USE_DRUG;
			}
		}

		// regeneration if seriously wounded
		if (pSoldier->stats.bLife < pSoldier->stats.bLifeMax * 3 / 4 &&
			pSoldier->bRegenBoostersUsedToday < 3 &&
			!MercUnderTheInfluence(pSoldier, DRUG_TYPE_REGENERATION) &&
			Chance(100 - 100 * (pSoldier->stats.bLife * pSoldier->stats.bLife) / (pSoldier->stats.bLifeMax * pSoldier->stats.bLifeMax)) &&
			FindDrug(pSoldier, DRUG_REGENERATION))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use regeneration drug"));
			pSoldier->aiData.usActionData = DRUG_REGENERATION;
			return AI_ACTION_USE_DRUG;
		}

		// sniper/marksman can use wisdom drug
		if ((AIRoleSniper(pSoldier) || AIRoleMarksman(pSoldier)) &&
			!pSoldier->aiData.bLastAttackHit &&
			!pSoldier->bBlindedCounter &&
			GuySawEnemy(pSoldier, SEEN_LAST_TURN) &&
			!MercUnderTheInfluence(pSoldier, DRUG_TYPE_WISDOM) &&
			Chance(10) &&
			FindDrug(pSoldier, DRUG_WISDOM))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("sniper/marksman, use wisdom drug"));
			pSoldier->aiData.usActionData = DRUG_WISDOM;
			return AI_ACTION_USE_DRUG;
		}

		// sniper/marksman can use vision drug if under attack and cannot see enemy
		if ((AIRoleSniper(pSoldier) || AIRoleMarksman(pSoldier)) &&
			pSoldier->aiData.bUnderFire &&
			!pSoldier->bBlindedCounter &&
			!GuySawEnemy(pSoldier) &&
			!MercUnderTheInfluence(pSoldier, DRUG_TYPE_VISION) &&
			WeaponReady(pSoldier) &&
			AIGunScoped(pSoldier) &&
			Chance(10) &&
			FindDrug(pSoldier, DRUG_VISION))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("sniper/marksman, use vision drug"));
			pSoldier->aiData.usActionData = DRUG_VISION;
			return AI_ACTION_USE_DRUG;
		}

		// adrenaline when need to get closer to enemy
		if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
			pSoldier->aiData.bAIMorale == MORALE_FEARLESS &&
			!pSoldier->aiData.bLastAttackHit &&
			!AISpecialRole(pSoldier) &&
			GuySawEnemy(pSoldier, SEEN_LAST_TURN) &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < TACTICAL_RANGE * 2 &&
			Chance(10 * CountSeenEnemiesLastTurn(pSoldier) + 10 * pSoldier->aiData.bUnderFire + pSoldier->ShockLevelPercent() / 2) &&
			FindDrug(pSoldier, DRUG_ADRENALINE) &&
			(!AICheckHasGun(pSoldier) || AICheckShortWeaponRange(pSoldier) || pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR))
		{
			BOOLEAN fClimbingNecessary;
			INT32 sClimbGridNo;
			INT32 sPathCost = EstimatePathCostToLocation(pSoldier, sClosestDisturbance, bClosestDisturbanceLevel, FALSE, &fClimbingNecessary, &sClimbGridNo);

			if (sPathCost != 0 && sPathCost < APBPConstants[AP_MAXIMUM] * 2)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("need to get closer to enemy, use adrenaline drug"));
				pSoldier->aiData.usActionData = DRUG_ADRENALINE;
				return AI_ACTION_USE_DRUG;
			}
		}

		// adrenaline when cowering/in high shock
		if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
			pSoldier->ShockLevelPercent() > 75 &&
			!AICheckSuccessfulAttack(pSoldier, TRUE) &&			
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < TACTICAL_RANGE * 2 &&
			Chance(10 * CountSeenEnemiesLastTurn(pSoldier) + 10 * pSoldier->aiData.bUnderFire + pSoldier->ShockLevelPercent() / 2) &&
			FindDrug(pSoldier, DRUG_ADRENALINE) &&
			(!AnyCoverAtSpot(pSoldier, TRUE) && !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE) || AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, FALSE)))
		{
			BOOLEAN fClimbingNecessary;
			INT32 sClimbGridNo;
			INT32 sPathCost = EstimatePathCostToLocation(pSoldier, sClosestDisturbance, bClosestDisturbanceLevel, FALSE, &fClimbingNecessary, &sClimbGridNo);

			if (sPathCost != 0 && sPathCost < APBPConstants[AP_MAXIMUM] * 2)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("cowering/high shock, use adrenaline drug"));
				pSoldier->aiData.usActionData = DRUG_ADRENALINE;
				return AI_ACTION_USE_DRUG;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	// IF POSSIBLE, FIRE LONG RANGE WEAPONS AT TARGETS REPORTED BY RADIO
	////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[sniper fire/use mortar]"));

	// can't do this in realtime, because the player could be shooting a gun or whatever at the same time!
	if( gfTurnBasedAI &&
		!fCivilian &&
		!bInWater &&
		!bInGas &&
		!pSoldier->IsFlanking() &&
		!pSoldier->IsBoxer())
	{
		if ((pSoldier->usSoldierFlagMask2 & SOLDIER_ALLOW_RED_SHOOTING_CODE) && (pSoldier->CheckInitialAP() || AllowComplexAI(pSoldier)))
		{
			if (pSoldier->CheckInitialAP())
				DebugAI(AI_MSG_INFO, pSoldier, String("new turn, allow Red shooting code"));
			else if (pSoldier->usSoldierFlagMask2 & SOLDIER_ALLOW_RED_SHOOTING_CODE)
				DebugAI(AI_MSG_INFO, pSoldier, String("soldier at new spot, allow Red shooting code"));

			// reset flag until new turn starts or soldier moves to new tile
			pSoldier->usSoldierFlagMask2 &= ~SOLDIER_ALLOW_RED_SHOOTING_CODE;

			DebugAI(AI_MSG_TOPIC, pSoldier, String("[mortar/grenade launcher]"));
			BestThrow.ubPossible = FALSE;    // by default, assume Throwing isn't possible
			CheckIfTossPossible(pSoldier, &BestThrow);

			if (BestThrow.ubPossible)
				DebugAI(AI_MSG_INFO, pSoldier, String("throw possible"));
			else
				DebugAI(AI_MSG_INFO, pSoldier, String("throw not possible"));

			if (BestThrow.ubPossible)
			{
				// sevenfm: allow using mortars, grenade launchers, flares and non lethal grenades in RED state
				if (Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].mortar ||
					//Item[pSoldier->inv[ BestThrow.bWeaponIn ].usItem].cannon ||
					Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].rocketlauncher ||
					Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].grenadelauncher ||
					Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].flare ||
					Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].usItemClass & IC_GRENADE)
				{
					// if firing mortar make sure we have room
					if (Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].mortar)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("using mortar, check room to deploy"));

						ubOpponentDir = AIDirection(pSoldier->sGridNo, BestThrow.sTarget);

						// Get new gridno!
						sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(ubOpponentDir));

						if (!OKFallDirection(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, ubOpponentDir, pSoldier->usAnimState))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("no room to deploy mortar, check if we can move behind"));							

							// try behind us, see if there's room to move back
							sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(gOppositeDirection[ubOpponentDir]));
							if (OKFallDirection(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, gOppositeDirection[ubOpponentDir], pSoldier->usAnimState))
							{
								// sevenfm: check if we can reach this gridno
								BOOLEAN bReverse = FALSE;
								if (pSoldier->ubDirection == ubOpponentDir || pSoldier->ubDirection == gOneCDirection[ubOpponentDir] || pSoldier->ubDirection == gOneCCDirection[ubOpponentDir])
									bReverse = TRUE;
								INT32 iPathCost = EstimatePlotPath(pSoldier, sCheckGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_GET_CLOSER), pSoldier->bStealthMode, bReverse, 0);
								if (iPathCost != 0 && iPathCost + BestThrow.ubAPCost + GetAPsToLook(pSoldier) + GetAPsCrouch(pSoldier, FALSE) <= pSoldier->bActionPoints)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("moving backwards to have more room to deploy mortar, reverse mode: %d", bReverse));
									pSoldier->aiData.usActionData = sCheckGridNo;
									pSoldier->bReverse = bReverse;

									DebugAI(AI_MSG_INFO, pSoldier, String("prepare next action throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

									// if necessary, swap the usItem
									if (BestThrow.bWeaponIn != HANDPOS)
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
										RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
									}

									pSoldier->aiData.usNextActionData = BestThrow.sTarget;
									pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

									pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;

									return AI_ACTION_GET_CLOSER;
								}
							}

							// can't fire!
							BestThrow.ubPossible = FALSE;
						}
					}

					// if still possible
					if (BestThrow.ubPossible)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

						// if necessary, swap the usItem
						if (BestThrow.bWeaponIn != HANDPOS)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
							RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
						}

						// sevenfm: correctly set weapon mode for attached GL
						if (IsGrenadeLauncherAttached(&pSoldier->inv[HANDPOS]))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("set attached GL mode"));
							pSoldier->bWeaponMode = WM_ATTACHED_GL;
						}

						// stand up before throwing if needed
						if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
							pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
						{
							pSoldier->aiData.usActionData = BestThrow.ubStance;
							pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
							pSoldier->aiData.usNextActionData = BestThrow.sTarget;
							pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
							pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
							return AI_ACTION_CHANGE_STANCE;
						}
						else
						{
							pSoldier->aiData.usActionData = BestThrow.sTarget;
							pSoldier->bTargetLevel = BestThrow.bTargetLevel;
							pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
						}

						return(AI_ACTION_TOSS_PROJECTILE);
					}
				}
			}
			else		// toss/throw/launch not possible
			{
				// WDS - Fix problem when there is no "best thrown" weapon (i.e., BestThrow.bWeaponIn == NO_SLOT)
				// if this dude has a long-range weapon on him (longer than normal
				// sight range), and there's at least one other team-mate around, and
				// spotters haven't already been called for, then DO SO!

				if ((BestThrow.bWeaponIn != NO_SLOT) &&
					(CalcMaxTossRange(pSoldier, pSoldier->inv[BestThrow.bWeaponIn].usItem, TRUE) > MaxNormalDistanceVisible()) &&
					(gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1) &&
					gTacticalStatus.ubSpottersCalledForBy == NOBODY)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("throw not possible, call for spotters!"));

					// then call for spotters!  Uses up the rest of his turn (whatever
					// that may be), but from now on, BLACK AI NPC may radio sightings!
					gTacticalStatus.ubSpottersCalledForBy = pSoldier->ubID;

					pSoldier->aiData.usActionData = NOWHERE;
					return(AI_ACTION_NONE);
				}
			}			

			// sevenfm: moved CanNPCAttack() check here
			if (CanNPCAttack(pSoldier) == TRUE)
			{
				// sevenfm: set bAimShotLocation
				pSoldier->bAimShotLocation = AIM_SHOT_RANDOM;
				CheckIfShotPossible(pSoldier, &BestShot);

				if (BestShot.ubPossible)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("shot is possible, weapon slot %d, target %d, level %d, opponent %d, AP cost %d, Aim time %d, CTH %d, friendly fire %d", BestShot.bWeaponIn, BestShot.sTarget, BestShot.bTargetLevel, BestShot.ubOpponent, BestShot.ubAPCost, BestShot.ubAimTime, BestShot.ubChanceToReallyHit, BestShot.ubFriendlyFireChance));
					if (BestShot.bWeaponIn != NO_SLOT)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("weapon %d ammo %d / %d", pSoldier->inv[BestShot.bWeaponIn].usItem, pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft, GetMagSize(&pSoldier->inv[BestShot.bWeaponIn])));
					}
				}
				else
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("shot not possible"));
				}

				// SNIPER!
				DebugAI(AI_MSG_TOPIC, pSoldier, String("[sniper fire]"));

				if (SoldierAI(pSoldier))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("possible %d, CTH=%d", BestShot.ubPossible, BestShot.ubChanceToReallyHit));

					if (BestShot.ubPossible &&
						BestShot.bWeaponIn != NO_SLOT &&
						BestShot.ubOpponent != NOBODY &&
						MercPtrs[BestShot.ubOpponent] &&
						// check min chance to hit, always allow shooting when under fire
						BestShot.ubChanceToReallyHit > 1 &&												// not suppression fire
						(BestShot.ubChanceToReallyHit > 10 || pSoldier->aiData.bUnderFire) &&			// prevent from shooting with low CTH unless for self defense
						Chance(BestShot.ubChanceToReallyHit + 25 * pSoldier->aiData.bUnderFire) &&		// randomize + bonus when under attack
						// shoot only if we know exact location or can see target after turning
						(PublicKnowledge(pSoldier->bTeam, BestShot.ubOpponent) == SEEN_CURRENTLY ||
						MercPtrs[BestShot.ubOpponent]->sGridNo == BestShot.sTarget && MercPtrs[BestShot.ubOpponent]->pathing.bLevel == BestShot.bTargetLevel ||
						LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS)))		// can see target after turning
					{
						// then do it! The functions have already made sure that we have a
						// pair of worthy opponents, etc., so we're not just wasting our time

						// if necessary, swap the usItem from holster into the hand position
						DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "decideactionred: sniper shot possible!");

						if (BestShot.bWeaponIn != HANDPOS)
							RearrangePocket(pSoldier, HANDPOS, BestShot.bWeaponIn, FOREVER);

						if (AIRoleSniper(pSoldier))
						{
							ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_SNIPER]);
						}

						//pSoldier->aiData.usActionData	= BestShot.sTarget;
						pSoldier->bTargetLevel = BestShot.bTargetLevel;
						pSoldier->aiData.bAimTime = BestShot.ubAimTime;
						pSoldier->bScopeMode = BestShot.bScopeMode;
						pSoldier->bDoAutofire = 0;
						pSoldier->bDoBurst = 0;

						INT16 sReserveAP = APBPConstants[AP_PRONE] + APBPConstants[AP_LOOK_CROUCHED];
						// reserve APs to hide if no cover or enemy is close
						if (Chance(SoldierDifficultyLevel(pSoldier) * 25) && 
							(!AnyCoverAtSpot(pSoldier, pSoldier->sGridNo) || PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) < DAY_VISION_RANGE / 2))
						{
							sReserveAP = APBPConstants[AP_MINIMUM] / 2;
						}

						// sevenfm: try to reserve AP by lowering aim time if needed
						INT16 sActualAimAP = CalcAPCostForAiming(pSoldier, BestShot.sTarget, (INT8)pSoldier->aiData.bAimTime);
						while (pSoldier->aiData.bAimTime > 0 && pSoldier->bActionPoints < BestShot.ubAPCost + sActualAimAP + sReserveAP)
						{
							pSoldier->aiData.bAimTime--;
							sActualAimAP = CalcAPCostForAiming(pSoldier, BestShot.sTarget, (INT8)pSoldier->aiData.bAimTime);
							DebugAI(AI_MSG_INFO, pSoldier, String("Lower aim to %d to allow reserving AP, new Aim AP cost=%d", pSoldier->aiData.bAimTime, sActualAimAP));
						}

						if (gAnimControl[pSoldier->usAnimState].ubEndHeight != BestShot.ubStance  &&
							IsValidStance(pSoldier, BestShot.ubStance))
						{
							pSoldier->aiData.bNextAction = AI_ACTION_FIRE_GUN;
							pSoldier->aiData.usNextActionData = BestShot.sTarget;
							pSoldier->aiData.bNextTargetLevel = BestShot.bTargetLevel;
							pSoldier->aiData.usActionData = BestShot.ubStance;

							DebugAI(AI_MSG_INFO, pSoldier, String("Change stance before shooting"));
							return(AI_ACTION_CHANGE_STANCE);
						}
						else
						{
							pSoldier->aiData.usActionData = BestShot.sTarget;
							return(AI_ACTION_FIRE_GUN);
						}
					}
				}

				// if this dude has a long-range weapon on him (longer than normal
				// sight range), and there's at least one other team-mate around, and
				// spotters haven't already been called for, then DO SO!

				if (SoldierAI(pSoldier) &&
					AIRoleSniper(pSoldier) &&
					(gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1) &&
					gTacticalStatus.ubSpottersCalledForBy == NOBODY &&
					IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT))
				{
					// then call for spotters!  Uses up the rest of his turn (whatever
					// that may be), but from now on, BLACK AI NPC may radio sightings!
					gTacticalStatus.ubSpottersCalledForBy = pSoldier->ubID;
					DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "decideactionred: calling for sniper spotters");
					pSoldier->aiData.usActionData = NOWHERE;
					//return(AI_ACTION_NONE);
					return AI_ACTION_FAKE_ALERT;
				}

				//RELOADING
				DebugAI(AI_MSG_TOPIC, pSoldier, String("[reload before checking suppression fire]"));

				// WarmSteel - Because of suppression fire, we need enough ammo to even consider suppressing
				// This means we need to reload. Also reload if we're just plainly low on bullets.
				if (BestShot.bWeaponIn != NO_SLOT &&
					pSoldier->bActionPoints > APBPConstants[AP_MINIMUM] &&
					(fSafeSpot || !fDangerousSpot && !GuySawEnemy(pSoldier, SEEN_LAST_TURN) || AIRoleMachinegunner(pSoldier) && Chance(50) || Chance(20)) &&
					!pSoldier->RushAttackAdvance() &&
					IsGunAutofireCapable(&pSoldier->inv[BestShot.bWeaponIn]) &&
					Weapon[pSoldier->inv[BestShot.bWeaponIn].usItem].swapClips &&
					pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft < gGameExternalOptions.ubAISuppressionMinimumAmmo &&
					GetMagSize(&pSoldier->inv[BestShot.bWeaponIn]) >= gGameExternalOptions.ubAISuppressionMinimumMagSize)
					//|| pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft < (UINT8)(GetMagSize(&pSoldier->inv[BestShot.bWeaponIn]) / 4)))
				{
					// HEADROCK HAM 5: Fixed an issue where no ammo was found, leading to a crash when overloading the
					// inventory vector (bAmmoSlot = -1...)
					INT8 bAmmoSlot = FindAmmoToReload(pSoldier, BestShot.bWeaponIn, NO_SLOT);
					if (bAmmoSlot > -1)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("found ammo slot %d item %d for weapon in slot %d item %d", bAmmoSlot, pSoldier->inv[bAmmoSlot].usItem, BestShot.bWeaponIn, pSoldier->inv[BestShot.bWeaponIn].usItem));
						OBJECTTYPE * pAmmo = &(pSoldier->inv[bAmmoSlot]);
						if ((*pAmmo)[0]->data.ubShotsLeft > pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft && 
							GetAPsToReloadGunWithAmmo(pSoldier, &(pSoldier->inv[BestShot.bWeaponIn]), pAmmo) <= (INT16)pSoldier->bActionPoints)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("new ammo %d remaining ammo %d", (*pAmmo)[0]->data.ubShotsLeft, pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft));
							DebugAI(AI_MSG_INFO, pSoldier, String("reload before checking suppression fire"));
							pSoldier->aiData.usActionData = BestShot.bWeaponIn;
							return AI_ACTION_RELOAD_GUN;
						}
					}
				}

				//SUPPRESSION FIRE
				DebugAI(AI_MSG_TOPIC, pSoldier, String("[suppression fire]"));

				BOOLEAN fExtraSuppression = FALSE;
				BOOLEAN fFullmagazine = FALSE;
				BOOLEAN fBigMagazine = FALSE;
				BOOLEAN fSpareMagazine = FALSE;

				if (pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft == GetMagSize(&pSoldier->inv[BestShot.bWeaponIn]))
					fFullmagazine = TRUE;

				if (pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft >= gGameExternalOptions.ubAISuppressionMinimumMagSize)
					fBigMagazine = TRUE;

				if (FindAmmoToReload(pSoldier, BestShot.bWeaponIn, NO_SLOT) != NO_SLOT)
					fSpareMagazine = TRUE;

				if (BestShot.ubPossible &&
					BestShot.bWeaponIn != NO_SLOT &&
					!TileIsOutOfBounds(BestShot.sTarget) &&
					BestShot.ubOpponent != NOBODY &&
					MercPtrs[BestShot.ubOpponent] &&
					!MercPtrs[BestShot.ubOpponent]->IsCowering() &&
					!MercPtrs[BestShot.ubOpponent]->IsUnconscious() &&
					AISuppressionEffectiveness() > 0 &&

					(pSoldier->RushAttackPrepare() ||
					MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) ||
					fSectorAttack && AIRoleMachinegunner(pSoldier) ||

					// no successful attack
					pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
					!pSoldier->bStealthMode &&
					(fSpareMagazine || fFullmagazine || fBigMagazine) &&
					!TileIsOutOfBounds(sClosestOpponent) && PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 2 && (PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < VISION_RANGE + TACTICAL_RANGE / 2 || AIRoleMachinegunner(pSoldier)) &&
					(!AICheckSuccessfulAttack(pSoldier, TRUE) ||
					CountFriendsBlack(pSoldier, sClosestOpponent) == 0 ||
					pSoldier->StopSeekValue() > 0 && CountFriendsFlankSameSpot(pSoldier, sClosestOpponent) > 0 ||
					CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE/ 2) > 0)))
				{
					fExtraSuppression = TRUE;
				}

				// suppression fire
				if (BestShot.ubPossible &&
					BestShot.bWeaponIn != NO_SLOT &&
					!TileIsOutOfBounds(BestShot.sTarget) &&
					// check valid opponent for suppression
					BestShot.ubOpponent != NOBODY &&
					MercPtrs[BestShot.ubOpponent] &&

					// limit suppression when sneaking or flanking
					(!pSoldier->bStealthMode || pSoldier->aiData.bUnderFire || pSoldier->StopSeekValue() > 0 || fExtraSuppression || CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE / 2) > 0) &&
					!pSoldier->IsFlanking() &&

					// chance depends on suppression effectiveness, shoot 100% at effectiveness = 100
					(Chance(BestShot.ubChanceToReallyHit) || Chance(AISuppressionEffectiveness()) || AIRoleMachinegunner(pSoldier) && AISuppressionEffectiveness() > 0 || fExtraSuppression) &&

					// check weapon capable of autofire and meets requirements for suppression fire
					IsGunAutofireCapable(&pSoldier->inv[BestShot.bWeaponIn]) &&
					GetMagSize(&pSoldier->inv[BestShot.bWeaponIn]) >= gGameExternalOptions.ubAISuppressionMinimumMagSize &&
					pSoldier->inv[BestShot.bWeaponIn][0]->data.gun.ubGunShotsLeft >= gGameExternalOptions.ubAISuppressionMinimumAmmo &&
					// check remaining ammo
					(fSpareMagazine || fBigMagazine || fExtraSuppression) &&

					// check overheated gun
					(!gGameExternalOptions.fWeaponOverheating || GetGunOverheatDisplayPercentage(&pSoldier->inv[BestShot.bWeaponIn]) < 1.0f) &&

					// don't shoot when in rush attack
					(!pSoldier->RushAttackAdvance() || AIRoleMachinegunner(pSoldier) || Chance(BestShot.ubChanceToReallyHit)) &&
					// don't shoot when retreating
					pSoldier->RetreatValue() < RETREAT_DURATION &&					
					// check close enemy
					(DistanceToClosestActiveOpponent(pSoldier, pSoldier->sGridNo) > TACTICAL_RANGE / 4 || Chance(BestShot.ubChanceToReallyHit)) &&
					
					// check safe position
					(fAnyCover ||
					pSoldier->pathing.bLevel > 0 && fProneSightCover ||
					!fCanBeSeen && NightLight() && CountFriendsFlankSameSpot(pSoldier, sClosestThreat) && Chance(50) ||
					pSoldier->aiData.bUnderFire && (pSoldier->ubPreviousAttackerID == BestShot.ubOpponent || pSoldier->ubNextToPreviousAttackerID == BestShot.ubOpponent || MercPtrs[BestShot.ubOpponent]->sLastTarget == pSoldier->sGridNo) ||	// return fire
					fExtraSuppression ||																	// prepare rush
					Chance((BestShot.ubChanceToReallyHit + 100) / 4) ||										// 50% chance to fire without cover
					LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS)) &&	// can see target after turning

					// avoid friendly fire
					BestShot.ubFriendlyFireChance <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE &&
					
					// less chance for SEEKENEMY
					(pSoldier->aiData.bOrders != SEEKENEMY ||
					pSoldier->aiData.bAIMorale < MORALE_FEARLESS ||
					pSoldier->StopSeekValue() > 0 ||
					fExtraSuppression ||
					AIRoleMachinegunner(pSoldier) ||
					pSoldier->aiData.bUnderFire && (pSoldier->ubPreviousAttackerID == BestShot.ubOpponent || pSoldier->ubNextToPreviousAttackerID == BestShot.ubOpponent || MercPtrs[BestShot.ubOpponent]->sLastTarget == pSoldier->sGridNo) ||	// return fire
					LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS) ||									// can see opponent after turning
					Chance((100 + TeamPercentKilled(pSoldier->bTeam)) / 2)) &&

					// don't waste bullets shooting at heard enemy at close range unless we know exact location
					(PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) > TACTICAL_RANGE ||												// far enough to shoot safely
					PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) > TACTICAL_RANGE / 2 && (fFullmagazine || Chance((BestShot.ubChanceToReallyHit + 100) / 4) || fAnyCover || pSoldier->pathing.bLevel > 0 && fProneSightCover || !fCanBeSeen && CountFriendsFlankSameSpot(pSoldier, sClosestThreat) || pSoldier->StopSeekValue() > 0 || pSoldier->RushAttackPrepare()) ||	// 50% chance to shoot if less than safe distance
					// 25% chance if at close range and exact location is known
					Chance(BestShot.ubChanceToReallyHit) && (PublicKnowledge(pSoldier->bTeam, BestShot.ubOpponent) == SEEN_CURRENTLY || MercPtrs[BestShot.ubOpponent]->sGridNo == BestShot.sTarget && MercPtrs[BestShot.ubOpponent]->pathing.bLevel == BestShot.bTargetLevel) ||
					LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS)) &&									// can see target after turning

					// shoot through wall check
					(BestShot.ubChanceToReallyHit >= 1 ||
					LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS) ||									// can see opponent after turning
					//LOS_Virtual(pSoldier, BestShot.sTarget, BestShot.bTargetLevel, ANIM_STAND, LOS_NORMAL, NO_DISTANCE_LIMIT) ||			// can suppress opponent
					LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, BestShot.sTarget, BestShot.bTargetLevel, NO_DISTANCE_LIMIT)) &&	// have standing LOS

					// reduce chance to shoot if target is beyond weapon range
					(fAnyCover ||
					pSoldier->pathing.bLevel > 0 && fProneSightCover ||
					//!fCanBeSeen && CountFriendsFlankSameSpot(pSoldier, sClosestThreat) ||
					pSoldier->StopSeekValue() > 0 ||
					fExtraSuppression ||												// prepare rush
					!AICheckSuccessfulAttack(pSoldier, TRUE) && CountCorpses(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE, FALSE) > 0 ||
					pSoldier->aiData.bUnderFire && (pSoldier->ubPreviousAttackerID == BestShot.ubOpponent || pSoldier->ubNextToPreviousAttackerID == BestShot.ubOpponent || MercPtrs[BestShot.ubOpponent]->sLastTarget == pSoldier->sGridNo) ||	// return fire
					Chance(100 * (GunRange(&pSoldier->inv[BestShot.bWeaponIn], pSoldier) / CELL_X_SIZE) / PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget)))

					//CheckSuppressionDirection( pSoldier, Menptr[BestShot.ubOpponent].sGridNo, Menptr[BestShot.ubOpponent].pathing.bLevel ) &&
					)
				{
					// then do it!
					DebugAI(AI_MSG_INFO, pSoldier, String("suppression fire possible! target %d level %d aim %d", BestShot.sTarget, BestShot.bTargetLevel, BestShot.ubAimTime));

					// if necessary, swap the usItem from holster into the hand position
					if (BestShot.bWeaponIn != HANDPOS)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
						RearrangePocket(pSoldier, HANDPOS, BestShot.bWeaponIn, FOREVER);
						// sevenfm: correct bWeaponIn for possible use later
						BestShot.bWeaponIn = HANDPOS;
					}

					pSoldier->bTargetLevel = BestShot.bTargetLevel;
					pSoldier->bDoAutofire = 0;
					pSoldier->bDoBurst = 1;
					pSoldier->bAimShotLocation = AIM_SHOT_RANDOM;
					// new code by Asdow
					OBJECTTYPE &weapon = pSoldier->inv[BestShot.bWeaponIn];
					UINT16 ubRemainingAmmo = weapon[0]->data.gun.ubGunShotsLeft;

					INT16 ubBurstAPs = 0;
					FLOAT dTotalRecoil = 0;
					INT32 sActualAimAP;
					UINT8 ubAutoPenalty;
					UINT8 ubMinAuto = 5;
					INT16 sReserveAP = APBPConstants[AP_PRONE] + APBPConstants[AP_LOOK_CROUCHED];
					INT8 bAmmoSlot = FindAmmoToReload(pSoldier, HANDPOS, NO_SLOT);
					BOOLEAN fSafeDistance = (PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) >= TACTICAL_RANGE / 2);

					// possible remove aim
					if (BestShot.ubAimTime > 0 &&
						!UsingNewCTHSystem() &&
						(Chance((100 - BestShot.ubChanceToReallyHit) * (100 - BestShot.ubChanceToReallyHit) / 100) || pSoldier->RushAttackPrepare()))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("set ubAimTime = 0 for OCTH suppression"));
						BestShot.ubAimTime = 0;
					}

					// possibly use alt mode
					if (pSoldier->IsMercBodyType() &&
						pSoldier->RushAttackPrepare() && 
						!AIRoleMachinegunner(pSoldier) &&
						BestShot.bScopeMode != USE_ALT_WEAPON_HOLD &&
						gGameExternalOptions.ubAllowAlternativeWeaponHolding == 3 &&
						pSoldier->AnimEndHeight() > ANIM_PRONE &&
						IsValidStance(pSoldier, ANIM_STAND) &&
						PickSoldierReadyAnimation(pSoldier, FALSE, TRUE) != INVALID_ANIMATION &&
						Item[pSoldier->inv[HANDPOS].usItem].twohanded &&
						(!WeaponReady(pSoldier) || BestShot.ubStance != pSoldier->AnimEndHeight()) &&
						SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("set ubAimTime = 0, set bScopeMode = USE_ALT_WEAPON_HOLD for suppression when preparing for rush attack"));
						BestShot.ubAimTime = 0;
						BestShot.bScopeMode = USE_ALT_WEAPON_HOLD;
						BestShot.ubStance = ANIM_STAND;
					}

					pSoldier->bScopeMode = BestShot.bScopeMode;
					pSoldier->aiData.bAimTime = BestShot.ubAimTime;

					// reserve APs to hide if no cover or enemy is close
					if (!fAnyCover || !fSafeDistance)
					{
						sReserveAP = APBPConstants[AP_MINIMUM] / 2;
					}

					if (fSafeDistance &&
						((fAnyCover || InARoom(pSoldier->sGridNo) && pSoldier->pathing.bLevel == 0) &&
						fProneSightCover ||
						pSoldier->RushAttackPrepare()))
					{
						sReserveAP = 0;
					}

					if (PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) > DAY_VISION_RANGE || 
						fAnyCover && fSafeDistance ||
						pSoldier->aiData.bUnderFire && fSafeDistance)
					{
						ubMinAuto *= 2;
					}

					sActualAimAP = CalcAPCostForAiming(pSoldier, BestShot.sTarget, (INT8)pSoldier->aiData.bAimTime);

					if (UsingNewCTHSystem())
					{
						do
						{
							pSoldier->bDoAutofire++;
							dTotalRecoil += AICalcRecoilForShot(pSoldier, &weapon, pSoldier->bDoAutofire);
							ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &weapon, pSoldier->bDoAutofire, pSoldier);
						} while (pSoldier->bActionPoints >= BestShot.ubAPCost + sActualAimAP + ubBurstAPs + sReserveAP &&
							ubRemainingAmmo >= pSoldier->bDoAutofire &&
							pSoldier->bDoAutofire <= 30 &&
							(dTotalRecoil <= 20.0f ||
							pSoldier->bDoAutofire < ubMinAuto || 
							MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) || 
							pSoldier->RushAttackPrepare() || 
							fAnyCover && Weapon[weapon.usItem].ubWeaponType == GUN_LMG));
					}
					else
					{
						ubAutoPenalty = GetAutoPenalty(pSoldier, &weapon, pSoldier->AnimEndHeight() == ANIM_PRONE);
						do
						{
							pSoldier->bDoAutofire++;
							ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &weapon, pSoldier->bDoAutofire, pSoldier);
						} while (pSoldier->bActionPoints >= BestShot.ubAPCost + sActualAimAP + ubBurstAPs + sReserveAP &&
							ubRemainingAmmo >= pSoldier->bDoAutofire &&
							pSoldier->bDoAutofire <= 30 &&
							(ubAutoPenalty * pSoldier->bDoAutofire <= 80 || 
							pSoldier->bDoAutofire < ubMinAuto || 
							MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) || 
							pSoldier->RushAttackPrepare() || 
							fAnyCover && Weapon[weapon.usItem].ubWeaponType == GUN_LMG ||
							fAnyCover && bAmmoSlot != NO_SLOT));
					}

					pSoldier->bDoAutofire--;

					// Make sure we decided to fire at least one shot!
					ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &weapon, pSoldier->bDoAutofire, pSoldier);
					DebugAI(AI_MSG_INFO, pSoldier, String("autofire shots %d APcost %d burst AP %d aimtime %d reserve AP %d", pSoldier->bDoAutofire, BestShot.ubAPCost, ubBurstAPs, sActualAimAP, sReserveAP));

					// minimum 3 bullets
					if (pSoldier->bDoAutofire >= 3 && pSoldier->bActionPoints >= BestShot.ubAPCost + sActualAimAP + ubBurstAPs + sReserveAP)
					{
						// try to shoot higher to avoid hitting cover
						SOLDIERTYPE *pTargetSoldier = SimpleFindSoldier(BestShot.sTarget, BestShot.bTargetLevel);
						if (!pTargetSoldier)
						{
							// if shooting at empty tile, increase bTargetCubeLevel
							pSoldier->bTargetCubeLevel = 1;

							if (SoldierToLocationChanceToGetThrough(pSoldier, BestShot.sTarget, BestShot.bTargetLevel, pSoldier->bTargetCubeLevel, NOBODY) < OK_CHANCE_TO_GET_THROUGH)
								pSoldier->bTargetCubeLevel = 2;

							if (SoldierToLocationChanceToGetThrough(pSoldier, BestShot.sTarget, BestShot.bTargetLevel, pSoldier->bTargetCubeLevel, NOBODY) < OK_CHANCE_TO_GET_THROUGH)
								pSoldier->bTargetCubeLevel = 3;
						}
						else
						{
							// if shooting at soldier, try to aim at head
							if (SoldierToSoldierBodyPartChanceToGetThrough(pSoldier, pTargetSoldier, AIM_SHOT_TORSO) < OK_CHANCE_TO_GET_THROUGH &&
								SoldierToSoldierBodyPartChanceToGetThrough(pSoldier, pTargetSoldier, AIM_SHOT_HEAD) > OK_CHANCE_TO_GET_THROUGH &&
								!(pSoldier->IsMaleBodyType() && pTargetSoldier->HasStunningLooks() && PersonalSeenKnowledge(pSoldier, pTargetSoldier->ubID)))
								pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
						}

						if (gAnimControl[pSoldier->usAnimState].ubEndHeight != BestShot.ubStance &&
							IsValidStance(pSoldier, BestShot.ubStance))
						{
							pSoldier->aiData.bNextAction = AI_ACTION_FIRE_GUN;
							pSoldier->aiData.usNextActionData = BestShot.sTarget;
							pSoldier->aiData.bNextTargetLevel = BestShot.bTargetLevel;
							pSoldier->aiData.usActionData = BestShot.ubStance;

							DebugAI(AI_MSG_INFO, pSoldier, String("Change stance before shooting"));

							// show "suppression fire" message only if opponent cannot be seen after turning
							if(!LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS))
								ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_SUPPRESSIONFIRE]);

							return(AI_ACTION_CHANGE_STANCE);
						}
						else
						{
							pSoldier->aiData.usActionData = BestShot.sTarget;

							// show "suppression fire" message only if opponent cannot be seen after turning
							if (!LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS))
								ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_SUPPRESSIONFIRE]);

							return(AI_ACTION_FIRE_GUN);
						}
					}
					else
					{
						pSoldier->bDoBurst = 0;
						pSoldier->bDoAutofire = 0;
					}
				}
				// suppression not possible, do something else
			}		
		}

		// use smoke to cover friend
		DebugAI(AI_MSG_TOPIC, pSoldier, String("[use smoke to cover friend]"));
		if (gfTurnBasedAI &&
			SoldierAI(pSoldier) &&
			!pSoldier->bBlindedCounter &&
			!bInWater &&
			!bInGas &&
			!pSoldier->IsFlanking() &&
			pSoldier->CheckInitialAP() &&
			!pSoldier->aiData.bUnderFire &&
			SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) &&
			!AIRoleSniper(pSoldier) &&
			!AIRoleMachinegunner(pSoldier) &&
			!AIRoleMortarOperator(pSoldier) &&
			Chance(100 - min(100, 10 * CountPublicKnownEnemies(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE))) &&
			!GuySawEnemy(pSoldier, SEEN_LAST_TURN))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("check if we can cover friend with smoke"));

			CheckTossFriendSmoke(pSoldier, &BestThrow);

			if (BestThrow.ubPossible)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

				// start retreating for several turns
				if (BestThrow.ubOpponent != NOBODY && 
					MercPtrs[BestThrow.ubOpponent]->bTacticalOrder == TACTICAL_ORDER_NONE)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("start retreat counter for %d", BestThrow.ubOpponent));
					MercPtrs[BestThrow.ubOpponent]->RetreatStart(RETREAT_DURATION);
				}

				// if necessary, swap the usItem from holster into the hand position
				if (BestThrow.bWeaponIn != HANDPOS)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
					RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
				}

				// stand up before throwing if needed
				if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
					pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
				{
					pSoldier->aiData.usActionData = BestThrow.ubStance;
					pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
					pSoldier->aiData.usNextActionData = BestThrow.sTarget;
					pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
					return AI_ACTION_CHANGE_STANCE;
				}
				else
				{
					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				}

				DebugAI(AI_MSG_INFO, pSoldier, String("throw smoke grenade to cover friend %d at spot %d level %d", BestThrow.ubOpponent, BestThrow.sTarget, BestThrow.bTargetLevel));

				return(AI_ACTION_TOSS_PROJECTILE);
			}
		}
	}

	//RELOADING
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[reload when in safe situation]"));
	if (SoldierAI(pSoldier) &&
		!bInWater &&
		!bInGas &&
		pSoldier->CheckInitialAP() &&
		!pSoldier->RushAttackAdvance() &&
		!pSoldier->IsFlanking() &&
		Chance(25) &&
		fSafeSpot &&
		(!gfTurnBasedAI || !pSoldier->aiData.bUnderFire && !GuySawEnemy(pSoldier) && (TileIsOutOfBounds(sClosestOpponent) || PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE)))
	{
		INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);
		if (bWeaponIn != NO_SLOT &&			
			pSoldier->inv[bWeaponIn][0]->data.gun.ubGunShotsLeft < GetMagSize(&pSoldier->inv[bWeaponIn]) / 2)
		{
			INT8 bAmmoSlot = FindAmmoToReload(pSoldier, bWeaponIn, NO_SLOT);
			if (bAmmoSlot > -1)
			{
				OBJECTTYPE * pAmmo = &(pSoldier->inv[bAmmoSlot]);
				if ((*pAmmo)[0]->data.ubShotsLeft > pSoldier->inv[bWeaponIn][0]->data.gun.ubGunShotsLeft && GetAPsToReloadGunWithAmmo(pSoldier, &(pSoldier->inv[bWeaponIn]), pAmmo) <= (INT16)pSoldier->bActionPoints)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("reload in safe situation"));
					pSoldier->aiData.usActionData = bWeaponIn;
					return AI_ACTION_RELOAD_GUN;
				}
			}
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[trait skills]"));
	// Flugente: trait skills
	// sevenfm: don't use skill when in interrupt
	if (!fCivilian &&
		!bInWater &&
		!bInGas &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->CheckInitialAP() &&
		pSoldier->CanUseRadio(TRUE))
	{
		UINT32 tmp;
		INT32 iArtilleryGridNo = NOWHERE;

		BOOLEAN fJamming = pSoldier->IsJamming();
		BOOLEAN fScanning = pSoldier->IsScanning();
		BOOLEAN fListening = pSoldier->IsListening();
		BOOLEAN fSectorJammed = SectorJammed();
		BOOLEAN fUsingRadio = pSoldier->IsUsingRadio();
		BOOLEAN fTeamSawEnemy = TeamSawEnemyPublic(pSoldier->bTeam, SEEN_3_TURNS_AGO);
		BOOLEAN fPlayerCanCallArtillery = TeamCanOrderArtilleryStrike(gbPlayerNum);
		BOOLEAN fPlayerCanCallReinforcements = MoreFriendsThanEnemiesInNearbysectors(gbPlayerNum, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
		BOOLEAN fPlayerJamming = TeamHasJammingRadioOperator(gbPlayerNum);
		BOOLEAN fPlayerListening = TeamHasListeningRadioOperator(gbPlayerNum);

		DebugAI(AI_MSG_INFO, pSoldier, String("fJamming %d fScanning %d fListening %d fSectorJammed %d fUsingRadio %d fTeamSawEnemy %d fPlayerCanCallArtillery %d fPlayerCanCallReinforcements %d fPlayerJamming %d", fJamming, fScanning, fListening, fSectorJammed, fUsingRadio, fTeamSawEnemy, fPlayerCanCallArtillery, fPlayerCanCallReinforcements, fPlayerJamming));

		BOOLEAN fCanCallReinforcements = !fPlayerJamming;
		BOOLEAN fCanCallArtillery = pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, FALSE) && pSoldier->CanAnyArtilleryStrikeBeOrdered(&tmp);

		BOOLEAN fCanScan = pSoldier->CanUseSkill(SKILLS_RADIO_SCAN_FOR_JAM, FALSE);
		BOOLEAN fCanListen = pSoldier->CanUseSkill(SKILLS_RADIO_LISTEN, FALSE);
		BOOLEAN fCanJam = pSoldier->CanUseSkill(SKILLS_RADIO_JAM, FALSE);
		BOOLEAN fCanTurnOff = pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, FALSE);

		DebugAI(AI_MSG_INFO, pSoldier, String("fCanCallReinforcements %d fCanCallArtillery %d fCanScan %d fCanListen %d fCanJam %d fCanTurnOff %d", fCanCallReinforcements, fCanCallArtillery, fCanScan, fCanListen, fCanJam, fCanTurnOff));

		BOOLEAN fWantCallReinforcements = fCanCallReinforcements && gbWorldSectorZ == 0 && MoreFriendsThanEnemiesInNearbysectors(pSoldier->bTeam, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ);
		BOOLEAN fWantCallArtillery = fCanCallArtillery && gbWorldSectorZ == 0 && !RedSmokeOnMap() && GetBestAoEGridNo(pSoldier, &iArtilleryGridNo, max(1, min(TACTICAL_RANGE, (INT16)gSkillTraitValues.usVOMortarRadius)), 1, 4 - gGameOptions.ubDifficultyLevel / 2, SoldierCondTrue, SoldierCondFalse);

		BOOLEAN fWantScan = fCanScan && fPlayerJamming;
		BOOLEAN fWantJam = fCanJam && gbWorldSectorZ == 0 && !fPlayerJamming && !fWantCallReinforcements && !fWantCallArtillery && (fPlayerCanCallArtillery || fPlayerCanCallReinforcements || fPlayerListening || FindRemoteBombsInSector() && WeAttack(pSoldier->bTeam) && gSkillTraitValues.fVOJammingBlocksRemoteBombs);
		BOOLEAN fWantListen = fCanListen && !fWantJam && !fWantScan && gSkillTraitValues.sVOListeningHearingBonus > 5 && (NightTime() || RainIntensity() > 0 || !fTeamSawEnemy || TileIsOutOfBounds(sClosestOpponent) || gbWorldSectorZ > 0);

		DebugAI(AI_MSG_INFO, pSoldier, String("fWantCallReinforcements %d fWantCallArtillery %d fWantScan %d fWantJam %d fWantListen %d", fWantCallReinforcements, fWantCallArtillery, fWantScan, fWantJam, fWantListen));

		// want call reinforcements or order artillery strike, stop jamming
		if ((fWantCallReinforcements || fWantCallArtillery) && fJamming)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to call reinforcements or order artillery strike, try to stop jamming"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio"));
				pSoldier->ubAISkillUse = SKILLS_RADIO_TURNOFF;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want call artillery strike
		else if (fWantCallArtillery && !fSectorJammed)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to order artillery strike"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("order artillery strike"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_ARTILLERY;
				pSoldier->aiData.usActionData = iArtilleryGridNo;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want scan frequencies to find enemy jammer
		else if (fWantScan && !fScanning)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to find enemy jammer"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_SCAN_FOR_JAM, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start scanning"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_SCAN_FOR_JAM;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want jam communications
		else if (fWantJam && !fJamming)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to jam communications"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_JAM, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start jamming"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_JAM;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want listen to find enemy location
		else if (fWantListen && !fListening)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to listen to know enemy location"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_LISTEN, FALSE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start listening"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_LISTEN;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// don't want to use radio anymore
		else if (!fWantJam && !fWantScan && !fWantListen && fUsingRadio)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("don't want to use radio anymore"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio"));
				pSoldier->ubAISkillUse = SKILLS_RADIO_TURNOFF;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[free prisoners]"));
	////////////////////////////////////////////////////////////////////////////
	// IF YOU SEE CAPTURED FRIENDS, FREE THEM!
	////////////////////////////////////////////////////////////////////////////

	// Flugente: if we see one of our buddies captured, it is a clear sign of enemy activity!
	if( gGameExternalOptions.fAllowPrisonerSystem && 
		pSoldier->bTeam == ENEMY_TEAM &&
		!pSoldier->aiData.bUnderFire &&
		!pSoldier->bBlindedCounter &&
		!GuySawEnemy(pSoldier) )
	{
		UINT8 ubPerson = GetClosestFlaggedSoldierID( pSoldier, DAY_VISION_RANGE, ENEMY_TEAM, SOLDIER_POW, TRUE );		

		if ( ubPerson != NOBODY )
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("found prisoner %d", ubPerson));

			// if we are close, we can release this guy
			// possible only if not handcuffed (binders can be opened, handcuffs not)
			// sevenfm: allow soldiers with tools or locksmith kits to open handcuffs, also technicians can open handcuffs
			if (!MercPtrs[ubPerson]->inv[HANDPOS].exists() ||
				!HasItemFlag( (&(MercPtrs[ubPerson]->inv[HANDPOS]))->usItem, HANDCUFFS ) ||
				!Item[(&(MercPtrs[ubPerson]->inv[HANDPOS]))->usItem].metal ||
				FindToolkit( MercPtrs[ubPerson] ) != NO_SLOT ||
				FindLocksmithKit( MercPtrs[ubPerson] ) != NO_SLOT)
				//HAS_SKILL_TRAIT( MercPtrs[ubPerson], TECHNICIAN_NT ) )
			{
				if ( SpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) <= 1 )
				{
					// see if we are facing this person
					UINT8 ubDesiredMercDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

					// if not already facing in that direction,
					if (pSoldier->ubDirection != ubDesiredMercDir)
					{
						if (pSoldier->InternalIsValidStance(ubDesiredMercDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
						{
							pSoldier->aiData.usActionData = ubDesiredMercDir;
							//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] turn to handcuffed soldier", pSoldier->ubID);
							return(AI_ACTION_CHANGE_FACING);
						}						
					}
					else
					{
						//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] free prisoner %d", pSoldier->ubID, MercPtrs[ubPerson]->ubID);
						return(AI_ACTION_FREE_PRISONER);
					}					
				}
				else
				{
					pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_FRIEND, 0);

					// check path for dangerous places
					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
					{
						INT16 sLoop;
						INT32 sCheckGridNo = pSoldier->sGridNo;
						for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)							
						{
							sCheckGridNo = NewGridNo( sCheckGridNo,DirectionInc( (UINT8) (pSoldier->pathing.usPathingData[sLoop]) ) );
							// stop if found fresh corpse near spot and enemy can attack me at spot
							if( CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel) &&
								EnemyCanAttackSpot(pSoldier, sCheckGridNo,  pSoldier->pathing.bLevel) )
							{
								pSoldier->aiData.usActionData = NOWHERE;
								break;
							}
						}
					}

					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
					{
						//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] move to handcuffed friend %d", pSoldier->ubID, MercPtrs[ubPerson]->ubID);
						return(AI_ACTION_SEEK_FRIEND);
					}
				}
			}
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[medic]"));
	if (DecideMedicCombat(pSoldier))
		return pSoldier->aiData.bAction;	

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Calculate AI morale]"));

	// calculate our morale
	pSoldier->aiData.bAIMorale = CalcMorale(pSoldier);
	DebugAI( AI_MSG_INFO, pSoldier, String("AI morale %d", pSoldier->aiData.bAIMorale));
	// WDS DEBUG - this will make all enemies run away (to test retreating into occupied sector bugs)
	//	pSoldier->aiData.bAIMorale = MORALE_HOPELESS;

	// if a guy is feeling REALLY discouraged, he may continue to run like hell
	if (pSoldier->aiData.bAIMorale == MORALE_HOPELESS && 
		ubCanMove &&
		(pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier)))
	{
		DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"decideactionred: run away");
		////////////////////////////////////////////////////////////////////////
		// RUN AWAY TO SPOT FARTHEST FROM KNOWN THREATS (ONLY IF MORALE HOPELESS)
		////////////////////////////////////////////////////////////////////////

		// look for best place to RUN AWAY to (farthest from the closest threat)
		pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_RUN_AWAY);
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[radio RED alert]"));
	////////////////////////////////////////////////////////////////////////////
	// RADIO RED ALERT: determine %chance to call others and report contact
	////////////////////////////////////////////////////////////////////////////

	// if we're a computer merc, and we have the action points remaining to RADIO
	// (we never want NPCs to choose to radio if they would have to wait a turn)
	if (!pSoldier->IsBoxer() &&
		!fCivilian &&
		!pSoldier->RaisedAlertThisTurn() &&	
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT) &&
		pSoldier->bActionPoints > APBPConstants[AP_RADIO] + APBPConstants[AP_MAXIMUM] / 4 &&
		!pSoldier->aiData.bUnderFire &&
		!fDangerousSpot &&
		gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1)
	{		
		DebugAI( AI_MSG_INFO, pSoldier, String("checking to radio red alert, knowledge %d, aware %d, need radio panic %d", WhatIKnowThatPublicDont(pSoldier, FALSE), gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition, NeedToRadioAboutPanicTrigger()) );

		// if there hasn't been an initial RED ALERT yet in this sector
		if( !gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition || NeedToRadioAboutPanicTrigger() )
			// since I'm at STATUS RED, I obviously know we're being invaded!
			iChance = gbDiff[DIFF_RADIO_RED_ALERT][ SoldierDifficultyLevel( pSoldier ) ];
		else // subsequent radioing (only to update enemy positions, request help)
			// base chance depends on how much new info we have to radio to the others
			iChance = 10 * WhatIKnowThatPublicDont(pSoldier,FALSE);  // use 10 * for RED alert

		DebugAI( AI_MSG_INFO, pSoldier, String("iChance %d", iChance));

		// if I actually know something they don't and I ain't swimming (deep water)
		if (iChance && !bInDeepWater)
		{
			// modify base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY:       iChance +=  20;  break;
			case ONGUARD:          iChance +=  15;  break;
			case ONCALL:           iChance +=  10;  break;
			case CLOSEPATROL:                       break;
			case RNDPTPATROL:
			case POINTPATROL:      iChance +=  -5;  break;
			case FARPATROL:        iChance += -10;  break;
			case SEEKENEMY:        iChance += -20;  break;
			}

			// modify base chance according to attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:        iChance +=  20;  break;
			case BRAVESOLO:        iChance += -10;  break;
			case BRAVEAID:                          break;
			case CUNNINGSOLO:      iChance +=  -5;  break;
			case CUNNINGAID:                        break;
			case AGGRESSIVE:       iChance += -20;  break;
			case ATTACKSLAYONLY:		iChance = 0;
			}

			DebugAI( AI_MSG_INFO, pSoldier, String("iChance modified by order and attitude %d", iChance));

			if ( (gTacticalStatus.fPanicFlags & PANIC_TRIGGERS_HERE) && !gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
			{
				// ignore morale (which could be really high
			}
			else
			{
				// modify base chance according to morale
				switch (pSoldier->aiData.bAIMorale)
				{
				case MORALE_HOPELESS:  iChance *= 3;    break;
				case MORALE_WORRIED:   iChance *= 2;    break;
				case MORALE_NORMAL:                     break;
				case MORALE_CONFIDENT: iChance /= 2;    break;
				case MORALE_FEARLESS:  iChance /= 3;    break;
				}
				DebugAI( AI_MSG_INFO, pSoldier, String("iChance modified by morale %d", iChance));
			}

			// sevenfm: lower chance to radio if some nearby friend radioed this turn
			iChance = iChance / (1 + CountFriendsUsedRadio(pSoldier, DAY_VISION_RANGE));

			if ((INT16) PreRandom(100) < iChance)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("decided to radio red alert, iChance = %d", iChance));
				return(AI_ACTION_RED_ALERT);
			}
		}
	}

	// sevenfm: no Main Red AI for civilians
	if (!fCivilian)
	{
		////////////////////////////////////////////////////////////////////////////
		// MAIN RED AI: Decide soldier's preference between SEEKING,HELPING & HIDING
		////////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_INFO, pSoldier, String("[MAIN RED AI]"));

		// get the location of the closest reachable opponent
		sClosestDisturbance = ClosestReachableDisturbance(pSoldier, &fSeekClimb, ubThreatID, sThreatSpot, bThreatLevel, ubSeekType);
		// determine opponent level
		if (fSeekClimb)
		{
			if (pSoldier->pathing.bLevel > 0)
				bClosestDisturbanceLevel = 0;
			else
				bClosestDisturbanceLevel = 1;
		}
		else
		{
			bClosestDisturbanceLevel = pSoldier->pathing.bLevel;
		}
		DebugAI(AI_MSG_INFO, pSoldier, String("closest reachable disturbance %d climb %d threat ID %d spot %d level %d type %d", sClosestDisturbance, fSeekClimb, ubThreatID, sThreatSpot, bThreatLevel, ubSeekType));

		// get the location of possible seen spot of closest opponent
		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Find closest seen opponent spot]"));
		if (gfTurnBasedAI &&
			InARoom(pSoldier->sGridNo) &&
			pSoldier->pathing.bLevel == 0 &&
			!TileIsOutOfBounds(sClosestDisturbance) &&
			!fSeekClimb &&
			PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < TACTICAL_RANGE)
		{
			gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;

			// check path to closest disturbance and find the point where enemy will appear in sight						
			if (FindBestPath(pSoldier, sClosestDisturbance, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE))
			{
				INT16 sLoop;
				INT32 sLastSeenSpot = NOWHERE;

				DebugAI(AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", sClosestDisturbance, pSoldier->pathing.usPathDataSize));
				DebugAI(AI_MSG_INFO, pSoldier, String("check path for seen spots"));

				sCheckGridNo = pSoldier->sGridNo;

				for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
				{
					sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

					if (LOS_Virtual(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS))
					{
						sLastSeenSpot = sCheckGridNo;
					}
				}

				// if found last seen spot
				if (!TileIsOutOfBounds(sLastSeenSpot))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("last seen spot %d level %d", sLastSeenSpot, pSoldier->pathing.bLevel));
					sClosestSeenPathSpot = sLastSeenSpot;
				}
			}
			gubNPCAPBudget = 0;
		}

		// sevenfm: avoid light if spot is dangerous
		if (ubCanMove &&
			InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
			fCanBeSeen &&
			pSoldier->aiData.bOrders != STATIONARY &&
			!pSoldier->RushAttackAdvance() &&
			pSoldier->GetBestEquippedFlashLightRange() == 0 &&
			!InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE) &&
			(pSoldier->aiData.bOrders != SEEKENEMY || !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || CountCorpses(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2, FALSE) > 0) &&
			(CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) > 0 || !AICheckSuccessfulAttack(pSoldier, TRUE) || pSoldier->aiData.bOrders != SEEKENEMY) &&
			CountFriendsBlack(pSoldier, sClosestDisturbance) == 0)
		{
			pSoldier->aiData.usActionData = FindNearbyDarkerSpot( pSoldier );

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				// move as if leaving water or gas
				return( AI_ACTION_LEAVE_WATER_GAS );
			}
		}

		sClosestFriend = ClosestReachableFriendInTrouble(pSoldier, &fHelpClimb );
		DebugAI( AI_MSG_INFO, pSoldier, String("closest reachable friend %d climb %d", sClosestFriend, fHelpClimb));

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[continue flanking]"));
		// continue flanking
		if (!TileIsOutOfBounds(sClosestDisturbance))
		{
			pSoldier->sTacticalSpot = sClosestDisturbance;
		}

		// continue flanking
		if (pSoldier->IsFlanking() && 
			gAnimControl[ pSoldier->usAnimState ].ubHeight != ANIM_PRONE &&
			!pSoldier->aiData.bUnderFire )
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("continue flanking"));
						
			INT16 currDir = AIDirection(pSoldier->sGridNo, pSoldier->sTacticalSpot);
			INT16 origDir = pSoldier->sTacticalData;

			if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT)
			{
				if ( origDir > currDir )
					origDir -= 8;

				// stop flanking condition
				if (currDir - origDir >= 2 && (currDir - origDir > 3 || pSoldier->aiData.bAttitude != CUNNINGSOLO || CountFriendsBetweenMeAndSpotFromSpot(pSoldier, pSoldier->sTacticalSpot) == 0))
				{
					// start end flank approach
					pSoldier->StartTacticalOrder(TACTICAL_ORDER_FLANK_APPROACH, pSoldier->sTacticalSpot, pSoldier->bTacticalLevel, 0, 0, END_FLANKING_DURATION);
				}
				else
				{
					pSoldier->aiData.usActionData = FindFlankingSpot(pSoldier, pSoldier->sTacticalSpot, AI_ACTION_FLANK_LEFT);

					if (pSoldier->CheckInitialAP() &&
						(TileIsOutOfBounds(pSoldier->aiData.usActionData) || Chance(SoldierDifficultyLevel(pSoldier) * 10)))
					{
						// check if we can cut fence here
						if (DecideCuttingFenceFlank(pSoldier))
							return pSoldier->aiData.bAction;						
					}

					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) )
					{
						if (FindThrowableGrenade(pSoldier, EXPLOSV_SMOKE) != NO_SLOT)
						{
							// check path for dangerous spots, use smoke to cover movement
							DebugAI(AI_MSG_INFO, pSoldier, String("check flanking path for dangerous spots"));

							gubNPCAPBudget = 0;
							gubNPCDistLimit = 0;

							sDangerousSpot = NOWHERE;

							if (FindBestPath(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, 0))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", pSoldier->aiData.usActionData, pSoldier->pathing.usPathDataSize));

								sCheckGridNo = pSoldier->sGridNo;

								// find farthest dangerous spot on flanking path
								for (INT16 sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
								{
									sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

									if (sCheckGridNo != pSoldier->sGridNo &&
										!TileIsOutOfBounds(sCheckGridNo) &&
										PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) < TACTICAL_RANGE / 2 &&
										!Water(sCheckGridNo, pSoldier->pathing.bLevel) &&
										!InSmoke(sCheckGridNo, pSoldier->pathing.bLevel, TRUE) &&
										!InSmokeNearby(sCheckGridNo, pSoldier->pathing.bLevel) &&
										(CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel) || InLightAtNight(sCheckGridNo, pSoldier->pathing.bLevel)) &&
										!SightCoverAtSpot(pSoldier, sCheckGridNo, ANIM_STAND, TRUE))
									{
										CheckTossGrenadeAt(pSoldier, &BestThrow, sCheckGridNo, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);
										if (BestThrow.ubPossible)
										{
											sDangerousSpot = sCheckGridNo;
										}
									}
								}

							}
							gubNPCAPBudget = 0;

							// try to use smoke to cover movement
							DebugAI(AI_MSG_TOPIC, pSoldier, String("[try to use smoke to cover flank movement]"));

							if (!TileIsOutOfBounds(sDangerousSpot) &&
								(Chance(SoldierDifficultyLevel(pSoldier) * 10) || pSoldier->RushAttackPrepare() || Chance(TeamPercentKilled(pSoldier->bTeam))))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("check if soldier can cover movement path with smoke"));

								CheckTossGrenadeAt(pSoldier, &BestThrow, sDangerousSpot, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);

								if (BestThrow.ubPossible)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

									// if necessary, swap the usItem from holster into the hand position
									if (BestThrow.bWeaponIn != HANDPOS)
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
										RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
									}

									// stand up before throwing if needed
									if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
										pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
									{
										pSoldier->aiData.usActionData = BestThrow.ubStance;
										pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
										pSoldier->aiData.usNextActionData = BestThrow.sTarget;
										pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
										pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
										return AI_ACTION_CHANGE_STANCE;
									}

									pSoldier->aiData.usActionData = BestThrow.sTarget;
									pSoldier->bTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

									return(AI_ACTION_TOSS_PROJECTILE);
								}
							}
						}						

						return AI_ACTION_FLANK_LEFT;
					}
					else
					{
						// wait for next turn if turnbased
						if (gfTurnBasedAI &&
							pSoldier->bActionPoints < pSoldier->bInitialActionPoints)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("cannot flank, wait for the next turn"));
							return(AI_ACTION_END_TURN);
						}

						// check if we can climb here
						if (CanClimbFlank(pSoldier))
						{
							// climb
							if (IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF))
							{
								return( AI_ACTION_CLIMB_ROOF );
							}
						}

						if (WeAttack(pSoldier->bTeam) ||
							pSoldier->aiData.bOrders == SEEKENEMY && Chance(20 * CountThrowableGrenades(pSoldier, EXPLOSV_NORMAL, 10)))
						{
							CheckTossFlankFence(pSoldier, &BestThrow);

							if (BestThrow.ubPossible)
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

								// if necessary, swap the usItem from holster into the hand position
								if (BestThrow.bWeaponIn != HANDPOS)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
									RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
								}

								// set grenade as delayed
								pSoldier->inv[HANDPOS][0]->data.sObjectFlag |= DELAYED_GRENADE_EXPLOSION;

								// stand up before throwing if needed
								if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
									pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
								{
									pSoldier->aiData.usActionData = BestThrow.ubStance;
									pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
									pSoldier->aiData.usNextActionData = BestThrow.sTarget;
									pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
									return AI_ACTION_CHANGE_STANCE;
								}
								else
								{
									pSoldier->aiData.usActionData = BestThrow.sTarget;
									pSoldier->bTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
								}

								DebugAI(AI_MSG_INFO, pSoldier, String("throw grenade at spot %d level %d", BestThrow.sTarget, BestThrow.bTargetLevel));

								//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] flank: throw grenade at fence %d", pSoldier->ubID, BestThrow.sTarget);
								//BeginMultiPurposeLocator(BestThrow.sTarget, BestThrow.bTargetLevel, FALSE);

								return(AI_ACTION_TOSS_PROJECTILE);
							}
						}
						
						// allow extra flanking and try again
						/*if (!(pSoldier->flags.ubTacticalFlags & TACTICAL_FLAG_EXTRA_FLANKING))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("allow extra flanking and try again"));
							pSoldier->flags.ubTacticalFlags |= TACTICAL_FLAG_EXTRA_FLANKING;
							
							UINT8 ubDir = Random(8);

							// if not already facing in that direction,
							if (pSoldier->ubDirection != ubDir &&
								pSoldier->InternalIsValidStance(ubDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
								pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
							{
								pSoldier->aiData.usActionData = ubDir;

								return(AI_ACTION_CHANGE_FACING);
							}
							else
							{
								return AI_ACTION_NONE;
							}							
						}*/

						if (DecideBlowUpObstacleFlank(pSoldier))
							return pSoldier->aiData.bAction;

						pSoldier->StopTacticalOrder();
					}
				}
			}
			else
			{
				if ( origDir < currDir )
					origDir += 8;

				// stop flanking condition
				if (origDir - currDir >= 2 && (origDir - currDir > 3 || pSoldier->aiData.bAttitude != CUNNINGSOLO || CountFriendsBetweenMeAndSpotFromSpot(pSoldier, pSoldier->sTacticalSpot) == 0))
				{
					// start end flank approach
					pSoldier->StartTacticalOrder(TACTICAL_ORDER_FLANK_APPROACH, pSoldier->sTacticalSpot, pSoldier->bTacticalLevel, 0, 0, END_FLANKING_DURATION);
				}
				else
				{
					pSoldier->aiData.usActionData = FindFlankingSpot(pSoldier, pSoldier->sTacticalSpot, AI_ACTION_FLANK_RIGHT);

					if (pSoldier->CheckInitialAP() &&
						(TileIsOutOfBounds(pSoldier->aiData.usActionData) || Chance(SoldierDifficultyLevel(pSoldier) * 10)))
					{
						// check if we can cut fence here
						if (DecideCuttingFenceFlank(pSoldier))
							return pSoldier->aiData.bAction;						
					}

					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) )
					{
						if (FindThrowableGrenade(pSoldier, EXPLOSV_SMOKE) != NO_SLOT)
						{
							// check path for dangerous spots, use smoke to cover movement
							DebugAI(AI_MSG_INFO, pSoldier, String("check flanking path for dangerous spots"));

							gubNPCAPBudget = 0;
							gubNPCDistLimit = 0;

							sDangerousSpot = NOWHERE;

							if (FindBestPath(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, 0))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", pSoldier->aiData.usActionData, pSoldier->pathing.usPathDataSize));

								sCheckGridNo = pSoldier->sGridNo;

								// find farthest dangerous spot
								for (INT16 sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
								{
									sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

									if (sCheckGridNo != pSoldier->sGridNo &&
										!TileIsOutOfBounds(sCheckGridNo) &&
										PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) < TACTICAL_RANGE / 2 &&
										!Water(sCheckGridNo, pSoldier->pathing.bLevel) &&
										!InSmokeNearby(sCheckGridNo, pSoldier->pathing.bLevel) &&
										!InSmoke(sCheckGridNo, pSoldier->pathing.bLevel, TRUE) &&
										(CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel) || InLightAtNight(sCheckGridNo, pSoldier->pathing.bLevel)) &&
										!SightCoverAtSpot(pSoldier, sCheckGridNo, ANIM_STAND, TRUE))
									{
										CheckTossGrenadeAt(pSoldier, &BestThrow, sCheckGridNo, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);
										if (BestThrow.ubPossible)
										{
											sDangerousSpot = sCheckGridNo;
										}
									}
								}
							}
							gubNPCAPBudget = 0;

							// try to use smoke to cover movement
							DebugAI(AI_MSG_TOPIC, pSoldier, String("[try to use smoke to cover flank movement]"));

							if (!TileIsOutOfBounds(sDangerousSpot) &&								
								(Chance(SoldierDifficultyLevel(pSoldier) * 10) || Chance(TeamPercentKilled(pSoldier->bTeam))))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("check if soldier can cover movement path with smoke"));

								CheckTossGrenadeAt(pSoldier, &BestThrow, sDangerousSpot, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);

								if (BestThrow.ubPossible)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

									// if necessary, swap the usItem from holster into the hand position
									if (BestThrow.bWeaponIn != HANDPOS)
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
										RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
									}

									// stand up before throwing if needed
									if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
										pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
									{
										pSoldier->aiData.usActionData = BestThrow.ubStance;
										pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
										pSoldier->aiData.usNextActionData = BestThrow.sTarget;
										pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
										pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
										return AI_ACTION_CHANGE_STANCE;
									}

									pSoldier->aiData.usActionData = BestThrow.sTarget;
									pSoldier->bTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

									return(AI_ACTION_TOSS_PROJECTILE);
								}
							}
						}						

						return AI_ACTION_FLANK_RIGHT ;
					}
					else
					{
						// wait for next turn if turnbased
						if (gfTurnBasedAI &&
							pSoldier->bActionPoints < pSoldier->bInitialActionPoints)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("cannot flank, wait for the next turn"));
							return(AI_ACTION_END_TURN);
						}

						// check if we can climb here
						if (CanClimbFlank(pSoldier))
						{
							// climb
							if (IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF))
							{
								return( AI_ACTION_CLIMB_ROOF );
							}
						}						

						// check if we can blow up fence with a grenade
						if (WeAttack(pSoldier->bTeam) ||
							pSoldier->aiData.bOrders == SEEKENEMY && Chance(20 * CountThrowableGrenades(pSoldier, EXPLOSV_NORMAL, 10)))
						{
							CheckTossFlankFence(pSoldier, &BestThrow);

							if (BestThrow.ubPossible)
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

								// if necessary, swap the usItem from holster into the hand position
								if (BestThrow.bWeaponIn != HANDPOS)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
									RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
								}

								// set grenade as delayed
								pSoldier->inv[HANDPOS][0]->data.sObjectFlag |= DELAYED_GRENADE_EXPLOSION;

								// stand up before throwing if needed
								if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
									pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
								{
									pSoldier->aiData.usActionData = BestThrow.ubStance;
									pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
									pSoldier->aiData.usNextActionData = BestThrow.sTarget;
									pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
									return AI_ACTION_CHANGE_STANCE;
								}
								else
								{
									pSoldier->aiData.usActionData = BestThrow.sTarget;
									pSoldier->bTargetLevel = BestThrow.bTargetLevel;
									pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
								}

								DebugAI(AI_MSG_INFO, pSoldier, String("throw grenade at spot %d level %d", BestThrow.sTarget, BestThrow.bTargetLevel));

								//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] flank: throw grenade at fence %d", pSoldier->ubID, BestThrow.sTarget);
								//BeginMultiPurposeLocator(BestThrow.sTarget, BestThrow.bTargetLevel, FALSE);

								return(AI_ACTION_TOSS_PROJECTILE);
							}
						}

						// allow extra flanking and try again
						/*if (!(pSoldier->flags.ubTacticalFlags & TACTICAL_FLAG_EXTRA_FLANKING))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("allow extra flanking and try again"));
							pSoldier->flags.ubTacticalFlags |= TACTICAL_FLAG_EXTRA_FLANKING;

							UINT8 ubDir = Random(8);

							// if not already facing in that direction,
							if (pSoldier->ubDirection != ubDir &&
								pSoldier->InternalIsValidStance(ubDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
								pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
							{
								pSoldier->aiData.usActionData = ubDir;

								return(AI_ACTION_CHANGE_FACING);
							}
							else
							{
								return AI_ACTION_NONE;
							}
						}*/

						if (DecideBlowUpObstacleFlank(pSoldier))
							return pSoldier->aiData.bAction;

						pSoldier->StopTacticalOrder();
					}
				}
			}
		}

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[end flanking approach]"));
		// stop end flanking approach if cannot find enemy or have contact with enemy or group has successful attack
		if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_APPROACH && 
			(TileIsOutOfBounds(sClosestDisturbance) ||
			pSoldier->aiData.bUnderFire ||
			MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) ||
			GuySawEnemy(pSoldier) ||
			AICheckSuccessfulAttack(pSoldier, TRUE) ||
			CountFriendsBlack(pSoldier, sClosestDisturbance) > 0))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("stop end flanking approach"));
			pSoldier->StopTacticalOrder();
		}

		// officer possibly start rush attack
		DebugAI(AI_MSG_TOPIC, pSoldier, String("[officer possibly start rush attack]"));
		if (pSoldier->bTeam == ENEMY_TEAM &&
			pSoldier->CheckInitialAP() &&
			!pSoldier->bBlindedCounter &&
			IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT) &&
			AIRoleOfficer(pSoldier) &&
			!pSoldier->aiData.bUnderFire &&
			!TileIsOutOfBounds(sClosestDisturbance) &&
			pSoldier->pathing.bLevel == 0 &&
			PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < VISION_RANGE + TACTICAL_RANGE / 2 &&
			CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) == 0 &&
			AICheckWeOutnumberPublic(pSoldier, sClosestDisturbance) &&
			(CountFriendsBlack(pSoldier, sClosestDisturbance) > 0 || 
			CountFriendsLastAttackHit(pSoldier, sClosestDisturbance, TACTICAL_RANGE) > 0 || 
			CountRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE) > 0))
		{
			UINT8 ubCanRushFriends;
			UINT8 ubRushAttackFriends;
			UINT8 ubFriendsUnderAttack;
			UINT8 ubFriendsShortRangeWeapon;
			UINT8 ubCorpses;
			UINT8 ubPublicEnemies;
			INT32 iPathCost;
			UINT8 ubFriendsAttackHit;
			UINT8 ubFriendsBlack;
			UINT8 ubOutnumberBonus = 0;

			iPathCost = EstimatePlotPath(pSoldier, sClosestDisturbance, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
			ubCanRushFriends = CountCanRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE, APBPConstants[AP_MAXIMUM] * 2, FALSE, FALSE);
			ubRushAttackFriends = CountRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE);
			ubFriendsUnderAttack = CountTeamUnderAttack(pSoldier->bTeam, sClosestDisturbance, TACTICAL_RANGE);
			ubFriendsShortRangeWeapon = CountFriendsShortRangeWeapon(pSoldier, sClosestDisturbance, TACTICAL_RANGE);
			ubCorpses = CountCorpses(pSoldier, sClosestDisturbance, VISION_RANGE, FALSE);
			ubPublicEnemies = CountPublicKnownEnemies(pSoldier, sClosestDisturbance, VISION_RANGE);
			ubFriendsAttackHit = CountFriendsLastAttackHit(pSoldier, sClosestDisturbance, TACTICAL_RANGE);
			ubFriendsBlack = CountFriendsBlack(pSoldier, sClosestDisturbance);

			if (ubCanRushFriends > ubPublicEnemies)
			{
				ubOutnumberBonus = ubCanRushFriends - ubPublicEnemies;
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("iPathCost %d ubCanRushFriends %d ubFriendsUnderAttack %d ubCorpses %d ubPublicEnemies %d ubFriendsAttackHit %d ubFriendsBlack %d", iPathCost, ubCanRushFriends, ubFriendsUnderAttack, ubCorpses, ubPublicEnemies, ubFriendsAttackHit, ubFriendsBlack));

			if (iPathCost > 0 &&
				iPathCost < APBPConstants[AP_MAXIMUM] * 2 &&
				(ubCorpses > 0 || ubFriendsUnderAttack > 0 || ubRushAttackFriends > 0) &&
				(ubCanRushFriends > ubCorpses &&
				ubCanRushFriends > ubPublicEnemies &&
				Chance(min(5 * (ubCanRushFriends - ubPublicEnemies), ubFriendsAttackHit + ubFriendsBlack + ubFriendsUnderAttack + ubFriendsShortRangeWeapon + ubOutnumberBonus)) ||
				ubCanRushFriends + ubRushAttackFriends > ubCorpses + ubCorpses * TeamPercentKilled(pSoldier->bTeam) / 100 &&
				ubCanRushFriends + ubRushAttackFriends > ubPublicEnemies + ubPublicEnemies * TeamPercentKilled(pSoldier->bTeam) / 100 &&
				Chance(10 * ubRushAttackFriends)))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start rush attack at spot %d", sClosestDisturbance));
				StartRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE, 0);

				// simulate radio communication
				return(AI_ACTION_FAKE_ALERT);
			}
		}

		/*DebugAI(AI_MSG_TOPIC, pSoldier, String("[Plant Explosives]"));
		INT8 bTNTSlot = FindTNT(pSoldier);
		DebugAI(AI_MSG_INFO, pSoldier, String("TNT slot %d", bTNTSlot));
		if (bTNTSlot != NO_SLOT &&
			IsActionAffordable(pSoldier, AI_ACTION_PLANT_BOMB) &&
			pSoldier->CheckInitialAP())
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found TNT, plant bomb"));
			RearrangePocket(pSoldier, HANDPOS, bTNTSlot, FOREVER);
			pSoldier->aiData.usActionData = 0;
			return AI_ACTION_PLANT_BOMB;
		}*/

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Use TNT to blow up enemies nearby]"));
		INT8 bTNTSlot = FindTNT(pSoldier);
		if (bTNTSlot != NO_SLOT &&
			SoldierAI(pSoldier) &&
			!pSoldier->bBlindedCounter &&
			pSoldier->bTeam == ENEMY_TEAM &&
			(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
			IsActionAffordable(pSoldier, AI_ACTION_PLANT_BOMB) &&
			!pSoldier->aiData.bUnderFire &&
			//pSoldier->pathing.bLevel == 0 &&
			//pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) &&
			//!TileIsOutOfBounds(sClosestOpponent) &&
			//PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 2 &&
			CountKnownEnemies(pSoldier, pSoldier->sGridNo, 2) > 0 &&			
			(Chance(SoldierDifficultyLevel(pSoldier) * 10) || InARoom(pSoldier->sGridNo, NULL) && CountKnownEnemies(pSoldier, pSoldier->sGridNo, 2, pSoldier->pathing.bLevel ? 0 : 1) > 0))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found TNT in slot %d, plant bomb", bTNTSlot));

			RearrangePocket(pSoldier, HANDPOS, bTNTSlot, FOREVER);
			pSoldier->aiData.usActionData = 0;
			return AI_ACTION_PLANT_BOMB;
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Use TNT to blow up walls and fences]"));
		if (SoldierAI(pSoldier) &&
			pSoldier->bTeam == ENEMY_TEAM &&
			!pSoldier->bBlindedCounter &&
			(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
			!pSoldier->aiData.bUnderFire &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			(Chance(SoldierDifficultyLevel(pSoldier) * 10) ||
			InARoom(sClosestOpponent) &&
			CountFriendsInRoom(pSoldier, RoomNo(sClosestOpponent)) == 0 &&
			!SameRoom(sClosestOpponent, pSoldier->sGridNo) &&
			(CountCorpsesInRoom(pSoldier, RoomNo(sClosestOpponent), 0) > 0 || TeamHighPercentKilled(pSoldier->bTeam) || EstimatePathCostToLocation(pSoldier, sClosestOpponent, pSoldier->pathing.bLevel, FALSE, NULL, NULL) == 0)))
		{
			if (DecideBlowUpObstacle(pSoldier, sClosestOpponent))
				return pSoldier->aiData.bAction;
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[use wire cutters to cut fence]"));
		INT8 bWirecutterSlot = FindWirecutters(pSoldier);
		if (SoldierAI(pSoldier) &&
			pSoldier->bTeam == ENEMY_TEAM &&
			!pSoldier->bBlindedCounter &&
			(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
			!pSoldier->aiData.bUnderFire &&
			bWirecutterSlot != NO_SLOT &&
			pSoldier->pathing.bLevel == 0 &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			//PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 2 &&
			Chance(SoldierDifficultyLevel(pSoldier) * 20) &&
			pSoldier->bActionPoints >= GetAPsToCutFence(pSoldier) + GetAPsToLook(pSoldier) &&
			FindFenceAroundSpot(pSoldier->sGridNo))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found wire cutter, check if we can find fence to cut"));

			UINT8 ubDesiredDir = AIDirection(pSoldier->sGridNo, sClosestOpponent);
			UINT8 ubCheckDir;
			INT32 sNewSpot;
			INT32 sNextSpot;
			INT32 sPathCost, sNewPathCost;
			INT32 sOriginalGridNo;

			for (ubCheckDir = 0; ubCheckDir < NUM_WORLD_DIRECTIONS; ubCheckDir++)
			{
				// cannot cut fence diagonally, check desired dir
				if (ubCheckDir % 2 != 0 ||
					ubCheckDir != ubDesiredDir &&
					ubCheckDir != gOneCDirection[ubDesiredDir] &&
					ubCheckDir != gOneCCDirection[ubDesiredDir])
				{
					continue;
				}

				sNewSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubCheckDir));
				sNextSpot = NewGridNo(sNewSpot, DirectionInc(ubCheckDir));

				if (sNewSpot != pSoldier->sGridNo &&
					sNextSpot != sNewSpot &&
					!Water(sNewSpot, pSoldier->pathing.bLevel) &&
					!Water(sNextSpot, pSoldier->pathing.bLevel) &&
					IsCuttableWireFenceAtGridNo(sNewSpot) &&
					!IsCutWireFenceAtGridNo(sNewSpot) &&
					IsLocationSittable(sNextSpot, pSoldier->pathing.bLevel) &&
					NewOKDestination(pSoldier, sNextSpot, TRUE, pSoldier->pathing.bLevel))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("found cuttable fence at %d", sNewSpot));

					// check if cutting the fence improves situation
					sPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
					sOriginalGridNo = pSoldier->sGridNo;
					pSoldier->sGridNo = sNewSpot;
					sNewPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
					pSoldier->sGridNo = sOriginalGridNo;

					if (sNewPathCost > 0 && (sPathCost == 0 || sPathCost > sNewPathCost && sPathCost - sNewPathCost > APBPConstants[AP_MAXIMUM]))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("cutting fence improves path cost, use wire cutter"));

						if (pSoldier->ubDirection == ubCheckDir)
						{
							RearrangePocket(pSoldier, HANDPOS, bWirecutterSlot, FOREVER);
							pSoldier->aiData.usActionData = sNewSpot;
							return AI_ACTION_HANDLE_ITEM;
						}
						else if (pSoldier->InternalIsValidStance(ubCheckDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("turn before cutting fence"));
							RearrangePocket(pSoldier, HANDPOS, bWirecutterSlot, FOREVER);
							pSoldier->aiData.usActionData = ubCheckDir;
							pSoldier->aiData.bNextAction = AI_ACTION_HANDLE_ITEM;
							pSoldier->aiData.usNextActionData = sNewSpot;
							return AI_ACTION_CHANGE_FACING;
						}
					}
				}
			}
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[try to jump into window]"));
		if (SoldierAI(pSoldier) &&
			(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
			!pSoldier->aiData.bUnderFire &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) &&
			!TileIsOutOfBounds(sClosestOpponent))
		{
			if (DecideJumpWindow(pSoldier, sClosestOpponent))
				return pSoldier->aiData.bAction;
		}

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[self smoke when under fire]"));
		if( gfTurnBasedAI &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			pSoldier->aiData.bUnderFire &&
			!InARoom(pSoldier->sGridNo, NULL) &&
			!InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, TRUE) &&
			RangeChangeDesire(pSoldier) <= 2 &&
			(!NightLight() || InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel)) &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
			(!fProneSightCover && !fAnyCover && !SightCoverNearSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE) || pSoldier->TakenLargeHit() || pSoldier->bBlindedCounter) &&
			(	pSoldier->TakenLargeHit() || 
				pSoldier->bBlindedCounter ||
				pSoldier->TakenHit() && pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2 ||
				pSoldier->ShockLevelPercent() > 20 + Random(80))
			)
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("check if soldier can cover himself with smoke"));

			CheckTossSelfSmoke(pSoldier, &BestThrow);

			if (BestThrow.ubPossible)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

				if (pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE)
				{
					// start retreating for several turns
					pSoldier->RetreatStart(RETREAT_DURATION);
				}				

				// if necessary, swap the usItem from holster into the hand position
				if (BestThrow.bWeaponIn != HANDPOS)
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket"));
					RearrangePocket(pSoldier,HANDPOS,BestThrow.bWeaponIn,FOREVER);
				}

				// stand up before throwing if needed
				if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
					pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
				{
					pSoldier->aiData.usActionData = BestThrow.ubStance;
					pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
					pSoldier->aiData.usNextActionData = BestThrow.sTarget;
					pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
					return AI_ACTION_CHANGE_STANCE;
				}
				else
				{
					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				}

				return(AI_ACTION_TOSS_PROJECTILE);
			}
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start retreating]"));
		if (gfTurnBasedAI &&
			pSoldier->IsMercBodyType() &&
			SoldierAI(pSoldier) &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
			pSoldier->aiData.bUnderFire &&
			pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
			(fDangerousSpot || pSoldier->TakenHit() || pSoldier->bBlindedCounter) &&
			//(!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE) && !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE) || pSoldier->TakenHit() || pSoldier->bBlindedCounter) &&
			!InARoom(pSoldier->sGridNo, NULL) &&
			pSoldier->aiData.bOrders != STATIONARY &&
			pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] == 0 &&
			!AICheckSuccessfulAttack(pSoldier, FALSE) &&
			Chance(1 + (pSoldier->ShockLevelPercent() + pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] + 5 * CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE / 2) + 5 * CountCorpses(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4, FALSE)) * 100 * 10 * 5 /
				(10 * (100 + pSoldier->aiData.bMorale) * (10 + pSoldier->stats.bExpLevel) * (5 + RangeChangeDesire(pSoldier))) + 10 * pSoldier->bBlindedCounter)
			)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start retreating"));
			pSoldier->RetreatStart(RETREAT_DURATION);
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start retreating when in shock or wounded]"));
		if (gfTurnBasedAI &&
			pSoldier->IsMercBodyType() &&
			SoldierAI(pSoldier) &&
			pSoldier->CheckInitialAP() &&
			pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
			pSoldier->aiData.bUnderFire &&
			pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
			!fAnyCover &&
			(pSoldier->aiData.bWoundShock > 0 || pSoldier->InShock()) &&
			!InARoom(pSoldier->sGridNo, NULL) &&
			pSoldier->aiData.bOrders != STATIONARY &&
			pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] == 0 &&
			!AICheckSuccessfulAttack(pSoldier, FALSE) &&
			(Chance(pSoldier->aiData.bWoundShock) || Chance(pSoldier->aiData.bShock / 10)) &&
			!Chance(SoldierDifficultyLevel(pSoldier) * 10))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start retreating when in shock or wounded"));
			pSoldier->RetreatStart(RETREAT_DURATION);
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start retreating when bleeding]"));
		if (gfTurnBasedAI &&
			pSoldier->IsMercBodyType() &&
			SoldierAI(pSoldier) &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
			//pSoldier->aiData.bUnderFire &&
			pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
			!fSafeSpot &&
			!InARoom(pSoldier->sGridNo, NULL) &&
			pSoldier->aiData.bOrders != STATIONARY &&
			pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] == 0 &&
			!AICheckSuccessfulAttack(pSoldier, FALSE) &&
			(!AICheckSuccessfulAttack(pSoldier, TRUE) || pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2) &&
			pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD &&
			pSoldier->iHealableInjury >= MIN_HEALABLE_INJURY &&
			Chance(pSoldier->bBleeding) &&
			(pSoldier->CanFirstAidAI() || GetClosestMedicSoldierID(pSoldier, TACTICAL_RANGE, pSoldier->bTeam) != NOBODY))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start retreating when bleeding"));
			pSoldier->RetreatStart(RETREAT_DURATION);
		}

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Set watched location]"));
		if( pSoldier->CheckInitialAP() &&
			pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&			
			gfTurnBasedAI &&
			pSoldier->pathing.bLevel == 0 &&
			!pSoldier->aiData.bUnderFire &&
			!pSoldier->bBlindedCounter &&
			!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
			SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE) &&
			!GuySawEnemy(pSoldier) &&
			!TileIsOutOfBounds(sClosestDisturbance) &&
			!fSeekClimb &&
			PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < DAY_VISION_RANGE &&
			(pSoldier->aiData.bOrders == STATIONARY || RangeChangeDesire(pSoldier) < 4) &&
			!LOS_Virtual(pSoldier, sClosestDisturbance, pSoldier->pathing.bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) &&
			CountFriendsBlack(pSoldier, sClosestDisturbance) == 0 &&
			!TileIsOutOfBounds(sClosestSeenPathSpot))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("set watched location at path seen spot %d level %d", sClosestSeenPathSpot, pSoldier->pathing.bLevel));
			IncrementWatchedLoc(pSoldier->ubID, sClosestSeenPathSpot, pSoldier->pathing.bLevel, FALSE);

			/*gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;

			// check path to closest disturbance and find the point where enemy will appear in sight						
			if( FindBestPath( pSoldier, sClosestDisturbance, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE ) )
			{
				INT16 sLoop;				
				INT32 sLastSeenSpot = NOWHERE;
				
				DebugAI( AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", sClosestDisturbance, pSoldier->pathing.usPathDataSize));
				DebugAI( AI_MSG_INFO, pSoldier, String("check path for seen spots"));

				sCheckGridNo = pSoldier->sGridNo;

				for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)							
				{
					sCheckGridNo = NewGridNo( sCheckGridNo,DirectionInc( (UINT8) (pSoldier->pathing.usPathingData[sLoop]) ) );

					if (LOS_Virtual(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS))
					{
						sLastSeenSpot = sCheckGridNo;
					}
				}

				// if found last seen spot
				if(!TileIsOutOfBounds(sLastSeenSpot))
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("last seen spot %d level %d", sLastSeenSpot, pSoldier->pathing.bLevel));
					IncrementWatchedLoc(pSoldier->ubID, sLastSeenSpot, pSoldier->pathing.bLevel, FALSE);

					//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] increment watch location at %d", pSoldier->ubID, sLastSeenSpot);
					//BeginMultiPurposeLocator(sLastSeenSpot, pSoldier->pathing.bLevel, FALSE);
				}
			}
			gubNPCAPBudget = 0;*/
		}

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Red flashlight]"));

		// maybe use flashlight to reveal enemy position
		if (pSoldier->bTeam == ENEMY_TEAM &&
			gfTurnBasedAI &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			!pSoldier->bBlindedCounter &&
			IsActionAffordable(pSoldier, AI_ACTION_SWITCH_FLASHLIGHT) &&
			!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT) &&
			AIUseFlashlightCondition(pSoldier, DIRECTION_IRRELEVANT) &&
			CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE) > 0 &&
			Chance(100 - 100 / (1 + SoldierDifficultyLevel(pSoldier))) &&
			!AICheckSuccessfulAttack(pSoldier, FALSE) &&
			(fAnyCover || !TileIsOutOfBounds(sClosestOpponent) && AICheckWeOutnumber(pSoldier, sClosestOpponent)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("choose flashlight direction"));
			ubOpponentDir = AIChooseFlashlightDirection(pSoldier);			

			if( ubOpponentDir != DIRECTION_IRRELEVANT )
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("found flashlight direction %d", ubOpponentDir));

				if (pSoldier->ubDirection == ubOpponentDir)
				{
					// turn on flashlight
					pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
					pSoldier->aiData.usNextActionData = 0;
					DebugAI(AI_MSG_INFO, pSoldier, String("activate flashlight, end turn"));
					return(AI_ACTION_SWITCH_FLASHLIGHT);
				}
			}			
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Smoke to cover movement]"));

		// try to use smoke to cover movement
		if (gfTurnBasedAI &&
			!pSoldier->bBlindedCounter &&
			FindThrowableGrenade(pSoldier, EXPLOSV_SMOKE) != NO_SLOT &&
			pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
			(pSoldier->bActionPoints == pSoldier->bInitialActionPoints || pSoldier->RushAttackPrepare()) &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			bOpponentLevel == pSoldier->pathing.bLevel &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			!AIRoleSniper(pSoldier) &&
			!AIRoleMachinegunner(pSoldier) &&
			pSoldier->aiData.bOrders != STATIONARY &&
			!pSoldier->bStealthMode &&
			!AICheckSuccessfulAttack(pSoldier, TRUE) &&
			pSoldier->bTacticalOrder != TACTICAL_ORDER_RETREAT &&
			(pSoldier->RushAttackPrepare() ||
			pSoldier->aiData.bUnderFire ||
			fSectorAttack && PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > DAY_VISION_RANGE ||
			CountSeenEnemiesLastTurn(pSoldier) > CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2) ||
			CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE) > CountFriendsLastAttackHit(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE) ||
			CountCorpses(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE, TRUE) > CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE)) &&
			(pSoldier->RushAttackPrepare() ||
			fSectorAttack ||
			InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE) ||
			Chance(SoldierDifficultyLevel(pSoldier) * 10) ||
			Chance(TeamPercentKilled(pSoldier->bTeam)) ||
			Chance(10 * CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE)) ||
			Chance(10 * CountVisibleSmokeEffects(pSoldier, DAY_VISION_RANGE, NORMAL_SMOKE_EFFECT)) ||
			Chance(10 * CountCorpses(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE, TRUE))))
		{
			//gubNPCAPBudget = APBPConstants[AP_MAXIMUM];
			gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;

			BestThrow.ubPossible = FALSE;

			// check path to closest disturbance
			if (FindBestPath(pSoldier, sClosestOpponent, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, 0))
			{
				INT16 sLoop;

				DebugAI(AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", sClosestOpponent, pSoldier->pathing.usPathDataSize));

				sCheckGridNo = pSoldier->sGridNo;
				sSmokeSpot = NOWHERE;

				for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
				{
					sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

					// find last dangerous spot or last spot seen by enemy if rushing
					if (!TileIsOutOfBounds(sCheckGridNo) &&
						PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) < TACTICAL_RANGE / 2 &&
						PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) > TACTICAL_RANGE / 4 &&
						!Water(sCheckGridNo, pSoldier->pathing.bLevel) &&
						!InSmokeNearby(sCheckGridNo, pSoldier->pathing.bLevel) &&
						!InSmoke(sCheckGridNo, pSoldier->pathing.bLevel, TRUE) &&
						/*(pSoldier->RushAttackPrepare() ||
						fSectorAttack ||
						CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel) ||
						InLightAtNight(sCheckGridNo, pSoldier->pathing.bLevel)) &&*/
						!SightCoverAtSpot(pSoldier, sCheckGridNo, ANIM_STAND, FALSE))
					{
						CheckTossGrenadeAt(pSoldier, &BestThrow, sCheckGridNo, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);
						if (BestThrow.ubPossible)
						{
							sSmokeSpot = sCheckGridNo;
						}
					}
				}

				if (!TileIsOutOfBounds(sSmokeSpot))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("found smoke spot %d ", sSmokeSpot));
					CheckTossGrenadeAt(pSoldier, &BestThrow, sSmokeSpot, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);
				}

				// found throw spot
				if (BestThrow.ubPossible)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

					// if necessary, swap the usItem from holster into the hand position
					if (BestThrow.bWeaponIn != HANDPOS)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
						RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
					}

					// stand up before throwing if needed
					if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
						pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
					{
						pSoldier->aiData.usActionData = BestThrow.ubStance;
						pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
						pSoldier->aiData.usNextActionData = BestThrow.sTarget;
						pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
						pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
						return AI_ACTION_CHANGE_STANCE;
					}

					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

					return(AI_ACTION_TOSS_PROJECTILE);
				}
			}
			gubNPCAPBudget = 0;
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Throw flashbang before entering room]"));
		// use flashbang before entering room
		if (gfTurnBasedAI &&
			!pSoldier->bBlindedCounter &&
			pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
			(pSoldier->bActionPoints == pSoldier->bInitialActionPoints || pSoldier->RushAttackPrepare()) &&
			pSoldier->pathing.bLevel == 0 &&			
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			!AIRoleSniper(pSoldier) &&
			!AIRoleMachinegunner(pSoldier) &&
			pSoldier->aiData.bOrders != STATIONARY &&
			!pSoldier->bStealthMode &&
			pSoldier->bTacticalOrder != TACTICAL_ORDER_RETREAT &&
			FindThrowableGrenade(pSoldier, EXPLOSV_FLASHBANG) != NO_SLOT &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			bOpponentLevel == pSoldier->pathing.bLevel &&
			InARoom(sClosestOpponent) &&
			CountFriendsInRoom(pSoldier, RoomNo(sClosestOpponent)) == 0 &&
			CountFriendsBlack(pSoldier, sClosestOpponent) == 0 &&
			(pSoldier->RushAttackActive() ||
				pSoldier->aiData.bUnderFire ||
				CountCorpsesInRoom(pSoldier, RoomNo(sClosestOpponent), pSoldier->pathing.bLevel) > 0 ||
				Chance(TeamPercentKilled(pSoldier->bTeam)) ||
				Chance(10 * SoldierDifficultyLevel(pSoldier)))
			)
		{
			gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;

			BestThrow.ubPossible = FALSE;

			INT8 bGrenadeSlot = FindThrowableGrenade(pSoldier, EXPLOSV_FLASHBANG);
			UINT16 usGrenade = pSoldier->inv[bGrenadeSlot].usItem;
			UINT8 ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 4, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));

			// check path to closest disturbance
			if (FindBestPath(pSoldier, sClosestOpponent, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, 0))
			{
				INT16 sLoop;

				DebugAI(AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", sClosestOpponent, pSoldier->pathing.usPathDataSize));

				sCheckGridNo = pSoldier->sGridNo;
				sSmokeSpot = NOWHERE;

				for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
				{
					sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

					// find last dangerous spot or last spot seen by enemy if rushing
					if (!TileIsOutOfBounds(sCheckGridNo) &&
						PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) < TACTICAL_RANGE / 2 &&
						PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) > TACTICAL_RANGE / 8 &&
						!Water(sCheckGridNo, pSoldier->pathing.bLevel) &&
						InARoom(sCheckGridNo) &&
						SameRoom(sCheckGridNo, sClosestOpponent) &&
						CountNearbyFriends(pSoldier, sCheckGridNo, ubSafetyMargin) == 0)
					{
						CheckTossGrenadeAt(pSoldier, &BestThrow, sCheckGridNo, pSoldier->pathing.bLevel, EXPLOSV_FLASHBANG);

						if (BestThrow.ubPossible)
						{
							sSmokeSpot = sCheckGridNo;
						}
					}
				}

				if (!TileIsOutOfBounds(sSmokeSpot))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("found smoke spot %d ", sSmokeSpot));
					CheckTossGrenadeAt(pSoldier, &BestThrow, sSmokeSpot, pSoldier->pathing.bLevel, EXPLOSV_FLASHBANG);
				}

				// found throw spot
				if (BestThrow.ubPossible)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

					// if necessary, swap the usItem from holster into the hand position
					if (BestThrow.bWeaponIn != HANDPOS)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
						RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
					}

					// stand up before throwing if needed
					if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
						pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
					{
						pSoldier->aiData.usActionData = BestThrow.ubStance;
						pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
						pSoldier->aiData.usNextActionData = BestThrow.sTarget;
						pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
						pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
						return AI_ACTION_CHANGE_STANCE;
					}

					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

					return(AI_ACTION_TOSS_PROJECTILE);
				}
			}
			gubNPCAPBudget = 0;
		}
		
		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Smoke to cover position]"));
		// try to use smoke to cover position from attackers
		if (gfTurnBasedAI &&
			!pSoldier->bBlindedCounter &&
			SoldierAI(pSoldier) &&
			pSoldier->bTeam == ENEMY_TEAM &&
			AllowComplexAI(pSoldier) &&
			pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
			!AICheckSuccessfulAttack(pSoldier, TRUE) &&
			!pSoldier->RushAttackAdvance() &&
			CountThrowableGrenades(pSoldier, EXPLOSV_SMOKE) > 1 &&
			(pSoldier->aiData.bUnderFire || 
			pSoldier->aiData.bShock > 0 || 
			fSectorAttack ||
			CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE) > 0) &&
			(Chance(SoldierDifficultyLevel(pSoldier) * 10) ||
				Chance(TeamPercentKilled(pSoldier->bTeam) / 2) ||
				Chance(10 * CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE))))
		{
			UINT32 uiCoverStart = GetJA2Clock();
			CheckTossCoverSmoke(pSoldier, &BestThrow);

			UINT32 uiCoverEnd = GetJA2Clock();
			//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"[%d] smoke cover time %.2f", pSoldier->ubID, (uiCoverEnd - uiCoverStart) / 1000.0f);

			if (BestThrow.ubPossible && Chance(BestThrow.iAttackValue))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

				// if necessary, swap the usItem from holster into the hand position
				if (BestThrow.bWeaponIn != HANDPOS)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
					RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
				}

				// stand up before throwing if needed
				if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
					pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
				{
					pSoldier->aiData.usActionData = BestThrow.ubStance;
					pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
					pSoldier->aiData.usNextActionData = BestThrow.sTarget;
					pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
					return AI_ACTION_CHANGE_STANCE;
				}
				else
				{
					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				}

				DebugAI(AI_MSG_INFO, pSoldier, String("throw smoke grenade to cover position at spot %d level %d", BestThrow.sTarget, BestThrow.bTargetLevel));

				return(AI_ACTION_TOSS_PROJECTILE);
			}
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Grenade for special purpose]"));
		// try to use regular grenade for special purpose
		if (gfTurnBasedAI &&
			!pSoldier->bBlindedCounter &&
			SoldierAI(pSoldier) &&
			pSoldier->bTeam == ENEMY_TEAM &&
			!gfHiddenInterrupt &&
			!gTacticalStatus.fInterruptOccurred &&
			pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			pSoldier->aiData.bOrders != STATIONARY &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			(pSoldier->RushAttackPrepare() || 
			pSoldier->aiData.bOrders == SEEKENEMY && TileIsOutOfBounds(sClosestDisturbance) ||
			Chance(20 * SoldierDifficultyLevel(pSoldier) + 10 * CountThrowableGrenades(pSoldier, EXPLOSV_NORMAL, 10))))
		{
			CheckTossGrenadeSpecial(pSoldier, &BestThrow);

			if (BestThrow.ubPossible && (pSoldier->RushAttackPrepare() || Chance(BestThrow.iAttackValue)))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

				// if necessary, swap the usItem from holster into the hand position
				if (BestThrow.bWeaponIn != HANDPOS)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
					RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
				}

				INT16 sTooCloseDistance = DAY_VISION_RANGE / 4;
				if (PythSpacesAway(pSoldier->sGridNo, BestThrow.sTarget) < sTooCloseDistance ||
					CountNearbyFriends(pSoldier, BestThrow.sTarget, sTooCloseDistance) > 0 ||
					CountNearbyNeutrals(pSoldier, BestThrow.sTarget, sTooCloseDistance) > 0)
				{
					// too close to soldier or any friend, set grenade as delayed
					if (Explosive[Item[pSoldier->inv[HANDPOS].usItem].ubClassIndex].ubType == EXPLOSV_NORMAL)
					{
						pSoldier->inv[HANDPOS][0]->data.sObjectFlag |= DELAYED_GRENADE_EXPLOSION;
					}					
				}

				// stand up before throwing if needed
				if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
					pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
				{
					pSoldier->aiData.usActionData = BestThrow.ubStance;
					pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
					pSoldier->aiData.usNextActionData = BestThrow.sTarget;
					pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
					return AI_ACTION_CHANGE_STANCE;
				}
				else
				{
					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				}

				DebugAI(AI_MSG_INFO, pSoldier, String("throw grenade at spot %d level %d", BestThrow.sTarget, BestThrow.bTargetLevel));

				//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] throw grenade at %d", pSoldier->ubID, BestThrow.sTarget);
				//BeginMultiPurposeLocator(BestThrow.sTarget, BestThrow.bTargetLevel, FALSE);

				return(AI_ACTION_TOSS_PROJECTILE);
			}
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[crouch if in building]"));
		// sevenfm: before anything else, if we are lying prone in a building and the enemy is close so he can be seen, try to crouch first
		if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE &&
			!pSoldier->bBlindedCounter &&
			IsValidStance(pSoldier, ANIM_CROUCH) &&
			pSoldier->CheckInitialAP() &&
			//pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints &&
			!TileIsOutOfBounds(sClosestOpponent) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) <= pSoldier->GetMaxDistanceVisible(sClosestOpponent) &&
			//LOS_Virtual(pSoldier, sClosestOpponent, bOpponentLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) &&
			LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestOpponent, bOpponentLevel, pSoldier->GetMaxDistanceVisible(sClosestOpponent)) &&
			InARoom(pSoldier->sGridNo, NULL))
		{
			// maybe raise weapon after crouching
			if (PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < 2 * pSoldier->GetMaxDistanceVisible(sClosestOpponent))
			{
				// determine direction from this soldier to the closest opponent
				ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestOpponent);

				if (PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
					AIGunInHandScoped(pSoldier) &&
					!WeaponReady(pSoldier) &&
					pSoldier->ubDirection == ubOpponentDir &&
					GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)) + GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
				{
					pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
					pSoldier->aiData.usNextActionData = 0;
				}
			}

			pSoldier->aiData.usActionData = ANIM_CROUCH;
			return(AI_ACTION_CHANGE_STANCE);
		}

		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Turn to closest possibly seen]"));
		// sevenfm: try to turn to closest possibly seen opponent
		if (//pSoldier->CheckInitialAP() &&
			pSoldier->AnimEndHeight() >= ANIM_CROUCH &&
			!pSoldier->bBlindedCounter &&
			!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_UNSEEN) &&
			pSoldier->bActionPoints >= GetAPsToLook(pSoldier) &&
			!TileIsOutOfBounds(sClosestPossiblySeenOpponent))
		{
			ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestPossiblySeenOpponent);

			// if soldier is not already facing in that direction,
			// and the opponent is close enough that he could possibly be seen
			if (pSoldier->ubDirection != ubOpponentDir &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
			{
				// turn
				pSoldier->aiData.usActionData = ubOpponentDir;
				pSoldier->aiData.bNextAction = AI_ACTION_NONE;
				pSoldier->aiData.usNextActionData = 0;

				// don't turn to unseen again this turn
				pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_UNSEEN;

				DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest possibly seen opponent"));
				return(AI_ACTION_CHANGE_FACING);
			}
		}

		// if we can move at least 1 square's worth
		// and have more APs than we want to reserve
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Main Red decisions]"));

		if (ubCanMove && pSoldier->bActionPoints > APBPConstants[MAX_AP_CARRIED])
		{
			// sevenfm: blinded soldier only interested in hiding
			if (pSoldier->bBlindedCounter)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("blinded soldier is only interested in hiding"));
				// only interested in hiding out...
				bSeekPts = -99;
				bHelpPts = -99;
				bHidePts =  +1;
				bWatchPts = -99;
			}
			else
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("checking hide/seek/help/watch points"));
				
				// calculate initial points for watch based on highest watch loc
				bWatchPts = GetHighestWatchedLocPoints(pSoldier->ubID);
				if (bWatchPts <= 0)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("no watch locations - disable watch"));
					bWatchPts = -99;
				}
				if (TileIsOutOfBounds(sClosestFriend))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("no reachable friend - disable help"));
					bHelpPts = -99;
				}
				if (TileIsOutOfBounds(sClosestDisturbance))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("no reachable disturbance - disable seek"));
					bSeekPts = -99;
				}
				if (TileIsOutOfBounds(sClosestOpponent))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("no opponents known - disable hide"));
					bHidePts = -99;
				}
				if (pSoldier->SkipCoverCheck())
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("skip cover check - disable hide, seek and help"));
					bHidePts = -99;
					bSeekPts = -99;
					bHelpPts = -99;
				}
				if (pSoldier->usSkillCounter[SOLDIER_COUNTER_COVER])
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("soldier executed AI_ACTION_TAKE_COVER this turn - disable seek and help"));
					bSeekPts = -99;
					bHelpPts = -99;
				}
				// can help only at the start of the turn
				if (!pSoldier->CheckInitialAP())
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("not at the start of the turn - disable help"));
					bHelpPts = -99;
				}
				// disable seek/help if soldier is lying prone in safe place and saw enemy recently and can attack him
				if (pSoldier->AnimEndHeight() == ANIM_PRONE &&
					pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
					fSafeSpot &&
					!pSoldier->RushAttackAdvance() &&
					!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
					GuySawEnemy(pSoldier) &&
					!TileIsOutOfBounds(sClosestDisturbance) &&
					AIGunRange(pSoldier) >= PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) &&
					CountFriendsBlack(pSoldier, sClosestDisturbance) == 0 &&
					!AICheckSuccessfulAttack(pSoldier, TRUE))
				{
					bSeekPts = -99;
					if (!AIRoleMedic(pSoldier))
						bHelpPts = -99;
					DebugAI(AI_MSG_INFO, pSoldier, String("lying prone - disable seek and help (except medics)"));
				}
				// disable seek/help not at the start of the turn
				/*if (!pSoldier->RushAttackAdvance() &&
					(pSoldier->bTilesMoved ||
					GuySawEnemy(pSoldier, SEEN_THIS_TURN) ||
					pSoldier->FiredThisTurn() || 
					pSoldier->bInitialActionPoints - pSoldier->bActionPoints > APBPConstants[MAX_AP_CARRIED]))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("not at the start of the turn - disable help and seek"));					
					bSeekPts = -99;
					bHelpPts = -99;
				}*/
				// no taking cover in deep water
				if( DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("deep water - disable hide"));
					bHidePts = -99;
				}
				// sevenfm: stationary should not seek/help
				if ( pSoldier->aiData.bOrders == STATIONARY )
				{
					bSeekPts = -99;
					bHelpPts = -99;
					DebugAI( AI_MSG_INFO, pSoldier, String("disable seek/help for stationary/snipers"));
				}
				// sevenfm: don't help if seen enemy recently or under fire
				if( (GuySawEnemy(pSoldier) || pSoldier->aiData.bUnderFire) && !AIRoleMedic(pSoldier) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("seen enemy recently or under fire - disable help"));
					bHelpPts = -99;
				}
				// special AI roles don't help
				if (AISpecialRole(pSoldier))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("special AI role - disable help"));
					bHelpPts = -99;
				}
				// rush attack
				if( pSoldier->RushAttackActive() )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("rush attack active - disable help"));
					bHelpPts = -99;
				}

				// don't search for cover in realtime
				if (!gfTurnBasedAI && !(pSoldier->CheckInitialAP() && (GuySawEnemy(pSoldier) || GuyHeardEnemy(pSoldier, HEARD_2_TURNS_AGO)) && Random(3) == 0))
				{					
					DebugAI(AI_MSG_INFO, pSoldier, String("don't search for cover in realtime"));
					bHidePts = -99;
				}

				PrepareMainRedAIWeights(pSoldier, bSeekPts, bHelpPts, bHidePts, bWatchPts);				
				DebugAI( AI_MSG_INFO, pSoldier, String("Basic weights: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));

				// sevenfm: snipers and soldiers with scoped guns should decide watch more often
				if( AIGunScoped(pSoldier) )
				{
					bWatchPts++;
				}
				if( AIRoleSniper(pSoldier) )
				{
					bWatchPts++;
				}

				// sevenfm: use watching more often when using focus skill and in safe spot
				if (pSoldier->IsFocusing() && fSafeSpot)
				{
					bWatchPts++;
				}

				// sevenfm: disable seek/help when in building and seen enemy recently
				// check that closest reachable enemy is not in the same building
				// check that we can attack enemy with current gun
				if( (GuySawEnemy(pSoldier) && RangeChangeDesire(pSoldier) < 4 || CountSeenEnemiesLastTurn(pSoldier) > 0 ) &&
					InARoom(pSoldier->sGridNo, NULL) &&
					!TileIsOutOfBounds(sClosestDisturbance) &&
					!SameBuilding(pSoldier->sGridNo, sClosestDisturbance) &&
					!pSoldier->RushAttackAdvance() &&
					AICheckHasGun(pSoldier) &&
					//!AICheckShortWeaponRange(pSoldier) &&
					AIGunRange(pSoldier) * 3 / 2 >= PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) )
				{					
					bSeekPts = -99;
					if(!AIRoleMedic(pSoldier))
						bHelpPts = -99;
					DebugAI( AI_MSG_INFO, pSoldier, String("disable seek/help when in a building and seen enemy recently"));
					DebugAI( AI_MSG_INFO, pSoldier, String("Weights: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));
				}

				// sevenfm: penalize watching if soldier is under fire or in dangerous place
				if( pSoldier->CheckInitialAP() &&
					(!fAnyCover && !fProneSightCover && pSoldier->pathing.bLevel == 0 ||
					pSoldier->aiData.bUnderFire ||
					InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) && !InARoom(pSoldier->sGridNo, NULL)) )
				{
					// prefer hiding when in dangerous place
					if (bHidePts > -90)
					{
						bWatchPts = min(bWatchPts, bHidePts - 1);
					}
					DebugAI( AI_MSG_INFO, pSoldier, String("prefer hiding when in dangerous place"));
					DebugAI( AI_MSG_INFO, pSoldier, String("Weights: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));
				}

				// sevenfm: don't watch when overcrowded and not in a building
				if( !InARoom(pSoldier->sGridNo, NULL) && !fAnyCover)
				{
					bWatchPts -= CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 8);
					DebugAI( AI_MSG_INFO, pSoldier, String("Overcrowded: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));
				}								

				// penalty to watch decision weight from successful attack
				if( AICheckSuccessfulAttack(pSoldier, TRUE) && !AIRoleSniper(pSoldier) )
				{
					bWatchPts -= 1;
					DebugAI( AI_MSG_INFO, pSoldier, String("Successful attack: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));
				}

				// sevenfm: watching modifiers depending on watched spot and enemy
				bHighestWatchLoc = GetHighestVisibleWatchedLoc( pSoldier->ubID );				
				if (bHighestWatchLoc != -1)
				{
					INT32 sWatchSpot = gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc];
					INT8 bWatchLevel = gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc];

					if (!TileIsOutOfBounds(sWatchSpot))
					{
						if (pSoldier->aiData.bOrders != STATIONARY && !AIRoleSniper(pSoldier))
						{
							UINT8 ubFriendsBlackWatch = CountFriendsBlack(pSoldier, sWatchSpot);
							bWatchPts -= ubFriendsBlackWatch;
							DebugAI(AI_MSG_INFO, pSoldier, String("Friends black at watched location %d", ubFriendsBlackWatch));
							DebugAI(AI_MSG_INFO, pSoldier, String("Weight: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));

							// penalize watching at night if soldier has no NVG and watched location is not in light
							if (NightLight() &&
								!InLightAtNight(sWatchSpot, bWatchLevel) &&
								!AICheckNVG(pSoldier) &&
								!InARoom(pSoldier->sGridNo, NULL))
							{
								bWatchPts -= 1;
								DebugAI(AI_MSG_INFO, pSoldier, String("penalize watching at night as soldier has no NVG and watched location is not in light"));
								DebugAI(AI_MSG_INFO, pSoldier, String("Weight: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));
							}

							// penalize watching if watched location is too far for weapon
							if (AIGunRange(pSoldier) < PythSpacesAway(pSoldier->sGridNo, sWatchSpot))
							{
								bWatchPts -= (PythSpacesAway(pSoldier->sGridNo, sWatchSpot) - AIGunRange(pSoldier)) / 4;
								DebugAI(AI_MSG_INFO, pSoldier, String("penalize watching because of short weapon range"));
								DebugAI(AI_MSG_INFO, pSoldier, String("Weight: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));
							}

							// penalize watching if enemy has high interrupt level and watching soldier has no prone sight cover
							INT8 bMaxEnemyLevel = FindMaxEnemyInterruptLevel(pSoldier, sWatchSpot, bWatchLevel, DAY_VISION_RANGE / 8);
							INT8 bMyEffectiveLevel = AIEstimateInterruptLevel(pSoldier) + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 8);
							if (bMaxEnemyLevel > bMyEffectiveLevel &&
								LOS_LocationStance(sWatchSpot, bWatchLevel, ANIM_STAND, pSoldier->sGridNo, pSoldier->pathing.bLevel, ANIM_PRONE, MAX_VISION_RANGE))
							{
								bWatchPts -= bMaxEnemyLevel - bMyEffectiveLevel;
								DebugAI(AI_MSG_INFO, pSoldier, String("penalize watching - high enemy interrupt level and no prone cover from watched location"));
								DebugAI(AI_MSG_INFO, pSoldier, String("Weight: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));
							}

							// penalize watching if we outnumber enemy
							if (AICheckWeOutnumber(pSoldier, sWatchSpot))
							{
								bWatchPts -= 1;
								DebugAI(AI_MSG_INFO, pSoldier, String("We outnumber: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));
							}
						}
					}				

					// sevenfm: prefer hiding if soldier cannot interrupt
					if (pSoldier->CheckInitialAP() &&
						!pSoldier->CanInterrupt())
					{
						if (bHidePts > -90)
						{
							bWatchPts = min(bWatchPts, bHidePts - 1);
						}
						DebugAI(AI_MSG_INFO, pSoldier, String("soldier cannot interrupt, prefer hiding instead of watching"));
						DebugAI(AI_MSG_INFO, pSoldier, String("Weights: hide = %d, seek = %d, watch = %d, help = %d", bHidePts, bSeekPts, bWatchPts, bHelpPts));
					}
				}

				// penalize seek/help if target is too far
				INT32 sFromSpot;
				INT16 sCheckRange;
				INT16 sSeekRange = SeekRange(pSoldier, &sFromSpot);

				if( pSoldier->bTeam == ENEMY_TEAM &&
					pSoldier->ubSoldierClass != SOLDIER_CLASS_ADMINISTRATOR &&
					pSoldier->aiData.bOrders != SEEKENEMY &&
					!pSoldier->RushAttackAdvance() &&
					(AICheckTown() || AICheckUnderground()) &&
					CountPublicKnownEnemies(pSoldier) < 10 - 2 * gGameOptions.ubDifficultyLevel &&
					!TileIsOutOfBounds(sFromSpot) && 
					sSeekRange > 0 )
				{
					// penalize seek
					if(!TileIsOutOfBounds(sClosestDisturbance))
					{
						sCheckRange = PythSpacesAway(sFromSpot, sClosestDisturbance);
						bSeekPts -= sCheckRange / sSeekRange;
						DebugAI( AI_MSG_INFO, pSoldier, String("Penalize seek distance: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));
					}
					// penalize help
					if (!TileIsOutOfBounds(sClosestFriend))
					{
						sCheckRange = PythSpacesAway(sFromSpot, sClosestFriend);
						bHelpPts -= sCheckRange / sSeekRange;
						DebugAI( AI_MSG_INFO, pSoldier, String("Penalize help distance: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));
					}
				}

				if( pSoldier->RetreatActive() )
				{
					// no seeking when retreating
					bSeekPts = -99;
					// no helping when retreating
					bHelpPts = -99;

					if( bHidePts > -90 )
					{
						bWatchPts = min(bWatchPts, bHidePts - 1);
					}
				}

				if( pSoldier->RushAttackAdvance() )
				{
					bSeekPts += 4;
					bWatchPts -= 4;
				}

				// sevenfm: more active civilians
				if(pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral && pSoldier->ubCivilianGroup != NON_CIV_GROUP)
				{
					bSeekPts += 2;
					bWatchPts -= 2;
				}

				// AI medics prefer help over seek
				if (AIRoleMedic(pSoldier))
				{
					if (bHidePts > -90)
						bSeekPts = min(bSeekPts, bHidePts - 1);
				}
			}

			DebugAI( AI_MSG_INFO, pSoldier, String("Decision weights: hide = %d, seek = %d, watch = %d, help = %d",bHidePts,bSeekPts,bWatchPts,bHelpPts));

			// while one of the three main RED REACTIONS remains viable
			while ((bSeekPts > -90) || (bHelpPts > -90) || (bHidePts > -90) )
			{	
				//////////////////////////////////////////////////////////////////////
				// RED Seek
				//////////////////////////////////////////////////////////////////////				

				// if SEEKING is possible and at least as desirable as helping or hiding
				if ( ((bSeekPts > -90) && (bSeekPts >= bHelpPts) && (bSeekPts >= bHidePts) && (bSeekPts >= bWatchPts )) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("[seek]"));

					// sevenfm: disable help if seek decided to prevent AI from going back and forth
					bHelpPts = -99;

					// if there is an opponent reachable					
					// sevenfm: allow seeking in prone stance if we haven't seen enemy for several turns
					// allow seeking if current spot is dangerous (no prone sight cover or under attack)
					if (!TileIsOutOfBounds(sClosestDisturbance))
					{
						// sevenfm: before anything else, try to turn to closest disturbance
						/*DebugAI( AI_MSG_TOPIC, pSoldier, String("[turn to closest threat before seeking]"));
						if( !TileIsOutOfBounds(sClosestThreat) &&
							GuySawEnemy(pSoldier) &&
							pSoldier->AnimEndHeight() > ANIM_PRONE &&
							!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_UNSEEN) &&
							LOS_Virtual(pSoldier, sClosestThreat, bClosestThreatLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
						{
							ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestThreat);

							// if soldier is not already facing in that direction,
							// and the opponent is close enough that he could possibly be seen
							if( pSoldier->ubDirection != ubOpponentDir &&
								pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
								pSoldier->bActionPoints >= GetAPsToLook(pSoldier)  )
							{
								// turn
								pSoldier->aiData.usActionData = ubOpponentDir;
								pSoldier->aiData.bNextAction = AI_ACTION_NONE;
								pSoldier->aiData.usNextActionData = 0;

								// don't turn to unseen again this turn
								pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_UNSEEN;

								DebugAI( AI_MSG_INFO, pSoldier, String("turn to closest threat before seeking"));
								return(AI_ACTION_CHANGE_FACING);
							}
						}*/

						//////////////////////////////////////////////////////////////////////
						// SEEK CLOSEST DISTURBANCE: GO DIRECTLY TOWARDS CLOSEST KNOWN OPPONENT
						//////////////////////////////////////////////////////////////////////

						INT16 bReserveSeekAP = 0;
						UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_SEEK_OPPONENT);
						INT32 sTestMoveSpot = InternalGoAsFarAsPossibleTowards(pSoldier, sClosestDisturbance, bReserveSeekAP, AI_ACTION_SEEK_OPPONENT, 0);

						if(!TileIsOutOfBounds(sClosestDisturbance) && 
							!TileIsOutOfBounds(sTestMoveSpot) &&
							PythSpacesAway(sTestMoveSpot, sClosestDisturbance) <= DAY_VISION_RANGE &&
							!SightCoverAtSpot(pSoldier, sTestMoveSpot, ANIM_STAND, FALSE) &&
							!pSoldier->RushAttackAdvance() &&
							!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE))
						{
							if (usMovementMode == RUNNING || usMovementMode == WALKING)
							{
								bReserveSeekAP = APBPConstants[AP_CHANGE_FACING] + GetAPsCrouch(pSoldier, TRUE);
								if (!SightCoverAtSpot(pSoldier, sTestMoveSpot, ANIM_CROUCH, FALSE))
								{
									bReserveSeekAP += GetAPsProne(pSoldier, TRUE);
								}
							}
							else if (usMovementMode == SWATTING)
							{
								bReserveSeekAP = APBPConstants[AP_LOOK_CROUCHED] + GetAPsProne(pSoldier, TRUE);
							}
						}

						DebugAI( AI_MSG_INFO, pSoldier, String("ReserveSeekAP %d", bReserveSeekAP));

						// sevenfm: sClosestDisturbance is the climb point
						// if we already there, then try to climb
						if (pSoldier->sGridNo == sClosestDisturbance)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("at seek spot already"));
							if (fSeekClimb)
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("climb spot"));

								// wait for next turn if turnbased (to climb with max APs)
								if (gfTurnBasedAI &&
									pSoldier->bActionPoints < pSoldier->bInitialActionPoints)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("wait for next turn to climb with full AP"));

									pSoldier->aiData.bNextAction = AI_ACTION_NONE;
									pSoldier->aiData.usNextActionData = 0;
									return AI_ACTION_END_TURN;
								}

								// climb
								if (IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("climb"));

									pSoldier->aiData.bNextAction = AI_ACTION_NONE;
									pSoldier->aiData.usNextActionData = 0;
									return AI_ACTION_CLIMB_ROOF;
								}

								// cannot climb at all
								DebugAI(AI_MSG_INFO, pSoldier, String("cannot climb"));
							}
							else if(ubSeekType == SEEK_JUMP_WINDOW)
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("jump window spot"));

								// wait for next turn if turnbased
								if (gfTurnBasedAI &&
									pSoldier->bActionPoints < pSoldier->bInitialActionPoints &&
									!IsActionAffordable(pSoldier, AI_ACTION_JUMP_WINDOW))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("wait for next turn to jump window with full AP"));

									pSoldier->aiData.bNextAction = AI_ACTION_NONE;
									pSoldier->aiData.usNextActionData = 0;
									return AI_ACTION_END_TURN;
								}

								if (DecideJumpWindow(pSoldier, sClosestOpponent))
									return pSoldier->aiData.bAction;

								DebugAI(AI_MSG_INFO, pSoldier, String("cannot jump window"));
							}
							else if (ubSeekType == SEEK_PLANT_BOMB)
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("plant bomb spot"));

								// wait for next turn if turnbased
								if (gfTurnBasedAI &&
									pSoldier->bActionPoints < pSoldier->bInitialActionPoints &&
									!IsActionAffordable(pSoldier, AI_ACTION_PLANT_BOMB))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("wait for next turn to plant bomb with full AP"));

									pSoldier->aiData.bNextAction = AI_ACTION_NONE;
									pSoldier->aiData.usNextActionData = 0;
									return AI_ACTION_END_TURN;
								}

								if (DecideBlowUpObstacle(pSoldier, sClosestOpponent))
									return pSoldier->aiData.bAction;
							}
						}
						else
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("try to move towards closest disturbance %d reserving %d AP", sClosestDisturbance, bReserveSeekAP));

							INT32 sAdvanceSpot = NOWHERE;

							// determine regular move spot
							INT32 sMoveSpot = InternalGoAsFarAsPossibleTowards(pSoldier, sClosestDisturbance, bReserveSeekAP, AI_ACTION_SEEK_OPPONENT, 0);
							DebugAI(AI_MSG_INFO, pSoldier, String("move spot %d", sMoveSpot));

							DebugAI(AI_MSG_INFO, pSoldier, String("possibly find unlimited sight cover advance spot"));
							// sevenfm: try to advance using sight cover
							if( gfTurnBasedAI &&
								AllowComplexAI(pSoldier) &&
								!TileIsOutOfBounds(sMoveSpot) &&
								TileIsOutOfBounds(sAdvanceSpot) &&
								UseSightCoverAdvance(pSoldier) &&
								!pSoldier->RushAttackAdvance() &&
								(AICheckTown() || AICheckUnderground() || AICheckShortWeaponRange(pSoldier) || !AICheckHasGun(pSoldier) || CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE) > 0) &&
								!AICheckSuccessfulAttack(pSoldier, TRUE) &&
								!TileIsOutOfBounds(sClosestOpponent) &&
								//PythSpacesAway(sMoveSpot, sClosestOpponent) < AIMaxDistanceVisible(NULL, sMoveSpot, pSoldier->pathing.bLevel) &&
								(PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE || 
								(AISpecialRole(pSoldier) || TeamHighPercentKilled(pSoldier->bTeam) || pSoldier->aiData.bAIMorale < MORALE_CONFIDENT) && PythSpacesAway(sMoveSpot, sClosestOpponent) <= MAX_VISION_RANGE) &&
								PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&								
								(!SightCoverAtSpot(pSoldier, sMoveSpot, ANIM_STAND, FALSE) || InLightAtNight(sMoveSpot, pSoldier->pathing.bLevel) || CorpseWarning(pSoldier, sMoveSpot, pSoldier->pathing.bLevel)))
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("find advance spot"));
								sAdvanceSpot = FindAdvanceSpot(pSoldier, sClosestDisturbance, AI_ACTION_SEEK_OPPONENT, ADVANCE_SPOT_SIGHT_COVER, TRUE);
								
								if( !TileIsOutOfBounds(sAdvanceSpot) )
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("found advance spot %d", sAdvanceSpot));
								}
								else if (pSoldier->bActionPoints < pSoldier->bInitialActionPoints && 
										(!pSoldier->RushAttackAdvance() || pSoldier->bActionPoints < APBPConstants[AP_MINIMUM] / 2) &&
										(pSoldier->aiData.bOrders != SEEKENEMY || pSoldier->aiData.bAIMorale < MORALE_FEARLESS) &&
										!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE) &&
										PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE &&
										SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("has sight cover at current spot, wait for next turn to check with full AP"));
									return(AI_ACTION_END_TURN);
								}
							}

							// sevenfm: try to find advance spot with prone sight cover
							/*if (gfTurnBasedAI &&
								!TileIsOutOfBounds(sMoveSpot) &&
								TileIsOutOfBounds(sAdvanceSpot) &&
								UseSightCoverAdvance(pSoldier) &&
								!AICheckSuccessfulAttack(pSoldier, TRUE) &&
								!TileIsOutOfBounds(sClosestOpponent) &&
								(PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE || 
								(AICheckSpecialRole(pSoldier) || TeamHighPercentKilled(pSoldier->bTeam) || pSoldier->aiData.bAIMorale < MORALE_CONFIDENT) && PythSpacesAway(sMoveSpot, sClosestOpponent) <= MAX_VISION_RANGE) &&
								PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
								!SightCoverAtSpot(pSoldier, sMoveSpot, ANIM_PRONE, FALSE))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("find prone advance spot"));
								sAdvanceSpot = FindAdvanceSpot(pSoldier, sClosestDisturbance, AI_ACTION_SEEK_OPPONENT, ADVANCE_SPOT_PRONE_COVER, FALSE);

								if (!TileIsOutOfBounds(sAdvanceSpot))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("found prone advance spot %d", sAdvanceSpot));
								}
							}*/

							DebugAI(AI_MSG_INFO, pSoldier, String("possibly find any cover advance spot"));
							// sevenfm: try to find advance spot with any cover
							if( gfTurnBasedAI &&
								!TileIsOutOfBounds(sMoveSpot) &&
								!TileIsOutOfBounds(sClosestOpponent) &&
								TileIsOutOfBounds(sAdvanceSpot) &&
								UseSightCoverAdvance(pSoldier) &&		
								//PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < DAY_VISION_RANGE * 2 &&
								(PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE || 
								(AISpecialRole(pSoldier) || TeamHighPercentKilled(pSoldier->bTeam) || pSoldier->aiData.bAIMorale < MORALE_CONFIDENT) && PythSpacesAway(sMoveSpot, sClosestOpponent) <= MAX_VISION_RANGE) &&
								PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
								(!SightCoverAtSpot(pSoldier, sMoveSpot, ANIM_PRONE, FALSE) || InLightAtNight(sMoveSpot, pSoldier->pathing.bLevel) || CorpseWarning(pSoldier, sMoveSpot, pSoldier->pathing.bLevel)) &&
								!AnyCoverAtSpot(pSoldier, sMoveSpot) )
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("find any cover advance spot to %d", sClosestDisturbance));
								sAdvanceSpot = FindAdvanceSpot(pSoldier, sClosestDisturbance, AI_ACTION_SEEK_OPPONENT, ADVANCE_SPOT_ANY_COVER, FALSE);
								DebugAI(AI_MSG_INFO, pSoldier, String("advance spot %d", sAdvanceSpot));

								if( !TileIsOutOfBounds(sAdvanceSpot) )
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("found any cover advance spot %d", sAdvanceSpot));
								}
								else if (pSoldier->bActionPoints < pSoldier->bInitialActionPoints &&
									(!pSoldier->RushAttackAdvance() || pSoldier->bActionPoints < APBPConstants[AP_MINIMUM] / 2) &&
									(pSoldier->aiData.bOrders != SEEKENEMY || pSoldier->aiData.bAIMorale < MORALE_FEARLESS) &&
									!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE) &&
									PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE &&
									AnyCoverAtSpot(pSoldier, pSoldier->sGridNo))
								{
									// if not already crouched, crouch down first
									if (pSoldier->AnimEndHeight() > ANIM_CROUCH &&
										IsValidStance(pSoldier, ANIM_CROUCH) &&
										GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints &&
										!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE))
									{
										pSoldier->aiData.usActionData = ANIM_CROUCH;
										pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
										pSoldier->aiData.usNextActionData = 0;
										return AI_ACTION_CHANGE_STANCE;
									}

									// if not prone, go prone first
									if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&										
										pSoldier->AnimEndHeight() > ANIM_PRONE &&
										IsValidStance(pSoldier, ANIM_PRONE) &&
										GetAPsToChangeStance(pSoldier, ANIM_PRONE) <= pSoldier->bActionPoints &&
										!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE))
									{
										pSoldier->aiData.usActionData = ANIM_PRONE;
										pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
										pSoldier->aiData.usNextActionData = 0;
										return AI_ACTION_CHANGE_STANCE;
									}

									DebugAI(AI_MSG_INFO, pSoldier, String("has any cover at current spot, wait for next turn to check with full AP"));
									return(AI_ACTION_END_TURN);
								}
							}

							DebugAI(AI_MSG_INFO, pSoldier, String("possibly find sight cover advance spot"));
							// try to find spot with sight cover - useful at night or when advancing using smoke
							if (gfTurnBasedAI &&
								AllowComplexAI(pSoldier) &&
								!TileIsOutOfBounds(sMoveSpot) &&
								TileIsOutOfBounds(sAdvanceSpot) &&
								UseSightCoverAdvance(pSoldier) &&
								!pSoldier->RushAttackAdvance() &&
								!AICheckSuccessfulAttack(pSoldier, TRUE) &&
								!TileIsOutOfBounds(sClosestOpponent) &&
								(PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE || 
								(AISpecialRole(pSoldier) || TeamHighPercentKilled(pSoldier->bTeam) || pSoldier->aiData.bAIMorale < MORALE_CONFIDENT) && PythSpacesAway(sMoveSpot, sClosestOpponent) <= MAX_VISION_RANGE) &&
								PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
								pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
								//(NightLight() || FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, FALSE)) &&
								FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, FALSE) &&
								!SightCoverAtSpot(pSoldier, sMoveSpot, ANIM_STAND, FALSE))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("find sight advance spot"));
								sAdvanceSpot = FindAdvanceSpot(pSoldier, sClosestDisturbance, AI_ACTION_SEEK_OPPONENT, ADVANCE_SPOT_SIGHT_COVER, FALSE, SPOT_FLAG_SMOKE);

								if (!TileIsOutOfBounds(sAdvanceSpot))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("found sight advance spot %d", sAdvanceSpot));
								}
								else if (pSoldier->bActionPoints < pSoldier->bInitialActionPoints &&
									(!pSoldier->RushAttackAdvance() || pSoldier->bActionPoints < APBPConstants[AP_MINIMUM] / 2) &&
									(pSoldier->aiData.bOrders != SEEKENEMY || pSoldier->aiData.bAIMorale < MORALE_FEARLESS) &&
									!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE) &&
									PythSpacesAway(sMoveSpot, sClosestOpponent) <= DAY_VISION_RANGE &&
									SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) && 
									(InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE) || AnyCoverAtSpot(pSoldier, pSoldier->sGridNo)))
								{
									// if not already crouched, crouch down first
									if (pSoldier->AnimEndHeight() > ANIM_CROUCH &&
										IsValidStance(pSoldier, ANIM_CROUCH) &&
										GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints &&
										!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE))
									{
										pSoldier->aiData.usActionData = ANIM_CROUCH;
										pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
										pSoldier->aiData.usNextActionData = 0;
										return AI_ACTION_CHANGE_STANCE;
									}

									DebugAI(AI_MSG_INFO, pSoldier, String("has sight cover at current spot, wait for next turn to check with full AP"));
									return(AI_ACTION_END_TURN);
								}
							}

							// update path							
							if( !TileIsOutOfBounds(sAdvanceSpot) )
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("find path to advance spot %d", sAdvanceSpot));
								pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, sAdvanceSpot, 0, AI_ACTION_SEEK_OPPONENT, 0);
								DebugAI(AI_MSG_INFO, pSoldier, String("check path to advance spot %d path spot %d", sAdvanceSpot, pSoldier->aiData.usActionData));
							}
							else
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("find path to disturbance %d", sClosestDisturbance));
								// failed to find any advance spot, use regular movement code								
								pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, sClosestDisturbance, bReserveSeekAP, AI_ACTION_SEEK_OPPONENT, 0);
								DebugAI(AI_MSG_INFO, pSoldier, String("check path to disturbance spot %d reserving %d AP, path spot %d", sClosestDisturbance, bReserveSeekAP, pSoldier->aiData.usActionData));
							}

							// if it's possible						
							if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
							{
								BOOLEAN fAbortSeek = FALSE;
								BOOLEAN fOvercrowded = FALSE;
								sDangerousSpot = NOWHERE;
								sLastSafeSpot = NOWHERE;

								DebugAI( AI_MSG_INFO, pSoldier, String("found seek spot %d", pSoldier->aiData.usActionData));

								fAbortSeek = AbortFinalSpot(pSoldier, pSoldier->aiData.usActionData, AI_ACTION_SEEK_OPPONENT, sClosestDisturbance, bClosestDisturbanceLevel, sDangerousSpot);

								// sevenfm: need to check path for fresh corpses
								if(!fAbortSeek)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("check path for dangerous spots"));
									fAbortSeek = AbortPath(pSoldier, AI_ACTION_SEEK_OPPONENT, sClosestDisturbance, bClosestDisturbanceLevel, sDangerousSpot, sLastSafeSpot);
									if(fAbortSeek && sLastSafeSpot != NOWHERE)
									{
										fAbortSeek = FALSE;
										pSoldier->aiData.usActionData = sLastSafeSpot;
									}
								}

								if( CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4) > 2 )
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("current location is overcrowded"));
									fOvercrowded = TRUE;
								}

								BOOLEAN fNoStartFlanking = FALSE;								

								if (pSoldier->bTeam == ENEMY_TEAM &&
									fSectorAttack && 
									ubSectorLeader != NOBODY &&
									(ubSectorLeader % 3) == 1)
								{
									fNoStartFlanking = TRUE;
								}

								DebugAI( AI_MSG_TOPIC, pSoldier, String("[Possibly start RED flanking]"));

								// sevenfm: possibly start RED flanking
								if (pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
									!(pSoldier->usSoldierFlagMask2 & SOLDIER_DISABLE_FLANKING) &&
									!AICheckUnderground() &&
									!fNoStartFlanking &&
									pSoldier->bTeam == ENEMY_TEAM &&
									( pSoldier->aiData.bAttitude == CUNNINGAID || pSoldier->aiData.bAttitude == CUNNINGSOLO ||
									( pSoldier->aiData.bAttitude == BRAVESOLO || pSoldier->aiData.bAttitude == BRAVEAID ) && (fOvercrowded || pSoldier->RetreatActive()) ) &&									
									pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
									pSoldier->CheckInitialAP() &&
									!pSoldier->RushAttackActive() &&
									pSoldier->ubSoldierClass != SOLDIER_CLASS_ADMINISTRATOR &&
									!AIRoleCommander(pSoldier) &&
									!AIRoleSniper(pSoldier) &&
									!AIRoleMachinegunner(pSoldier) &&
									!AIRoleMortarOperator(pSoldier) &&
									(gAnimControl[ pSoldier->usAnimState ].ubHeight != ANIM_PRONE || fOvercrowded || pSoldier->RetreatActive()) &&
									!pSoldier->aiData.bUnderFire &&
									pSoldier->pathing.bLevel == 0 &&
									( pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->aiData.bOrders == FARPATROL ||	pSoldier->aiData.bOrders == CLOSEPATROL && NightTime() ) &&
									(!GuySawEnemy( pSoldier ) || fOvercrowded || pSoldier->RetreatActive() || fAbortSeek) &&
									!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
									(fAbortSeek || CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) < CountNearbyFriends(pSoldier, sClosestDisturbance, MAX_FLANK_DIST) / 2) &&
									PythSpacesAway ( pSoldier->sGridNo, sClosestDisturbance ) > MIN_FLANK_DIST &&
									(PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < MAX_FLANK_DIST || fAbortSeek && pSoldier->aiData.bAttitude == CUNNINGSOLO) &&
									(fAbortSeek || CountFriendsBetweenMeAndSpotFromSpot(pSoldier, sClosestDisturbance) > 0 || NightTime() || fOvercrowded))
								{
									UINT8 ubFriends, ubFriendsLeft, ubFriendsRight;
									UINT8 ubDirection = AIDirection(sClosestDisturbance, pSoldier->sGridNo);

									ubFriends = CountFriendsInDirectionFromSpot(pSoldier, sClosestDisturbance, ubDirection, VISION_RANGE * 2);
									ubFriendsLeft = CountFriendsInDirectionFromSpot(pSoldier, sClosestDisturbance, gOneCDirection[ubDirection], VISION_RANGE * 2) +
													CountFriendsInDirectionFromSpot(pSoldier, sClosestDisturbance, gTwoCDirection[ubDirection], VISION_RANGE * 2);
									ubFriendsRight = CountFriendsInDirectionFromSpot(pSoldier, sClosestDisturbance, gOneCCDirection[ubDirection], VISION_RANGE * 2) +
													CountFriendsInDirectionFromSpot(pSoldier, sClosestDisturbance, gTwoCCDirection[ubDirection], VISION_RANGE * 2);

									BOOLEAN fLeftFlankPossible = FALSE;
									BOOLEAN fRightFlankPossible = FALSE;

									if (ubFriendsLeft < ubFriends)
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("left flanking possible"));
										fLeftFlankPossible = TRUE;
									}

									if (ubFriendsRight < ubFriends)
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("right flanking possible"));
										fRightFlankPossible = TRUE;
									}

									INT32 sFlankingSpot = NOWHERE;									
									INT8 bAction = AI_ACTION_NONE;

									// decide flanking direction
									if (fLeftFlankPossible && !fRightFlankPossible)
									{
										bAction = AI_ACTION_FLANK_LEFT;
									}
									else if (fRightFlankPossible && !fLeftFlankPossible)
									{
										bAction = AI_ACTION_FLANK_RIGHT;
									}
									else if (fLeftFlankPossible && fRightFlankPossible)
									{
										if (Random(6) < 3)
											bAction = AI_ACTION_FLANK_LEFT;
										else
											bAction = AI_ACTION_FLANK_RIGHT;
									}
									
									if(bAction != AI_ACTION_NONE)
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("search flanking spot for %s [%d]", szAction[bAction], bAction));

										sFlankingSpot = FindFlankingSpot(pSoldier, sClosestDisturbance, bAction);

										// if found good flank spot, start flanking
										if (!TileIsOutOfBounds(sFlankingSpot))
										{
											DebugAI(AI_MSG_INFO, pSoldier, String("found flanking spot %d", sFlankingSpot));

											INT8 bTacticalOrder;
											if (bAction == AI_ACTION_FLANK_LEFT)
												bTacticalOrder = TACTICAL_ORDER_FLANK_LEFT;
											else
												bTacticalOrder = TACTICAL_ORDER_FLANK_RIGHT;

											pSoldier->aiData.usActionData = sFlankingSpot;

											// start flanking order
											pSoldier->StartTacticalOrder(bTacticalOrder, sClosestDisturbance, bClosestDisturbanceLevel, AIDirection(pSoldier->sGridNo, sClosestDisturbance), 0, FLANKING_DURATION);

											// prevent this soldier from flanking in the future
											pSoldier->usSoldierFlagMask2 |= SOLDIER_DISABLE_FLANKING;

											// sevenfm: change orders when starting to flank
											if (pSoldier->aiData.bOrders == CLOSEPATROL)
											{
												DebugAI(AI_MSG_INFO, pSoldier, String("change CLOSEPATROL to FARPATROL"));
												pSoldier->aiData.bOrders = FARPATROL;
											}

											DebugAI(AI_MSG_INFO, pSoldier, String("start %s flanking", pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT ? "left" : "right"));

											//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] starts %s flanking around %d friends flank %d", pSoldier->ubID, pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT ? L"left" : L"right", sClosestDisturbance, CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance));
											//BeginMultiPurposeLocator(sClosestDisturbance, pSoldier->pathing.bLevel, FALSE);

											return(bAction);
										}
										else
										{
											DebugAI(AI_MSG_INFO, pSoldier, String("cannot find flanking spot"));
										}
									}
									else
									{
										DebugAI(AI_MSG_INFO, pSoldier, String("no valid flanking direction"));
									}
								}							

								// if it's possible
								if( !fAbortSeek )
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("seek move is possible"));

									if (fSeekClimb)
									{
										DebugAI( AI_MSG_INFO, pSoldier, String("move to climb spot"));
										return( AI_ACTION_SEEK_OPPONENT );
										//return AI_ACTION_MOVE_TO_CLIMB;
									}
									else
									{
										DebugAI( AI_MSG_INFO, pSoldier, String("move to disturbance"));

										BOOLEAN fSkipCautiousMove = TRUE;
										BOOLEAN fSlowMovement = TRUE;

										// reserve APs for a possible crouch plus a shot
										INT8 bReserveAP = (INT8)(MinAPsToAttack(pSoldier, sClosestDisturbance, ADDTURNCOST, 0, TRUE) + GetAPsCrouch(pSoldier, TRUE));										

										if (pSoldier->RushAttackAdvance() ||
											pSoldier->aiData.bUnderFire && !GuySawEnemy(pSoldier) ||
											CountFriendsBlack(pSoldier, sClosestDisturbance) > 0)
										{
											DebugAI(AI_MSG_INFO, pSoldier, String("use fast movement"));
											fSlowMovement = FALSE;
										}

										// determine cautious move spot
										INT32 sCautiousMoveSpot = InternalGoAsFarAsPossibleTowards(pSoldier, pSoldier->aiData.usActionData, bReserveAP, AI_ACTION_SEEK_OPPONENT, fSlowMovement ? FLAG_CAUTIOUS : 0);
										DebugAI(AI_MSG_INFO, pSoldier, String("found cautious move spot %d reserving %d AP moving to %d slow movement %d", sCautiousMoveSpot, bReserveAP, pSoldier->aiData.usActionData, fSlowMovement));

										if (!TileIsOutOfBounds(sCautiousMoveSpot) &&
											(pSoldier->RushAttackAdvance() ||
											!TileIsOutOfBounds(sAdvanceSpot) ||
											InLightAtNight(sCautiousMoveSpot, pSoldier->pathing.bLevel) ||
											CountFriendsBlack(pSoldier, sClosestDisturbance) > 0 || 
											pSoldier->aiData.bUnderFire && !GuySawEnemy(pSoldier) ||
											FindBombNearby(pSoldier, sCautiousMoveSpot, BOMB_DETECTION_RANGE) ||
											CorpseWarning(pSoldier, sCautiousMoveSpot, pSoldier->pathing.bLevel) ||
											EnemyCanAttackSpot(pSoldier, sCautiousMoveSpot, pSoldier->pathing.bLevel) ||
											!SightCoverAtSpot(pSoldier, sCautiousMoveSpot, ANIM_STAND, FALSE)))
										{
											DebugAI(AI_MSG_INFO, pSoldier, String("moving to advance spot or new spot is dangerous - skip cautious move"));
											fSkipCautiousMove = TRUE;
										}

										// let's be a bit cautious about going right up to a location without enough APs to shoot
										if (!fSkipCautiousMove &&
											!TileIsOutOfBounds(sCautiousMoveSpot) &&
											(PythSpacesAway(pSoldier->aiData.usActionData, sClosestDisturbance) < 5 ||
											SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE) &&
											!SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE) &&
											PythSpacesAway(pSoldier->aiData.usActionData, sClosestDisturbance) < TACTICAL_RANGE / 2) &&
											CountFriendsBlack(pSoldier, sClosestDisturbance) == 0)
										{
											DebugAI(AI_MSG_INFO, pSoldier, String("cautious move to %d", sCautiousMoveSpot));
											DebugAI(AI_MSG_INFO, pSoldier, String("reserve %d AP for shot", bReserveAP));

											ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestDisturbance);

											// raise gun first
											if( PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
												pSoldier->ubDirection == ubOpponentDir &&												
												(!gfTurnBasedAI || pSoldier->bActionPoints == pSoldier->bInitialActionPoints) &&
												gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_STAND &&
												pSoldier->aiData.bLastAction != AI_ACTION_RAISE_GUN &&
												fSlowMovement &&
												!WeaponReady(pSoldier) &&
												(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30))
											{
												if( GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) <= pSoldier->bActionPoints )
												{
													DebugAI(AI_MSG_INFO, pSoldier, String("raise gun and move cautious to %d", sCautiousMoveSpot));

													pSoldier->aiData.bNextAction = AI_ACTION_SEEK_OPPONENT;
													pSoldier->aiData.usNextActionData = sCautiousMoveSpot;
													pSoldier->aiData.ubAIFlags |= AI_CAUTIOUS;

													return AI_ACTION_RAISE_GUN;
												}
											}
											
											DebugAI(AI_MSG_INFO, pSoldier, String("found cautious move spot %d", sCautiousMoveSpot));

											if (fSlowMovement)
											{
												pSoldier->aiData.ubAIFlags |= AI_CAUTIOUS;
											}

											pSoldier->aiData.usActionData = sCautiousMoveSpot;
											DebugAI(AI_MSG_INFO, pSoldier, String("end turn after movement"));
											pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
											pSoldier->aiData.usNextActionData = 0;

											DebugAI(AI_MSG_INFO, pSoldier, String("movement mode %d", DetermineMovementMode(pSoldier, AI_ACTION_SEEK_OPPONENT)));

											// alt movement mode when seeking opponent
											if (DecideSeekReverse(pSoldier, sClosestThreat, bClosestThreatLevel))
												return pSoldier->aiData.bAction;

											return AI_ACTION_SEEK_OPPONENT;

											//DebugAI( AI_MSG_INFO, pSoldier, String("don't have enough AP to reserve for attack"));
											// trying cautious move, don't have enough APs to reserve for attack
										}
										else
										{
											// not doing cautious move, seek opponent
											DebugAI(AI_MSG_INFO, pSoldier, String("not doing cautious move, seek to %d", pSoldier->aiData.usActionData));

											// alt movement mode when seeking opponent
											if (DecideSeekReverse(pSoldier, sClosestThreat, bClosestThreatLevel))
												return pSoldier->aiData.bAction;

											return(AI_ACTION_SEEK_OPPONENT);
										}
										// not climbing, trying cautious move, don't have enough APs to reserve for attack
										DebugAI( AI_MSG_INFO, pSoldier, String("not climbing, trying cautious move, don't have enough APs to reserve for attack"));
									}
								}
								else
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("abort seek, maybe try to climb here?"));

									// increment seek counter
									pSoldier->StopSeekIncrement();

									// abort seek, maybe try to climb here?
									if( SoldierAI(pSoldier) &&
										(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM ) && 
										RangeChangeDesire(pSoldier) < 4 &&
										pSoldier->aiData.bOrders != STATIONARY &&
										pSoldier->pathing.bLevel == 0 &&
										gfTurnBasedAI &&
										pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
										pSoldier->aiData.bLastAction != AI_ACTION_CLIMB_ROOF &&
										CountNearbyFriendsOnRoof(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4) < CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4) / 2 &&
										Chance(100 - 100 / (1 + 2*CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4))) &&
										CanClimbFromHere(pSoldier) &&
										IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF) )
									{
										return( AI_ACTION_CLIMB_ROOF );
									}

									// try to use smoke to cover movement
									DebugAI( AI_MSG_TOPIC, pSoldier, String("[try to use smoke to cover movement]"));

									if( !TileIsOutOfBounds(sDangerousSpot) &&
										!Water(sDangerousSpot, pSoldier->pathing.bLevel) &&
										(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
										!InSmoke(sDangerousSpot, pSoldier->pathing.bLevel, FALSE) &&
										!InSmokeNearby(sDangerousSpot, pSoldier->pathing.bLevel) &&
										(CorpseWarning(pSoldier, sDangerousSpot, pSoldier->pathing.bLevel) || 
										InLightAtNight(sDangerousSpot, pSoldier->pathing.bLevel) && !SightCoverAtSpot(pSoldier, sDangerousSpot, ANIM_STAND, FALSE) ||
										InARoom(sDangerousSpot, NULL)) &&
										(	pSoldier->RushAttackAdvance() ||
											InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE) ||
											Chance(min(SoldierDifficultyLevel(pSoldier) * 10, pSoldier->StopSeekValue() * 10)) || 
											Chance(TeamPercentKilled(pSoldier->bTeam)) || 
											Chance(10 * CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2))) )
									{
										DebugAI( AI_MSG_INFO, pSoldier, String("check if soldier can cover movement path with smoke"));

										CheckTossGrenadeAt(pSoldier, &BestThrow, sDangerousSpot, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);

										// check adjacent spots
										if (!BestThrow.ubPossible)
										{
											UINT8	ubMovementCost;
											INT32	sTempGridNo;
											UINT8	ubDirection;

											// check adjacent reachable tiles
											for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
											{
												sTempGridNo = NewGridNo(sDangerousSpot, DirectionInc(ubDirection));

												if (sTempGridNo != sDangerousSpot)
												{
													ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][pSoldier->pathing.bLevel];

													if (ubMovementCost < TRAVELCOST_BLOCKED )
													{
														CheckTossGrenadeAt(pSoldier, &BestThrow, sTempGridNo, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);
														if (BestThrow.ubPossible)
														{
															break;
														}
													}
												}
											}
										}

										if (BestThrow.ubPossible)
										{
											DebugAI( AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

											// if necessary, swap the usItem from holster into the hand position
											if (BestThrow.bWeaponIn != HANDPOS)
											{
												DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket"));
												RearrangePocket(pSoldier,HANDPOS,BestThrow.bWeaponIn,FOREVER);
											}

											// stand up before throwing if needed
											if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
												pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
											{
												pSoldier->aiData.usActionData = BestThrow.ubStance;
												pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
												pSoldier->aiData.usNextActionData = BestThrow.sTarget;
												pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
												pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
												return AI_ACTION_CHANGE_STANCE;
											}

											pSoldier->aiData.usActionData	= BestThrow.sTarget;
											pSoldier->bTargetLevel			= BestThrow.bTargetLevel;
											pSoldier->aiData.bAimTime		= BestThrow.ubAimTime;

											return(AI_ACTION_TOSS_PROJECTILE);
										}
									}

									// possibly start rush attack
									DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start local rush attack]"));
									if (pSoldier->bTeam == ENEMY_TEAM &&										
										pSoldier->CheckInitialAP() &&
										IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT) &&
										pSoldier->StopSeekValue() > 1 &&
										CanRushAttack(pSoldier) &&										
										!TileIsOutOfBounds(sClosestDisturbance) &&
										AICheckWeOutnumberLocal(pSoldier, sClosestDisturbance) &&
										pSoldier->pathing.bLevel == 0 &&
										PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < TACTICAL_RANGE &&
										CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) == 0)
									{
										UINT8 ubCanRushFriends;
										UINT8 ubRushAttackFriends;										
										UINT8 ubFriendsUnderAttack;
										UINT8 ubCorpses;
										UINT8 ubKnownEnemies;
										UINT8 ubOutnumberBonus = 0;
										INT32 iPathCost;

										iPathCost = EstimatePlotPath(pSoldier, sClosestDisturbance, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
										ubCanRushFriends = CountCanRushAttackFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4, APBPConstants[AP_MAXIMUM], TRUE, FALSE);
										ubRushAttackFriends = CountRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE);
										ubFriendsUnderAttack = CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE / 4);
										ubCorpses = CountCorpses(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4, FALSE);
										ubKnownEnemies = CountPublicKnownEnemies(pSoldier, sClosestDisturbance, TACTICAL_RANGE / 4);

										if (ubCanRushFriends > ubKnownEnemies)
										{
											ubOutnumberBonus = ubCanRushFriends - ubKnownEnemies;
										}

										//INT32 sMoveSpot = InternalGoAsFarAsPossibleTowards(pSoldier, sClosestDisturbance, 0, AI_ACTION_SEEK_OPPONENT, 0);
										//UINT8 ubCorpses = CountPathCorpses(pSoldier);										

										DebugAI(AI_MSG_INFO, pSoldier, String("ubCanRushFriends %d ubFriendsUnderAttack %d ubCorpses %d ubKnownEnemies %d", ubCanRushFriends, ubFriendsUnderAttack, ubCorpses, ubKnownEnemies));

										if (iPathCost > 0 &&
											iPathCost < APBPConstants[AP_MAXIMUM] * 2 &&
											ubCanRushFriends + ubRushAttackFriends > ubCorpses + ubCorpses * TeamPercentKilled(pSoldier->bTeam) / 100 &&
											ubCanRushFriends + ubRushAttackFriends > ubKnownEnemies + ubKnownEnemies * TeamPercentKilled(pSoldier->bTeam) / 100 &&
											Chance(10 * pSoldier->StopSeekValue() + ubRushAttackFriends * 10) &&
											(Chance(ubOutnumberBonus * 5) || Chance(ubFriendsUnderAttack * 10) || Chance(ubRushAttackFriends * 10)))
										{
											DebugAI(AI_MSG_INFO, pSoldier, String("start local rush attack at spot %d", sClosestDisturbance));
											StartRushAttackFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4, APBPConstants[AP_MAXIMUM]);
											pSoldier->RushAttackStart();

											// simulate radio communication
											return(AI_ACTION_FAKE_ALERT);
										}										
									}
								}

								DebugAI( AI_MSG_INFO, pSoldier, String("found enemy, movement is possible but seek aborted, disable help decision also"));
								// found enemy, movement is possible but seek aborted (InLightAtNight or found fresh corpse)
							}
							else
							{
								// found reachable disturbance but cannot find path
								DebugAI( AI_MSG_INFO, pSoldier, String("found enemy, cannot find path"));
							}					
						}
					}
					else
					{
						DebugAI( AI_MSG_INFO, pSoldier, String("no reachable disturbance or lying prone so cannot seek"));
					}

					// mark SEEKING as impossible for next time through while loop
					bSeekPts = -99;

					DebugAI( AI_MSG_INFO, pSoldier, String("seek failed"));
				}				

				// if WATCHING is possible and at least as desirable as anything else
				if ((bWatchPts > -90) && (bWatchPts >= bSeekPts) && (bWatchPts >= bHelpPts) && (bWatchPts >= bHidePts ))
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("[watch]"));

					// take a look at our highest watch point... if it's still visible, turn to face it and then wait
					bHighestWatchLoc = GetHighestVisibleWatchedLoc( pSoldier->ubID );
					//sDistVisible =  DistanceVisible( pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, gsWatchedLoc[ pSoldier->ubID ][ bHighestWatchLoc ] );

					if ( bHighestWatchLoc != -1 )
					{
						// see if we need turn to face that location
						ubOpponentDir = AIDirection(pSoldier->sGridNo, gsWatchedLoc[ pSoldier->ubID ][ bHighestWatchLoc ]);

						DebugAI(AI_MSG_INFO, pSoldier, String("Highest watch location: [%d] %d %d watch dir: %d", bHighestWatchLoc, gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc], gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc], ubOpponentDir));

						// consider at least crouching
						if( gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_STAND &&
							IsValidStance( pSoldier, ANIM_CROUCH ) &&
							pSoldier->bActionPoints >= GetAPsCrouch(pSoldier, TRUE) )
						{
							pSoldier->aiData.usActionData = ANIM_CROUCH;

							DebugAI( AI_MSG_INFO, pSoldier, String("crouch to watch"));
							return(AI_ACTION_CHANGE_STANCE);
						}

						// raise weapon if not raised
						if( PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
							!WeaponReady(pSoldier) &&
							(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
							pSoldier->bActionPoints >= GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("raise weapon"));
							return AI_ACTION_RAISE_GUN;
						}

						// if soldier is not already facing in that direction,
						// and the opponent is close enough that he could possibly be seen
						if( pSoldier->ubDirection != ubOpponentDir &&
							pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
							pSoldier->bActionPoints >= GetAPsToLook(pSoldier)  )
						{
							// turn
							pSoldier->aiData.usActionData = ubOpponentDir;
							//pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
							DebugAI( AI_MSG_INFO, pSoldier, String("turn to watched location"));
							return(AI_ACTION_CHANGE_FACING);
						}						

						// possibly go prone, check that we'll have line of sight to standing enemy at watched location
						// only in turnbased and only at the start of new turn
						if (gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_CROUCH &&
							IsValidStance( pSoldier, ANIM_PRONE ) &&
							pSoldier->bActionPoints >= GetAPsProne(pSoldier, TRUE) &&
							(!InARoom(pSoldier->sGridNo, NULL) || pSoldier->pathing.bLevel > 0 || pSoldier->aiData.bUnderFire) &&
							gfTurnBasedAI &&
							//pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
							LOS_LocationStance(pSoldier->sGridNo, pSoldier->pathing.bLevel, ANIM_PRONE, gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc], gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc], ANIM_STAND, MAX_VISION_RANGE))
						{
							pSoldier->aiData.usActionData = ANIM_PRONE;

							if (PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
								pSoldier->bActionPoints >= GetAPsProne(pSoldier, TRUE) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon after going prone"));
								pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
							}
							//pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
							//pSoldier->aiData.usNextActionData = 0;

							DebugAI( AI_MSG_INFO, pSoldier, String("go prone"));
							return(AI_ACTION_CHANGE_STANCE);
						}

						// possibly start focus on watched location
						/*if (pSoldier->ubDirection == ubOpponentDir &&
							WeaponReady(pSoldier) &&
							AIGunInHandScoped(pSoldier) &&
							!pSoldier->IsFocusing() &&
							pSoldier->bActionPoints >= GetAPsToFocus(pSoldier, gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc]) &&
							CheckFocusSpot(pSoldier, gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc], gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc]) &&
							pSoldier->CanFocus(gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc], gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc]))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("start focusing at watched location: spot %d level %d", gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc], gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc]));

							pSoldier->ubAISkillUse = SKILLS_FOCUS;
							pSoldier->aiData.usActionData = gsWatchedLoc[pSoldier->ubID][bHighestWatchLoc];
							pSoldier->bTargetLevel = gbWatchedLocLevel[pSoldier->ubID][bHighestWatchLoc];
							return AI_ACTION_USE_SKILL;
						}*/

						// sevenfm: possibly use focus to scan area
						if ((gfTurnBasedAI || pSoldier->CheckInitialAP()) &&
							SoldierAI(pSoldier) &&
							WeaponReady(pSoldier) &&
							AIGunInHandScoped(pSoldier) &&
							pSoldier->bActionPoints >= APBPConstants[AP_SPOTTER] &&
							//!pSoldier->aiData.bUnderFire &&
							//!GuySawEnemy(pSoldier, SEEN_LAST_TURN) &&
							!TileIsOutOfBounds(sClosestOpponent) &&
							PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 2 &&
							!pSoldier->IsFocusing() &&
							pSoldier->CanUseSkill(SKILLS_FOCUS, FALSE) &&
							(AIRoleSniper(pSoldier) || Chance(10 * SoldierDifficultyLevel(pSoldier))))
						{
							INT32 sFocusSpot;
							INT8 bFocusLevel;

							if (FindCombatFocusSpot(pSoldier, sFocusSpot, bFocusLevel))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("start focusing at location: spot %d level %d", sFocusSpot, bFocusLevel));

								pSoldier->ubAISkillUse = SKILLS_FOCUS;
								pSoldier->aiData.usActionData = sFocusSpot;
								pSoldier->bTargetLevel = bFocusLevel;

								if (!gfTurnBasedAI)
								{
									pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
									pSoldier->aiData.usNextActionData = RealtimeDelay(pSoldier);
								}

								return AI_ACTION_USE_SKILL;
							}
						}

						//ScreenMsg(FONT_MCOLOR_LTBLUE, MSG_INTERFACE, L"[%d] watch", pSoldier->ubID);
						//BeginMultiPurposeLocator(gsWatchedLoc[ pSoldier->ubID ][ bHighestWatchLoc ], gbWatchedLocLevel[ pSoldier->ubID ][ bHighestWatchLoc ], FALSE);

						DebugAI( AI_MSG_INFO, pSoldier, String("watch at %d level %d", gsWatchedLoc[ pSoldier->ubID ][ bHighestWatchLoc ], gbWatchedLocLevel[ pSoldier->ubID ][ bHighestWatchLoc ]));
						return(AI_ACTION_NONE);
						//return(AI_ACTION_END_TURN);
					}

					bWatchPts = -99;
					DebugAI( AI_MSG_INFO, pSoldier, String("watch failed"));
				}
				
				// if HELPING is possible and at least as desirable as seeking or hiding
				if ((bHelpPts > -90) && (bHelpPts >= bSeekPts) && (bHelpPts >= bHidePts) && (bHelpPts >= bWatchPts ))
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("[help]"));

					// sevenfm: disable seek if help decided to prevent AI from going back and forth
					bSeekPts = -99;

					if (!TileIsOutOfBounds(sClosestFriend) )
					{
						//////////////////////////////////////////////////////////////////////
						// GO DIRECTLY TOWARDS CLOSEST FRIEND UNDER FIRE OR WHO LAST RADIOED
						//////////////////////////////////////////////////////////////////////
						DebugAI( AI_MSG_INFO, pSoldier, String("closest friend %d", sClosestFriend));

						INT16 bReserveAP = GetAPsCrouch( pSoldier, TRUE);
						if(fHelpClimb || DetermineMovementMode(pSoldier, AI_ACTION_SEEK_FRIEND) == RUNNING)
						{
							bReserveAP = 0;
						}
						DebugAI( AI_MSG_INFO, pSoldier, String("reserve %d AP", bReserveAP));

						pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier,sClosestFriend,bReserveAP, AI_ACTION_SEEK_FRIEND,0);
						DebugAI( AI_MSG_INFO, pSoldier, String("found help spot %d", pSoldier->aiData.usActionData));

						sDangerousSpot = NOWHERE;
						sLastSafeSpot = NOWHERE;

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) && 
							AbortFinalSpot(pSoldier, pSoldier->aiData.usActionData, AI_ACTION_SEEK_FRIEND, sClosestFriend, bClosestDisturbanceLevel, sDangerousSpot))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("dangerous spot! abort!"));
							pSoldier->aiData.usActionData = NOWHERE;
						}

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
							AbortPath(pSoldier, AI_ACTION_SEEK_FRIEND, sClosestDisturbance, bClosestDisturbanceLevel, sDangerousSpot, sLastSafeSpot))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("dangerous spot! abort!"));
							pSoldier->aiData.usActionData = NOWHERE;
						}

						// if new spot is safe to move
						if( !TileIsOutOfBounds(pSoldier->aiData.usActionData))
						{
							if ( fHelpClimb )
							{
								// need to climb AND have enough APs to get there this turn
								if (pSoldier->sGridNo == sClosestFriend)
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("at climb gridno"));

									// wait for next turn to have full APs
									if( gfTurnBasedAI && pSoldier->bActionPoints < pSoldier->bInitialActionPoints )
									{
										DebugAI( AI_MSG_INFO, pSoldier, String("wait for next turn"));
										return( AI_ACTION_END_TURN);
									}
									// climb
									if( IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF) )
									{
										DebugAI( AI_MSG_INFO, pSoldier, String("climb"));

										return( AI_ACTION_CLIMB_ROOF );
									}
									// don't have enough AP for climbing
									DebugAI( AI_MSG_INFO, pSoldier, String("don't have enough AP for climbing"));
								}
								else
								{
									DebugAI( AI_MSG_INFO, pSoldier, String("move to climbing spot"));
									// move to climbing spot
									return(AI_ACTION_SEEK_FRIEND);
									//return AI_ACTION_MOVE_TO_CLIMB;
								}
							}
							else
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("seek friend"));

								return(AI_ACTION_SEEK_FRIEND);
							}
						}
						else
						{
							// help aborted
						}
					}

					// mark HELPING as impossible for next time through while loop
					DebugAI( AI_MSG_INFO, pSoldier, String("help failed"));
					bHelpPts = -99;
				}

				// if HIDING is possible and at least as desirable as seeking or helping
				if ((bHidePts > -90) && (bHidePts >= bSeekPts) && (bHidePts >= bHelpPts) && (bHidePts >= bWatchPts ))
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("[hide]"));

					// disable help if hiding is preferred action
					bHelpPts = -99;
					// disable seek if soldier is in safe spot
					if (fSafeSpot || InARoom(pSoldier->sGridNo, NULL) && (fAnyCover || fProneSightCover))
					{
						bSeekPts = -99;
					}

					//////////////////////////////////////////////////////////////////////
					// TAKE BEST NEARBY COVER FROM ALL KNOWN OPPONENTS
					//////////////////////////////////////////////////////////////////////
					DebugAI( AI_MSG_INFO, pSoldier, String("closest opponent %d", sClosestOpponent));

					pSoldier->aiData.usActionData = FindBestNearbyCover(pSoldier, &iDummy);

					DebugAI( AI_MSG_INFO, pSoldier, String("found cover %d", pSoldier->aiData.usActionData));

					sDangerousSpot = NOWHERE;
					sLastSafeSpot = NOWHERE;

					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) && 
						AbortFinalSpot(pSoldier, pSoldier->aiData.usActionData, AI_ACTION_TAKE_COVER, sClosestDisturbance, bClosestDisturbanceLevel, sDangerousSpot))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("dangerous spot! abort!"));
						// abort! abort!
						pSoldier->aiData.usActionData = NOWHERE;
					}
					
					// also check path for dangerous places
					if( !TileIsOutOfBounds(pSoldier->aiData.usActionData) )
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("check path for dangerous places"));
						// try to move towards him
						INT32 sDestGridNo = InternalGoAsFarAsPossibleTowards(pSoldier,pSoldier->aiData.usActionData, 0, AI_ACTION_TAKE_COVER, 0);
						DebugAI( AI_MSG_INFO, pSoldier, String("found move spot %d", sDestGridNo));

						//if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
						if (!TileIsOutOfBounds(sDestGridNo))
						{
							sCheckGridNo = pSoldier->sGridNo;

							DebugAI( AI_MSG_INFO, pSoldier, String("check path for dangerous places"));
							if (AbortPath(pSoldier, AI_ACTION_TAKE_COVER, sClosestDisturbance, bClosestDisturbanceLevel, sDangerousSpot, sLastSafeSpot))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("dangerous path! abort!"));
								pSoldier->aiData.usActionData = NOWHERE;
							}
						}	
					}						

					// sevenfm: moved APs to attack check and prone sight cover check to FindBestNearbyCover
					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
					{
						// alt movement mode
						if (DecideCoverReverse(pSoldier, sClosestThreat, bClosestThreatLevel))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("move to cover spot using reverse mode"));
							return pSoldier->aiData.bAction;
						}

						DebugAI( AI_MSG_INFO, pSoldier, String("move to cover spot"));
						return(AI_ACTION_TAKE_COVER);
					}				

					// mark HIDING as impossible for next time through while loop
					bHidePts = -99;					
					
					DebugAI( AI_MSG_INFO, pSoldier, String("hide failed"));
				}
			}
		}
		DebugAI( AI_MSG_INFO, pSoldier, String("Nothing to do!"));

		////////////////////////////////////////////////////////////////////////////
		// NOTHING USEFUL POSSIBLE!  IF NPC IS CURRENTLY UNDER FIRE, TRY TO RUN AWAY
		////////////////////////////////////////////////////////////////////////////
		
		// if we're currently under fire (presumably, attacker is hidden)
		if (pSoldier->aiData.bUnderFire)
		{
			// sevenfm: disabled
			/*if (pSoldier->ShockLevelPercent() > 50 + 10 * SoldierDifficultyLevel(pSoldier))
			{
				// look for best place to RUN AWAY to (farthest from the closest threat)
				pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				return(AI_ACTION_RUN_AWAY);
			}
			}*/

			////////////////////////////////////////////////////////////////////////////
			// UNDER FIRE, DON'T WANNA/CAN'T RUN AWAY, SO CROUCH
			////////////////////////////////////////////////////////////////////////////

			DebugAI( AI_MSG_TOPIC, pSoldier, String("[soldier is under fire, try to crouch]"));

			// if not in water and not already crouched
			if (!bInWater)
			{
				if( gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_STAND &&
					IsValidStance( pSoldier, ANIM_CROUCH ) )
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("current stance %d", gAnimControl[pSoldier->usAnimState].ubEndHeight));

					if( GetAPsToChangeStance( pSoldier, ANIM_CROUCH ) <= pSoldier->bActionPoints )
					{
						pSoldier->aiData.usActionData = ANIM_CROUCH;
						return(AI_ACTION_CHANGE_STANCE);
					}
				}
			}
		}		
	}

	// civilians are only interested in running away
	// sevenfm: limit civilian AI
	if( fCivilian && 
		//pSoldier->CheckInitialAP() &&
		(pSoldier->aiData.bUnderFire ||
		!TileIsOutOfBounds( sClosestOpponent ) && PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < DAY_VISION_RANGE / 2 ) )
		//!TileIsOutOfBounds( sClosestNoise ) && PythSpacesAway(pSoldier->sGridNo, sClosestNoise) < DAY_VISION_RANGE / 2) )
		//CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel)
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[civilians run away]"));

		// look for best place to RUN AWAY to (farthest from the closest threat)
		pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);
		DebugAI( AI_MSG_INFO, pSoldier, String("found run away spot %d", pSoldier->aiData.usActionData));
		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_RUN_AWAY);
		}
		else if (!pSoldier->SkipCoverCheck() && gfTurnBasedAI ) // only do in turnbased
		{
			// try to take cover
			pSoldier->aiData.bAIMorale = MORALE_WORRIED;
			pSoldier->aiData.usActionData = FindBestNearbyCover(pSoldier, &iDummy);

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				return(AI_ACTION_TAKE_COVER);
			}
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[look around towards opponent]"));
	////////////////////////////////////////////////////////////////////////////
	// LOOK AROUND TOWARD CLOSEST KNOWN OPPONENT, IF KNOWN
	////////////////////////////////////////////////////////////////////////////

	// sevenfm: turn to closest threat
	if( SoldierAI(pSoldier) &&
		GetAPsToLook( pSoldier ) <= pSoldier->bActionPoints &&
		!TileIsOutOfBounds(sClosestThreat) &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_THREAT) &&
		pSoldier->aiData.bLastAction != AI_ACTION_CHANGE_FACING )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("soldier dir %d", pSoldier->ubDirection));

		// determine direction from this soldier to the closest opponent
		ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestThreat);

		// when in a room, turn to closest spot where enemy can be seen
		if (InARoom(pSoldier->sGridNo, NULL) &&
			pSoldier->pathing.bLevel == 0 &&
			!TileIsOutOfBounds(sClosestSeenPathSpot))
		{
			ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestSeenPathSpot);
			DebugAI(AI_MSG_INFO, pSoldier, String("use closest seen path spot %d dir %d", sClosestSeenPathSpot, ubOpponentDir));
		}

		DebugAI( AI_MSG_INFO, pSoldier, String("closest threat dir %d", ubOpponentDir));

		// if soldier is not already facing in that direction,
		// and the opponent is close enough that he could possibly be seen
		if( pSoldier->ubDirection != ubOpponentDir )
		{
			// set base chance according to orders
			if( pSoldier->aiData.bOrders == STATIONARY || pSoldier->aiData.bOrders == ONGUARD )
				iChance = 50;
			else           // all other orders
				iChance = 25;

			if (pSoldier->aiData.bAttitude == DEFENSIVE)
				iChance += 25;

			if (GuySawEnemy(pSoldier))
			{
				iChance = 100;
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("chance to turn %d", iChance));

			if( (INT16)PreRandom(100) < iChance &&
				pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
			{
				pSoldier->aiData.usActionData = ubOpponentDir;
				pSoldier->aiData.bNextAction = AI_ACTION_NONE;
				pSoldier->aiData.usNextActionData = 0;

				// don't turn to unseen again this turn
				pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_THREAT;

				return(AI_ACTION_CHANGE_FACING);
			}
		}
	}
	
	////////////////////////////////////////////////////////////////////////////
	// LEAVE THE SECTOR
	////////////////////////////////////////////////////////////////////////////

	// NOT IMPLEMENTED

	////////////////////////////////////////////////////////////////////////////
	// PICKUP A NEARBY ITEM THAT'S USEFUL
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[pick up useful item]"));

	if( ubCanMove &&
		//gfTurnBasedAI &&
		SoldierAI(pSoldier) &&
		!( pSoldier->usSoldierFlagMask & SOLDIER_SEARCH_ITEMS ) )
	{
		// limit searching to 1 time per turn
		pSoldier->usSoldierFlagMask |= SOLDIER_SEARCH_ITEMS;

		pSoldier->aiData.bAction = SearchForItems( pSoldier, SEARCH_GENERAL_ITEMS, pSoldier->inv[HANDPOS].usItem );

		// sevenfm: check that location is safe
		if( pSoldier->aiData.bAction != AI_ACTION_NONE &&
			!TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
			pSoldier->aiData.usActionData != pSoldier->sGridNo &&
			(	CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) ||
				EnemyCanAttackSpot(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) ||
				FindBombNearby(pSoldier, pSoldier->aiData.usActionData, BOMB_DETECTION_RANGE) ||
				InLightAtNight( pSoldier->aiData.usActionData, pSoldier->pathing.bLevel ) && !InLightAtNight(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel)
			) &&
			!InSmoke(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, FALSE) )
		{
			// abort! abort!
			pSoldier->aiData.bAction = AI_ACTION_NONE;
		}

		if (pSoldier->aiData.bAction != AI_ACTION_NONE)
		{
			return( pSoldier->aiData.bAction );
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// SEEK CLOSEST FRIENDLY MEDIC
	////////////////////////////////////////////////////////////////////////////

	// NOT IMPLEMENTED


	////////////////////////////////////////////////////////////////////////////
	// GIVE FIRST AID TO A NEARBY INJURED/DYING FRIEND
	////////////////////////////////////////////////////////////////////////////
	// - must be BRAVEAID or CUNNINGAID (medic) ?

	// NOT IMPLEMENTED

	////////////////////////////////////////////////////////////////////////////
	// SWITCH TO GREEN: soldier does ordinary regular patrol, seeks friends
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[switch to green]"));

	// if not in combat or under fire, and we COULD have moved, just chose not to	
	// sevenfm: check not only reachable disturbance, but any known opponent (if we are in a closed room for example)
	if( pSoldier->aiData.bAlertStatus != STATUS_BLACK &&
		!pSoldier->aiData.bUnderFire &&
		!pSoldier->bBlindedCounter &&
		CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2) == 0 &&
		ubCanMove &&
		(!gfTurnBasedAI || pSoldier->bActionPoints == pSoldier->bInitialActionPoints) &&
		TileIsOutOfBounds(sClosestOpponent) &&
		!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel))
	{
		// addition:  if soldier is bleeding then reduce bleeding and do nothing
		if ( pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD )
		{
			// reduce bleeding by 1 point per AP (in RT, APs will get recalculated so it's okay)
			pSoldier->bBleeding = max( 0, pSoldier->bBleeding - (pSoldier->bActionPoints/2) );
			pSoldier->bPoisonBleeding = max(0, pSoldier->bPoisonBleeding - max( 0, pSoldier->bBleeding - (pSoldier->bActionPoints/2) ) );
			return( AI_ACTION_NONE ); // will end-turn/wait depending on whether we're in TB or realtime
		}

		// Skip RED until new situation/next turn, 30% extra chance to do GREEN actions
		pSoldier->aiData.bBypassToGreen = 30;
		if( !TileIsOutOfBounds(sClosestNoise) )
			return(DecideActionYellow(pSoldier));
		else
			return(DecideActionGreen(pSoldier));
	}

	////////////////////////////////////////////////////////////////////////////
	// CROUCH IF NOT CROUCHING ALREADY
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[crouch if not crouched already]"));

	// if not in water and not already crouched, try to crouch down first
	if( !fCivilian &&
		!bInWater &&
		!pSoldier->IsBoxer() &&
		gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_STAND &&
		IsValidStance( pSoldier, ANIM_CROUCH ) &&
		ConsiderCrouch(pSoldier) &&
		GetAPsToChangeStance( pSoldier, ANIM_CROUCH ) <= pSoldier->bActionPoints )
	{		
		pSoldier->aiData.usActionData = ANIM_CROUCH;
		return(AI_ACTION_CHANGE_STANCE);
	}

	////////////////////////////////////////////////////////////////////////////
	// TURN TO CLOSEST COVERT OPPONENT
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[turn to covert]"));

	if (!fCivilian &&
		pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		!pSoldier->aiData.bUnderFire &&
		!GuySawEnemy(pSoldier, SEEN_LAST_TURN) &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_COVERT) &&
		!bInWater &&
		!bInGas)
	{
		UINT8 ubPerson = GetClosestFlaggedSoldierID(pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, FALSE);

		// lower chance to turn if soldier is prone or recently seen enemy
		if (ubPerson != NOBODY &&
			CountTeamSeeSoldier(pSoldier->bTeam, MercPtrs[ubPerson]) == 0 &&
			Chance(25 + 75 * MercPtrs[ubPerson]->SuspicionPercent() / (100 + 50 * CountSeenEnemiesLastTurn(pSoldier, FALSE) + 200 / (ANIM_PRONE + gAnimControl[pSoldier->usAnimState].ubHeight))) &&
			LOS_Soldier(pSoldier, MercPtrs[ubPerson], LOS_NORMAL, CALC_FROM_ALL_DIRS))
		{
			UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

			// and the opponent is close enough that he could possibly be seen
			if (pSoldier->ubDirection != ubOpponentDir &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
				pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
			{
				// turn
				pSoldier->aiData.usActionData = ubOpponentDir;

				// end turn when in turnbased
				if(gfTurnBasedAI)
				{
					pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
					pSoldier->aiData.usNextActionData = 0;
					DebugAI(AI_MSG_INFO, pSoldier, String("end turn after turning"));
				}
				// in realtime, possibly raise weapon
				else if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					!WeaponReady(pSoldier) &&
					Chance(MercPtrs[ubPerson]->SuspicionPercent() / 2) &&
					PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
					(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
					GetAPsToLook(pSoldier) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)) <= pSoldier->bActionPoints)
				{
					pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
					pSoldier->aiData.usNextActionData = 0;
					DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
				}

				// don't turn to covert again this turn
				pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_COVERT;

				DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest covert opponent"));
				return(AI_ACTION_CHANGE_FACING);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// IF UNDER FIRE, FACE THE MOST IMPORTANT NOISE WE KNOW AND GO PRONE
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[if under fire, face most important noise and go prone]"));

	if( !fCivilian &&
		!bInWater &&
		!pSoldier->IsBoxer() &&
		!TileIsOutOfBounds(sClosestNoise) && 
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_THREAT) &&
		(pSoldier->aiData.bUnderFire || RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel)))
	{
		// if not facing closest opponent - turn
		ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestNoise);

		// when in a room, turn to closest spot where enemy can be seen
		if (InARoom(pSoldier->sGridNo, NULL) &&
			pSoldier->pathing.bLevel == 0 &&
			!TileIsOutOfBounds(sClosestSeenPathSpot))
		{
			ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestSeenPathSpot);
			DebugAI(AI_MSG_INFO, pSoldier, String("use closest seen path spot %d dir %d", sClosestSeenPathSpot, ubOpponentDir));
		}

		if( pSoldier->ubDirection != ubOpponentDir &&
			pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
			GetAPsToLook( pSoldier ) <= pSoldier->bActionPoints )
		{
			pSoldier->aiData.usActionData = ubOpponentDir;
			//pSoldier->aiData.bNextAction = AI_ACTION_NONE;

			pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_THREAT;
			return( AI_ACTION_CHANGE_FACING );
		}

		// go prone
		if( fAllowProne &&
			gAnimControl[ pSoldier->usAnimState ].ubEndHeight != ANIM_PRONE &&
			GetAPsToChangeStance( pSoldier, ANIM_PRONE ) <= pSoldier->bActionPoints &&
			IsValidStance( pSoldier, ANIM_PRONE ) )
		{
			// go prone, end turn
			pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
			pSoldier->aiData.usActionData = ANIM_PRONE;
			return( AI_ACTION_CHANGE_STANCE );
		}
	}

	// possibly raise gun
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[possibly raise gun]"));

	if( !fCivilian &&
		!bInWater &&
		!pSoldier->bBlindedCounter &&
		PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
		!WeaponReady(pSoldier) &&
		pSoldier->aiData.bLastAction != AI_ACTION_RAISE_GUN &&
		pSoldier->bActionPoints > APBPConstants[MIN_APS_TO_INTERRUPT] &&
		GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) <= pSoldier->bActionPoints )
	{
		if( AIRoleSniper(pSoldier) ||
			AIGunInHandScoped(pSoldier) ||
			AIRoleMachinegunner(pSoldier) && gAnimControl[ pSoldier->usAnimState ].ubEndHeight <= ANIM_CROUCH ||			
			!AIGunInHandScoped(pSoldier) && Random(100) < 30 )
		{
			//pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
			return AI_ACTION_RAISE_GUN;
		}
	}

	// sevenfm: possibly use focus to scan area
	if ((gfTurnBasedAI || pSoldier->CheckInitialAP()) &&
		SoldierAI(pSoldier) &&
		!pSoldier->bBlindedCounter &&
		WeaponReady(pSoldier) &&
		AIGunInHandScoped(pSoldier) &&
		pSoldier->bActionPoints >= APBPConstants[AP_SPOTTER] &&
		!pSoldier->IsFocusing() &&
		pSoldier->CanUseSkill(SKILLS_FOCUS, FALSE) &&
		(AIRoleSniper(pSoldier) || Chance(10 * SoldierDifficultyLevel(pSoldier))))
	{
		INT32 sFocusSpot;
		INT8 bFocusLevel;

		if (FindCombatFocusSpot(pSoldier, sFocusSpot, bFocusLevel) || FindRandomFocusSpot(pSoldier, sFocusSpot, bFocusLevel, Chance(75)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start focusing at location: spot %d level %d", sFocusSpot, bFocusLevel));

			pSoldier->ubAISkillUse = SKILLS_FOCUS;
			pSoldier->aiData.usActionData = sFocusSpot;
			pSoldier->bTargetLevel = bFocusLevel;

			if (!gfTurnBasedAI)
			{
				pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
				pSoldier->aiData.usNextActionData = RealtimeDelay(pSoldier);
			}

			return AI_ACTION_USE_SKILL;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// If sniper and nothing else to do then raise gun
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[sniper raise gun]"));

	if (SoldierAI(pSoldier) &&
		!pSoldier->bBlindedCounter &&
		AIRoleSniper(pSoldier) )
	{
		if( PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
			!WeaponReady(pSoldier) &&
			pSoldier->aiData.bLastAction != AI_ACTION_RAISE_GUN &&
			GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) <= pSoldier->bActionPoints &&
			(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30))
		{
			DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("DecideActionRed: sniper raising gun..."));
			return AI_ACTION_RAISE_GUN;
		}

		// if weapon is raised and still cannot see anybody - go to yellow
		if( pSoldier->aiData.bAlertStatus != STATUS_BLACK &&
			WeaponReady(pSoldier) &&
			CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2) == 0 &&
			!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel))
		{
			return DecideActionYellow(pSoldier);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// DO NOTHING: Not enough points left to move, so save them for next turn
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_INFO, pSoldier, String("[END: nothing to do]"));

	pSoldier->aiData.usActionData = NOWHERE;
	return(AI_ACTION_NONE);
}

INT8 DecideActionBlack(SOLDIERTYPE *pSoldier)
{
	INT32	iCoverPercentBetter, iAttack, iMove, iChance;
	INT32	sClosestOpponent = NOWHERE,sBestCover = NOWHERE;//dnl ch58 160813	
	INT32	sClosestDisturbance = NOWHERE;
	INT8	bClosestDisturbanceLevel = 0;
	BOOLEAN fSeekClimb;
	INT16	ubMinAPCost;
	UINT8	ubCanMove;
	INT8	bInWater,bInDeepWater,bInGas, bInGasSpot;
	INT8	bDirection;
	UINT8	ubBestAttackAction = AI_ACTION_NONE;
	INT8	bCanAttack,bActionReturned;
	INT8	bWeaponIn;
	BOOLEAN	fTryPunching = FALSE;

	DebugAI( AI_MSG_START, pSoldier, String("[Black]"));
	LogDecideInfo(pSoldier);

	ATTACKTYPE BestShot, BestThrow, BestStab ,BestAttack;//dnl ch69 150913
	BOOLEAN fCivilian = pSoldier->IsCivilian();
	//UINT8	ubBestStance = 1;
	//BOOLEAN fChangeStanceFirst; // before firing
	//BOOLEAN fClimb;
	INT16	ubBurstAPs;
	UINT8	ubOpponentDir;
	INT32	sCheckGridNo;
	UINT8	ubSeekType = SEEK_NORMAL;

	// sevenfm
	INT32	sClosestSeenOpponent = NOWHERE;
	INT32	sClosestPossiblySeenOpponent = NOWHERE;
	BOOLEAN fAllowCoverCheck = FALSE;
	BOOLEAN fExtraClip = FALSE;
	//INT32	iTotalEnemyThreat = 0;
	//INT32	iThreatTolerance = 0;
	BOOLEAN fDangerousSpot = FALSE;
	BOOLEAN fSafeSpot = FALSE;
	BOOLEAN fProneSightCover = FALSE;
	BOOLEAN fAnyCover = FALSE;
	BOOLEAN fStopMovement = FALSE;
	UINT8	ubThreatID = NOBODY;
	INT32	sThreatSpot = NOWHERE;
	INT8	bThreatLevel = 0;	

	INT32	sSmokeSpot = NOWHERE;
	INT8	bSmokeLevel = 0;
	
	// sevenfm: initialize data
	pSoldier->bWeaponMode = WM_NORMAL;
	// sevenfm: disable stealth mode
	pSoldier->bStealthMode = FALSE;
	// disable reverse movement mode
	pSoldier->bReverse = FALSE;
	// reset stop seek counter
	pSoldier->StopSeekReset();

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Check if can do anything]"));
	// if we have absolutely no action points, we can't do a thing under BLACK!
	if (pSoldier->bActionPoints <= 0 || pSoldier->IsUnconscious())
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("No action points - nothing to do"));

		pSoldier->aiData.usActionData = NOWHERE;
		return(AI_ACTION_NONE);
	}

	// prepare data
	BOOLEAN fSectorAttack = SectorAttack(pSoldier->bTeam);

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Possibly update knowledge]"));
	if (pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		(gfTurnBasedAI || pSoldier->CheckInitialAP()))
	{
		PossiblyUpdateKnowledge(pSoldier);
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Prepare data]"));
	sClosestOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL);
	sClosestSeenOpponent = ClosestSeenOpponent(pSoldier, NULL, NULL);
	sClosestPossiblySeenOpponent = ClosestPossiblySeenOpponent(pSoldier, NULL, NULL);

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Find closest reachable opponent]"));

	if (SoldierAI(pSoldier))
		//!gfHiddenInterrupt &&
		//!gTacticalStatus.fInterruptOccurred &&
		//AllowComplexAI(pSoldier))
	{
		// get the location of the closest reachable opponent
		sClosestDisturbance = ClosestReachableDisturbance(pSoldier, &fSeekClimb, ubThreatID, sThreatSpot, bThreatLevel, ubSeekType);

		// determine opponent level		
		if (fSeekClimb)
		{
			if (pSoldier->pathing.bLevel > 0)
				bClosestDisturbanceLevel = 0;
			else
				bClosestDisturbanceLevel = 1;
		}
		else
		{
			bClosestDisturbanceLevel = pSoldier->pathing.bLevel;
		}
		DebugAI(AI_MSG_INFO, pSoldier, String("closest reachable disturbance %d climb %d threat ID %d spot %d level %d type %d", sClosestDisturbance, fSeekClimb, ubThreatID, sThreatSpot, bThreatLevel, ubSeekType));
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop flanking when we see enemy]"));
	// sevenfm: stop flanking when we see enemy
	if( pSoldier->IsFlanking() &&
		(pSoldier->aiData.bUnderFire ||
		CountFriendsBetweenMeAndSpotFromSpot(pSoldier, pSoldier->sTacticalSpot) == 0 ||	
		!TileIsOutOfBounds(sClosestOpponent) && PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < MIN_FLANK_DIST) )
	{
		pSoldier->StopTacticalOrder();
		// allow another flanking attempt in the future
		pSoldier->usSoldierFlagMask2 &= ~SOLDIER_DISABLE_FLANKING;
	}	

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Black continue deep water flanking]"));
	if (pSoldier->IsFlanking() &&
		!pSoldier->aiData.bUnderFire &&
		DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("try to continue deep water flanking"));
		return DecideActionRed(pSoldier);
	}	

	// calculate personal/team threat at current gridno
	//iTotalEnemyThreat = AICalcTotalEnemyThreatAtSpot(pSoldier, pSoldier->sGridNo);
	//iThreatTolerance = AICalcThreatTolerance(pSoldier);

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Check sight]"));
	fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
	fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);
	fDangerousSpot = DangerousSpot(pSoldier);
	fSafeSpot = SafeSpot(pSoldier);	

	// can this guy move to any of the neighbouring squares ? (sets TRUE/FALSE)
	ubCanMove = (pSoldier->bActionPoints >= MinPtsToMove(pSoldier));

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Turn off flashlight]"));
	// always turn off flashlight at the start of a new turn
	if( 	//pSoldier->bTeam == ENEMY_TEAM &&
		SoldierAI(pSoldier) &&
		pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT &&
		IsActionAffordable(pSoldier, AI_ACTION_SWITCH_FLASHLIGHT) &&
		(!gfTurnBasedAI || pSoldier->bActionPoints == pSoldier->bInitialActionPoints))
		//!AIUseFlashlightCondition(pSoldier, pSoldier->ubDirection) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("turn off flashlight"));
		return AI_ACTION_SWITCH_FLASHLIGHT;
	}

	// sevenfm: before deciding anything, stop cowering
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop cowering]"));
	if (!fCivilian &&
		SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->stats.bLife >= OKLIFE &&
		!pSoldier->bCollapsed &&
		!pSoldier->bBreathCollapsed &&
		pSoldier->IsCowering())
	{
		//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] stop cowering", pSoldier->ubID);
		return AI_ACTION_STOP_COWERING;
	}

	// sevenfm: stop giving aid
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop giving aid]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->stats.bLife >= OKLIFE &&
		!pSoldier->bCollapsed &&
		!pSoldier->bBreathCollapsed &&
		pSoldier->IsGivingAid())
	{
		//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] stop medic", pSoldier->ubID);
		return AI_ACTION_STOP_MEDIC;
	}

	// sevenfm: stop dragging
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Stop dragging]"));
	if (SoldierAI(pSoldier) &&
		pSoldier->bActionPoints > 0 &&
		pSoldier->IsDragging())
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop dragging"));
		return AI_ACTION_CANCEL_DRAG;
	}

	// sevenfm: chance to stop focusing
	/*if (pSoldier->CheckInitialAP() &&
		pSoldier->IsFocusing() &&
		pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] > 1 &&
		Chance(pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS] * 10))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("stop focusing"));
		return AI_ACTION_STOP_FOCUS;
	}*/

	// raise alert for others
	if ((pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral && pSoldier->ubCivilianGroup != NON_CIV_GROUP) &&
		SoldierAI(pSoldier) &&
		!fCivilian &&
		!pSoldier->IsBoxer() &&
		!pSoldier->RaisedAlertThisTurn() &&
		!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT))
	{
		ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT]);
		return(AI_ACTION_RED_ALERT);
	}

	if ( (pSoldier->bTeam == ENEMY_TEAM || pSoldier->ubProfile == WARDEN) && (gTacticalStatus.fPanicFlags & PANIC_TRIGGERS_HERE) && (gTacticalStatus.ubTheChosenOne == NOBODY) )
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Possibly make chosen one]"));
		INT8 bPanicTrigger;

		bPanicTrigger = ClosestPanicTrigger( pSoldier );
		// if it's an alarm trigger and team is alerted, ignore it
		if (bPanicTrigger != -1 &&
			!(gTacticalStatus.bPanicTriggerIsAlarm[bPanicTrigger] && 
			gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition) && 
			PythSpacesAway(pSoldier->sGridNo, gTacticalStatus.sPanicTriggerGridNo[bPanicTrigger]) < 10)
		{
			PossiblyMakeThisEnemyChosenOne( pSoldier );
		}
	}
	
	// if this soldier is the "Chosen One" (enemies only)
	if (pSoldier->ubID == gTacticalStatus.ubTheChosenOne)
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Panic AI]"));

		// do some special panic AI decision making
		bActionReturned = PanicAI(pSoldier,ubCanMove);

		// if we decided on an action while in there, we're done
		if (bActionReturned != -1)
			return(bActionReturned);
	}

	
	if ( pSoldier->ubProfile != NO_PROFILE )
	{
		// if they see enemies, the Queen will keep going to the staircase, but Joe will fight
		if ( (pSoldier->ubProfile == QUEEN) && ubCanMove )
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Queen head the staircase]"));
			if ( gWorldSectorX == 3 && gWorldSectorY == MAP_ROW_P && gbWorldSectorZ == 0 && !gfUseAlternateQueenPosition )
			{
				bActionReturned = HeadForTheStairCase( pSoldier );
				if ( bActionReturned != AI_ACTION_NONE )
				{
					return( bActionReturned );
				}
			}
		}
	}

	if(pSoldier->IsBoxer())
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Boxing AI]"));

		if ( gTacticalStatus.bBoxingState == PRE_BOXING )
		{
			return( DecideActionBoxerEnteringRing( pSoldier ) );
		}
		else if ( gTacticalStatus.bBoxingState == BOXING )
		{

			bInWater = FALSE;
			bInDeepWater = FALSE;
			bInGas = FALSE;
			bInGasSpot = FALSE;

			// calculate our morale
			// sevenfm: for boxer, always use high morale
			//pSoldier->aiData.bAIMorale = CalcMorale(pSoldier);
			pSoldier->aiData.bAIMorale = MORALE_FEARLESS;
			// and continue on...
		}
		else //????
		{
			return( AI_ACTION_NONE );
		}
	}
	else
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Water/Gas]"));

		// determine if we happen to be in water (in which case we're in BIG trouble!)
		bInWater = Water( pSoldier->sGridNo, pSoldier->pathing.bLevel );
		bInDeepWater = WaterTooDeepForAttacks( pSoldier->sGridNo, pSoldier->pathing.bLevel );

		// check if standing in tear gas without a gas mask on
		bInGas = InGas( pSoldier, pSoldier->sGridNo );
		bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);

		// calculate our morale
		pSoldier->aiData.bAIMorale = CalcMorale(pSoldier);

		////////////////////////////////////////////////////////////////////////////
		// WHEN LEFT IN GAS, WEAR GAS MASK IF AVAILABLE AND NOT WORN
		////////////////////////////////////////////////////////////////////////////

		if ( !bInGas && (gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y) )
		{
			// only chance if we happen to be caught with our gas mask off
			if ( PreRandom( 10 ) == 0 && WearGasMaskIfAvailable( pSoldier ) )
			{
				bInGas = InGas(pSoldier, pSoldier->sGridNo);
				bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);
			}
		}

		//Only put mask on in gas
		if ((bInGas || InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE)) && WearGasMaskIfAvailable(pSoldier))//dnl ch40 200909
		{
			bInGas = InGas(pSoldier, pSoldier->sGridNo);
			bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);
		}

		// sevenfm: chance to wear gas mask if soldier sees gas nearby
		if( SoldierAI(pSoldier) &&
			pSoldier->CheckInitialAP() &&
			pSoldier->HasGasMask() &&
			!pSoldier->WearsGasMask() && 
			FindClosestVisibleSmoke(pSoldier, sSmokeSpot, bSmokeLevel, TRUE, TACTICAL_RANGE / 2) &&
			Chance( 10 + SoldierDifficultyLevel(pSoldier) * 10 ) )
		{
			WearGasMaskIfAvailable(pSoldier);
		}

		// sevenfm: possibly remove gas mask
		if (SoldierAI(pSoldier) &&
			pSoldier->CheckInitialAP() &&
			gbWorldSectorZ == 0 &&
			pSoldier->WearsGasMask() &&
			!InGas(pSoldier, pSoldier->sGridNo) &&
			!pSoldier->aiData.bUnderFire &&
			!FindClosestGas(pSoldier, sSmokeSpot, bSmokeLevel, TACTICAL_RANGE / 2) &&
			Chance(10 + SoldierDifficultyLevel(pSoldier) * 10))
		{
			RemoveGasMask(pSoldier);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN IN DEEP WATER, CREATE SCUBA FINS
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[when in deep water, create scuba fins]"));

	// sevenfm: when in deep water, create scuba fins
	if (ubCanMove &&
		bInDeepWater &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->IsFlanking() &&
		pSoldier->CheckInitialAP() &&
		Chance(25) &&
		20 * SoldierDifficultyLevel(pSoldier) >= 100 - HighestPlayerProgressPercentage() &&
		!FindNotDeepWaterNearby(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		FindUsableItemWithFlag(pSoldier, SCUBA_FINS) == NO_SLOT)
	{
		// create scuba fins and place in the inventory
		UINT16 usItem;
		if (GetFirstItemWithFlag(&usItem, SCUBA_FINS))
		{
			OBJECTTYPE newobj;
			CreateItem(usItem, 80 + Random(20), &newobj);
			newobj.fFlags |= OBJECT_UNDROPPABLE;

			// try to place item in inventory
			if (AutoPlaceObject(pSoldier, &newobj, FALSE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("successfully created and placed scuba fins in inventory"));
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// WHEN NOT IN WATER, DROP SCUBA FINS
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[when not in water, drop scuba fins]"));

	// when not in deep water, drop scuba fins
	if (pSoldier->CheckInitialAP() &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		pSoldier->inv[LEGPOS].exists() &&
		HasItemFlag(pSoldier->inv[LEGPOS].usItem, SCUBA_FINS) &&
		IsActionAffordable(pSoldier, AI_ACTION_DROP_ITEM))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found scuba fins in LEGPOS, drop to the ground"));
		//if (!AutoPlaceObject(pSoldier, &(pSoldier->inv[LEGPOS]), FALSE, LEGPOS))
		//RearrangePocket(pSoldier, HANDPOS, LEGPOS, FOREVER);

		pSoldier->inv[LEGPOS].fFlags |= OBJECT_AI_UNUSABLE;
		pSoldier->aiData.usActionData = LEGPOS;

		return AI_ACTION_DROP_ITEM;
	}

	////////////////////////////////////////////////////////////////////////////
	// STUCK IN WATER OR GAS, NO COVER, GO TO NEAREST SPOT OF UNGASSED LAND
	////////////////////////////////////////////////////////////////////////////

	// when in deep water, move to closest opponent
	if (ubCanMove && 
		bInDeepWater && 
		!pSoldier->aiData.bNeutral && 
		pSoldier->aiData.bOrders == SEEKENEMY &&
		!pSoldier->IsFlanking() &&
		!TileIsOutOfBounds(sClosestDisturbance))
	{
		// find closest reachable opponent, excluding opponents in deep water
		pSoldier->aiData.usActionData = sClosestDisturbance;
		return(AI_ACTION_LEAVE_WATER_GAS);
	}

	// if soldier in water/gas has enough APs left to move at least 1 square
	if (ubCanMove && (bInDeepWater&& !pSoldier->IsFlanking() || bInGas || FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE) || RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel)))
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Leave water/gas/bomb]"));

		pSoldier->aiData.usActionData = FindNearestUngassedLand(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_LEAVE_WATER_GAS);
		}

		// couldn't find ANY land within 25 tiles(!), this should never happen...

		// look for best place to RUN AWAY to (farthest from the closest threat)
		pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found run away spot %d", pSoldier->aiData.usActionData));
			return(AI_ACTION_RUN_AWAY);
		}

		if (bInDeepWater || bInGasSpot)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("Stuck in water/gas - set AI morale to MORALE_HOPELESS"));
			pSoldier->aiData.bAIMorale = MORALE_HOPELESS;
		}		
	}

	////////////////////////////////////////////////////////////////////////
	// DRINK IF LOW IN BREATH
	////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[drink if low on breath]"));
	if (pSoldier->CheckInitialAP() &&
		!bInWater &&
		pSoldier->bBreath < OKBREATH &&
		fSafeSpot &&
		pSoldier->IsMercBodyType() &&
		Chance(30 - 10 * pSoldier->aiData.bUnderFire) &&
		!(pSoldier->usSoldierFlagMask2 & SOLDIER_USED_CANTEEN) &&
		IsActionAffordable(pSoldier, AI_ACTION_DRINK_CANTEEN) &&
		FindCanteen(pSoldier) != NO_SLOT)
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("drink from canteen"));
		return AI_ACTION_DRINK_CANTEEN;
	}

	////////////////////////////////////////////////////////////////////////////
	// IF REALLY TIRED (ON THE VERGE OF COLLAPSING), TRY TO RUN AWAY
	////////////////////////////////////////////////////////////////////////////

	// if we're desperately short on breath
	if (!pSoldier->IsBoxer() && pSoldier->bBreath < OKBREATH)
	{
		DebugAI(AI_MSG_TOPIC, pSoldier, String("[Too tired]"));

		// if soldier has enough APs left to move at least 1 square's worth
		if (ubCanMove)
		{
			// look for best place to RUN AWAY to (farthest from the closest threat)
			pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("found run away spot %d", pSoldier->aiData.usActionData));
				return(AI_ACTION_RUN_AWAY);
			}
		}

		// if not already crouched, try to crouch down first
		if (!fCivilian &&
			!PTR_CROUCHED &&
			!pSoldier->IsBoxer() &&
			IsValidStance(pSoldier, ANIM_CROUCH) &&
			gAnimControl[pSoldier->usAnimState].ubEndHeight > ANIM_CROUCH)
		{
			if (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
			{
				pSoldier->aiData.usActionData = ANIM_CROUCH;
				DebugAI(AI_MSG_TOPIC, pSoldier, String("crouch first"));
				return(AI_ACTION_CHANGE_STANCE);
			}
		}

		// REALLY tired, can't get away, force soldier's morale to hopeless state
		DebugAI(AI_MSG_INFO, pSoldier, String("Really tired, can't get away - set AI morale to MORALE_HOPELESS"));
		pSoldier->aiData.bAIMorale = MORALE_HOPELESS;
	}

	////////////////////////////////////////////////////////////////////////
	// USE DRUGS
	////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[use drugs]"));

	if (pSoldier->CheckInitialAP() &&
		!bInWater &&
		fSafeSpot &&
		pSoldier->IsMercBodyType() &&
		SoldierAI(pSoldier) &&
		!pSoldier->IsFlanking() &&
		IsActionAffordable(pSoldier, AI_ACTION_USE_DRUG))
	{
		// alcohol to reduce shock level
		if (pSoldier->aiData.bShock > 0 &&
			(!MercUnderTheInfluence(pSoldier) || MercUnderTheInfluence(pSoldier, DRUG_TYPE_ALCOHOL)) &&
			Chance(100 / (1 + GetDrunkLevel(pSoldier))) &&
			Chance(pSoldier->ShockLevelPercent() * pSoldier->ShockLevelPercent() / 400) &&
			FindDrug(pSoldier, DRUG_ALCOHOL))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("under suppression, use alcohol"));
			pSoldier->aiData.usActionData = DRUG_ALCOHOL;
			return AI_ACTION_USE_DRUG;
		}

		// combat drugs to prepare rush attack
		if (pSoldier->RushAttackPrepare() &&
			(!MercUnderTheInfluence(pSoldier) || MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE)))
		{
			if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_RESISTANCE) && FindDrug(pSoldier, DRUG_DAMAGERESISTANCE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use damage resistance drug"));
				pSoldier->aiData.usActionData = DRUG_DAMAGERESISTANCE;
				return AI_ACTION_USE_DRUG;
			}
			if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_AGILITY) && FindDrug(pSoldier, DRUG_AGILITY))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use agility drug"));
				pSoldier->aiData.usActionData = DRUG_AGILITY;
				return AI_ACTION_USE_DRUG;
			}
			if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_REGENERATION) && pSoldier->stats.bLife < pSoldier->stats.bLifeMax && FindDrug(pSoldier, DRUG_REGENERATION))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use regeneration drug"));
				pSoldier->aiData.usActionData = DRUG_REGENERATION;
				return AI_ACTION_USE_DRUG;
			}
		}

		// regeneration if seriously wounded
		if (pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2 &&
			pSoldier->bRegenBoostersUsedToday < 3 &&
			!MercUnderTheInfluence(pSoldier, DRUG_TYPE_REGENERATION) &&
			Chance(100 - 100 * (pSoldier->stats.bLife * pSoldier->stats.bLife) / (pSoldier->stats.bLifeMax * pSoldier->stats.bLifeMax)))
		{
			if (FindDrug(pSoldier, DRUG_REGENERATION))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare rush attack, use regeneration drug"));
				pSoldier->aiData.usActionData = DRUG_REGENERATION;
				return AI_ACTION_USE_DRUG;
			}
		}
		
		// sniper/marksman can use wisdom
		if ((AIRoleSniper(pSoldier) || AIRoleMarksman(pSoldier)) &&			
			!pSoldier->aiData.bLastAttackHit &&
			!pSoldier->bBlindedCounter &&
			GuySawEnemy(pSoldier, SEEN_LAST_TURN) &&
			!MercUnderTheInfluence(pSoldier, DRUG_TYPE_WISDOM) &&
			Chance(10))
		{
			if (FindDrug(pSoldier, DRUG_WISDOM))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("sniper/marksman, use wisdom drug"));
				pSoldier->aiData.usActionData = DRUG_WISDOM;
				return AI_ACTION_USE_DRUG;
			}
		}

		// adrenaline when need to get closer to enemy
		if (!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
			pSoldier->aiData.bAIMorale == MORALE_FEARLESS &&
			!pSoldier->aiData.bLastAttackHit &&
			!AISpecialRole(pSoldier) &&
			GuySawEnemy(pSoldier, SEEN_LAST_TURN) &&
			!TileIsOutOfBounds(sClosestDisturbance) &&
			PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < TACTICAL_RANGE * 2 &&
			Chance(10 * CountSeenEnemiesLastTurn(pSoldier) + 10 * pSoldier->aiData.bUnderFire + pSoldier->ShockLevelPercent() / 2) &&
			(!AICheckHasGun(pSoldier) || AICheckShortWeaponRange(pSoldier) || pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR))
		{
			BOOLEAN fClimbingNecessary;
			INT32 sClimbGridNo;
			INT32 sPathCost = EstimatePathCostToLocation(pSoldier, sClosestDisturbance, bClosestDisturbanceLevel, FALSE, &fClimbingNecessary, &sClimbGridNo);

			if (sPathCost != 0 && sPathCost < APBPConstants[AP_MAXIMUM] * 2 && FindDrug(pSoldier, DRUG_ADRENALINE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("need to get closer to enemy, use adrenaline drug"));
				pSoldier->aiData.usActionData = DRUG_ADRENALINE;
				return AI_ACTION_USE_DRUG;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// TURN TO CLOSEST SUSPICIOUS COVERT OPPONENT
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[turn to suspicious covert]"));

	if (!fCivilian &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->CheckInitialAP() &&
		SoldierAI(pSoldier) &&
		!bInWater &&
		!bInGas)
	{
		UINT8 ubPerson = GetClosestFlaggedSoldierID(pSoldier, VISION_RANGE, OUR_TEAM, SOLDIER_COVERT_SOLDIER | SOLDIER_COVERT_CIV, FALSE);

		// check that covert opponent is closer than any seen or possibly seen opponent
		if (ubPerson != NOBODY &&
			CountTeamSeeSoldier(pSoldier->bTeam, MercPtrs[ubPerson]) == 0 &&
			(MercPtrs[ubPerson]->SuspicionPercent() >= 100 || MercPtrs[ubPerson]->usSoldierFlagMask2 & SOLDIER_COVERT_ACTION) &&
			(TileIsOutOfBounds(sClosestSeenOpponent) || PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) < PythSpacesAway(pSoldier->sGridNo, sClosestSeenOpponent)) &&
			(TileIsOutOfBounds(sClosestPossiblySeenOpponent) || PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) < PythSpacesAway(pSoldier->sGridNo, sClosestPossiblySeenOpponent)) &&
			LOS_Soldier(pSoldier, MercPtrs[ubPerson], LOS_NORMAL, CALC_FROM_ALL_DIRS))
		{
			UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

			// and the opponent is close enough that he could possibly be seen
			if (pSoldier->ubDirection != ubOpponentDir &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
				pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
			{
				// turn
				pSoldier->aiData.usActionData = ubOpponentDir;

				// possibly raise weapon
				if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					!WeaponReady(pSoldier) &&
					Chance(MercPtrs[ubPerson]->SuspicionPercent() / 2) &&
					PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
					(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
					GetAPsToLook(pSoldier) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)) <= pSoldier->bActionPoints)
				{
					pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
					pSoldier->aiData.usNextActionData = 0;
					DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
				}

				DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest suspicious covert opponent"));
				return(AI_ACTION_CHANGE_FACING);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////
	// WAIT IF BEING BANDAGED
	////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Being bandaged]"));
	if( AICheckIsBandaged( pSoldier ) &&
		!bInWater &&
		!bInGas &&
		!pSoldier->aiData.bUnderFire &&
		fSafeSpot )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("ubServiceCount %d, stop movement", pSoldier->ubServiceCount));
		fStopMovement = TRUE;
		//return AI_ACTION_NONE;
	}

	// offer surrender?
#ifdef JA2UB
#else
	if( pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->bVisible == TRUE &&
		!( gTacticalStatus.fEnemyFlags & ENEMY_OFFERED_SURRENDER ) &&
		pSoldier->stats.bLife >= pSoldier->stats.bLifeMax / 2 &&
		!pSoldier->aiData.bUnderFire )
	{
		if( gTacticalStatus.Team[ MILITIA_TEAM ].bMenInSector == 0 &&
			gTacticalStatus.Team[ CREATURE_TEAM ].bMenInSector == 0 &&
			NumPCsInSector() < 4 && 
			gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector >= NumPCsInSector() * 3 )
		{
			//if( GetWorldDay() > STARTDAY_ALLOW_PLAYER_CAPTURE_FOR_RESCUE && !( gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE ) )
			{
				if( gubQuest[ QUEST_HELD_IN_ALMA ] == QUESTNOTSTARTED || ( gubQuest[ QUEST_HELD_IN_ALMA ] == QUESTDONE && gubQuest[ QUEST_INTERROGATION ] == QUESTNOTSTARTED ) )
				{
					gTacticalStatus.fEnemyFlags |= ENEMY_OFFERED_SURRENDER;
					return( AI_ACTION_OFFER_SURRENDER );
				}
			}
		}
	}
#endif

	////////////////////////////////////////////////////////////////////////////
	// SOLDIER CAN ATTACK IF NOT IN WATER/GAS AND NOT DOING SOMETHING TOO FUNKY
	////////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Can attack]"));

	// NPCs in water/tear gas without masks are not permitted to shoot/stab/throw
	// sevenfm: removed AP limit, check only gas spot (can attack if standing near gas)
	//if ((pSoldier->bActionPoints < 2) || bInDeepWater || bInGas)
	if (bInDeepWater || bInGasSpot)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("AP < 2 or NPCs in water/tear gas without masks are not permitted to shoot/stab/throw"));
		bCanAttack = FALSE;
	}
	else if (pSoldier->IsBoxer())
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("boxer can attack, try punching"));
		bCanAttack = TRUE;
		//fTryPunching = TRUE;
	}
	else
	{
		do
		{
			bCanAttack = CanNPCAttack(pSoldier);
			DebugAI( AI_MSG_INFO, pSoldier, String("CanNPCAttack = %d", bCanAttack));

			if (bCanAttack != TRUE)
			{
				if (fCivilian)
				{
					if ((bCanAttack == NOSHOOT_NOWEAPON) && !pSoldier->IsBoxer() && pSoldier->ubBodyType != COW && pSoldier->ubBodyType != CRIPPLECIV)
					{
						// cower in fear!!
						if ( pSoldier->flags.uiStatusFlags & SOLDIER_COWERING )
						{
							if ( pSoldier->aiData.bLastAction == AI_ACTION_COWER )
							{
								// do nothing
								pSoldier->aiData.usActionData = NOWHERE;
								return( AI_ACTION_NONE );
							}
							else
							{
								// set up next action to run away
								pSoldier->aiData.usNextActionData = FindSpotMaxDistFromOpponents( pSoldier );

								if (!TileIsOutOfBounds(pSoldier->aiData.usNextActionData))
								{
									pSoldier->aiData.bNextAction = AI_ACTION_RUN_AWAY;
									pSoldier->aiData.usActionData = ANIM_STAND;
									return( AI_ACTION_STOP_COWERING );
								}
								else
								{
									return( AI_ACTION_NONE );
								}
							}
						}
						else
						{
							// cower!!!
							pSoldier->aiData.usActionData = ANIM_CROUCH;
							return( AI_ACTION_COWER );
						}
					}
				}
				else if (bCanAttack == NOSHOOT_NOAMMO && ubCanMove && !pSoldier->aiData.bNeutral)
				{
					int handPOS;
					//CHRISL: We need to know which weapon has no ammo in case the soldier is holding a weapon in SECONDHANDPOS
					if(pSoldier->inv[SECONDHANDPOS].exists() == true && pSoldier->inv[SECONDHANDPOS][0]->data.gun.ubGunShotsLeft == 0)
						handPOS = SECONDHANDPOS;
					else
						handPOS = HANDPOS;

					// try to find more ammo
					pSoldier->aiData.bAction = SearchForItems( pSoldier, SEARCH_AMMO, pSoldier->inv[handPOS].usItem );

					if (pSoldier->aiData.bAction == AI_ACTION_NONE)
					{
						// the current weapon appears is useless right now!
						// (since we got a return code of noammo, we know the hand usItem
						// is our gun)
						pSoldier->inv[handPOS].fFlags |= OBJECT_AI_UNUSABLE;

						//ScreenMsg(FONT_MCOLOR_LTRED, MSG_INTERFACE, L"[%d] item %d OBJECT_AI_UNUSABLE", pSoldier->ubID, pSoldier->inv[handPOS].usItem);
						DebugAI( AI_MSG_INFO, pSoldier, String("*** NOSHOOT_NOAMMO: item %d OBJECT_AI_UNUSABLE ***", pSoldier->inv[handPOS].usItem));

						// move the gun into another pocket...
						if (!AutoPlaceObject( pSoldier, &(pSoldier->inv[handPOS]), FALSE, SECONDHANDPOS ) )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("failed to auto place item, drop to the ground"));
							// If there's no room in his pockets for the useless gun, just throw it away
							pSoldier->aiData.usActionData = handPOS;
							return AI_ACTION_DROP_ITEM;
						}
					}
					else
					{
						return( pSoldier->aiData.bAction );
					}
				}
				// sevenfm: allow enemy team to attack with hands
				else if( pSoldier->bTeam == ENEMY_TEAM && ubCanMove )
					//else if( !pSoldier->aiData.bNeutral && ubCanMove )
				{
					pSoldier->aiData.bAIMorale = MORALE_FEARLESS;
					bCanAttack = TRUE;
					fTryPunching = TRUE;
				}
				else
				{
					bCanAttack = FALSE;
				}
			}
		} while( bCanAttack != TRUE && bCanAttack != FALSE );

		if (!bCanAttack)
		{
			if (pSoldier->aiData.bAIMorale > MORALE_WORRIED)
			{
				pSoldier->aiData.bAIMorale = MORALE_WORRIED;
			}

			if (!fCivilian)
			{
				// can always attack with HTH as a last resort
				bCanAttack = TRUE;
				fTryPunching = TRUE;
			}
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Use TNT to blow up enemies nearby]"));
	INT8 bTNTSlot = FindTNT(pSoldier);
	if (bTNTSlot != NO_SLOT &&
		SoldierAI(pSoldier) &&		
		(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
		IsActionAffordable(pSoldier, AI_ACTION_PLANT_BOMB) &&
		!pSoldier->aiData.bUnderFire &&
		//pSoldier->pathing.bLevel == 0 &&
		//pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
		(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) &&
		//!TileIsOutOfBounds(sClosestOpponent) &&
		!TileIsOutOfBounds(sClosestSeenOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestSeenOpponent) > TACTICAL_RANGE / 2 &&
		CountKnownEnemies(pSoldier, pSoldier->sGridNo, 2) > 0 &&				
		(Chance(SoldierDifficultyLevel(pSoldier) * 10) || InARoom(pSoldier->sGridNo, NULL) && CountKnownEnemies(pSoldier, pSoldier->sGridNo, 2, pSoldier->pathing.bLevel ? 0 : 1) > 0))
		//(Chance(SoldierDifficultyLevel(pSoldier) * 20) || InARoom(pSoldier->sGridNo, NULL)))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found TNT in slot %d, plant bomb", bTNTSlot));

		RearrangePocket(pSoldier, HANDPOS, bTNTSlot, FOREVER);
		pSoldier->aiData.usActionData = 0;
		return AI_ACTION_PLANT_BOMB;
	}

	if (SoldierAI(pSoldier) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		!pSoldier->bBlindedCounter &&
		(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
		!pSoldier->aiData.bUnderFire &&
		pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
		(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		(Chance(SoldierDifficultyLevel(pSoldier) * 10) ||
		InARoom(sClosestOpponent) &&
		CountFriendsInRoom(pSoldier, RoomNo(sClosestOpponent)) == 0 &&
		!SameRoom(sClosestOpponent, pSoldier->sGridNo) &&
		(CountCorpsesInRoom(pSoldier, RoomNo(sClosestOpponent), 0) > 0 || TeamHighPercentKilled(pSoldier->bTeam) || EstimatePathCostToLocation(pSoldier, sClosestOpponent, pSoldier->pathing.bLevel, FALSE, NULL, NULL) == 0)))
	{
		if (DecideBlowUpObstacle(pSoldier, sClosestOpponent))
			return pSoldier->aiData.bAction;
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[use wire cutters to cut fence]"));
	INT8 bWirecutterSlot = FindWirecutters(pSoldier);
	if (SoldierAI(pSoldier) &&
		(pSoldier->CheckInitialAP() || gfTurnBasedAI) &&
		!pSoldier->aiData.bUnderFire &&
		bWirecutterSlot != NO_SLOT &&
		pSoldier->pathing.bLevel == 0 &&
		pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
		(pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->RushAttackActive()) && 
		!TileIsOutOfBounds(sClosestOpponent) &&
		//PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 2 &&
		Chance(SoldierDifficultyLevel(pSoldier) * 20) &&
		pSoldier->bActionPoints >= GetAPsToCutFence(pSoldier) + GetAPsToLook(pSoldier) &&
		FindFenceAroundSpot(pSoldier->sGridNo))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found wire cutter, check if we can find fence to cut"));

		UINT8 ubDesiredDir = AIDirection(pSoldier->sGridNo, sClosestOpponent);
		UINT8 ubCheckDir;
		INT32 sNewSpot;
		INT32 sNextSpot;
		INT32 sPathCost, sNewPathCost;
		INT32 sOriginalGridNo;

		for (ubCheckDir = 0; ubCheckDir < NUM_WORLD_DIRECTIONS; ubCheckDir++)
		{
			// cannot cut fence diagonally, check desired dir
			if (ubCheckDir % 2 != 0 ||
				ubCheckDir != ubDesiredDir &&
				ubCheckDir != gOneCDirection[ubDesiredDir] &&
				ubCheckDir != gOneCCDirection[ubDesiredDir])
			{
				continue;
			}

			sNewSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubCheckDir));
			sNextSpot = NewGridNo(sNewSpot, DirectionInc(ubCheckDir));

			if (sNewSpot != pSoldier->sGridNo &&
				sNextSpot != sNewSpot &&
				!Water(sNewSpot, pSoldier->pathing.bLevel) &&
				!Water(sNextSpot, pSoldier->pathing.bLevel) &&
				IsCuttableWireFenceAtGridNo(sNewSpot) &&
				!IsCutWireFenceAtGridNo(sNewSpot) &&
				IsLocationSittable(sNextSpot, pSoldier->pathing.bLevel))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("found cuttable fence at %d", sNewSpot));

				// check if cutting the fence improves situation
				sPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
				sOriginalGridNo = pSoldier->sGridNo;
				pSoldier->sGridNo = sNewSpot;
				sNewPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
				pSoldier->sGridNo = sOriginalGridNo;

				if (sNewPathCost > 0 && (sPathCost == 0 || sPathCost > sNewPathCost && sPathCost - sNewPathCost > APBPConstants[AP_MAXIMUM]))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("cutting fence improves path cost, use wire cutter"));
					if (pSoldier->ubDirection == ubCheckDir)
					{
						RearrangePocket(pSoldier, HANDPOS, bWirecutterSlot, FOREVER);
						pSoldier->aiData.usActionData = sNewSpot;
						return AI_ACTION_HANDLE_ITEM;
					}
					else if (pSoldier->InternalIsValidStance(ubCheckDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("turn before cutting fence"));
						RearrangePocket(pSoldier, HANDPOS, bWirecutterSlot, FOREVER);
						pSoldier->aiData.usActionData = ubCheckDir;
						pSoldier->aiData.bNextAction = AI_ACTION_HANDLE_ITEM;
						pSoldier->aiData.usNextActionData = sNewSpot;
						return AI_ACTION_CHANGE_FACING;
					}
				}
			}
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[self smoke when under fire]"));
	if( SoldierAI(pSoldier) &&
		gfTurnBasedAI &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->aiData.bUnderFire &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		!InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, TRUE) &&
		RangeChangeDesire(pSoldier) <= 2 &&
		(!NightLight() || InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel)) &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > DAY_VISION_RANGE / 4 &&
		(!fProneSightCover && !fAnyCover && !SightCoverNearSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE) || pSoldier->TakenLargeHit()) &&
		(	pSoldier->TakenLargeHit() || 
			pSoldier->TakenHit() && pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2 ||
			pSoldier->ShockLevelPercent() > 20 + Random(80))
		)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("check if soldier can cover himself with smoke"));

		CheckTossSelfSmoke(pSoldier, &BestThrow);

		if (BestThrow.ubPossible)
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

			if (pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE)
			{
				// start retreating for several turns
				pSoldier->RetreatStart(RETREAT_DURATION);
			}
			
			// if necessary, swap the usItem from holster into the hand position
			if (BestThrow.bWeaponIn != HANDPOS)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket"));
				RearrangePocket(pSoldier,HANDPOS,BestThrow.bWeaponIn,FOREVER);
			}			

			// stand up before throwing if needed
			if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
				pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
			{
				pSoldier->aiData.usActionData = BestThrow.ubStance;
				pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
				pSoldier->aiData.usNextActionData = BestThrow.sTarget;
				pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
				pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				return AI_ACTION_CHANGE_STANCE;
			}
			else
			{
				pSoldier->aiData.usActionData = BestThrow.sTarget;
				pSoldier->bTargetLevel = BestThrow.bTargetLevel;
				pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
			}

			return(AI_ACTION_TOSS_PROJECTILE);
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start retreating]"));
	if (gfTurnBasedAI &&
		pSoldier->IsMercBodyType() &&
		SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
		pSoldier->aiData.bUnderFire &&
		pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
		(fDangerousSpot || pSoldier->TakenHit() || pSoldier->bBlindedCounter) &&
		//(!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE) && !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE) || pSoldier->TakenHit()) &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] == 0 &&
		!AICheckSuccessfulAttack(pSoldier, FALSE) &&
		Chance(1 + (pSoldier->ShockLevelPercent() + pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] + 5 * CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2) + 5 * CountCorpses(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4, FALSE)) * 100 * 10 * 5 /
		(10 * (100 + pSoldier->aiData.bMorale) * (10 + pSoldier->stats.bExpLevel) * (5 + RangeChangeDesire(pSoldier))))
		)
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("start retreating"));
		pSoldier->RetreatStart(RETREAT_DURATION);
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start retreating when in shock or wounded]"));
	if (gfTurnBasedAI &&
		pSoldier->IsMercBodyType() &&
		SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
		pSoldier->aiData.bUnderFire &&
		pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
		!fAnyCover &&
		(pSoldier->aiData.bWoundShock > 0 || pSoldier->InShock()) &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] == 0 &&
		!AICheckSuccessfulAttack(pSoldier, FALSE) &&
		(Chance(pSoldier->aiData.bWoundShock) || Chance(pSoldier->aiData.bShock / 10)) &&
		!Chance(SoldierDifficultyLevel(pSoldier) * 10))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("start retreating when in shock or wounded"));
		pSoldier->RetreatStart(RETREAT_DURATION);
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[possibly start retreating when bleeding]"));
	if (gfTurnBasedAI &&
		pSoldier->IsMercBodyType() &&
		SoldierAI(pSoldier) &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
		//pSoldier->aiData.bUnderFire &&
		pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4 &&
		!fSafeSpot &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] == 0 &&
		!AICheckSuccessfulAttack(pSoldier, FALSE) &&
		(!AICheckSuccessfulAttack(pSoldier, TRUE) || pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2) &&
		pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD &&
		pSoldier->iHealableInjury >= MIN_HEALABLE_INJURY &&
		Chance(pSoldier->bBleeding) &&
		(pSoldier->CanFirstAidAI() || GetClosestMedicSoldierID(pSoldier, TACTICAL_RANGE, pSoldier->bTeam) != NOBODY))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("start retreating when bleeding"));
		pSoldier->RetreatStart(RETREAT_DURATION);
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[check to stop retreating]"));
	// stop retreating if current spot is safe
	if (SoldierAI(pSoldier) &&
		pSoldier->CheckInitialAP() &&
		pSoldier->RetreatActive() &&
		pSoldier->RetreatValue() < RETREAT_DURATION)
	{
		if (!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE) &&
			SpotDangerLevel(pSoldier, pSoldier->sGridNo) < 2 &&
			AnyCoverAtSpot(pSoldier, pSoldier->sGridNo) &&
			SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("current spot is safe, stop retreating"));
			pSoldier->StopTacticalOrder();
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Pick up weapon]"));
	// if we don't have a gun, look around for a weapon!
	INT8 bGunSlot = FindAIUsableObjClass( pSoldier, IC_GUN );
	DebugAI( AI_MSG_INFO, pSoldier, String("found gun slot %d", bGunSlot));

	if( (pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM ) &&
		ubCanMove && 
		!pSoldier->aiData.bNeutral &&
		gTacticalStatus.bBoxingState == NOT_BOXING &&
		!InWaterOrGas(pSoldier, pSoldier->sGridNo) &&
		(bGunSlot == NO_SLOT || (pSoldier->inv[bGunSlot][0]->data.gun.ubGunShotsLeft == 0 && FindAmmoToReload( pSoldier, bGunSlot, NO_SLOT ) == NO_SLOT)))
	{
		UINT16 usItem = (bGunSlot == NO_SLOT ? pSoldier->inv[HANDPOS].usItem : pSoldier->inv[bGunSlot].usItem);
		// look around for a gun...
		pSoldier->aiData.bAction = SearchForItems(pSoldier, SEARCH_WEAPONS, usItem);
		if (pSoldier->aiData.bAction != AI_ACTION_NONE )
		{
			return( pSoldier->aiData.bAction );
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[trait skills]"));
	// Flugente: trait skills
	// sevenfm: don't use skill when in interrupt
	if (!fCivilian &&
		!bInWater &&
		!bInGas &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->CheckInitialAP() &&
		pSoldier->CanUseRadio(TRUE))
	{
		UINT32 tmp;
		INT32 iArtilleryGridNo = NOWHERE;

		BOOLEAN fJamming = pSoldier->IsJamming();
		BOOLEAN fScanning = pSoldier->IsScanning();
		BOOLEAN fListening = pSoldier->IsListening();
		BOOLEAN fSectorJammed = SectorJammed();
		BOOLEAN fUsingRadio = pSoldier->IsUsingRadio();
		BOOLEAN fTeamSawEnemy = TeamSawEnemyPublic(pSoldier->bTeam, SEEN_3_TURNS_AGO);
		BOOLEAN fPlayerCanCallArtillery = TeamCanOrderArtilleryStrike(gbPlayerNum);
		BOOLEAN fPlayerCanCallReinforcements = MoreFriendsThanEnemiesInNearbysectors(gbPlayerNum, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
		BOOLEAN fPlayerJamming = TeamHasJammingRadioOperator(gbPlayerNum);
		BOOLEAN fPlayerListening = TeamHasListeningRadioOperator(gbPlayerNum);

		DebugAI(AI_MSG_INFO, pSoldier, String("fJamming %d fScanning %d fListening %d fSectorJammed %d fUsingRadio %d fTeamSawEnemy %d fPlayerCanCallArtillery %d fPlayerCanCallReinforcements %d fPlayerJamming %d", fJamming, fScanning, fListening, fSectorJammed, fUsingRadio, fTeamSawEnemy, fPlayerCanCallArtillery, fPlayerCanCallReinforcements, fPlayerJamming));

		BOOLEAN fCanCallReinforcements = !fPlayerJamming;
		BOOLEAN fCanCallArtillery = pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, FALSE) && pSoldier->CanAnyArtilleryStrikeBeOrdered(&tmp);

		BOOLEAN fCanScan = pSoldier->CanUseSkill(SKILLS_RADIO_SCAN_FOR_JAM, FALSE);
		BOOLEAN fCanListen = pSoldier->CanUseSkill(SKILLS_RADIO_LISTEN, FALSE);
		BOOLEAN fCanJam = pSoldier->CanUseSkill(SKILLS_RADIO_JAM, FALSE);
		BOOLEAN fCanTurnOff = pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, FALSE);

		DebugAI(AI_MSG_INFO, pSoldier, String("fCanCallReinforcements %d fCanCallArtillery %d fCanScan %d fCanListen %d fCanJam %d fCanTurnOff %d", fCanCallReinforcements, fCanCallArtillery, fCanScan, fCanListen, fCanJam, fCanTurnOff));

		BOOLEAN fWantCallReinforcements = fCanCallReinforcements && gbWorldSectorZ == 0 && MoreFriendsThanEnemiesInNearbysectors(pSoldier->bTeam, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ);
		BOOLEAN fWantCallArtillery = fCanCallArtillery && gbWorldSectorZ == 0 && !RedSmokeOnMap() && GetBestAoEGridNo(pSoldier, &iArtilleryGridNo, max(1, min(TACTICAL_RANGE, (INT16)gSkillTraitValues.usVOMortarRadius)), 1, 4 - gGameOptions.ubDifficultyLevel / 2, SoldierCondTrue, SoldierCondFalse);

		BOOLEAN fWantScan = fCanScan && fPlayerJamming;
		BOOLEAN fWantJam = fCanJam && gbWorldSectorZ == 0 && !fPlayerJamming && !fWantCallReinforcements && !fWantCallArtillery && (fPlayerCanCallArtillery || fPlayerCanCallReinforcements || fPlayerListening || FindRemoteBombsInSector() && WeAttack(pSoldier->bTeam) && gSkillTraitValues.fVOJammingBlocksRemoteBombs);
		BOOLEAN fWantListen = fCanListen && !fWantJam && !fWantScan && gSkillTraitValues.sVOListeningHearingBonus > 5 && (NightTime() || RainIntensity() > 0 || !fTeamSawEnemy || TileIsOutOfBounds(sClosestOpponent) || gbWorldSectorZ > 0);

		DebugAI(AI_MSG_INFO, pSoldier, String("fWantCallReinforcements %d fWantCallArtillery %d fWantScan %d fWantJam %d fWantListen %d", fWantCallReinforcements, fWantCallArtillery, fWantScan, fWantJam, fWantListen));

		// want call reinforcements or order artillery strike, stop jamming
		if ((fWantCallReinforcements || fWantCallArtillery) && fJamming)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to call reinforcements or order artillery strike, try to stop jamming"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio"));
				pSoldier->ubAISkillUse = SKILLS_RADIO_TURNOFF;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want call artillery strike
		else if (fWantCallArtillery && !fSectorJammed)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to order artillery strike"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_ARTILLERY, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("order artillery strike"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_ARTILLERY;
				pSoldier->aiData.usActionData = iArtilleryGridNo;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want scan frequencies to find enemy jammer
		else if (fWantScan && !fScanning)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to find enemy jammer"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_SCAN_FOR_JAM, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start scanning"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_SCAN_FOR_JAM;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want jam communications
		else if (fWantJam && !fJamming)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to jam communications"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_JAM, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start jamming"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_JAM;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// want listen to find enemy location
		else if (fWantListen && !fListening)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("want to listen to know enemy location"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_LISTEN, FALSE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("start listening"));
				// ready to use skill
				pSoldier->ubAISkillUse = SKILLS_RADIO_LISTEN;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
		// don't want to use radio anymore
		else if (!fWantJam && !fWantScan && !fWantListen && fUsingRadio)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("don't want to use radio anymore"));
			if (pSoldier->CanUseSkill(SKILLS_RADIO_TURNOFF, TRUE))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("turn off radio"));
				pSoldier->ubAISkillUse = SKILLS_RADIO_TURNOFF;
				pSoldier->aiData.usActionData = NOWHERE;
				return AI_ACTION_USE_SKILL;
			}
		}
	}

	BestShot.ubPossible  = FALSE;	// by default, assume Shooting isn't possible
	BestThrow.ubPossible = FALSE;	// by default, assume Throwing isn't possible
	BestStab.ubPossible  = FALSE;	// by default, assume Stabbing isn't possible

	BestAttack.ubChanceToReallyHit = 0;
	
	// if we are able attack
	if (bCanAttack)
	{
		pSoldier->bAimShotLocation = AIM_SHOT_RANDOM;
		pSoldier->bAimMeleeLocation = AIM_SHOT_RANDOM;

		//////////////////////////////////////////////////////////////////////////
		// FIRE A GUN AT AN OPPONENT
		//////////////////////////////////////////////////////////////////////////
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Shooting]"));

		CheckIfShotPossible(pSoldier, &BestShot);
		DebugAI(AI_MSG_INFO, pSoldier, String("CalcBestShot: possible %d opponent %d CTH %d AP cost %d target gridno %d", BestShot.ubPossible, BestShot.ubOpponent, BestShot.ubChanceToReallyHit, BestShot.ubAPCost, BestShot.sTarget));

		if (BestShot.ubFriendlyFireChance)//dnl ch61 180813
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("Friendly fire chance %d", BestShot.ubFriendlyFireChance));

			// determine chance to shoot
			INT32 iChanceToShoot;

			iChanceToShoot = 100 - BestShot.ubFriendlyFireChance;
			//iChanceToShoot = iChanceToShoot * iChanceToShoot / 100;

			DebugAI(AI_MSG_INFO, pSoldier, String("Friendly fire chance %d, chance to shoot %d", BestShot.ubFriendlyFireChance, iChanceToShoot));

			if (Chance(100 - iChanceToShoot))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("Skip shooting!"));
				BestShot.ubPossible = FALSE;
			}
		}

		if (BestShot.ubPossible)
		{
			// avoid shooting at dying opponents if there is reachable alive opponent
			if ((Menptr[BestShot.ubOpponent].stats.bLife < OKLIFE) &&
				!Menptr[BestShot.ubOpponent].bService &&
				!TileIsOutOfBounds(sClosestOpponent) &&
				(pSoldier->aiData.bAttitude != AGGRESSIVE && !MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) || Chance(100 - BestShot.ubChanceToReallyHit)) &&
				!TileIsOutOfBounds(sClosestDisturbance) &&
				ubThreatID != NOBODY &&
				MercPtrs[ubThreatID] &&
				MercPtrs[ubThreatID]->stats.bLife >= OKLIFE)
			{
				// don't bother checking GRENADES/KNIVES, he can't have conscious targets
				// then make decision as if at alert status RED, but make sure
				// we don't try to SEEK OPPONENT the unconscious guy!
				DebugAI(AI_MSG_INFO, pSoldier, String("Skip shooting at dying opponent, call DecideActionRed()"));
				return DecideActionRed(pSoldier);
			}

			// now we KNOW FOR SURE that we will do something (shoot, at least)
			DebugAI(AI_MSG_INFO, pSoldier, String("Shot is possible"));
			NPCDoesAct(pSoldier);
		}

		//////////////////////////////////////////////////////////////////////////
		// THROW A TOSSABLE ITEM AT OPPONENT(S)
		// 	- HTH: THIS NOW INCLUDES FIRING THE GRENADE LAUNCHAR AND MORTAR!
		//////////////////////////////////////////////////////////////////////////

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Throw]"));

		// this looks for throwables, and sets BestThrow.ubPossible if it can be done
		CheckIfTossPossible(pSoldier,&BestThrow);

		if (BestThrow.ubPossible)
		{
			DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"good throw possible");
			if (Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].mortar)
			{
				ubOpponentDir = AIDirection(pSoldier->sGridNo, BestThrow.sTarget);

				// Get new gridno!
				sCheckGridNo = NewGridNo( pSoldier->sGridNo, (UINT16)DirectionInc( ubOpponentDir ) );

				if (!OKFallDirection(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, ubOpponentDir, pSoldier->usAnimState))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("no room to deploy mortar, check if we can move behind"));

					// try behind us, see if there's room to move back and we have enough AP to move and fire
					sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(gOppositeDirection[ubOpponentDir]));
					BOOLEAN bReverse = FALSE;
					if (pSoldier->ubDirection == ubOpponentDir || pSoldier->ubDirection == gOneCDirection[ubOpponentDir] || pSoldier->ubDirection == gOneCCDirection[ubOpponentDir])
						bReverse = TRUE;
					INT32 iPathCost = EstimatePlotPath(pSoldier, sCheckGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_GET_CLOSER), pSoldier->bStealthMode, bReverse, 0);
					if (!OKFallDirection(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, gOppositeDirection[ubOpponentDir], pSoldier->usAnimState) ||
						iPathCost == 0 ||
						pSoldier->bActionPoints < BestThrow.ubAPCost + GetAPsToLook(pSoldier) + GetAPsCrouch(pSoldier, FALSE) + iPathCost)
					{
						BestThrow.ubPossible = FALSE;
					}
				}
			}

			if ( BestThrow.ubPossible )
			{
				// now we KNOW FOR SURE that we will do something (throw, at least)
				NPCDoesAct(pSoldier);
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// GO STAB AN OPPONENT WITH A KNIFE
		//////////////////////////////////////////////////////////////////////////

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Stabbing]"));

		// if soldier has a knife in his hand
		bWeaponIn = FindAIUsableObjClass( pSoldier, (IC_BLADE | IC_THROWING_KNIFE) );

		// if the soldier does have a usable knife somewhere
		if (bWeaponIn != NO_SLOT)
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("try to stab, found weapon in slot %d item %d", bWeaponIn, pSoldier->inv[bWeaponIn].usItem ));
			BestStab.bWeaponIn = bWeaponIn;
			// if it's in his holster, swap it into his hand temporarily
			if (bWeaponIn != HANDPOS)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket before stab check"));
				RearrangePocket(pSoldier,HANDPOS,bWeaponIn,TEMPORARILY);
			}

			// get the minimum cost to attack with this knife
			ubMinAPCost = MinAPsToAttack(pSoldier,pSoldier->sLastTarget,DONTADDTURNCOST,0,0);
			DebugAI( AI_MSG_INFO, pSoldier, String("min Ap to stab %d", ubMinAPCost));

			// if we can afford the minimum AP cost to stab with/throw this knife weapon
			if (pSoldier->bActionPoints >= ubMinAPCost)
			{
				// NB throwing knife in hand now
				if ( Item[ pSoldier->inv[HANDPOS].usItem ].usItemClass & IC_THROWING_KNIFE )
				{
					// throwing knife code works like shooting
					DebugAI( AI_MSG_INFO, pSoldier, String("check throwing"));

					// look around for a worthy target (which sets BestStab.ubPossible)
					CalcBestShot(pSoldier,&BestStab);

					if (BestStab.ubPossible)
					{
						DebugAI( AI_MSG_INFO, pSoldier, String("throwing is possible"));

						// if the selected opponent is not a threat (unconscious & !serviced)
						// (usually, this means all the guys we see are unconscious, but, on
						//  rare occasions, we may not be able to shoot a healthy guy, too)
						if ((Menptr[BestStab.ubOpponent].stats.bLife < OKLIFE) &&
							!Menptr[BestStab.ubOpponent].bService)
						{
							// don't throw a knife at him.
							BestStab.ubPossible = FALSE;
							DebugAI( AI_MSG_INFO, pSoldier, String("opponent is not a threat, skip throwing"));
						}

						// now we KNOW FOR SURE that we will do something (shoot, at least)
						NPCDoesAct(pSoldier);
						DebugAI( AI_MSG_INFO, pSoldier, String("NPC decided to throw"));
					}
				}
				else
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("check stabbing"));

					//sprintf((CHAR *)tempstr,"%s - ubMinAPCost = %d",pSoldier->name,ubMinAPCost);
					//PopMessage(tempstr);
					// then look around for a worthy target (which sets BestStab.ubPossible)
					CalcBestStab(pSoldier,&BestStab, TRUE);

					if (BestStab.ubPossible)
					{
						INT32 sAttackDist = PythSpacesAway(pSoldier->sGridNo, BestStab.sTarget);
						INT32 sMaxStabAttackDist = DAY_VISION_RANGE / 8;
						// sevenfm: limit HTH attacks when target is not very close
						if( sAttackDist > sMaxStabAttackDist )
						{
							//BestStab.iAttackValue /= 2;
							BestStab.iAttackValue = BestStab.iAttackValue * sMaxStabAttackDist / sAttackDist;
						}

						if (!(gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, MELEE_NT)))
						{
							BestStab.iAttackValue /= 4;
						}

						// now we KNOW FOR SURE that we will do something (stab, at least)
						NPCDoesAct(pSoldier);
						DebugAI( AI_MSG_INFO, pSoldier, String("NPC decided to stab"));
					}
				}

			}

			// if it was in his holster, swap it back into his holster for now
			if (bWeaponIn != HANDPOS)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket after stab check"));
				RearrangePocket(pSoldier,HANDPOS,bWeaponIn,TEMPORARILY);
			}
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		// SANDRO - even if we don't have any blade, calculate how much damage we could do unarmed
		else
		{
			bWeaponIn = FindAIUsableObjClass( pSoldier, IC_PUNCH );
			if (bWeaponIn == NO_SLOT) // if no punch-type weapon found, just calculate it with empty hands
			{
				bWeaponIn = FindEmptySlotWithin( pSoldier, HANDPOS, NUM_INV_SLOTS );
				DebugAI( AI_MSG_INFO, pSoldier, String("No weapon for punch attack"));
			}
			if (bWeaponIn != NO_SLOT)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("Found slot with punch weapon %d item %d", bWeaponIn, pSoldier->inv[bWeaponIn].usItem));

				BestStab.bWeaponIn = bWeaponIn;
				// if it's in his holster, swap it into his hand temporarily
				if (bWeaponIn != HANDPOS)
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket before punch check"));
					RearrangePocket(pSoldier,HANDPOS,bWeaponIn,TEMPORARILY);
				}

				// get the minimum cost to attack with punch
				ubMinAPCost = MinAPsToAttack(pSoldier,pSoldier->sLastTarget,DONTADDTURNCOST,0,0);
				DebugAI( AI_MSG_INFO, pSoldier, String("min AP to stab %d", ubMinAPCost));

				// if we can afford the minimum AP cost to punch
				if (pSoldier->bActionPoints >= ubMinAPCost)
				{
					// then look around for a worthy target (which sets BestStab.ubPossible)
					CalcBestStab(pSoldier,&BestStab, FALSE);					

					if (BestStab.ubPossible)
					{
						DebugAI( AI_MSG_INFO, pSoldier, String("stab possible %d target %d opponent %d cth %d AP cost %d", BestStab.ubPossible, BestStab.sTarget, BestStab.ubOpponent, BestStab.ubChanceToReallyHit, BestStab.ubAPCost));

						INT32 sAttackDist = PythSpacesAway(pSoldier->sGridNo, BestStab.sTarget);
						INT32 sMaxStabAttackDist = DAY_VISION_RANGE / 8;
						// sevenfm: limit HTH attacks when target is not very close
						if( sAttackDist > sMaxStabAttackDist )
						{
							//BestStab.iAttackValue /= 2;
							BestStab.iAttackValue = BestStab.iAttackValue * sMaxStabAttackDist / sAttackDist;
						}

						// if we are not specialized, reduce the attack attractiveness generally
						if (!(gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, MARTIAL_ARTS_NT)))
						{
							BestStab.iAttackValue /= 4; 
						}
						else
						{
							// if too far and not having APs for at least 2 hits
							if ((CalcTotalAPsToAttack(pSoldier, BestStab.sTarget, ADDTURNCOST, 0) + ApsToPunch(pSoldier)) > pSoldier->bActionPoints)
							{
								BestStab.iAttackValue /= 2;
							}
						}

						// now we KNOW FOR SURE that we will do something (stab, at least)
						NPCDoesAct(pSoldier);
						DebugAI( AI_MSG_INFO, pSoldier, String("NPC decided to punch"));
					}
					else
					{
						DebugAI( AI_MSG_INFO, pSoldier, String("punching not possible"));
					}

				}	
				// if it was in his holster, swap it back into his holster for now
				if (bWeaponIn != HANDPOS)
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("rearrange pocket after punch check"));
					RearrangePocket(pSoldier,HANDPOS,bWeaponIn,TEMPORARILY);
				}		
			}
		}
		/////////////////////////////////////////////////////////////////////////////////////////////////////		

		//////////////////////////////////////////////////////////////////////////
		// CHOOSE THE BEST TYPE OF ATTACK OUT OF THOSE FOUND TO BE POSSIBLE
		//////////////////////////////////////////////////////////////////////////
		
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Choose best attack]"));

		BestAttack.iAttackValue = 0;

		// sevenfm: special code to attack zombies, disable shooting since we cannot kill lying zombie with bullets
		if (BestShot.ubPossible &&
			BestShot.ubOpponent != NOBODY &&
			MercPtrs[BestShot.ubOpponent] &&
			MercPtrs[BestShot.ubOpponent]->IsZombie() &&
			gAnimControl[MercPtrs[BestShot.ubOpponent]->usAnimState].ubEndHeight == ANIM_PRONE &&
			gGameExternalOptions.fZombieOnlyHeadshotsWork &&
			BestStab.ubPossible &&
			Item[pSoldier->inv[BestStab.bWeaponIn].usItem].usItemClass & IC_BLADE)
		{
			BestShot.ubPossible = FALSE;
		}

		if (BestShot.ubPossible)
		{
			BestAttack.iAttackValue = BestShot.iAttackValue;
			ubBestAttackAction = AI_ACTION_FIRE_GUN;
			DebugAI( AI_MSG_INFO, pSoldier, String("best action = fire gun, iAttackValue = %d", BestAttack.iAttackValue));	

			// sevenfm: disable HtH attacks against zombies
			if( BestStab.ubPossible && 
				BestStab.ubOpponent != NOBODY &&
				MercPtrs[BestStab.ubOpponent]->IsZombie() &&
				!(Item[pSoldier->inv[BestStab.bWeaponIn].usItem].usItemClass & IC_BLADE) &&
				!(Item[ pSoldier->inv[BestStab.bWeaponIn].usItem ].usItemClass & IC_THROWING_KNIFE))
			{
				BestStab.ubPossible = FALSE;
				DebugAI( AI_MSG_INFO, pSoldier, String("disable stab attack against zombie"));
			}

			// sevenfm: limit HTH/melee attacks if not specialist and the target is not very close
			if( BestStab.ubPossible && 
				!TileIsOutOfBounds(BestStab.sTarget) &&
				!pSoldier->RushAttackAdvance() &&
				SpacesAway(pSoldier->sGridNo, BestStab.sTarget) > 1 &&
				!(Item[ pSoldier->inv[BestStab.bWeaponIn].usItem ].usItemClass & IC_THROWING_KNIFE) &&
				!(gGameOptions.fNewTraitSystem && (HAS_SKILL_TRAIT(pSoldier, MARTIAL_ARTS_NT) || HAS_SKILL_TRAIT(pSoldier, MELEE_NT))))
			{
				BestStab.ubPossible = FALSE;
				DebugAI( AI_MSG_INFO, pSoldier, String("soldier is not specialist and target is not very close - disable stab attack"));
			}

			// reduce chance for throw when shot is possible
			if (BestThrow.ubPossible && !MercPtrs[BestThrow.ubOpponent]->IsTank())
			{
				BestThrow.iAttackValue /= 2;
				DebugAI(AI_MSG_INFO, pSoldier, String("reduce chance to throw when shot is possible"));
			}
		}

		// cautious boxer approach, reserve AP for two attacks (only if not attacking from the back)
		if (BestStab.ubPossible &&
			pSoldier->IsBoxer() &&
			SpacesAway(pSoldier->sGridNo, BestStab.sTarget) > 2 &&
			BestStab.ubOpponent != NOBODY &&
			MercPtrs[BestStab.ubOpponent] &&
			AIDirection(pSoldier->sGridNo, MercPtrs[BestStab.ubOpponent]->sGridNo) != MercPtrs[BestStab.ubOpponent]->ubDirection &&
			AIDirection(pSoldier->sGridNo, MercPtrs[BestStab.ubOpponent]->sGridNo) != gOneCDirection[MercPtrs[BestStab.ubOpponent]->ubDirection] &&
			AIDirection(pSoldier->sGridNo, MercPtrs[BestStab.ubOpponent]->sGridNo) != gOneCCDirection[MercPtrs[BestStab.ubOpponent]->ubDirection] &&
			pSoldier->bInitialActionPoints >= 2 * MinAPsToAttack(pSoldier, pSoldier->sLastTarget, FALSE, 0, 0) + APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK] &&
			pSoldier->bActionPoints < BestStab.ubAPCost + MinAPsToAttack(pSoldier, pSoldier->sLastTarget, FALSE, 0, 0))
		{
			BestStab.ubPossible = FALSE;
			DebugAI(AI_MSG_INFO, pSoldier, String("boxer cannot reserve APs for second attack - disable stab attack"));
		}

		// boxer should not attack dying/unconscious opponent
		if (BestStab.ubPossible &&
			pSoldier->IsBoxer() &&
			BestStab.ubOpponent != NOBODY &&
			MercPtrs[BestStab.ubOpponent] &&
			(MercPtrs[BestStab.ubOpponent]->stats.bLife < OKLIFE || MercPtrs[BestStab.ubOpponent]->IsUnconscious()))
		{
			BestStab.ubPossible = FALSE;
			DebugAI(AI_MSG_INFO, pSoldier, String("boxer should not attack dying/unconscious target - disable stab attack"));
		}

		// try to avoid frontal attack
		if (BestStab.ubPossible &&
			pSoldier->IsBoxer() &&
			SpacesAway(pSoldier->sGridNo, BestStab.sTarget) > 1 &&
			BestStab.ubOpponent != NOBODY &&
			MercPtrs[BestStab.ubOpponent] &&
			MercPtrs[BestStab.ubOpponent]->AnimEndHeight() == ANIM_STAND &&
			MercPtrs[BestStab.ubOpponent]->bActionPoints > 0 &&
			Chance(EffectiveAgility(MercPtrs[BestStab.ubOpponent], FALSE) * (100 + MercPtrs[BestStab.ubOpponent]->bBreath) * EffectiveWisdom(pSoldier) / (100 * 200)))
		{
			// find closest spot around opponent, avoid front direction
			UINT8	ubMovementCost;
			INT32	sTempGridNo;
			UINT8	ubDirection;
			INT32	sPathCost;
			INT32	sBestSpot = NOWHERE;
			INT32	sBestPathCost = 0;

			for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
			{
				sTempGridNo = NewGridNo(BestStab.sTarget, DirectionInc(ubDirection));

				if (sTempGridNo != BestStab.sTarget)
				{
					ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][pSoldier->pathing.bLevel];

					if (ubMovementCost < TRAVELCOST_BLOCKED &&
						NewOKDestination(pSoldier, sTempGridNo, FALSE, pSoldier->pathing.bLevel) &&
						AIDirection(BestStab.sTarget, sTempGridNo) != MercPtrs[BestStab.ubOpponent]->ubDirection)
					{
						sPathCost = PlotPath(pSoldier, sTempGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_GET_CLOSER), pSoldier->bStealthMode, pSoldier->bReverse, 0);
						if (TileIsOutOfBounds(sBestSpot) || sPathCost < sBestPathCost)
						{
							sBestSpot = sTempGridNo;
							sBestPathCost = sPathCost;
						}
					}
				}
			}

			if (!TileIsOutOfBounds(sBestSpot) && 
				pSoldier->bActionPoints >= sPathCost + GetAPsToLook(pSoldier) + MinAPsToAttack(pSoldier, pSoldier->sLastTarget, FALSE, 0, 0))
			{
				pSoldier->aiData.usActionData = sBestSpot;
				DebugAI(AI_MSG_INFO, pSoldier, String("boxer: get closer to opponent, avoid front direction"));
				return(AI_ACTION_GET_CLOSER);
			}
		}

		if (BestStab.ubPossible && ((BestStab.iAttackValue > BestAttack.iAttackValue) || (ubBestAttackAction == AI_ACTION_NONE)))
		{
			BestAttack.iAttackValue = BestStab.iAttackValue;
			if ( Item[ pSoldier->inv[BestStab.bWeaponIn].usItem ].usItemClass & IC_THROWING_KNIFE )
			{
				ubBestAttackAction = AI_ACTION_THROW_KNIFE;
				DebugAI( AI_MSG_INFO, pSoldier, String("best action = throw knife, iAttackValue = %d", BestAttack.iAttackValue));
			}
			else if ( Item[ pSoldier->inv[BestStab.bWeaponIn].usItem ].usItemClass & IC_BLADE ) // SANDRO - check specifically for blade attack
			{
				ubBestAttackAction = AI_ACTION_KNIFE_MOVE;
				DebugAI( AI_MSG_INFO, pSoldier, String("best action = move to stab, iAttackValue = %d", BestAttack.iAttackValue));
			}
			////////////////////////////////////////////////////////////////////////////////////
			// SANDRO - added a chance to try to steal merc's gun from hands
			else
			{
				if (AIDetermineStealingWeaponAttempt( pSoldier, MercPtrs[BestStab.ubOpponent] ) == TRUE)
				{
					ubBestAttackAction = AI_ACTION_STEAL_MOVE;
					DebugAI( AI_MSG_INFO, pSoldier, String("best action = move to steal weapon, iAttackValue = %d", BestStab.iAttackValue));
				}
				else
				{
					ubBestAttackAction = AI_ACTION_KNIFE_MOVE;
					DebugAI( AI_MSG_INFO, pSoldier, String("best action = knife move, iAttackValue = %d", BestStab.iAttackValue));
				}
			}
			////////////////////////////////////////////////////////////////////////////////////
		}

		if (BestThrow.ubPossible && 
			((BestThrow.iAttackValue > BestAttack.iAttackValue) || (ubBestAttackAction == AI_ACTION_NONE)) && 
			// sevenfm: limit smoke grenade use
			(!(Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].usItemClass & IC_GRENADE && Explosive[Item[pSoldier->inv[BestThrow.bWeaponIn].usItem].ubClassIndex].ubType == EXPLOSV_SMOKE) || ubBestAttackAction == AI_ACTION_NONE || Random(25) > (UINT8)BestAttack.ubChanceToReallyHit))
		{
			ubBestAttackAction = AI_ACTION_TOSS_PROJECTILE;
			DebugAI( AI_MSG_INFO, pSoldier, String("best action = throw something, iAttackValue = %d", BestThrow.iAttackValue));
		}

		if ( ubBestAttackAction == AI_ACTION_NONE && fTryPunching )
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Try punching]"));

			// nothing (else) to attack with so let's try hand-to-hand
			bWeaponIn = FindObj( pSoldier, NOTHING, HANDPOS, NUM_INV_SLOTS );

			if (bWeaponIn != NO_SLOT)
			{
				BestStab.bWeaponIn = bWeaponIn;
				// if it's in his holster, swap it into his hand temporarily
				if (bWeaponIn != HANDPOS)
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("swap knife into hand"));
					RearrangePocket(pSoldier,HANDPOS,bWeaponIn,TEMPORARILY);
				}

				// get the minimum cost to attack by HTH
				ubMinAPCost = MinAPsToAttack(pSoldier,pSoldier->sLastTarget,DONTADDTURNCOST,0,0);

				// if we can afford the minimum AP cost to use HTH combat
				if (pSoldier->bActionPoints >= ubMinAPCost)
				{
					// then look around for a worthy target (which sets BestStab.ubPossible)
					CalcBestStab(pSoldier,&BestStab, FALSE);

					if (BestStab.ubPossible)
					{
						// now we KNOW FOR SURE that we will do something (stab, at least)
						NPCDoesAct(pSoldier);
						ubBestAttackAction = AI_ACTION_KNIFE_MOVE;
						DebugAI( AI_MSG_INFO, pSoldier, String("best action = move to stab, iAttackValue = %d", BestStab.iAttackValue));
					}
				}

				// if it was in his holster, swap it back into his holster for now
				if (bWeaponIn != HANDPOS)
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("put knife away"));
					RearrangePocket(pSoldier,HANDPOS,bWeaponIn,TEMPORARILY);
				}
			}
		}

		// copy the information on the best action selected into BestAttack struct
		DebugAI( AI_MSG_INFO, pSoldier, String("copy the information on the best action selected into BestAttack struct"));
		switch (ubBestAttackAction)
		{
		case AI_ACTION_FIRE_GUN:
			memcpy(&BestAttack,&BestShot,sizeof(BestAttack));
			DebugAI( AI_MSG_INFO, pSoldier, String("Best attack - shooting"));
			break;

		case AI_ACTION_TOSS_PROJECTILE:
			memcpy(&BestAttack,&BestThrow,sizeof(BestAttack));
			DebugAI( AI_MSG_INFO, pSoldier, String("Best attack - throwing grenade"));
			break;

		case AI_ACTION_THROW_KNIFE:
		case AI_ACTION_KNIFE_MOVE:
			DebugAI( AI_MSG_INFO, pSoldier, String("Best attack - stab"));
			memcpy(&BestAttack,&BestStab,sizeof(BestAttack));
			break;
		case AI_ACTION_STEAL_MOVE: // added by SANDRO
			DebugAI( AI_MSG_INFO, pSoldier, String("Best attack - steal weapon"));
			memcpy(&BestAttack,&BestStab,sizeof(BestAttack));
			break;

		default:
			// set to empty
			DebugAI( AI_MSG_INFO, pSoldier, String("Best attack - no good attack"));
			memset( &BestAttack, 0, sizeof( BestAttack ) );
			break;
		}
	}

	// avoid attacking empty vehicle if there is reachable regular opponent
	if (BestAttack.ubPossible &&
		MercPtrs[BestAttack.ubOpponent]->IsEmptyVehicle() &&
		!TileIsOutOfBounds(sClosestDisturbance) &&
		ubThreatID != NOBODY &&
		MercPtrs[ubThreatID] &&
		!MercPtrs[ubThreatID]->IsEmptyVehicle())
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("Skip attacking empty vehicle, call DecideActionRed()"));
		return DecideActionRed(pSoldier);
	}

	UINT16 usRange = BestAttack.bWeaponIn==NO_SLOT ? 0 : GetModifiedGunRange(pSoldier->inv[BestAttack.bWeaponIn].usItem);
	INT8 bClosestThreatLevel = 0;
	INT32 sClosestThreat = ClosestKnownOpponent(pSoldier, NULL, &bClosestThreatLevel);

	// sevenfm: check to continue RED flanking
	if( !gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!fStopMovement &&
		SoldierAI(pSoldier) &&
		pSoldier->IsFlanking() &&
		(TileIsOutOfBounds(sClosestThreat) || PythSpacesAway(pSoldier->sGridNo, sClosestThreat) > TACTICAL_RANGE / 2 ) &&
		(ubBestAttackAction == AI_ACTION_NONE || BestAttack.ubChanceToReallyHit < 10) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("Continue RED flanking"));
		return(DecideActionRed(pSoldier));
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[medic]"));
	if (!pSoldier->RushAttackAdvance() && 
		(TileIsOutOfBounds(sClosestThreat) || PythSpacesAway(pSoldier->sGridNo, sClosestThreat) > TACTICAL_RANGE / 2) &&
		(ubBestAttackAction == AI_ACTION_NONE || BestAttack.ubChanceToReallyHit < 10))
	{
		if (DecideMedicCombat(pSoldier))
			return pSoldier->aiData.bAction;
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Black Retreat]"));
	if( gfTurnBasedAI &&
		!bInWater &&
		ubCanMove &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		SoldierAI(pSoldier) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->RetreatActive() &&
		(ubBestAttackAction == AI_ACTION_NONE ||
		ubBestAttackAction == AI_ACTION_FIRE_GUN && (UINT8)BestAttack.ubChanceToReallyHit < 10 + Random(pSoldier->ShockLevelPercent() / 4) + Random(pSoldier->aiData.bWoundShock) ||
		ubBestAttackAction == AI_ACTION_FIRE_GUN && (UINT8)BestAttack.ubChanceToReallyHit < 50 + Random(pSoldier->ShockLevelPercent() / 2) && pSoldier->usSoldierFlagMask & SOLDIER_MILITIA_ORDER) &&
		(pSoldier->CheckInitialAP() || !fAnyCover || AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE)))
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("search for retreat spot"));
		INT32 sRetreatSpot = FindRetreatSpot(pSoldier);

		if( !TileIsOutOfBounds(sRetreatSpot) )
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("found retreat spot %d", sRetreatSpot));

			//BeginMultiPurposeLocator(sRetreatSpot, pSoldier->pathing.bLevel, FALSE);

			pSoldier->aiData.usActionData = sRetreatSpot;

			// alt movement mode for retreat
			if (DecideRetreatReverse(pSoldier, sClosestThreat, bClosestThreatLevel))
				return pSoldier->aiData.bAction;

			return AI_ACTION_TAKE_COVER;
		}
		else if (pSoldier->CheckInitialAP() && 
				(pSoldier->bTeam == MILITIA_TEAM || !fAnyCover && (pSoldier->InShock() || pSoldier->aiData.bWoundShock > 0)) &&
				pSoldier->bActionPoints > APBPConstants[AP_MAXIMUM] / 2)
			// (pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("couldn't find retreat spot, try to run away"));
			// look for best place to RUN AWAY to (farthest from the closest threat)
			pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

			if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
			{
				return AI_ACTION_RUN_AWAY;
			}
		}
	}

	// officer possibly start rush attack
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[officer possibly start black rush attack]"));
	if (gfTurnBasedAI &&
		!bInWater &&
		ubCanMove &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		SoldierAI(pSoldier) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->CheckInitialAP() &&
		IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT) &&
		AIRoleOfficer(pSoldier) &&
		!pSoldier->aiData.bUnderFire &&
		pSoldier->pathing.bLevel == 0 &&
		!TileIsOutOfBounds(sClosestDisturbance) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) < TACTICAL_RANGE &&
		CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) == 0 &&
		AICheckWeOutnumberPublic(pSoldier, sClosestDisturbance) &&
		CountFriendsBlack(pSoldier, sClosestDisturbance) > 0 &&
		CountFriendsLastAttackHit(pSoldier, sClosestDisturbance, TACTICAL_RANGE) > 0)
	{
		UINT8 ubCanRushFriends;
		UINT8 ubRushAttackFriends;		
		UINT8 ubFriendsUnderAttack;
		UINT8 ubFriendsShortRangeWeapon;
		UINT8 ubCorpses;
		UINT8 ubPublicEnemies;
		INT32 iPathCost;
		UINT8 ubFriendsAttackHit;
		UINT8 ubOutnumberBonus = 0;

		iPathCost = EstimatePlotPath(pSoldier, sClosestDisturbance, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
		ubCanRushFriends = CountCanRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE, APBPConstants[AP_MAXIMUM] * 2, FALSE, TRUE);
		ubRushAttackFriends = CountRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE);
		ubFriendsUnderAttack = CountTeamUnderAttack(pSoldier->bTeam, sClosestDisturbance, TACTICAL_RANGE);
		ubFriendsShortRangeWeapon = CountFriendsShortRangeWeapon(pSoldier, sClosestDisturbance, TACTICAL_RANGE);
		ubCorpses = CountCorpses(pSoldier, sClosestDisturbance, VISION_RANGE, FALSE);
		ubPublicEnemies = CountPublicKnownEnemies(pSoldier, sClosestDisturbance, VISION_RANGE);
		ubFriendsAttackHit = CountFriendsLastAttackHit(pSoldier, sClosestDisturbance, TACTICAL_RANGE);

		if (ubCanRushFriends > ubPublicEnemies)
		{
			ubOutnumberBonus = ubCanRushFriends - ubPublicEnemies;
		}

		DebugAI(AI_MSG_INFO, pSoldier, String("iPathCost %d ubCanRushFriends %d ubFriendsUnderAttack %d ubCorpses %d ubPublicEnemies %d ubFriendsAttackHit %d", iPathCost, ubCanRushFriends, ubFriendsUnderAttack, ubCorpses, ubPublicEnemies, ubFriendsAttackHit));

		if (iPathCost > 0 &&
			iPathCost < APBPConstants[AP_MAXIMUM] * 2 &&
			(ubCorpses > 0 || ubFriendsUnderAttack || ubRushAttackFriends > 0) &&
			(ubCanRushFriends > ubCorpses &&
			ubCanRushFriends > ubPublicEnemies &&
			Chance(min(5 * (ubCanRushFriends - ubPublicEnemies), ubFriendsAttackHit + ubFriendsShortRangeWeapon + ubOutnumberBonus)) ||
			ubCanRushFriends + ubRushAttackFriends > ubCorpses + ubCorpses * TeamPercentKilled(pSoldier->bTeam) / 100 &&
			ubCanRushFriends + ubRushAttackFriends > ubPublicEnemies + ubPublicEnemies * TeamPercentKilled(pSoldier->bTeam) / 100 &&
			Chance(10 * ubRushAttackFriends)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("start black rush attack at spot %d", sClosestDisturbance));
			StartRushAttackFriends(pSoldier, sClosestDisturbance, TACTICAL_RANGE, 0);

			// simulate radio communication
			return(AI_ACTION_FAKE_ALERT);
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Black climb]"));
	// sevenfm: black climb
	if( SoldierAI(pSoldier) &&
		!fStopMovement &&
		!pSoldier->RushAttackAdvance() &&
		ubBestAttackAction == AI_ACTION_FIRE_GUN &&
		!TileIsOutOfBounds(BestAttack.sTarget) &&
		BestAttack.ubOpponent != NOBODY &&
		!TileIsOutOfBounds(sClosestThreat) &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		ubCanMove )
	{
		if( (pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM ) && 
			BestAttack.ubChanceToReallyHit < 30 &&
			BestAttack.ubChanceToReallyHit >= 1 &&
			RangeChangeDesire(pSoldier) < 4 &&
			pSoldier->aiData.bOrders != STATIONARY &&
			PythSpacesAway(pSoldier->sGridNo, sClosestThreat) > DAY_VISION_RANGE/2 &&
			PythSpacesAway(pSoldier->sGridNo, sClosestThreat) <= usRange/CELL_X_SIZE &&
			usRange / CELL_X_SIZE > 3*DAY_VISION_RANGE/4 &&
			pSoldier->pathing.bLevel == 0 &&
			gfTurnBasedAI &&
			pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
			CountNearbyFriendsOnRoof(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4) < CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4) / 2 &&
			Chance(100 - 100 / (1 + BestAttack.bTargetLevel + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4))) )
		{
			// check if we can climb right here
			if( CanClimbFromHere(pSoldier) )
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("can climb - check if action is affordable"));
				if(IsActionAffordable(pSoldier, AI_ACTION_CLIMB_ROOF))
				{
					return( AI_ACTION_CLIMB_ROOF );
				}				
			}
			else
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("cannot climb here - check if we can move to climb"));

				// check if we can move to climb
				INT32 sClimbSpot;
				sClimbSpot = FindClosestClimbPoint(pSoldier, TRUE );

				if ( !TileIsOutOfBounds(sClimbSpot) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("found climb spot nearby %d", sClimbSpot));

					INT8 bReserveAP = GetAPsToClimbRoof( pSoldier, FALSE ) + APBPConstants[MAX_AP_CARRIED];
					pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, sClimbSpot, bReserveAP, AI_ACTION_MOVE_TO_CLIMB, 0 );

					// check that we can move and climb on the same turn
					if( !TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
						pSoldier->aiData.usActionData == sClimbSpot )
					{
						return( AI_ACTION_MOVE_TO_CLIMB  );
					}			
				}
			}	
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Black smoke to cover position]"));
	// try to use smoke to cover position from attackers
	if (gfTurnBasedAI &&
		SoldierAI(pSoldier) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		AllowComplexAI(pSoldier) &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!pSoldier->RushAttackAdvance() &&
		pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
		!AICheckSuccessfulAttack(pSoldier, TRUE) &&
		CountThrowableGrenades(pSoldier, EXPLOSV_SMOKE) > 1 &&
		(ubBestAttackAction == AI_ACTION_NONE || ubBestAttackAction == AI_ACTION_FIRE_GUN && Random(25) > (UINT8)BestAttack.ubChanceToReallyHit) &&
		(pSoldier->aiData.bUnderFire ||
		pSoldier->aiData.bShock > 0 ||
		fSectorAttack ||
		CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE) > 0) &&
		(Chance(SoldierDifficultyLevel(pSoldier) * 10) ||
			Chance(TeamPercentKilled(pSoldier->bTeam) / 2) ||
			Chance(10 * CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE))))
	{
		CheckTossCoverSmoke(pSoldier, &BestThrow);

		if (BestThrow.ubPossible && Chance(BestThrow.iAttackValue))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

			// if necessary, swap the usItem from holster into the hand position
			if (BestThrow.bWeaponIn != HANDPOS)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
				RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
			}

			// stand up before throwing if needed
			if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
				pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
			{
				pSoldier->aiData.usActionData = BestThrow.ubStance;
				pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
				pSoldier->aiData.usNextActionData = BestThrow.sTarget;
				pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
				pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				return AI_ACTION_CHANGE_STANCE;
			}
			else
			{
				pSoldier->aiData.usActionData = BestThrow.sTarget;
				pSoldier->bTargetLevel = BestThrow.bTargetLevel;
				pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("throw smoke grenade to cover position at spot %d level %d", BestThrow.sTarget, BestThrow.bTargetLevel));

			//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] throw smoke to cover position %d value %d", pSoldier->ubID, BestThrow.sTarget, BestThrow.iAttackValue);
			//BeginMultiPurposeLocator(BestThrow.sTarget, BestThrow.bTargetLevel, FALSE);

			return(AI_ACTION_TOSS_PROJECTILE);
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Grenade for special purpose]"));
	// try to use regular grenade for special purpose
	if (SoldierAI(pSoldier) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		gfTurnBasedAI &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
		(ubBestAttackAction == AI_ACTION_NONE || ubBestAttackAction == AI_ACTION_FIRE_GUN && Random(25) >(UINT8)BestAttack.ubChanceToReallyHit) &&
		(pSoldier->RushAttackPrepare() ||
		pSoldier->aiData.bOrders == SEEKENEMY && TileIsOutOfBounds(sClosestDisturbance) && AllowComplexAI(pSoldier) ||
		Chance(10 * SoldierDifficultyLevel(pSoldier) + 10 * (CountThrowableGrenades(pSoldier, EXPLOSV_NORMAL, 10)))))
	{
		CheckTossGrenadeSpecial(pSoldier, &BestThrow);

		if (BestThrow.ubPossible && (pSoldier->RushAttackPrepare() || Chance(BestThrow.iAttackValue)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

			// if necessary, swap the usItem from holster into the hand position
			if (BestThrow.bWeaponIn != HANDPOS)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
				RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
			}

			INT16 sTooCloseDistance = DAY_VISION_RANGE / 4;
			if (PythSpacesAway(pSoldier->sGridNo, BestThrow.sTarget) < sTooCloseDistance ||
				CountNearbyFriends(pSoldier, BestThrow.sTarget, sTooCloseDistance) > 0 ||
				CountNearbyNeutrals(pSoldier, BestThrow.sTarget, sTooCloseDistance) > 0)
			{
				// too close to soldier or any friend, set grenade as delayed
				if (Explosive[Item[pSoldier->inv[HANDPOS].usItem].ubClassIndex].ubType == EXPLOSV_NORMAL)
				{
					pSoldier->inv[HANDPOS][0]->data.sObjectFlag |= DELAYED_GRENADE_EXPLOSION;
				}
			}

			// stand up before throwing if needed
			if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
				pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
			{
				pSoldier->aiData.usActionData = BestThrow.ubStance;
				pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
				pSoldier->aiData.usNextActionData = BestThrow.sTarget;
				pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
				pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
				return AI_ACTION_CHANGE_STANCE;
			}
			else
			{
				pSoldier->aiData.usActionData = BestThrow.sTarget;
				pSoldier->bTargetLevel = BestThrow.bTargetLevel;
				pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("throw grenade at spot %d level %d", BestThrow.sTarget, BestThrow.bTargetLevel));

			//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] throw grenade at %d", pSoldier->ubID, BestThrow.sTarget);
			//BeginMultiPurposeLocator(BestThrow.sTarget, BestThrow.bTargetLevel, FALSE);

			return(AI_ACTION_TOSS_PROJECTILE);
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Black cover advance]"));
	//INT16 sGunRange = AIGunRange(pSoldier);
	// Black cover advance
	if( SoldierAI(pSoldier) &&
		gfTurnBasedAI &&
		!pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->bInitialActionPoints > APBPConstants[AP_MINIMUM] &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!fStopMovement &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		!TileIsOutOfBounds(sClosestDisturbance) &&
		//!InARoom(pSoldier->sGridNo, NULL) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->aiData.bOrders != ONGUARD &&
		!AISpecialRole(pSoldier) &&
		(ubBestAttackAction == AI_ACTION_NONE || ubBestAttackAction == AI_ACTION_FIRE_GUN && BestAttack.ubChanceToReallyHit < 5 * RangeChangeDesire(pSoldier) ) &&
		AIGunRange(pSoldier) < DAY_VISION_RANGE &&
		pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
		(!fAnyCover || 
		!fProneSightCover || 
		AIGunRange(pSoldier) < PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) || 
		ubBestAttackAction == AI_ACTION_FIRE_GUN && BestAttack.ubOpponent != NOBODY && (MercPtrs[BestAttack.ubOpponent]->IsUnconscious() || MercPtrs[BestAttack.ubOpponent]->IsCowering() || MercPtrs[BestAttack.ubOpponent]->stats.bLife < OKLIFE)) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < MAX_VISION_RANGE &&
		DetermineMovementMode(pSoldier, AI_ACTION_GET_CLOSER) != CRAWLING &&
		pSoldier->aiData.bShock < RangeChangeDesire(pSoldier) * 2 &&
		(	fSectorAttack ||
			AIGunRange(pSoldier) < PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) ||
			pSoldier->RushAttackAdvance() ||
			pSoldier->LastAttackHit() || 
			pSoldier->LastTargetSuppressed() ||
			AICheckSuccessfulAttack(pSoldier, TRUE) ||
			AICheckWeOutnumber(pSoldier, sClosestOpponent) ||
			ubBestAttackAction == AI_ACTION_NONE ||
			ubBestAttackAction == AI_ACTION_FIRE_GUN && BestAttack.ubChanceToReallyHit == 1 || 
			ubBestAttackAction == AI_ACTION_FIRE_GUN && BestAttack.ubOpponent != NOBODY && (MercPtrs[BestAttack.ubOpponent]->IsUnconscious() || MercPtrs[BestAttack.ubOpponent]->IsCowering() || MercPtrs[BestAttack.ubOpponent]->stats.bLife < OKLIFE) ||
			!fAnyCover)
		)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("find cover advance spot"));
		DebugAI( AI_MSG_INFO, pSoldier, String("closest reachable disturbance %d", sClosestDisturbance));

		INT32 sAdvanceSpot = NOWHERE;

		DebugAI(AI_MSG_INFO, pSoldier, String("search for any cover advance spot"));
		sAdvanceSpot = FindAdvanceSpot(pSoldier, sClosestDisturbance, AI_ACTION_GET_CLOSER, ADVANCE_SPOT_ANY_COVER, FALSE);

		if (!TileIsOutOfBounds(sAdvanceSpot))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("found cover advance spot %d", sAdvanceSpot));

			// check that we can reach desired location
			pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, sAdvanceSpot, 0, AI_ACTION_GET_CLOSER, 0);
			if (pSoldier->aiData.usActionData == sAdvanceSpot)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("cover advance spot ok"));
				pSoldier->aiData.usActionData = sAdvanceSpot;

				//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] found cover advance spot %d", pSoldier->ubID, sAdvanceSpot);
				//BeginMultiPurposeLocator(sAdvanceSpot, pSoldier->pathing.bLevel, FALSE);

				return AI_ACTION_GET_CLOSER;
			}
			else
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("cannot reach cover advance spot!"));
			}
		}
		else
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("cannot find cover advance spot"));

			// try to use smoke to cover advance movement
			//gubNPCAPBudget = pSoldier->bActionPoints;
			gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;

			// check path to closest disturbance
			if (gfTurnBasedAI &&
				pSoldier->bActionPoints >= APBPConstants[AP_MINIMUM] &&
				pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
				!TileIsOutOfBounds(sClosestDisturbance) &&
				RangeChangeDesire(pSoldier) > 3 &&
				!AIRoleSniper(pSoldier) &&
				!AIRoleMachinegunner(pSoldier) &&
				!pSoldier->RetreatActive() &&
				pSoldier->aiData.bOrders != STATIONARY &&
				(!AICheckSuccessfulAttack(pSoldier, TRUE) || pSoldier->RushAttackAdvance()) &&
				(pSoldier->aiData.bUnderFire ||
				pSoldier->aiData.bShock > 0 ||
				pSoldier->stats.bLife < pSoldier->stats.bLifeMax * 3 / 4 ||
				pSoldier->RushAttackAdvance() ||
				CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2) > CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2) / 2 ||
				CountSeenEnemiesLastTurn(pSoldier) > CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2) ||
				CountCorpses(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2, TRUE) > CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2)) &&
				(pSoldier->RushAttackAdvance() || Chance(SoldierDifficultyLevel(pSoldier) * 10) || Chance(TeamPercentKilled(pSoldier->bTeam)) || Chance(CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2))) &&
				FindBestPath(pSoldier, sClosestDisturbance, pSoldier->pathing.bLevel, RUNNING, COPYROUTE, 0))
			{
				INT16 sLoop;
				INT32 sCoverSpot = NOWHERE;

				DebugAI(AI_MSG_INFO, pSoldier, String("found path to %d, path size %d ", sClosestDisturbance, pSoldier->pathing.usPathDataSize));

				sCheckGridNo = pSoldier->sGridNo;

				for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
				{
					sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

					if (!TileIsOutOfBounds(sCheckGridNo) &&
						PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) < TACTICAL_RANGE / 2 &&
						PythSpacesAway(pSoldier->sGridNo, sCheckGridNo) > TACTICAL_RANGE / 4 &&
						!Water(sCheckGridNo, pSoldier->pathing.bLevel) &&
						!InSmokeNearby(sCheckGridNo, pSoldier->pathing.bLevel) &&
						!InSmoke(sCheckGridNo, pSoldier->pathing.bLevel, TRUE) &&
						(CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel) ||
						InLightAtNight(sCheckGridNo, pSoldier->pathing.bLevel)) &&
						EnemyCanAttackSpot(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel))
					{
						CheckTossGrenadeAt(pSoldier, &BestThrow, sCheckGridNo, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);

						if (BestThrow.ubPossible)
						{
							sCoverSpot = sCheckGridNo;
						}
					}
				}

				if (!TileIsOutOfBounds(sCoverSpot))
				{
					CheckTossGrenadeAt(pSoldier, &BestThrow, sCoverSpot, pSoldier->pathing.bLevel, EXPLOSV_SMOKE);

					if (BestThrow.ubPossible)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

						// if necessary, swap the usItem from holster into the hand position
						if (BestThrow.bWeaponIn != HANDPOS)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("rearrange pocket"));
							RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
						}

						// stand up before throwing if needed
						if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
							pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
						{
							pSoldier->aiData.usActionData = BestThrow.ubStance;
							pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
							pSoldier->aiData.usNextActionData = BestThrow.sTarget;
							pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
							pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
							return AI_ACTION_CHANGE_STANCE;
						}

						pSoldier->aiData.usActionData = BestThrow.sTarget;
						pSoldier->bTargetLevel = BestThrow.bTargetLevel;
						pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

						return(AI_ACTION_TOSS_PROJECTILE);
					}
				}
			}
			gubNPCAPBudget = 0;
		}
	}

	// Black change position
	/*if(	ubCanMove &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->bActionPoints > APBPConstants[AP_MINIMUM] &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!pSoldier->IsBoxer() &&
		pSoldier->aiData.bOrders > ONGUARD &&
		pSoldier->aiData.bOrders != SNIPER &&
		ubBestAttackAction == AI_ACTION_FIRE_GUN && 
		RangeChangeDesire( pSoldier ) <= 4 &&
		DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER) == RUNNING &&
		!TileIsOutOfBounds( sClosestOpponent ) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) >= DAY_VISION_RANGE / 4 &&
		Random(100) > BestAttack.ubChanceToReallyHit &&
		!AICheckShortWeaponRange(pSoldier) &&
		(	AIGunRange(pSoldier) * 3 / 2 < PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) ||
			Random(100) > 100 / (1 + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4)) ||
			!fAnyCover ||
			!CanSeeProneEnemyAtSpot(pSoldier, pSoldier->sGridNo) )
		)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("Change position"));

		// check if we really can find better cover
		iCoverPercentBetter = 0;
		sBestCover = FindBestNearbyCover(pSoldier, &iCoverPercentBetter, 0, NOBODY, FALSE, TRUE);

		if( !TileIsOutOfBounds(sBestCover) && 
			(iCoverPercentBetter > BestAttack.ubChanceToReallyHit || !fAnyCover || CanSeeProneEnemyAtSpot(pSoldier, sBestCover)) )
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("Found change position spot: %d", sBestCover));
			ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] changes position to %d percent better %d", pSoldier->ubID, sBestCover, iCoverPercentBetter);
			BeginMultiPurposeLocator(sBestCover, pSoldier->pathing.bLevel, FALSE);

			pSoldier->aiData.usActionData = sBestCover;
			return AI_ACTION_TAKE_COVER;
		}
	}*/

	BOOLEAN fOvercrowded = FALSE;
	if (ubCanMove &&
		gfTurnBasedAI &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->bActionPoints > APBPConstants[AP_MINIMUM] &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		SoldierAI(pSoldier) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		!AIRoleSniper(pSoldier) &&
		!AIRoleMachinegunner(pSoldier) &&
		!AIRoleMortarOperator(pSoldier) &&
		!AIRoleCommander(pSoldier) &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4) > 2 &&
		ubBestAttackAction == AI_ACTION_FIRE_GUN &&
		PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) > DAY_VISION_RANGE / 2 &&
		Chance(10 * (CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4) - 2)) &&
		!fAnyCover &&
		!AnyCoverNearSpot(pSoldier, pSoldier->sGridNo) &&
		!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE) &&
		!SightCoverNearSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE))
	{
		fOvercrowded = TRUE;
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Black advance]"));	
	// Black advance
	if(	ubCanMove &&
		gfTurnBasedAI &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		pSoldier->bActionPoints > APBPConstants[AP_MINIMUM] &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!pSoldier->IsBoxer() &&
		pSoldier->aiData.bOrders > ONGUARD &&
		!AIRoleSniper(pSoldier) &&
		!AIRoleMachinegunner(pSoldier) &&
		ubBestAttackAction == AI_ACTION_FIRE_GUN && 
		RangeChangeDesire(pSoldier) >= 4 &&
		pSoldier->aiData.bAIMorale > MORALE_CONFIDENT &&
		(!fAnyCover || !fProneSightCover || pSoldier->RushAttackAdvance()) &&
		(gAnimControl[ pSoldier->usAnimState ].ubEndHeight >= ANIM_CROUCH || !pSoldier->aiData.bUnderFire) &&
		20 + MercPtrs[BestAttack.ubOpponent]->aiData.bShock > BestAttack.ubChanceToReallyHit &&
		(	fOvercrowded ||
			pSoldier->RushAttackAdvance() ||
			pSoldier->LastAttackHit() ||
			BestAttack.ubChanceToReallyHit == 1 ||
			MercPtrs[BestAttack.ubOpponent]->IsCowering() ||
			MercPtrs[BestAttack.ubOpponent]->bBreathCollapsed ||
			MercPtrs[BestAttack.ubOpponent]->bCollapsed ||
			MercPtrs[BestAttack.ubOpponent]->stats.bLife < OKLIFE ||
			RangeChangeDesire( pSoldier ) >= 4 &&
			(	BestAttack.ubChanceToReallyHit == 1 ||
				PythSpacesAway( pSoldier->sGridNo, BestAttack.sTarget ) > usRange / CELL_X_SIZE ||
				CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4) > CountSeenEnemiesLastTurn(pSoldier) ) )
		)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("Allow cover check"));
		fAllowCoverCheck = TRUE;

		/*if ( PreRandom( MercPtrs[BestAttack.ubOpponent]->aiData.bShock + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4) ) > BestAttack.ubChanceToReallyHit )
		{
			// check if we really can find better cover
			iCoverPercentBetter = 0;
			sBestCover = FindBestNearbyCover(pSoldier, &iCoverPercentBetter);

			if(!TileIsOutOfBounds(sBestCover))
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("Screw the attack!"));
				// screw the attack!
				ubBestAttackAction = AI_ACTION_NONE;
			}			
		}*/
	}

	// sevenfm: try to turn to closest unseen opponent
	INT32 sClosestUnseenOpponent = ClosestUnseenOpponent(pSoldier, NULL, NULL);

	if( ubBestAttackAction == AI_ACTION_FIRE_GUN && 
		gfTurnBasedAI &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&			
		pSoldier->bActionPoints > APBPConstants[MAX_AP_CARRIED] &&
		!TileIsOutOfBounds(sClosestUnseenOpponent) &&
		Random(30) > (UINT8)BestAttack.ubChanceToReallyHit	)
	{
		ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestUnseenOpponent);

		// if soldier is not already facing in that direction,
		// and the opponent is close enough that he could possibly be seen
		if( pSoldier->ubDirection != ubOpponentDir &&
			pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
			pSoldier->bActionPoints >= GetAPsToLook(pSoldier)  )
		{
			// turn
			pSoldier->aiData.usActionData = ubOpponentDir;

			DebugAI( AI_MSG_INFO, pSoldier, String("turn to closest unseen opponent"));
			return(AI_ACTION_CHANGE_FACING);
		}
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Cover]"));
	////////////////////////////////////////////////////////////////////////////
	// LOOK FOR SOME KIND OF COVER BETTER THAN WHAT WE HAVE NOW
	////////////////////////////////////////////////////////////////////////////

	// if soldier has enough APs left to move at least 1 square's worth,
	// and either he can't attack any more, or his attack did wound someone
	iCoverPercentBetter = 0;

	DebugAI(AI_MSG_INFO, pSoldier, String("ubCanMove %d SkipCoverCheck %d gfHiddenInterrupt %d fInterruptOccurred %d fStopMovement %d ubBestAttackAction %d", ubCanMove, pSoldier->SkipCoverCheck(), gfHiddenInterrupt, gTacticalStatus.fInterruptOccurred, fStopMovement, ubBestAttackAction));

	// allow cover check if:
	// no cover check this turn if soldier has no cover at spot
	// cannot attack or already attacked this turn or allowed to advance
	if( ubCanMove &&
		!fStopMovement &&										// not being bandaged or something
		(!pSoldier->SkipCoverCheck() || !fSafeSpot) &&			// allow taking cover at the start of the turn or after attack
		!pSoldier->IsBoxer() &&
		(!gfHiddenInterrupt || ubBestAttackAction == AI_ACTION_NONE) &&																		// not in hidden interrupt if can attack
		(!gTacticalStatus.fInterruptOccurred || ubBestAttackAction == AI_ACTION_NONE || BestAttack.ubChanceToReallyHit == 1) &&				// not in interrupt if can attack
			(pSoldier->CheckInitialAP() && (pSoldier->aiData.bUnderFire || Chance(SoldierDifficultyLevel(pSoldier) * 10) || !fAnyCover)	||	// no cover or under attack at the start of turn
			ubBestAttackAction == AI_ACTION_NONE ||																							// cannot attack
			fAllowCoverCheck ||																												// black advance
			pSoldier->LastAttackHit() ||																									// last attack was successful
			(pSoldier->usSoldierFlagMask2 & SOLDIER_ATTACKED_THIS_TURN) && (!fAnyCover || !fProneSightCover) ||								// already attacked this turn, try to hide
			fOvercrowded))
	{
		sBestCover = FindBestNearbyCover(pSoldier, &iCoverPercentBetter);
		DebugAI( AI_MSG_INFO, pSoldier, String("Found cover spot %d percent better %d movement mode %d", sBestCover, iCoverPercentBetter, DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER)));
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Decide between attacking and moving]"));
	//////////////////////////////////////////////////////////////////////////
	// IF NECESSARY, DECIDE BETWEEN ATTACKING AND MOVING (TAKING COVER)
	//////////////////////////////////////////////////////////////////////////

	// if both are possible	
	if( ubBestAttackAction != AI_ACTION_NONE && !TileIsOutOfBounds(sBestCover) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("Both attack and move are possible. CTH %d CoverPercentBetter %d", BestAttack.ubChanceToReallyHit, iCoverPercentBetter));

		// gotta compare their merits and select the more desirable option
		iAttack = BestAttack.ubChanceToReallyHit;
		iMove = iCoverPercentBetter;

		INT32 iMoveBonus = 0;
		INT32 iAttackBonus = 0;

		// sevenfm: take into account possible suppression (returns no possible suppression if target is cowering already)
		if( ubBestAttackAction == AI_ACTION_FIRE_GUN &&
			fAnyCover &&
			BestAttack.ubOpponent != NOBODY )
		{
			iAttack = max(iAttack, AIEstimateSuppression(pSoldier, MercPtrs[BestAttack.ubOpponent], pSoldier->bActionPoints));
		}

		DebugAI( AI_MSG_INFO, pSoldier, String("Attack %d Move %d", iAttack, iMove));

		switch (pSoldier->aiData.bAttitude)
		{
		case DEFENSIVE:		iMoveBonus += 20;	break;
		case BRAVESOLO:		iMoveBonus += 30;	break;
		case BRAVEAID:		iMoveBonus += 30;	break;
		case CUNNINGSOLO:	iMoveBonus += 10;	break;
		case CUNNINGAID:	iMoveBonus += 10;	break;
		case AGGRESSIVE:	iAttackBonus += 30;	break;
		case ATTACKSLAYONLY:iAttackBonus += 30;	break;
		}

		switch (pSoldier->aiData.bOrders)
		{
		case STATIONARY:    iMoveBonus -= 30;	break;
		case ONGUARD:       iMoveBonus -= 20;	break;
		case ONCALL:        iMoveBonus += 10;	break;
		case CLOSEPATROL:	iMoveBonus += 20;	break;
		case RNDPTPATROL:	iMoveBonus += 10;	break;
		case POINTPATROL:   iMoveBonus += 10;	break;
		case FARPATROL:     iMoveBonus += 20;	break;
		case SEEKENEMY:     iMoveBonus += 30;	break;
		}

		switch (pSoldier->aiData.bAIMorale)
		{
		case MORALE_FEARLESS:	iMoveBonus += 30;	break;
		case MORALE_CONFIDENT:	iMoveBonus += 20;	break;
		case MORALE_NORMAL:		iMoveBonus += 10;	break;
		case MORALE_WORRIED:	iMoveBonus += 30;	break;
		case MORALE_HOPELESS:	iMoveBonus += 50;	break;
		}

		if (RangeChangeDesire(pSoldier) >= 4 && (pSoldier->RushAttackAdvance() || fOvercrowded))
		{
			iMoveBonus += 30;
		}

		iAttack += iAttack * iAttackBonus / 100;
		iMove += iMove * iMoveBonus / 100;

		DebugAI(AI_MSG_INFO, pSoldier, String("Attack bonus %d Move bonus %d", iAttack, iMove));
		DebugAI( AI_MSG_INFO, pSoldier, String("Attack %d Move %d", iAttack, iMove));
		//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"[%d] Attack %d Move %d", pSoldier->ubID, iAttack, iMove);

		DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"DecideActionBlack: if his defensive instincts win out, forget all about the attack");
		// if his defensive instincts win out, forget all about the attack
		if (iMove > iAttack)
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("Decided to move"));
			//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] Decided to move", pSoldier->ubID);
			ubBestAttackAction = AI_ACTION_NONE;
		}
		else
		{
			// sevenfm: still allow taking cover in some situations
			if (pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
				(AnyCoverAtSpot(pSoldier, sBestCover, FALSE) && !AnyCoverAtSpot(pSoldier, pSoldier->sGridNo, FALSE) ||
				SightCoverAtSpot(pSoldier, sBestCover, ANIM_PRONE, FALSE) && !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE)))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("Allow taking cover"));
				//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] Allow taking cover", pSoldier->ubID);
			}
			else
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("Decided to attack"));
				//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] Decided to attack", pSoldier->ubID);
				sBestCover = NOWHERE;
			}			
		}
	}

	// BLACK Watch

	// if we see less opponents currently than total opponents seen this/last turn
	// and our total enemy threat at this spot is low and soldier is prone or crouched and there are friends nearby
	// try to watch to allow covering fire	
	// also check that we have cover at spot and not decided to take cover

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Black watch]"));

	if(	gfTurnBasedAI &&
		!pSoldier->RushAttackAdvance() &&
		!fOvercrowded &&
		!pSoldier->aiData.bUnderFire &&
		fAnyCover &&
		pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
		ubBestAttackAction == AI_ACTION_FIRE_GUN &&
		MercPtrs[BestAttack.ubOpponent] &&
		pSoldier->bActionPoints > BestAttack.ubAPCost + APBPConstants[MAX_AP_CARRIED] &&
		(BestAttack.ubChanceToReallyHit == 1 || MercPtrs[BestAttack.ubOpponent]->IsCowering() || MercPtrs[BestAttack.ubOpponent]->stats.bLife < OKLIFE) &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!gGameOptions.fImprovedInterruptSystem &&
		PythSpacesAway( pSoldier->sGridNo, BestAttack.sTarget ) > DAY_VISION_RANGE / 2 &&
		gAnimControl[ pSoldier->usAnimState ].ubEndHeight <= ANIM_CROUCH &&
		pSoldier->aiData.bOppCnt < CountSeenEnemiesLastTurn(pSoldier) &&
		// check that we are already looking at the right direction
		pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, BestAttack.sTarget) &&
		CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4) > CountSeenEnemiesLastTurn(pSoldier) &&
		Chance(100 / (1 + CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE/4))) )
	{
		// turn to face opponent
		/*ubOpponentDir = AIDirection(pSoldier->sGridNo, BestAttack.sTarget);
		if( pSoldier->ubDirection != ubOpponentDir &&
			pSoldier->InternalIsValidStance( ubOpponentDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) &&
			pSoldier->bActionPoints >= GetAPsToLook(pSoldier)  )
		{
			pSoldier->aiData.usActionData = ubOpponentDir;
			return(AI_ACTION_CHANGE_FACING);
		}*/

		// raise gun
		if( PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) != INVALID_ANIMATION &&
			!WeaponReady(pSoldier) &&
			(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
			pSoldier->bActionPoints >= GetAPsToReadyWeapon( pSoldier, PickSoldierReadyAnimation( pSoldier, FALSE, FALSE ) ) )
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("black watch (raise gun)"));
			pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
			return AI_ACTION_RAISE_GUN;
		}

		DebugAI( AI_MSG_INFO, pSoldier, String("black watch"));
		//ScreenMsg(FONT_BLUE, MSG_INTERFACE, L"[%d] black watch", pSoldier->ubID);
		return( AI_ACTION_END_TURN );
	}

	DebugAI( AI_MSG_INFO, pSoldier, String("Best attack %s [%d]", szAction[ubBestAttackAction], ubBestAttackAction));

	// if attack is still desirable (meaning it's also preferred to taking cover)
	if (ubBestAttackAction != AI_ACTION_NONE)
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Prepare attack]"));
		// if we wanted to be REALLY mean, we could look at chance to hit and decide whether
		// to shoot at the head...

		// default settings
		pSoldier->aiData.bAimTime	= BestAttack.ubAimTime;		
		pSoldier->bScopeMode		= BestAttack.bScopeMode;
		pSoldier->bDoBurst			= 0;

		INT16 sAimTimeNoBurst = pSoldier->aiData.bAimTime;

		// HEADROCK HAM 3.6: bAimTime represents how MANY aiming levels are used, not how much APs they cost necessarily.
		INT16 sActualAimAP = CalcAPCostForAiming( pSoldier, BestAttack.sTarget, (INT8)pSoldier->aiData.bAimTime );

		if (ubBestAttackAction == AI_ACTION_FIRE_GUN)
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Prepare shooting]"));

			BOOLEAN fAllowAutofire = TRUE;
			INT16 sReserveAP = 0;

			// sevenfm: check that we have a clip to reload		
			if (FindAmmoToReload(pSoldier, BestAttack.bWeaponIn, NO_SLOT) != NO_SLOT)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("Has extra clip to reload"));
				fExtraClip = TRUE;
			}

			// sevenfm: reserve APs when in dangerous place and not shooting prone (to go prone or take cover after shot)			
			if (BestAttack.ubStance != ANIM_PRONE &&
				!pSoldier->RushAttackAdvance() &&
				!MercPtrs[BestAttack.ubOpponent]->IsCowering() &&
				Chance(SoldierDifficultyLevel(pSoldier) * 25) &&
				PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) > TACTICAL_RANGE / 2)
				//pSoldier->bActionPoints >= BestAttack.ubAPCost + GetAPsProne(pSoldier, TRUE) )
			{
				sReserveAP = APBPConstants[AP_PRONE] + APBPConstants[AP_LOOK_CROUCHED];
				DebugAI(AI_MSG_INFO, pSoldier, String("Reserve %d AP to go prone", sReserveAP));
				//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"[%d] reserve AP %d to go prone", pSoldier->ubID, ubReserveAP );
			}

			// sevenfm: reserve APs to hide after shot when in dangerous place
			if (!pSoldier->RushAttackAdvance() &&
				!MercPtrs[BestAttack.ubOpponent]->IsCowering() &&
				!MercPtrs[BestAttack.ubOpponent]->bCollapsed &&
				!MercPtrs[BestAttack.ubOpponent]->bBreathCollapsed &&
				MercPtrs[BestAttack.ubOpponent]->stats.bLife >= OKLIFE &&
				//pSoldier->bActionPoints >= BestAttack.ubAPCost + APBPConstants[MAX_AP_CARRIED] &&
				Chance(SoldierDifficultyLevel(pSoldier) * 25) &&
				//Chance((100 - BestAttack.ubChanceToReallyHit) * min(100, CELL_X_SIZE * PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget)) / 100) &&
				(!fAnyCover && AnyCoverNearSpot(pSoldier, pSoldier->sGridNo) && PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) > TACTICAL_RANGE / 2 ||
				!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) && SightCoverNearSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) && PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) > TACTICAL_RANGE / 4))
			{
				sReserveAP = APBPConstants[AP_MINIMUM] / 2;
				DebugAI(AI_MSG_INFO, pSoldier, String("Reserve %d AP to hide after shot", sReserveAP));
			}

			// sevenfm: try to reserve AP by lowering aim time if needed
			while (pSoldier->aiData.bAimTime > 0 && pSoldier->bActionPoints < BestAttack.ubAPCost + sActualAimAP + sReserveAP)
			{
				pSoldier->aiData.bAimTime--;
				sActualAimAP = CalcAPCostForAiming(pSoldier, BestAttack.sTarget, (INT8)pSoldier->aiData.bAimTime);
				DebugAI(AI_MSG_INFO, pSoldier, String("Lower aim to %d to allow reserving AP, new Aim AP cost=%d", pSoldier->aiData.bAimTime, sActualAimAP));
			}

			// sevenfm: check to allow autofire depending on number of bullets left
			if (!fExtraClip &&
				!Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].NoSemiAuto)
			{
				// find remaining ammo
				UINT16 ubAmmo = pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft;

				// if more than 20 bullets left - any CTH
				// 10 bullets left - 50% CTH
				// 2 bullets left - 90% CTH
				if (100 - ubAmmo * 5 > BestAttack.ubChanceToReallyHit)
				{
					fAllowAutofire = FALSE;
					DebugAI(AI_MSG_INFO, pSoldier, String("Skip autofire, remaining ammo %d", ubAmmo));

					// allow 30% chance for aggressive soldiers to autofire
					if (pSoldier->aiData.bAttitude == AGGRESSIVE &&
						PreRandom(100) < 30)
					{
						fAllowAutofire = TRUE;
						DebugAI(AI_MSG_INFO, pSoldier, String("Allow autofire for aggressive"));
					}
				}
			}

			// sevenfm: possibly skip autofire completely if not enough AP to reserve for crouching/cover
			if (BestAttack.ubStance != ANIM_PRONE &&
				!pSoldier->RushAttackAdvance() &&
				pSoldier->bActionPoints <= BestAttack.ubAPCost + sActualAimAP + GetAPsProne(pSoldier, TRUE) &&
				PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) > TACTICAL_RANGE / 4 &&
				BestAttack.ubChanceToReallyHit < 10 * SoldierDifficultyLevel(pSoldier) &&
				!Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].NoSemiAuto)
			{
				fAllowAutofire = FALSE;
				DebugAI(AI_MSG_INFO, pSoldier, String("Not enough AP to go prone after shot - skip autofire"));
			}

			// skip autofire if have big chance of friendly fire
			if (Random(100) < BestShot.ubFriendlyFireChance &&
				//!CheckSuppressionDirection(pSoldier, BestAttack.sTarget, BestAttack.bTargetLevel) &&
				!Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].NoSemiAuto)
			{
				fAllowAutofire = FALSE;
				DebugAI(AI_MSG_INFO, pSoldier, String("Skip autofire, chance of friendly fire %d", BestShot.ubFriendlyFireChance));
			}

			//////////////////////////////////////////////////////////////////////////
			// IF ENOUGH APs TO BURST, RANDOM CHANCE OF DOING SO
			//////////////////////////////////////////////////////////////////////////

			if (IsGunBurstCapable( &pSoldier->inv[BestAttack.bWeaponIn], FALSE, pSoldier ) &&
				fAllowAutofire &&	// sevenfm: limit autofire when low on ammo
				!(Menptr[BestShot.ubOpponent].stats.bLife < OKLIFE) && // don't burst at downed targets
				pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft > 1 )
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("enough APs to burst, random chance of doing so"));

				ubBurstAPs = CalcAPsToBurst( pSoldier->CalcActionPoints(), &(pSoldier->inv[BestAttack.bWeaponIn]), pSoldier );

				// HEADROCK HAM 3.6: Use Actual Aiming Time.
				// sevenfm: reserve APs when in dangerous place
				if (pSoldier->bActionPoints >= BestAttack.ubAPCost + sActualAimAP + ubBurstAPs + sReserveAP)
				{
					// Base chance of bursting is 25% if best shot was +0 aim, down to 8% at +4
					iChance = (25 / max((BestAttack.ubAimTime + 1), 1));
					switch (pSoldier->aiData.bAttitude)
					{
					case DEFENSIVE:		iChance += -5; break;
					case BRAVESOLO:		iChance += 5; break;
					case BRAVEAID:		iChance += 5; break;
					case CUNNINGSOLO:	iChance += 0; break;
					case CUNNINGAID:	iChance += 0; break;
					case AGGRESSIVE:	iChance += 10; break;
					case ATTACKSLAYONLY:iChance += 30; break;
					}

					// SANDRO: more likely to burst when firing from hip
					if (BestAttack.bScopeMode == USE_ALT_WEAPON_HOLD && Item[pSoldier->inv[BestAttack.bWeaponIn].usItem].twohanded)
						iChance += 40;

					// CHRISL: Changed from a simple flag to two externalized values for more modder control over AI suppression
					if (GetMagSize(&pSoldier->inv[BestAttack.bWeaponIn], 0) >= gGameExternalOptions.ubAISuppressionMinimumMagSize &&
						pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= gGameExternalOptions.ubAISuppressionMinimumAmmo)
						iChance += 20;

					// increase chance based on proximity and difficulty of enemy
					if (PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) < 15)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("check chance to burst"));
						iChance += (15 - PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget)) * (1 + SoldierDifficultyLevel(pSoldier));
						if (pSoldier->aiData.bAttitude == ATTACKSLAYONLY)
						{
							// increase it more!
							iChance += 5 * (15 - PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget));
						}
					}

					if (pSoldier->RushAttackAdvance())
					{
						iChance *= 20;
					}

					// HEADROCK HAM 3.6: due to the "else", this part of the formula is NEVER hit. Removing.
					//else if (PythSpacesAway( pSoldier->sGridNo, BestAttack.sTarget ) < 10 && gGameOptions.ubDifficultyLevel > DIF_LEVEL_EASY )
					if (PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) < 10 && gGameOptions.ubDifficultyLevel > DIF_LEVEL_EASY)
					{
						iChance += 100;
					}

					if ( (INT32) PreRandom( 100 ) < iChance)
					{
						BestAttack.ubAPCost += ubBurstAPs + sActualAimAP;//dnl ch58 130913
						// check for spread burst possibilities
						/*if (pSoldier->aiData.bAttitude != ATTACKSLAYONLY)
						{
							CalcSpreadBurst( pSoldier, BestAttack.sTarget, BestAttack.bTargetLevel );
						}*/
						
						pSoldier->bDoBurst = 1;
						pSoldier->bDoAutofire = 0;
					}
				}
			}

			if (IsGunAutofireCapable( &pSoldier->inv[BestAttack.bWeaponIn] ) &&
				fAllowAutofire &&	// sevenfm: limit autofire when low on ammo
				//!(Menptr[BestShot.ubOpponent].stats.bLife < OKLIFE) && // don't burst at downed targets
				(( pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft > 1 && !pSoldier->bDoBurst ) || Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].NoSemiAuto) )
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("enough APs to autofire, random chance of doing so"));
				
				BOOLEAN fAddBullets;
				do
				{
					fAddBullets = FALSE;
					FLOAT dTotalRecoil = 0.0f;
					pSoldier->bDoAutofire = 0;

					UINT8 ubMinAuto = 2;
					if (PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) > DAY_VISION_RANGE || AnyCoverAtSpot(pSoldier, pSoldier->sGridNo) || pSoldier->aiData.bUnderFire)
					{
						ubMinAuto++;
					}

					INT8 bAmmoSlot = FindAmmoToReload(pSoldier, BestShot.bWeaponIn, NO_SLOT);

					if (UsingNewCTHSystem())
					{
						do
						{
							pSoldier->bDoAutofire++;
							dTotalRecoil += AICalcRecoilForShot(pSoldier, &(pSoldier->inv[BestShot.bWeaponIn]), pSoldier->bDoAutofire);
							ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &(pSoldier->inv[BestShot.bWeaponIn]), pSoldier->bDoAutofire, pSoldier);
						} while (pSoldier->bActionPoints >= BestShot.ubAPCost + ubBurstAPs + sActualAimAP + sReserveAP &&
							pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= pSoldier->bDoAutofire &&
							(pSoldier->bDoAutofire < 3 || !gGameExternalOptions.fWeaponOverheating || GetGunOverheatDisplayPercentage(&pSoldier->inv[BestAttack.bWeaponIn]) < 1.0f) &&	// check overheated gun
							(dTotalRecoil <= 10.0f || 
							MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) ||
							pSoldier->RushAttackPrepare() ||
							fAnyCover && Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].ubWeaponType == GUN_LMG ||
							pSoldier->bDoAutofire < ubMinAuto));//dnl ch64 260813 pSoldier->ubAttackingHand is wrong because decision is to use BestAttack.bWeaponIn
					}
					else
					{
						do
						{
							pSoldier->bDoAutofire++;
							ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &(pSoldier->inv[BestAttack.bWeaponIn]), pSoldier->bDoAutofire, pSoldier);
						} while (pSoldier->bActionPoints >= BestAttack.ubAPCost + ubBurstAPs + sActualAimAP + sReserveAP &&
							pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= pSoldier->bDoAutofire &&
							(pSoldier->bDoAutofire < 3 || !gGameExternalOptions.fWeaponOverheating || GetGunOverheatDisplayPercentage(&pSoldier->inv[BestAttack.bWeaponIn]) < 1.0f) &&	// check overheated gun
							// sevenfm limit max auto penalty if target has shock (suppressed)
							(GetAutoPenalty(pSoldier, &pSoldier->inv[BestAttack.bWeaponIn], gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE) * pSoldier->bDoAutofire <= max(BestAttack.ubChanceToReallyHit, 40 + 80 / (2 + MercPtrs[BestAttack.ubOpponent]->aiData.bShock)) || 
							pSoldier->bDoAutofire < ubMinAuto ||
							MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) ||
							pSoldier->RushAttackPrepare() ||
							fAnyCover && Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].ubWeaponType == GUN_LMG ||
							fAnyCover && bAmmoSlot != NO_SLOT));
					}

					pSoldier->bDoAutofire--;

					DebugAI(AI_MSG_INFO, pSoldier, String("possible bDoAutofire %d bAimTime %d", pSoldier->bDoAutofire, pSoldier->aiData.bAimTime));

					sAimTimeNoBurst = pSoldier->aiData.bAimTime;
					//dnl ch69 130913 let try increase autofire rate for aim cost
					// sevenfm: prefer bullets over aim (for suppression) only when soldier is under fire or has AGGRESSIVE attitude
					if (//!UsingNewCTHSystem() &&
						pSoldier->bDoAutofire < 5 &&
						pSoldier->aiData.bAimTime > 0 &&
						fExtraClip &&						
						(!gGameExternalOptions.fWeaponOverheating || GetGunOverheatDisplayPercentage(&pSoldier->inv[BestAttack.bWeaponIn]) < 1.0f) &&	// check overheated gun
						!(gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SNIPER_NT)) &&
						BestAttack.ubFriendlyFireChance <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE &&
						BestAttack.ubChanceToReallyHit < 25 &&
						Chance(100 - 4 * BestAttack.ubChanceToReallyHit) &&
						BestAttack.ubOpponent != NOBODY &&
						Chance(100 - MercPtrs[BestAttack.ubOpponent]->ShockLevelPercent()) &&
						Chance(AISuppressionEffectiveness()) &&
						!MercPtrs[BestAttack.ubOpponent]->IsCowering() &&
						(pSoldier->aiData.bUnderFire || pSoldier->aiData.bAttitude == AGGRESSIVE || pSoldier->RushAttackAdvance() || MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO)) &&
						pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= 3 &&
						(CheckSuppressionDirection(pSoldier, BestAttack.sTarget, BestAttack.bTargetLevel) || pSoldier->RushAttackAdvance() || MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO)))
					{
						pSoldier->aiData.bAimTime--;
						sActualAimAP = CalcAPCostForAiming(pSoldier, BestAttack.sTarget, (INT8)pSoldier->aiData.bAimTime);
						fAddBullets = TRUE;
					}

					DebugAI(AI_MSG_INFO, pSoldier, String("bDoAutofire %d bAimTime %d sActualAimAP %d", pSoldier->bDoAutofire, pSoldier->aiData.bAimTime, sActualAimAP));
				} 
				while (fAddBullets);

				if (pSoldier->bDoAutofire > 0)
				{
					ubBurstAPs = CalcAPsToAutofire( pSoldier->CalcActionPoints(), &(pSoldier->inv[BestAttack.bWeaponIn]), pSoldier->bDoAutofire, pSoldier );

					// sevenfm: reserve APs when in dangerous place
					if (pSoldier->bActionPoints >= BestAttack.ubAPCost + sActualAimAP + ubBurstAPs + sReserveAP )
					{
						// Base chance of bursting is 25% if best shot was +0 aim, down to 8% at +4
						iChance = (100 / max((BestAttack.ubAimTime + 1), 1));
						switch (pSoldier->aiData.bAttitude)
						{
						case DEFENSIVE:		iChance += -5; break;
						case BRAVESOLO:		iChance += 5; break;
						case BRAVEAID:		iChance += 5; break;
						case CUNNINGSOLO:	iChance += 0; break;
						case CUNNINGAID:	iChance += 0; break;
						case AGGRESSIVE:	iChance += 10; break;
						case ATTACKSLAYONLY:iChance += 30; break;
						}

						// SANDRO: more likely to burst when firing from hip
						if (BestAttack.bScopeMode == USE_ALT_WEAPON_HOLD && Item[pSoldier->inv[BestAttack.bWeaponIn].usItem].twohanded)
							iChance += 40;

						// CHRISL: Changed from a simple flag to two externalized values for more modder control over AI suppression
						if (GetMagSize(&pSoldier->inv[BestAttack.bWeaponIn], 0) >= gGameExternalOptions.ubAISuppressionMinimumMagSize &&
							pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= gGameExternalOptions.ubAISuppressionMinimumAmmo)
							iChance += 30;

						if (bInGas)
							iChance += 50; //Madd: extra chance of going nuts and autofiring if stuck in gas

						// increase chance based on proximity and difficulty of enemy
						if (PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) < 15)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("check chance to autofire"));
							iChance += (15 - PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget)) * (1 + SoldierDifficultyLevel(pSoldier));
							if (pSoldier->aiData.bAttitude == ATTACKSLAYONLY)
							{
								// increase it more!
								iChance += 5 * (15 - PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget));
							}
						}

						if (pSoldier->RushAttackAdvance())
						{
							iChance += 20;
						}

						if (MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO))
						{
							iChance += 20;
						}

						// HEADROCK HAM 3.6: Forcing enemies to autofire at close range if possible, similar to forced burst (see above)
						if (PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) < 10 && gGameOptions.ubDifficultyLevel > DIF_LEVEL_EASY)
						{
							iChance += 100;
						}

						DebugAI(AI_MSG_INFO, pSoldier, String("autofire chance %d", iChance));

						if (Chance(iChance) || Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].NoSemiAuto)
						{
							//dnl ch69 140913 return aiming for autofire with halfautofire fix
							// decided to use autofire, determine if use half burst
							pSoldier->bDoBurst = 1;
							INT16 ubHalfBurstAPs = 0;

							if(pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft < 4)
							{								
								iChance = 0;
								DebugAI(AI_MSG_INFO, pSoldier, String("not enough ammo for 2 short burst"));
							}
							else
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("have enough ammo for 2 short bursts"));
								iChance = BestAttack.ubChanceToReallyHit / 2;
								ubHalfBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &pSoldier->inv[BestAttack.bWeaponIn], 2, pSoldier);

								if (Weapon[pSoldier->inv[BestAttack.bWeaponIn].usItem].NoSemiAuto || pSoldier->aiData.bOppCnt > 1 || !fAnyCover || !CheckSuppressionDirection(pSoldier, BestAttack.sTarget, BestAttack.bTargetLevel))
								{
									iChance += (100 - iChance) / 2;
								}
								// reduce chance when in high shock
								iChance = iChance * (100 - pSoldier->ShockLevelPercent()) / 100;
								DebugAI(AI_MSG_INFO, pSoldier, String("short autofire chance = %d", iChance));
							}

							// sevenfm: limit autofire if there's a chance to hit friends with autofire
							if (pSoldier->bActionPoints >= 2 * (BestAttack.ubAPCost + ubHalfBurstAPs + sActualAimAP) + sReserveAP &&
								!pSoldier->RushAttackAdvance() &&
								!MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) &&
								Chance(iChance))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("decided to use short autofire"));
								pSoldier->bDoAutofire = 2;
								BestAttack.ubAPCost += ubHalfBurstAPs + sActualAimAP;
							}
							else
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("decided to use autofire"));
								BestAttack.ubAPCost += ubBurstAPs + sActualAimAP;
							}
						}
						else
						{
							pSoldier->aiData.bAimTime = sAimTimeNoBurst;
							pSoldier->bDoAutofire = 0;
							pSoldier->bDoBurst = 0;
						}
					}
				}
			}

			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Decide shot location]"));
			// sevenfm: dynamically decide shot location
			if ( BestAttack.ubOpponent != NOBODY )
			{
				UINT32	uiRoll;
				UINT8	ubChanceLegs = 0;
				UINT8	ubChanceHead = 0;
				INT16	sDistance = PythSpacesAway(pSoldier->sGridNo, MercPtrs[BestAttack.ubOpponent]->sGridNo);
				UINT8	ubRealCTH = (UINT8)BestAttack.ubChanceToReallyHit;
				UINT32	uiCTGT_Legs, uiCTGT_Torso, uiCTGT_Head;					

				uiCTGT_Legs = SoldierToSoldierBodyPartChanceToGetThrough( pSoldier, MercPtrs[BestAttack.ubOpponent], AIM_SHOT_LEGS );
				uiCTGT_Torso = SoldierToSoldierBodyPartChanceToGetThrough( pSoldier, MercPtrs[BestAttack.ubOpponent], AIM_SHOT_TORSO );
				uiCTGT_Head = SoldierToSoldierBodyPartChanceToGetThrough( pSoldier, MercPtrs[BestAttack.ubOpponent], AIM_SHOT_HEAD );

				// check legs/head only if target is not prone
				if( gAnimControl[ MercPtrs[BestAttack.ubOpponent]->usAnimState ].ubEndHeight != ANIM_PRONE )
				{
					// check if leg shot is possible
					// only shoot at legs for standing opponents as they can fall then
					// only for human bodytypes
					if (uiCTGT_Legs > 0 && 
						(sDistance < TACTICAL_RANGE || AIRoleSniper(pSoldier)) &&
						MercPtrs[BestAttack.ubOpponent]->AnimEndHeight() >= ANIM_CROUCH &&
						MercPtrs[BestAttack.ubOpponent]->IsMercBodyType())
					{
						// basic chance to shoot legs
						ubChanceLegs = 15;

						// more often when running
						if(pSoldier->AnimMovementMode() == RUNNING)
						{
							ubChanceLegs += 15;
						}

						// don't shoot legs at close distance
						if( sDistance < TACTICAL_RANGE / 2 )
						{
							ubChanceLegs = ubChanceLegs * sDistance / (TACTICAL_RANGE / 2);
						}
						// when using NCTH system, shoot legs more often
						else if (UsingNewCTHSystem() && (pSoldier->bDoBurst > 0 || pSoldier->bDoAutofire > 0))
						{
							ubChanceLegs += 15;
						}

						// shoot legs more often against melee opponents
						if ((MercPtrs[BestAttack.ubOpponent]->IsZombie() || !AICheckHasGun(MercPtrs[BestAttack.ubOpponent])) && sDistance > TACTICAL_RANGE / 2)
						{
							ubChanceLegs += 30;
						}

						// don't waste bullets shooting at legs with low CTH
						//ubChanceLegs = ubChanceLegs * ubRealCTH / 100;
						ubChanceLegs = ubChanceLegs * (100 + ubRealCTH) / 200;
					}

					// check if head shot is possible
					if( uiCTGT_Head > 0 )
					{
						// basic chance to shoot head
						ubChanceHead = 5;

						// snipers shoot at heads more often
						if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SNIPER_NT))
						{
							ubChanceHead += 10 * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT );
						}

						// if we just hit our enemy, aim at head if have good CTH
						if( pSoldier->sLastTarget == BestAttack.sTarget && pSoldier->LastAttackHit() )
						{
							ubChanceHead += ubRealCTH / 4;
						}

						if (MercPtrs[BestAttack.ubOpponent]->IsZombie())
						{
							if (gGameExternalOptions.fZombieOnlyHeadshotsWork)
								ubChanceHead += 50;
							else if (gGameExternalOptions.fZombieOnlyHeadShotsPermanentlyKill)
								ubChanceHead += 25;
						}

						// if shooting at tank - try turret
						if (MercPtrs[BestAttack.ubOpponent]->IsTank())
						{
							ubChanceHead += 25;
						}

						// don't waste bullets shooting at heads with low CTH
						ubChanceHead = ubChanceHead * (100 + ubRealCTH) / 200;
					}
				}					

				// randomly decide hit location
				uiRoll = PreRandom( 100 );					

				if (uiRoll < ubChanceLegs)
				{
					pSoldier->bAimShotLocation = AIM_SHOT_LEGS;
					DebugAI( AI_MSG_INFO, pSoldier, String("decided to shoot at legs"));
				}
				else if ((INT32)uiRoll > 100 - min(100, ubChanceHead) &&
					!(pSoldier->IsMaleBodyType() && MercPtrs[BestAttack.ubOpponent]->HasStunningLooks() && PersonalSeenKnowledge(pSoldier, BestAttack.ubOpponent)))
				{
					pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
					DebugAI( AI_MSG_INFO, pSoldier, String("decided to shoot at head"));
				}
				else
				{
					pSoldier->bAimShotLocation = AIM_SHOT_TORSO;
					DebugAI( AI_MSG_INFO, pSoldier, String("decided to shoot at torso"));
				}

				// maybe switch to torso
				if (pSoldier->bAimShotLocation == AIM_SHOT_LEGS &&
					uiCTGT_Torso > uiCTGT_Legs * 4)
				{
					pSoldier->bAimShotLocation = AIM_SHOT_TORSO;
					DebugAI( AI_MSG_INFO, pSoldier, String("switch from legs to torso"));
				}
				// maybe switch to head (check that target is not prone)
				if (pSoldier->bAimShotLocation == AIM_SHOT_TORSO &&
					uiCTGT_Head > uiCTGT_Torso * 4 &&
					MercPtrs[BestAttack.ubOpponent]->AnimEndHeight() != ANIM_PRONE &&
					!(pSoldier->IsMaleBodyType() && MercPtrs[BestAttack.ubOpponent]->HasStunningLooks() && PersonalSeenKnowledge(pSoldier, BestAttack.ubOpponent)))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("switch from torso to head"));
					pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
				}

				// zombies: only headshots work
				if (MercPtrs[BestAttack.ubOpponent]->IsZombie() && 
					MercPtrs[BestAttack.ubOpponent]->AnimEndHeight() != ANIM_PRONE &&
					gGameExternalOptions.fZombieOnlyHeadshotsWork &&
					uiCTGT_Head > 0)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("zombie: only headshots work"));
					pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
				}

				// always aim at head for swimming target
				if (MercPtrs[BestAttack.ubOpponent]->IsMercBodyType() &&
					MercPtrs[BestAttack.ubOpponent]->MercInDeepWater())
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("swimming target, aim at head"));
					pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
				}
			}

			if (!pSoldier->bDoBurst)
			{
				pSoldier->aiData.bAimTime = sAimTimeNoBurst;
				pSoldier->bDoBurst			= 0;
				pSoldier->bDoAutofire		= 0;
			}

			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Take cover before shooting]"));
			//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"[%d] check advance AP=%d AP to shoot=%d", pSoldier->ubID, pSoldier->bActionPoints, BestAttack.ubAPCost);

			BOOLEAN fTargetUnconscious = FALSE;
			if( BestAttack.ubOpponent != NOBODY &&
				(MercPtrs[BestAttack.ubOpponent]->IsCowering() ||
				MercPtrs[BestAttack.ubOpponent]->bBreathCollapsed ||
				MercPtrs[BestAttack.ubOpponent]->bCollapsed ||
				MercPtrs[BestAttack.ubOpponent]->stats.bLife < OKLIFE) )
			{
				fTargetUnconscious = TRUE;
			}

			// check if soldier can find better position before firing
			if(	!TileIsOutOfBounds(BestAttack.sTarget) &&
				BestAttack.ubOpponent != NOBODY &&
				ubCanMove &&
				!fStopMovement &&
				!gfHiddenInterrupt &&
				!gTacticalStatus.fInterruptOccurred &&
				//!pSoldier->SkipCoverCheck() &&
				//pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
				//pSoldier->bActionPoints > BestAttack.ubAPCost + APBPConstants[MAX_AP_CARRIED] &&
				pSoldier->bActionPoints > (INT8)(MinAPsToAttack(pSoldier, BestAttack.sTarget, ADDTURNCOST, 0, TRUE) + APBPConstants[AP_MAXIMUM] / 5) &&
				//(gAnimControl[ pSoldier->usAnimState ].ubEndHeight >= ANIM_CROUCH || pSoldier->aiData.bShock == 0) &&
				(	fOvercrowded ||
					pSoldier->RushAttackAdvance() ||
					PythSpacesAway( pSoldier->sGridNo, BestAttack.sTarget ) > usRange / CELL_X_SIZE ||
					pSoldier->LastAttackHit() ||
					fTargetUnconscious ||
					pSoldier->aiData.bUnderFire ||
					!fAnyCover ||
					!TileIsOutOfBounds(sClosestOpponent) && PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) < DAY_VISION_RANGE / 2) )
			{
				INT32	iShootCoverPercentBetter;
				//INT8	bReserveAP = (INT8) (MinAPsToAttack( pSoldier, BestAttack.sTarget, ADDTURNCOST, BestAttack.ubAimTime, TRUE ) + GetAPsCrouch( pSoldier, TRUE) + GetAPsToLook(pSoldier));
				INT8	bReserveAP = (INT8) (MinAPsToAttack( pSoldier, BestAttack.sTarget, ADDTURNCOST, 0, TRUE ) + APBPConstants[MAX_AP_CARRIED]);
				
				DebugAI( AI_MSG_INFO, pSoldier, String("Search cover reserving %d AP, target soldier %d, CTH %d", bReserveAP, BestAttack.ubOpponent, BestAttack.ubChanceToReallyHit));

				// check LOS only if we see enemy currently, otherwise find generally better position
				/*if( pSoldier->aiData.bOppList[BestAttack.ubOpponent] == SEEN_CURRENTLY )
					pSoldier->aiData.usActionData = FindBestNearbyCover(pSoldier, &iShootCoverPercentBetter, bReserveAP, BestAttack.ubOpponent);
				else
					pSoldier->aiData.usActionData = FindBestNearbyCover(pSoldier, &iShootCoverPercentBetter, bReserveAP);*/

				// find location where we can see any enemy
				pSoldier->aiData.usActionData = FindBestNearbyCover(pSoldier, &iShootCoverPercentBetter, bReserveAP, TRUE);

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("Found cover spot %d  PercentBetter %d", pSoldier->aiData.usActionData, iShootCoverPercentBetter));
					//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"[%d] found cover %d percent better %d cth %d", pSoldier->ubID, pSoldier->aiData.usActionData, iShootCoverPercentBetter, BestAttack.ubChanceToReallyHit);
				}
				//pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, sBestShootCover, bReserveAP, AI_ACTION_GET_CLOSER, 0 );

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
					!Water(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) &&
					(iShootCoverPercentBetter > BestAttack.ubChanceToReallyHit || 
					!fAnyCover && AnyCoverAtSpot(pSoldier, pSoldier->aiData.usActionData)))
				{
					//ScreenMsg(FONT_MCOLOR_LTBLUE, MSG_INTERFACE, L"[%d] advance to %d reserving %d AP", pSoldier->ubID, pSoldier->aiData.usActionData, bReserveAP);
					//BeginMultiPurposeLocator(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, FALSE);

					// alt movement mode
					if (DecideCoverReverse(pSoldier, sClosestThreat, bClosestThreatLevel))
						return pSoldier->aiData.bAction;

					DebugAI( AI_MSG_INFO, pSoldier, String("Take cover before shooting to %d reserving %d AP", pSoldier->aiData.usActionData, bReserveAP));
					return AI_ACTION_TAKE_COVER;
				}
			}

			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Get closer before shooting]"));

			// get closer to enemy before shooting
			// only if we see this enemy currently
			if(	!TileIsOutOfBounds(BestAttack.sTarget) &&
				BestAttack.ubOpponent != NOBODY &&
				pSoldier->aiData.bOppList[BestAttack.ubOpponent] == SEEN_CURRENTLY &&
				ubCanMove &&
				!fStopMovement &&
				!gfHiddenInterrupt &&
				!gTacticalStatus.fInterruptOccurred &&
				gfTurnBasedAI &&
				pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
				//pSoldier->bActionPoints > BestAttack.ubAPCost + APBPConstants[MAX_AP_CARRIED] &&
				pSoldier->bActionPoints > (INT8) (MinAPsToAttack( pSoldier, BestAttack.sTarget, ADDTURNCOST, 0, TRUE ) + APBPConstants[AP_MAXIMUM] / 5) &&
				!pSoldier->aiData.bUnderFire &&
				//!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, TRUE) &&
				//(gAnimControl[ pSoldier->usAnimState ].ubEndHeight >= ANIM_CROUCH || pSoldier->aiData.bShock == 0) &&
				pSoldier->aiData.bOrders > ONGUARD &&
				!AIRoleSniper(pSoldier) &&
				!AIRoleCommander(pSoldier) &&
				!AIRoleMachinegunner(pSoldier) &&
				!AIRoleMortarOperator(pSoldier) &&
				!InARoom(pSoldier->sGridNo, NULL) &&
				//(!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE) || fTargetUnconscious) &&
				(!fAnyCover || fTargetUnconscious || pSoldier->RushAttackAdvance()) &&
				RangeChangeDesire(pSoldier) >= 3 &&
				// sevenfm: increased to 10-40 depending on target shock
				BestAttack.ubChanceToReallyHit < 10 + MercPtrs[BestAttack.ubOpponent]->aiData.bShock + 20 * pSoldier->RushAttackAdvance() &&
				(Chance((100 - BestAttack.ubChanceToReallyHit) * (100 - BestAttack.ubChanceToReallyHit) / 100) || pSoldier->RushAttackAdvance() || fTargetUnconscious) &&
				// advance when target is cowering or collapsed or CTH is very low
				(	fOvercrowded ||
					BestAttack.ubChanceToReallyHit == 1 ||
					pSoldier->RushAttackAdvance() ||
					CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4) > CountSeenEnemiesLastTurn(pSoldier) ||
					AICheckWeOutnumber(pSoldier, MercPtrs[BestAttack.ubOpponent]->sGridNo) ||
					PythSpacesAway( pSoldier->sGridNo, BestAttack.sTarget ) > usRange / (CELL_X_SIZE) ||
					pSoldier->LastAttackHit() ||
					fTargetUnconscious ) &&
				pSoldier->aiData.bOppCnt <= 5 - SoldierDifficultyLevel(pSoldier) + CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 2))
				//usRange / (CELL_X_SIZE) < DAY_VISION_RANGE )
			{
				//INT8 bReserveAP = (INT8) (MinAPsToAttack( pSoldier, BestAttack.sTarget, ADDTURNCOST, BestAttack.ubAimTime, TRUE ) + GetAPsCrouch( pSoldier, TRUE) + GetAPsToLook(pSoldier));
				INT8 bReserveAP = (INT8)(MinAPsToAttack(pSoldier, BestAttack.sTarget, ADDTURNCOST, 0, TRUE) + APBPConstants[AP_MAXIMUM] / 5);
				
				DebugAI( AI_MSG_TOPIC, pSoldier, String("search get closer spot, reserve %d AP, target soldier %d, CTH %d", bReserveAP, BestAttack.ubOpponent, BestAttack.ubChanceToReallyHit));

				pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, BestAttack.sTarget, bReserveAP, AI_ACTION_GET_CLOSER, 0 );
				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("Found get closer spot %d", pSoldier->aiData.usActionData));
					//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"[%d] found advance spot %d cth %d", pSoldier->ubID, pSoldier->aiData.usActionData, BestAttack.ubChanceToReallyHit);
				}

				if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
					!Water(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) &&
					!InGas(pSoldier, pSoldier->aiData.usActionData) &&
					PythSpacesAway(pSoldier->aiData.usActionData, BestAttack.sTarget) < PythSpacesAway(pSoldier->sGridNo, BestAttack.sTarget) &&
					LOS_SoldierPos(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, ANIM_STAND, MercPtrs[BestAttack.ubOpponent], BestAttack.sTarget, BestAttack.bTargetLevel, MercPtrs[BestAttack.ubOpponent]->AnimEndHeight(), LOS_NORMAL, CALC_FROM_ALL_DIRS) &&
					(fOvercrowded && CountNearbyFriends(pSoldier, pSoldier->aiData.usActionData, DAY_VISION_RANGE / 4) < CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 4) ||
					pSoldier->RushAttackAdvance() ||
					fTargetUnconscious && BestAttack.ubChanceToReallyHit == 1 && usRange / CELL_X_SIZE < TACTICAL_RANGE / 2 && PythSpacesAway(pSoldier->aiData.usActionData, BestAttack.sTarget) < usRange / (CELL_X_SIZE) ||
					AnyCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, FALSE) ||
					!EnemyCanAttackSpot(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel)))
					//SoldierAtGridNoToSoldierLineOfSightTest( pSoldier, pSoldier->aiData.usActionData, BestAttack.ubOpponent ) )
					//LOS( pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, BestAttack.sTarget, BestAttack.bTargetLevel, CALC_FROM_ALL_DIRS ) )
				{
					//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"[%d] Get closer to %d reserving %d AP", pSoldier->ubID, pSoldier->aiData.usActionData, bReserveAP);
					//BeginMultiPurposeLocator(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, FALSE);

					DebugAI( AI_MSG_INFO, pSoldier, String("Get closer before shooting to %d reserving %d AP", pSoldier->aiData.usActionData, bReserveAP));
					return( AI_ACTION_GET_CLOSER );
				}
			}

			// check to turn on flashlight before shooting
			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Black flashlight]"));

			if(	!TileIsOutOfBounds(BestAttack.sTarget) &&
				BestAttack.ubOpponent != NOBODY &&
				!pSoldier->aiData.bUnderFire &&
				pSoldier->stats.bLife == pSoldier->stats.bLifeMax &&
				//pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE &&
				pSoldier->bTeam == ENEMY_TEAM &&
				pSoldier->bActionPoints == pSoldier->bInitialActionPoints &&
				pSoldier->bActionPoints > (INT8) (MinAPsToAttack( pSoldier, BestAttack.sTarget, ADDTURNCOST, 0, TRUE ) + GetAPsToUseFlashlight()) &&
				//gAnimControl[ pSoldier->usAnimState ].ubEndHeight > ANIM_PRONE &&
				!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_FLASHLIGHT) &&
				pSoldier->ubDirection == AIDirection( pSoldier->sGridNo, BestAttack.sTarget) &&
				BestAttack.bTargetLevel == 0 &&
				!InLightAtNight(BestAttack.sTarget, BestAttack.bTargetLevel) &&
				AIUseFlashlightCondition(pSoldier, AIDirection( pSoldier->sGridNo, BestAttack.sTarget)) &&
				Chance(100 - 100 / (1 + SoldierDifficultyLevel(pSoldier))) &&
				CountFriendsPossiblySeeFlashlightOpponent(pSoldier, BestAttack.sTarget, BestAttack.bTargetLevel, BestAttack.ubOpponent) > 0 &&
				CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 2) > 0 &&
				(fAnyCover || AICheckWeOutnumber(pSoldier, BestAttack.sTarget) || AICheckSuccessfulAttack(pSoldier, TRUE)))
			{
				// turn on flashlight
				DebugAI( AI_MSG_INFO, pSoldier, String("activate flashlight"));
				return(AI_ACTION_SWITCH_FLASHLIGHT);
			}
		}
		else if (ubBestAttackAction == AI_ACTION_THROW_KNIFE)
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Prepare throwing knife]"));

			if (BestAttack.bWeaponIn != HANDPOS && gAnimControl[ pSoldier->usAnimState ].ubEndHeight == ANIM_STAND )
			{
				// we had better make sure we lower our gun first!
				if (WeaponReady(pSoldier) && 
					PickSoldierReadyAnimation(pSoldier, TRUE, FALSE) != INVALID_ANIMATION)
				{
					pSoldier->aiData.bAction = AI_ACTION_LOWER_GUN;
					pSoldier->aiData.usActionData = 0;

					// queue up attack for after we lower weapon if any
					pSoldier->aiData.bNextAction = AI_ACTION_THROW_KNIFE;
					pSoldier->aiData.usNextActionData = BestAttack.sTarget;
					pSoldier->aiData.bNextTargetLevel = BestAttack.bTargetLevel;
				}
				else
				{
					pSoldier->aiData.bAction = AI_ACTION_THROW_KNIFE;
					pSoldier->aiData.usActionData = BestAttack.sTarget;
					pSoldier->bTargetLevel = BestAttack.bTargetLevel;
				}
			}
		}
		else if (ubBestAttackAction == AI_ACTION_KNIFE_MOVE)
		{
			DebugAI( AI_MSG_TOPIC, pSoldier, String("[Prepare knife attack]"));

			// sevenfm: dynamically decide stab location
			if (BestAttack.ubOpponent != NOBODY && MercPtrs[BestAttack.ubOpponent] && MercPtrs[BestAttack.ubOpponent]->IsMercBodyType())
			{
				UINT32	uiRoll;
				UINT8	ubChanceHead = 0;
				UINT8	ubChanceLegs = 0;
				UINT8	ubRealCTH = (UINT8)BestAttack.ubChanceToReallyHit;

				// attack to head randomly
				if( gAnimControl[ MercPtrs[BestAttack.ubOpponent]->usAnimState ].ubEndHeight != ANIM_PRONE )
				{	
					ubChanceHead = 6 + EffectiveDexterity(pSoldier, FALSE) / 10;
					if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, MARTIAL_ARTS_NT))
					{
						ubChanceHead += 5 * NUM_SKILL_TRAITS(pSoldier, MARTIAL_ARTS_NT);
					}
					// bonus chance to attack head if opponent was hit this turn
					if (pSoldier->aiData.bLastAttackHit && MercPtrs[BestAttack.ubOpponent]->TakenHit())
					{
						ubChanceHead += ubRealCTH / 5;
					}
					ubChanceHead = ubChanceHead * 100 / (100 + MercPtrs[BestAttack.ubOpponent]->bBreath);

					if (pSoldier->IsBoxer())
					{
						ubChanceLegs = MercPtrs[BestAttack.ubOpponent]->bBreath * EffectiveWisdom(pSoldier) / (100 + ubRealCTH);
						// lower chance to attack legs if opponent was hit this turn
						if (MercPtrs[BestAttack.ubOpponent]->TakenHit())
						{
							ubChanceLegs /= 2;
						}
					}
				}

				// randomly decide hit location
				uiRoll = PreRandom( 100 );

				if ((INT32)uiRoll > 100 - min(100, ubChanceHead) &&
					!(pSoldier->IsMaleBodyType() && MercPtrs[BestAttack.ubOpponent]->HasStunningLooks() && PersonalSeenKnowledge(pSoldier, BestAttack.ubOpponent)))
				{
					pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
					DebugAI( AI_MSG_INFO, pSoldier, String("aim at head"));
				}
				else if ((INT32)uiRoll > 100 - min(100, ubChanceLegs))
				{
					pSoldier->bAimShotLocation = AIM_SHOT_LEGS;
					DebugAI(AI_MSG_INFO, pSoldier, String("aim at legs"));
				}
				else
				{
					pSoldier->bAimShotLocation = AIM_SHOT_TORSO;
					DebugAI( AI_MSG_INFO, pSoldier, String("aim at torso"));
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// OTHERWISE, JUST GO AHEAD & ATTACK!
		//////////////////////////////////////////////////////////////////////////
		DebugAI(AI_MSG_INFO, pSoldier, String("Attack!"));

		//dnl ch64 270813 must be as below RearrangePocket with FOREVER will screw already decided BURST or AUTOFIRE
		INT8 bDoBurst = pSoldier->bDoBurst;
		UINT8 bDoAutofire = pSoldier->bDoAutofire;
		// swap weapon to hand if necessary
		if (BestAttack.bWeaponIn != HANDPOS)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("swap weapon into hand from %d", BestAttack.bWeaponIn));
			RearrangePocket(pSoldier, HANDPOS, BestAttack.bWeaponIn, FOREVER);
		}
		if (ubBestAttackAction == AI_ACTION_FIRE_GUN && bDoBurst == 1)//dnl ch64 270813
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("using burst/autofire"));

			pSoldier->bDoAutofire = bDoAutofire;
			pSoldier->bDoBurst = bDoBurst;
			if (bDoAutofire > 1)
				pSoldier->bWeaponMode = WM_AUTOFIRE;
			else
				pSoldier->bWeaponMode = WM_BURST;
		}

		DebugAI(AI_MSG_INFO, pSoldier, String("prepare attack at target %d level %d aim %d ap %d cth %d opponent %d", BestAttack.sTarget, BestAttack.bTargetLevel, BestAttack.ubAimTime, BestAttack.ubAPCost, BestAttack.ubChanceToReallyHit, BestAttack.ubOpponent));

		if (ubBestAttackAction == AI_ACTION_FIRE_GUN)
		{
			if (gAnimControl[pSoldier->usAnimState].ubEndHeight != BestAttack.ubStance  &&
				IsValidStance(pSoldier, BestAttack.ubStance))
			{
				pSoldier->aiData.bNextAction = AI_ACTION_FIRE_GUN;
				pSoldier->aiData.usNextActionData = BestAttack.sTarget;
				pSoldier->aiData.bNextTargetLevel = BestAttack.bTargetLevel;
				pSoldier->aiData.usActionData = BestAttack.ubStance;

				DebugAI(AI_MSG_INFO, pSoldier, String("Change stance before shooting"));
				return(AI_ACTION_CHANGE_STANCE);
			}
			else
			{
				pSoldier->aiData.usActionData = BestAttack.sTarget;
				pSoldier->bTargetLevel = BestAttack.bTargetLevel;
				return(AI_ACTION_FIRE_GUN);
			}
		}
		else if (ubBestAttackAction == AI_ACTION_TOSS_PROJECTILE)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("toss attack, disable burst/autofire"));
			pSoldier->bDoBurst = 0;
			pSoldier->bDoAutofire = 0;

			// if firing mortar make sure we have room
			if (Item[pSoldier->inv[BestAttack.bWeaponIn].usItem].mortar)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("using mortar, check room to deploy"));

				ubOpponentDir = AIDirection(pSoldier->sGridNo, BestAttack.sTarget);

				// Get new gridno!
				sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(ubOpponentDir));

				if (!OKFallDirection(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, ubOpponentDir, pSoldier->usAnimState))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("no room to deploy mortar, check if we can move behind"));

					// try behind us, see if there's room to move back
					sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(gOppositeDirection[ubOpponentDir]));
					if (OKFallDirection(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, gOppositeDirection[ubOpponentDir], pSoldier->usAnimState))
					{
						// sevenfm: check if we can reach this gridno
						BOOLEAN bReverse = FALSE;
						if (pSoldier->ubDirection == ubOpponentDir || pSoldier->ubDirection == gOneCDirection[ubOpponentDir] || pSoldier->ubDirection == gOneCCDirection[ubOpponentDir])
							bReverse = TRUE;
						INT32 iPathCost = EstimatePlotPath(pSoldier, sCheckGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_GET_CLOSER), pSoldier->bStealthMode, bReverse, 0);
						if (iPathCost != 0 && iPathCost + BestAttack.ubAPCost + GetAPsToLook(pSoldier) + GetAPsCrouch(pSoldier, FALSE) <= pSoldier->bActionPoints)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("moving backwards to have more room to deploy mortar, reverse mode: %d", bReverse));
							pSoldier->aiData.usActionData = sCheckGridNo;
							pSoldier->bReverse = bReverse;

							DebugAI(AI_MSG_INFO, pSoldier, String("prepare next action throw at spot %d level %d aimtime %d", BestAttack.sTarget, BestAttack.bTargetLevel, BestAttack.ubAimTime));

							pSoldier->aiData.usNextActionData = BestAttack.sTarget;
							pSoldier->aiData.bNextTargetLevel = BestAttack.bTargetLevel;
							pSoldier->aiData.bAimTime = BestAttack.ubAimTime;

							pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;

							return AI_ACTION_GET_CLOSER;
						}
					}

					// can't fire!
					BestAttack.ubPossible = FALSE;
				}
			}

			// if still possible
			if (BestAttack.ubPossible)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestAttack.sTarget, BestAttack.bTargetLevel, BestAttack.ubAimTime));

				// sevenfm: correctly set weapon mode for attached GL
				if (IsGrenadeLauncherAttached(&pSoldier->inv[HANDPOS]))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("set attached GL mode"));
					pSoldier->bWeaponMode = WM_ATTACHED_GL;
				}

				// stand up before throwing if needed
				if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestAttack.ubStance &&
					pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestAttack.sTarget), BestAttack.ubStance))
				{
					pSoldier->aiData.usActionData = BestAttack.ubStance;
					pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
					pSoldier->aiData.usNextActionData = BestAttack.sTarget;
					pSoldier->aiData.bNextTargetLevel = BestAttack.bTargetLevel;
					pSoldier->aiData.bAimTime = BestAttack.ubAimTime;
					return AI_ACTION_CHANGE_STANCE;
				}
				else
				{
					pSoldier->aiData.usActionData = BestAttack.sTarget;
					pSoldier->bTargetLevel = BestAttack.bTargetLevel;
					pSoldier->aiData.bAimTime = BestAttack.ubAimTime;
				}

				return(AI_ACTION_TOSS_PROJECTILE);
			}
		}
		// other attacks
		else
		{
			pSoldier->aiData.usActionData = BestAttack.sTarget;
			pSoldier->bTargetLevel = BestAttack.bTargetLevel;
			return(ubBestAttackAction);
		}
	}

	// get the location of the closest reachable opponent
	/*	Flugente 22.02.2012 - A few clarifications: I changed ClosestSeenOpponent so that for zombies, this function also returns an opponent if he is on the
	*	roof of a building, we are not, but our GridNo belongs to that same building. 
	*	If that is the case, it is clear that we have to get on that roof. However, we cannot do that in BlackState. If, by pure chance, we can still see our
	*	enemy, we cannot climb (there is  no climbing option in BlackState sofar).
	*	So, I changed the code so that now we will climb the roof.
	*/
	/*INT32	targetGridNo = -1;
	INT8	targetbLevel =  0;
	sClosestOpponent = ClosestSeenOpponentWithRoof(pSoldier, &targetGridNo, &targetbLevel);
	if( !gfHiddenInterrupt &&
	!gTacticalStatus.fInterruptOccurred &&
	!TileIsOutOfBounds(sClosestOpponent) &&
	!TileIsOutOfBounds(targetGridNo) &&
	SameBuilding( pSoldier->sGridNo, targetGridNo ) )
	{
	if ( targetbLevel == pSoldier->pathing.bLevel && targetbLevel == 0 )
	{
	//////////////////////////////////////////////////////////////////////
	// GO DIRECTLY TOWARDS CLOSEST KNOWN OPPONENT
	//////////////////////////////////////////////////////////////////////

	// try to move towards him
	pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards(pSoldier,sClosestOpponent,AI_ACTION_GET_CLOSER);

	// Flugente: if on the same level and there is a jumpable window here, jump through it
	if ( gGameExternalOptions.fCanJumpThroughWindows )
	{
	// determine if there is a jumpable window in the direction to our target
	// if yes, and we are not facing it, face it now
	// if yes, and we are facing it, jump
	// if no, go on, nothing to see here
	// determine direction of our target
	INT8 targetdirection = (INT8)GetDirectionToGridNoFromGridNo(pSoldier->sGridNo, sClosestOpponent);

	// determine if there is a jumpable window here, in the direction of our target
	// store old direction for this check
	UINT8 tmpdirection = pSoldier->ubDirection;
	pSoldier->ubDirection = targetdirection;

	INT8 windowdirection = DIRECTION_IRRELEVANT;
	if ( FindWindowJumpDirection(pSoldier, pSoldier->sGridNo, pSoldier->ubDirection, &windowdirection) && targetdirection == windowdirection )
	{
	pSoldier->ubDirection = tmpdirection;

	// are we already looking in that direction?
	if ( pSoldier->ubDirection == targetdirection )
	{
	// jump through the window
	return(AI_ACTION_JUMP_WINDOW);
	}
	else
	{
	// look into that direction
	if ( pSoldier->InternalIsValidStance( targetdirection, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
	{
	pSoldier->aiData.usActionData = targetdirection;
	return(AI_ACTION_CHANGE_FACING);
	}

	}
	}

	pSoldier->ubDirection = tmpdirection;
	}
	}
	// The situation mentioned above happens...
	else
	{
	// need to climb AND have enough APs to get there this turn
	BOOLEAN fUp = TRUE;
	if (pSoldier->pathing.bLevel > 0 )
	fUp = FALSE;

	if ( pSoldier->bActionPoints > GetAPsToClimbRoof ( pSoldier, fUp ) )
	{
	pSoldier->aiData.usActionData = targetGridNo;//FindClosestClimbPoint(pSoldier, fUp );

	// Necessary test: can we climb up at this position? It might happen that our target is directly above us, then we'll have to move
	INT8 newdirection;
	if ( ( fUp && FindHeigherLevel( pSoldier, pSoldier->sGridNo, pSoldier->ubDirection, &newdirection ) ) || ( !fUp && FindLowerLevel( pSoldier, pSoldier->sGridNo, pSoldier->ubDirection, &newdirection ) ) )
	{							
	return( AI_ACTION_CLIMB_ROOF );
	}
	else
	{
	return(AI_ACTION_SEEK_OPPONENT);
	}
	}
	}
	}*/

	// try to make boxer close if possible
	if(pSoldier->IsBoxer())
	{
		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Make boxer close if possible]"));

		UINT8 ubOpponentID;
		//sClosestOpponent = ClosestSeenOpponent(pSoldier, NULL, NULL);
		sClosestOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL, &ubOpponentID);
		DebugAI(AI_MSG_INFO, pSoldier, String("boxer: found closest opponent [%d] at %d", ubOpponentID, sClosestOpponent));

		if (!TileIsOutOfBounds(sClosestOpponent) &&
			ubOpponentID != NOBODY &&
			MercPtrs[ubOpponentID])
		{
			// if opponent boxer uses dusters, give this boxer dusters too
			if ((MercPtrs[ubOpponentID]->inv[HANDPOS].exists() && Item[MercPtrs[ubOpponentID]->inv[HANDPOS].usItem].brassknuckles || 
				MercPtrs[ubOpponentID]->inv[SECONDHANDPOS].exists() && Item[MercPtrs[ubOpponentID]->inv[SECONDHANDPOS].usItem].brassknuckles) &&
				!pSoldier->inv[HANDPOS].exists() &&
				Item[BRASS_KNUCKLES].ubCoolness > 0 &&
				Item[BRASS_KNUCKLES].usItemClass & IC_PUNCH &&
				Item[BRASS_KNUCKLES].brassknuckles)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("boxer: give dusters"));
				CreateItem(BRASS_KNUCKLES, 60 + Random(40), &pSoldier->inv[HANDPOS]);
				pSoldier->inv[HANDPOS].fFlags |= OBJECT_UNDROPPABLE;
			}

			if (pSoldier->bActionPoints > 0)
			{
				if (SpacesAway(pSoldier->sGridNo, sClosestOpponent) > 1)
				{
					INT16 sReserveAP = GetAPsToLook(pSoldier) + 2 * MinAPsToAttack(pSoldier, pSoldier->sLastTarget, FALSE, 0, 0);
					BOOLEAN fLimitOneStep = FALSE;

					if (pSoldier->bInitialActionPoints < sReserveAP + APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK])
					{
						sReserveAP = GetAPsToLook(pSoldier) + MinAPsToAttack(pSoldier, pSoldier->sLastTarget, FALSE, 0, 0);
					}

					if (pSoldier->bInitialActionPoints < sReserveAP + APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK] &&
						pSoldier->bInitialActionPoints >= GetAPsToLook(pSoldier) + MinAPsToAttack(pSoldier, pSoldier->sLastTarget, FALSE, 0, 0) &&
						SpacesAway(pSoldier->sGridNo, sClosestOpponent) > 2)
					{
						sReserveAP = GetAPsToLook(pSoldier) + 1;
						fLimitOneStep = TRUE;
					}

					// temporarily make boxer have orders of CLOSEPATROL rather than STATIONARY
					// And make him patrol the ring, not his usual place
					// so he has a good roaming range
					INT32 tgrd = pSoldier->aiData.sPatrolGrid[0];
					pSoldier->aiData.sPatrolGrid[0] = pSoldier->sGridNo;
					pSoldier->aiData.bOrders = CLOSEPATROL;
					//pSoldier->aiData.usActionData = GoAsFarAsPossibleTowards( pSoldier, sClosestOpponent, AI_ACTION_GET_CLOSER );
					pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, sClosestOpponent, sReserveAP, AI_ACTION_GET_CLOSER, 0);
					pSoldier->aiData.sPatrolGrid[0] = tgrd;
					pSoldier->aiData.bOrders = STATIONARY;

					// decide to restore breath
					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
						(pSoldier->bBreath < OKBREATH || 
						pSoldier->bBreath < pSoldier->bBreathMax && 
						pSoldier->bBreath < MercPtrs[ubOpponentID]->bBreath &&
						Chance((100 - pSoldier->bBreath) * (100 - pSoldier->bBreath) / (2 * 100 * 100))))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("boxer: restore breath"));
						pSoldier->aiData.usActionData = NOWHERE;
					}

					if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
					{
						// truncate path to 1 step
						if (fLimitOneStep)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("boxer: limit movement to one step"));
							pSoldier->aiData.usActionData = pSoldier->sGridNo + DirectionInc( (UINT8) pSoldier->pathing.usPathingData[0] );
							pSoldier->pathing.sFinalDestination = pSoldier->aiData.usActionData;
						}
						
						//pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
						DebugAI(AI_MSG_INFO, pSoldier, String("boxer: get closer to opponent"));
						return(AI_ACTION_GET_CLOSER);
					}
				}
				else if (pSoldier->bBreath < OKBREATH ||
						pSoldier->bBreath < pSoldier->bBreathMax &&
						(pSoldier->bBreath < MercPtrs[ubOpponentID]->bBreath || !pSoldier->aiData.bLastAttackHit && pSoldier->TakenLargeHit() ))
						//(Chance((100 - pSoldier->bBreath) / 100) || !pSoldier->aiData.bLastAttackHit && pSoldier->TakenLargeHit()))
				{
					// maybe move away from opponent
					UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestOpponent);
					INT32 sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(gOppositeDirection[ubOpponentDir]));

					// only use reverse movement if we are facing opponent
					if (pSoldier->ubDirection == ubOpponentDir ||
						pSoldier->ubDirection == gOneCDirection[ubOpponentDir] ||
						pSoldier->ubDirection == gOneCCDirection[ubOpponentDir])
					{
						pSoldier->bReverse = TRUE;
					}					

					if (!NewOKDestination(pSoldier, sCheckGridNo, FALSE, pSoldier->pathing.bLevel) ||
						PlotPath(pSoldier, sCheckGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER), pSoldier->bStealthMode, pSoldier->bReverse, 0) > pSoldier->bActionPoints - 1)
					{
						//if (sPathcost > pSoldier->bActionPoints - (GetAPsToLook(pSoldier) + 1))
						DebugAI(AI_MSG_INFO, pSoldier, String("boxer: bad destination or high path cost, cannot move away"));
						sCheckGridNo = NOWHERE;
					}

					// maybe try diagonal movement
					if(TileIsOutOfBounds(sCheckGridNo) && Chance(50))
					{
						sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(gOneCDirection[gOppositeDirection[ubOpponentDir]]));
						if (!NewOKDestination(pSoldier, sCheckGridNo, FALSE, pSoldier->pathing.bLevel) ||
							PlotPath(pSoldier, sCheckGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER), pSoldier->bStealthMode, pSoldier->bReverse, 0) > pSoldier->bActionPoints - 1)
						{
							//if (sPathcost > pSoldier->bActionPoints - (GetAPsToLook(pSoldier) + 1))
							DebugAI(AI_MSG_INFO, pSoldier, String("boxer: bad destination or high path cost, cannot move away"));
							sCheckGridNo = NOWHERE;
						}
					}
					if (TileIsOutOfBounds(sCheckGridNo) && Chance(50))
					{
						sCheckGridNo = NewGridNo(pSoldier->sGridNo, DirectionInc(gOneCCDirection[gOppositeDirection[ubOpponentDir]]));
						if (!NewOKDestination(pSoldier, sCheckGridNo, FALSE, pSoldier->pathing.bLevel) ||
							PlotPath(pSoldier, sCheckGridNo, FALSE, FALSE, FALSE, DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER), pSoldier->bStealthMode, pSoldier->bReverse, 0) > pSoldier->bActionPoints - 1)
						{
							//if (sPathcost > pSoldier->bActionPoints - (GetAPsToLook(pSoldier) + 1))
							DebugAI(AI_MSG_INFO, pSoldier, String("boxer: bad destination or high path cost, cannot move away"));
							sCheckGridNo = NOWHERE;
						}
					}

					if (!TileIsOutOfBounds(sCheckGridNo))
					{
						pSoldier->aiData.usActionData = sCheckGridNo;
						DebugAI(AI_MSG_INFO, pSoldier, String("boxer: get away from opponent"));
						return(AI_ACTION_TAKE_COVER);
					}
					pSoldier->bReverse = FALSE;
				}
			}

			UINT8 ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestOpponent);

			// possibly turn to closest opponent
			if (pSoldier->ubDirection != ubOpponentDir &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
				pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
			{
				pSoldier->aiData.usActionData = ubOpponentDir;
				DebugAI(AI_MSG_INFO, pSoldier, String("boxer: turn to closest opponent"));
				return(AI_ACTION_CHANGE_FACING);
			}
		}

		// otherwise do nothing
		DebugAI(AI_MSG_INFO, pSoldier, String("boxer: nothing to do"));
		return( AI_ACTION_NONE );
	}

	////////////////////////////////////////////////////////////////////////////
	// IF A LOCATION WITH BETTER COVER IS AVAILABLE & REACHABLE, GO FOR IT!
	////////////////////////////////////////////////////////////////////////////

	if (!TileIsOutOfBounds(sBestCover))
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("Take cover %d", sBestCover));

		//ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_TESTVERSION, L"AI %d taking cover, morale %d, from %d to %d", pSoldier->ubID, pSoldier->aiData.bAIMorale, pSoldier->sGridNo, sBestCover );
		pSoldier->aiData.usActionData = sBestCover;

		// alt movement mode
		if (pSoldier->IsMercBodyType() &&
			!pSoldier->bReverse &&
			pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] == 0 &&
			!pSoldier->RushAttackActive() &&
			(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral) &&
			(pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE_MILITIA || 
			pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE || 
			CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || 
			pSoldier->aiData.bUnderFire || 
			Chance(SoldierDifficultyLevel(pSoldier) * 10)) &&
			PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.usActionData) == 1 &&
			(pSoldier->aiData.bUnderFire || 
			CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || 
			!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || 
			!SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE) || 
			EnemyCanAttackSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) || 
			EnemyCanAttackSpot(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel)))
		{
			UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER);
			UINT8 ubMoveDirection = AIDirection(pSoldier->sGridNo, pSoldier->aiData.usActionData);
			UINT8 ubOppositeDirection = gOppositeDirection[pSoldier->ubDirection];
			UINT8 ubAnimEndHeight = pSoldier->AnimEndHeight();
			INT32 sClosestThreatAtSpot;
			INT8 bClosestThreatAtSpotLevel;

			// find closest possibly seen opponent after moving to new spot
			// remember current location
			INT32 iCurrentSpot = pSoldier->sGridNo;
			FLOAT dXPos = pSoldier->dXPos;
			FLOAT dYPos = pSoldier->dYPos;
			// put soldier at new spot
			INT16 sTempX, sTempY;
			ConvertGridNoToCenterCellXY(pSoldier->aiData.usActionData, &sTempX, &sTempY);
			pSoldier->sGridNo = pSoldier->aiData.usActionData;
			pSoldier->dXPos = (FLOAT)sTempX;
			pSoldier->dYPos = (FLOAT)sTempY;
			INT32 sClosestPossiblySeenOpponentAsSpot = ClosestPossiblySeenOpponent(pSoldier, &sClosestThreatAtSpot, &bClosestThreatAtSpotLevel);
			// restore original location
			pSoldier->sGridNo = iCurrentSpot;
			pSoldier->dXPos = dXPos;
			pSoldier->dYPos = dYPos;

			// if not found possibly seen opponent, use closest threat
			if (TileIsOutOfBounds(sClosestPossiblySeenOpponentAsSpot))
			{
				sClosestThreatAtSpot = sClosestThreat;
				bClosestThreatAtSpotLevel = bClosestThreatLevel;
			}

			UINT8 ubClosestThreatDirection = AIDirection(pSoldier->aiData.usActionData, sClosestThreatAtSpot);
			INT16 sMinAPsToAttack = MinAPsToAttack(pSoldier, sClosestThreat, TRUE, 0);
			INT16 sMinAPsToAttackRaise = MinAPsToAttack(pSoldier, sClosestThreat, TRUE, 0, TRUE);
			INT16 sMinAPsToRaise = GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE));
			INT32 iPathCost = EstimatePlotPath(pSoldier, pSoldier->aiData.usActionData, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);

			// reverse replaces RUNNING with WALKING
			UINT16 usMovementModeReverse = DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER, TRUE);
			INT32 iPathCostReverse = EstimatePlotPath(pSoldier, pSoldier->aiData.usActionData, FALSE, FALSE, FALSE, usMovementModeReverse, pSoldier->bStealthMode, TRUE, 0);

			if (iPathCostReverse > 0 && pSoldier->bActionPoints >= iPathCostReverse)
			{
				// use reverse movement mode when moving backwards from closest threat
				if ((ubMoveDirection == ubOppositeDirection || ubMoveDirection == gOneCDirection[ubOppositeDirection] || ubMoveDirection == gOneCCDirection[ubOppositeDirection]) &&
					(ubClosestThreatDirection == pSoldier->ubDirection || ubClosestThreatDirection == gOneCDirection[pSoldier->ubDirection] || ubClosestThreatDirection == gOneCCDirection[pSoldier->ubDirection]) &&
					(pSoldier->bActionPoints < sMinAPsToAttack || pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttackRaise))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("moving backwards keeping the same threat direction, use reverse movement mode"));
					pSoldier->bReverse = TRUE;

					return AI_ACTION_TAKE_COVER;
				}

				// when standing or crawling - move sideways
				if (!pSoldier->bReverse &&
					(usMovementModeReverse == CRAWLING && ubAnimEndHeight == ANIM_PRONE || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND) &&
					(ubMoveDirection == gTwoCDirection[pSoldier->ubDirection] || ubMoveDirection == gTwoCCDirection[pSoldier->ubDirection]) &&
					(ubClosestThreatDirection == pSoldier->ubDirection || ubClosestThreatDirection == gOneCDirection[pSoldier->ubDirection] || ubClosestThreatDirection == gOneCCDirection[pSoldier->ubDirection]) &&
					(pSoldier->bActionPoints < sMinAPsToAttack || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND && pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttack || pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttackRaise))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("moving sideways keeping the same threat direction, use reverse movement mode"));
					pSoldier->bReverse = TRUE;
				}

				// possibly, first turn to threat and raise gun when moving sideways
				//if ((usMovementModeReverse == CRAWLING && ubAnimEndHeight == ANIM_PRONE || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND) &&
				if (pSoldier->bReverse &&
					usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
					pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToRaise + GetAPsToLook(pSoldier) &&
					(ubMoveDirection == gTwoCDirection[ubClosestThreatDirection] || ubMoveDirection == gTwoCCDirection[ubClosestThreatDirection]) &&
					!LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, MAX_VISION_RANGE) &&
					LOS_Location(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, pSoldier->GetMaxDistanceVisible(sClosestThreatAtSpot, bClosestThreatAtSpotLevel, CALC_FROM_ALL_DIRS)))
				{
					// turn to closest threat
					/*if (pSoldier->ubDirection != ubClosestThreatDirection &&
						pSoldier->InternalIsValidStance(ubClosestThreatDirection, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
						pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
					{
						pSoldier->aiData.usActionData = ubOpponentDir;*/

						// possibly raise weapon (only when standing)
						/*if (usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
							!WeaponReady(pSoldier) &&
							PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
							(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
							pSoldier->bActionPoints >= GetAPsToLook(pSoldier) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
							pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
						}*/

						// prepare next action to take cover
						/*pSoldier->aiData.bNextAction = AI_ACTION_TAKE_COVER;
						pSoldier->aiData.usNextActionData = sBestCover;

						DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest threat before moving"));
						return AI_ACTION_CHANGE_FACING;
					}*/

					// possibly raise weapon (only when standing)
					if (usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
						!WeaponReady(pSoldier) &&
						PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
						(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
						pSoldier->bActionPoints >= GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
					{
						// prepare next action to take cover
						pSoldier->aiData.bNextAction = AI_ACTION_TAKE_COVER;
						pSoldier->aiData.usNextActionData = sBestCover;

						DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon and take cover"));
						return AI_ACTION_RAISE_GUN;
					}
				}				
			}
		}

		return AI_ACTION_TAKE_COVER;
	}


	////////////////////////////////////////////////////////////////////////////
	// IF THINGS ARE REALLY HOPELESS, OR UNARMED, TRY TO RUN AWAY
	////////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Run away]"));

	// if soldier has enough APs left to move at least 1 square's worth
	if (ubCanMove &&
		(pSoldier->aiData.bAIMorale == MORALE_HOPELESS || !bCanAttack) &&
		(pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier)))
	{
		// look for best place to RUN AWAY to (farthest from the closest threat)
		//pSoldier->aiData.usActionData = RunAway( pSoldier );
		pSoldier->aiData.usActionData = FindSpotMaxDistFromOpponents(pSoldier);

		if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
		{
			return(AI_ACTION_RUN_AWAY);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// IF SPOTTERS HAVE BEEN CALLED FOR, AND WE HAVE SOME NEW SIGHTINGS, RADIO!
	////////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Radio sightings]"));

	// if we're a computer merc, and we have the action points remaining to RADIO
	// (we never want NPCs to choose to radio if they would have to wait a turn)
	// and we're not swimming in deep water, and somebody has called for spotters
	// and we see the location of at least 2 opponents
	// sevenfm: disable for boxers
	// disable in interrupt
	if (!fCivilian &&
		SoldierAI(pSoldier) &&
		!bInDeepWater &&
		!gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!pSoldier->IsBoxer() &&
		!pSoldier->RaisedAlertThisTurn() &&
		gTacticalStatus.ubSpottersCalledForBy != NOBODY &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT) &&
		pSoldier->bActionPoints > APBPConstants[AP_RADIO] + APBPConstants[AP_MAXIMUM] / 4 &&
		!fDangerousSpot &&
		!pSoldier->aiData.bUnderFire &&
		pSoldier->aiData.bOppCnt > 1 && 
		gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1)
	{
		// base chance depends on how much new info we have to radio to the others
		iChance = 10 * WhatIKnowThatPublicDont(pSoldier,TRUE);	// just count them
		
		// sevenfm: lower chance to radio if some nearby friend radioed this turn
		iChance = iChance / (1 + CountFriendsUsedRadio(pSoldier, TACTICAL_RANGE));

		// if I actually know something they don't
		if ((INT16)PreRandom(100) < iChance)
		{
			return(AI_ACTION_RED_ALERT);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// CROUCH IF NOT CROUCHING ALREADY
	////////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Crouch if not crouched already]"));

	// if not in water and not already crouched, try to crouch down first
	if( pSoldier->IsMercBodyType() &&
		pSoldier->AnimEndHeight() == ANIM_STAND &&
		GetAPsToChangeStance( pSoldier, ANIM_CROUCH ) <= pSoldier->bActionPoints &&
		IsValidStance(pSoldier, ANIM_CROUCH) &&
		ConsiderCrouch(pSoldier) &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!fCivilian &&
		!pSoldier->IsBoxer() &&
		//!gfHiddenInterrupt &&		
		ubBestAttackAction != AI_ACTION_KNIFE_MOVE &&
		ubBestAttackAction != AI_ACTION_KNIFE_STAB &&
		ubBestAttackAction != AI_ACTION_STEAL_MOVE &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestOpponent) > TACTICAL_RANGE / 4)
	{
		pSoldier->aiData.usActionData = ANIM_CROUCH;
		return AI_ACTION_CHANGE_STANCE;
	}

	////////////////////////////////////////////////////////////////////////////
	// TURN TO FACE CLOSEST KNOWN OPPONENT (IF NOT FACING THERE ALREADY)
	////////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Turn to closest known opponent]"));

	// hopeless guys shouldn't waste their time this way, UNLESS they CAN move
	// but chose not to to get this far (which probably means they're cornered)
	// ALSO, don't bother turning if we're already aiming a gun
	if (!(pSoldier->usSoldierFlagMask2 & SOLDIER_AI_TURN_THREAT) &&
		pSoldier->bActionPoints >= GetAPsToLook(pSoldier) &&
		//!gfHiddenInterrupt &&
		(pSoldier->aiData.bAIMorale > MORALE_HOPELESS || ubCanMove) &&
		!TileIsOutOfBounds(sClosestPossiblySeenOpponent))
	{
		bDirection = AIDirection(pSoldier->sGridNo, sClosestPossiblySeenOpponent);

		// if we're not facing towards him
		if (pSoldier->ubDirection != bDirection &&
			pSoldier->InternalIsValidStance(bDirection, gAnimControl[pSoldier->usAnimState].ubEndHeight))
		{
			pSoldier->aiData.usActionData = bDirection;
			pSoldier->aiData.bNextAction = AI_ACTION_NONE;
			pSoldier->aiData.usNextActionData = 0;

			pSoldier->usSoldierFlagMask2 |= SOLDIER_AI_TURN_THREAT;

			return AI_ACTION_CHANGE_FACING;
		}
	}

	// if a militia has absofreaking nothing else to do, maybe they should radio in a report!

	////////////////////////////////////////////////////////////////////////////
	// RADIO RED ALERT: determine %chance to call others and report contact
	////////////////////////////////////////////////////////////////////////////

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Radio Red Alert for militia]"));

	if( !gfHiddenInterrupt &&
		!gTacticalStatus.fInterruptOccurred &&
		!pSoldier->IsBoxer() &&
		!pSoldier->RaisedAlertThisTurn() &&
		pSoldier->bTeam == MILITIA_TEAM &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT) &&
		pSoldier->bActionPoints > APBPConstants[AP_RADIO] + APBPConstants[AP_MAXIMUM] / 4 &&
		!fDangerousSpot &&
		!pSoldier->aiData.bUnderFire &&
		gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1 )
	{
		// if there hasn't been an initial RED ALERT yet in this sector
		if ( !gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition || NeedToRadioAboutPanicTrigger() )
		{     
			// since I'm at STATUS RED, I obviously know we're being invaded!
			iChance = gbDiff[DIFF_RADIO_RED_ALERT][ SoldierDifficultyLevel( pSoldier ) ];
		}
		else
		{
			// subsequent radioing (only to update enemy positions, request help)
			// base chance depends on how much new info we have to radio to the others
			iChance = 10 * WhatIKnowThatPublicDont(pSoldier,FALSE);  // use 10 * for RED alert
		}

		// if I actually know something they don't and I ain't swimming (deep water)
		if (iChance && !bInDeepWater)
		{
			// modify base chance according to orders
			switch (pSoldier->aiData.bOrders)
			{
			case STATIONARY:       iChance +=  20;  break;
			case ONGUARD:          iChance +=  15;  break;
			case ONCALL:           iChance +=  10;  break;
			case CLOSEPATROL:                       break;
			case RNDPTPATROL:
			case POINTPATROL:      iChance +=  -5;  break;
			case FARPATROL:        iChance += -10;  break;
			case SEEKENEMY:        iChance += -20;  break;
			}

			// modify base chance according to attitude
			switch (pSoldier->aiData.bAttitude)
			{
			case DEFENSIVE:        iChance +=  20;  break;
			case BRAVESOLO:        iChance += -10;  break;
			case BRAVEAID:                          break;
			case CUNNINGSOLO:      iChance +=  -5;  break;
			case CUNNINGAID:                        break;
			case AGGRESSIVE:       iChance += -20;  break;
			case ATTACKSLAYONLY:		iChance = 0;
			}

			if (gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition)
			{
				// ignore morale (which could be really high)
			}
			else
			{
				// modify base chance according to morale
				switch (pSoldier->aiData.bAIMorale)
				{
				case MORALE_HOPELESS:  iChance *= 3;    break;
				case MORALE_WORRIED:   iChance *= 2;    break;
				case MORALE_NORMAL:                     break;
				case MORALE_CONFIDENT: iChance /= 2;    break;
				case MORALE_FEARLESS:  iChance /= 3;    break;
				}
			}

			// reduce chance because we're in combat
			iChance /= 2;

			// sevenfm: lower chance to radio if some nearby friend radioed this turn
			iChance = iChance / (1 + CountFriendsUsedRadio(pSoldier, TACTICAL_RANGE));

			if ((INT16) PreRandom(100) < iChance)
			{
				return(AI_ACTION_RED_ALERT);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// PRONE IF NOT PRONE ALREADY
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Possibly go prone]"));

	// if not in water and not already crouched, try to crouch down first
	if (pSoldier->IsMercBodyType() &&
		pSoldier->AnimEndHeight() > ANIM_PRONE &&
		GetAPsToChangeStance(pSoldier, ANIM_PRONE) <= pSoldier->bActionPoints &&
		IsValidStance(pSoldier, ANIM_PRONE) &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!fCivilian &&
		!pSoldier->IsBoxer() &&
		//!gfHiddenInterrupt &&
		ubBestAttackAction != AI_ACTION_KNIFE_MOVE &&
		ubBestAttackAction != AI_ACTION_KNIFE_STAB &&
		ubBestAttackAction != AI_ACTION_STEAL_MOVE &&
		ConsiderProne(pSoldier))
	{
		pSoldier->aiData.usActionData = ANIM_PRONE;
		return AI_ACTION_CHANGE_STANCE;
	}

	////////////////////////////////////////////////////////////////////////////
	// LEAVE THE SECTOR
	////////////////////////////////////////////////////////////////////////////

	// NOT IMPLEMENTED

	////////////////////////////////////////////////////////////////////////////
	// DO NOTHING: Not enough points left to move, so save them for next turn
	////////////////////////////////////////////////////////////////////////////

	// by default, if everything else fails, just stand in place and wait
	pSoldier->aiData.usActionData = NOWHERE;
	return(AI_ACTION_NONE);
}

void DecideAlertStatus( SOLDIERTYPE *pSoldier )
{
	INT8	bOldStatus;
	INT32	iDummy;
	BOOLEAN fClimbDummy,fReachableDummy;

	DebugAI(String("DecideAlertStatus %s [%d]", pSoldier->GetNameS().c_str(), pSoldier->ubID));

	// THE FOUR (4) POSSIBLE ALERT STATUSES ARE:
	// GREEN - No one seen, no suspicious noise heard, go about regular duties
	// YELLOW - Suspicious noise was heard personally or radioed in by buddy
	// RED - Either saw opponents in person, or definite contact had been radioed
	// BLACK - Currently has one or more opponents in sight

	// save the man's previous status

	if (pSoldier->flags.uiStatusFlags & SOLDIER_MONSTER)
	{
		CreatureDecideAlertStatus( pSoldier );
		return;
	}

	bOldStatus = pSoldier->aiData.bAlertStatus;

	// determine the current alert status for this category of man
	//if (!(pSoldier->flags.uiStatusFlags & SOLDIER_PC))
	{
		if (pSoldier->aiData.bOppCnt > 0)        // opponent(s) in sight
		{
			pSoldier->aiData.bAlertStatus = STATUS_BLACK;
			CheckForChangingOrders( pSoldier );
		}
		else                        // no opponents are in sight
		{
			switch (bOldStatus)
			{
			case STATUS_BLACK:
				// then drop back to RED status
				pSoldier->aiData.bAlertStatus = STATUS_RED;
				break;

			case STATUS_RED:
				// RED can never go back down below RED, only up to BLACK
				break;

			case STATUS_YELLOW:
				// if all enemies have been RED alerted, or we're under fire
				if (!PTR_CIVILIAN && (gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition || pSoldier->aiData.bUnderFire))
				{
					pSoldier->aiData.bAlertStatus = STATUS_RED;
				}
				else
				{
					// if we are NOT aware of any uninvestigated noises right now
					// and we are not currently in the middle of an action
					// (could still be on his way heading to investigate a noise!)					
					if (( TileIsOutOfBounds(MostImportantNoiseHeard(pSoldier,&iDummy,&fClimbDummy,&fReachableDummy))) && !pSoldier->aiData.bActionInProgress)
					{
						// then drop back to GREEN status
						pSoldier->aiData.bAlertStatus = STATUS_GREEN;
						CheckForChangingOrders( pSoldier );
					}
				}
				break;

			case STATUS_GREEN:
				// if all enemies have been RED alerted, or we're under fire
				if (!PTR_CIVILIAN && (gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition || pSoldier->aiData.bUnderFire))
				{
					pSoldier->aiData.bAlertStatus = STATUS_RED;
				}
				else
				{
					// if we ARE aware of any uninvestigated noises right now					
					if (!TileIsOutOfBounds(MostImportantNoiseHeard(pSoldier,&iDummy,&fClimbDummy,&fReachableDummy)))
					{
						// then move up to YELLOW status
						pSoldier->aiData.bAlertStatus = STATUS_YELLOW;
					}
				}
				break;
			}
			// otherwise, RED stays RED, YELLOW stays YELLOW, GREEN stays GREEN
		}
	}

	if ( gTacticalStatus.bBoxingState == NOT_BOXING )
	{
		// if the man's alert status has changed in any way
		if (pSoldier->aiData.bAlertStatus != bOldStatus)
		{
			// HERE ARE TRYING TO AVOID NPCs SHUFFLING BACK & FORTH BETWEEN RED & BLACK
			// if either status is < RED (ie. anything but RED->BLACK && BLACK->RED)
			if ((bOldStatus < STATUS_RED) || (pSoldier->aiData.bAlertStatus < STATUS_RED))
			{
				// force a NEW action decision on next pass through HandleManAI()
				SetNewSituation( pSoldier );
			}

			// if this guy JUST discovered that there were opponents here for sure...
			if ((bOldStatus < STATUS_RED) && (pSoldier->aiData.bAlertStatus >= STATUS_RED))
			{
				CheckForChangingOrders(pSoldier);
			}
		}
		else   // status didn't change
		{
			// only do this stuff in TB
			// if a guy on status GREEN or YELLOW is running low on breath
			if (((pSoldier->aiData.bAlertStatus == STATUS_GREEN)  && (pSoldier->bBreath < 75)) ||
				((pSoldier->aiData.bAlertStatus == STATUS_YELLOW) && (pSoldier->bBreath < 50)))
			{
				// as long as he's not in water (standing on a bridge is OK)
				if (!pSoldier->MercInWater())
				{
					// force a NEW decision so that he can get some rest
					SetNewSituation( pSoldier );

					// current action will be canceled. if noise is no longer important					
					if ((pSoldier->aiData.bAlertStatus == STATUS_YELLOW) &&
						( TileIsOutOfBounds(MostImportantNoiseHeard(pSoldier,&iDummy,&fClimbDummy,&fReachableDummy))))
					{
						// then drop back to GREEN status
						pSoldier->aiData.bAlertStatus = STATUS_GREEN;
						CheckForChangingOrders( pSoldier );
					}
				}
			}
		}
	}
	else if (gTacticalStatus.bBoxingState == DISQUALIFIED ||
		gTacticalStatus.bBoxingState == WON_ROUND ||
		gTacticalStatus.bBoxingState == LOST_ROUND)
	{
		pSoldier->aiData.bAlertStatus = STATUS_GREEN;
	}

}

void LogDecideInfo( SOLDIERTYPE *pSoldier )
{
	DebugAI(AI_MSG_INFO, pSoldier, String("Turn num %d aware %d", gTacticalStatus.ubCombatTurn, gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition));
	DebugAI(AI_MSG_INFO, pSoldier, String("current team %d interrupt (top message) %d interrupt occurred %d", gTacticalStatus.ubCurrentTeam, AICheckInterrupt(), gTacticalStatus.fInterruptOccurred));
	DebugAI( AI_MSG_INFO, pSoldier, String("AP=%d/%d %s %s %s %s %s", pSoldier->bActionPoints, pSoldier->bInitialActionPoints, gStr8AlertStatus[pSoldier->aiData.bAlertStatus], gStr8Orders[pSoldier->aiData.bOrders], gStr8Attitude[pSoldier->aiData.bAttitude], gStr8Team[pSoldier->bTeam], gStr8Class[pSoldier->ubSoldierClass]) );
	DebugAI( AI_MSG_INFO, pSoldier, String("Health %d/%d Breath %d/%d Shock %d Tolerance %d AI Morale %d Morale %d", pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bBreath, pSoldier->bBreathMax, pSoldier->aiData.bShock, CalcSuppressionTolerance(pSoldier), pSoldier->aiData.bAIMorale, pSoldier->aiData.bMorale) );
	DebugAI(AI_MSG_INFO, pSoldier, String("Spot %d level %d opponents %d", pSoldier->sGridNo, pSoldier->pathing.bLevel, pSoldier->aiData.bOppCnt));
	DebugAI(AI_MSG_INFO, pSoldier, String("ubServiceCount %d ubServicePartner %d fDoingSurgery %d", pSoldier->ubServiceCount, pSoldier->ubServicePartner, pSoldier->fDoingSurgery));
	if(pSoldier->IsCowering())
		DebugAI(AI_MSG_INFO, pSoldier, String("Cowering"));
	if(pSoldier->IsGivingAid())
		DebugAI(AI_MSG_INFO, pSoldier, String("Giving aid"));
	if(pSoldier->IsFocusing())
		DebugAI(AI_MSG_INFO, pSoldier, String("Focusing at spot %d level %d turns %d", pSoldier->sFocusGridNo, pSoldier->bFocusLevel, pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS]));
	if (pSoldier->IsSpotting())
		DebugAI(AI_MSG_INFO, pSoldier, String("Spotting at spot %d level %d turns %d", pSoldier->sFocusGridNo, pSoldier->bFocusLevel, pSoldier->usSkillCounter[SOLDIER_COUNTER_SPOTTER]));
	if (pSoldier->IsFlanking())
		DebugAI(AI_MSG_INFO, pSoldier, String("Flanking at spot %d level %d data %d turns %d", pSoldier->sTacticalSpot, pSoldier->bTacticalLevel, pSoldier->sTacticalData, pSoldier->ubTacticalTurns));
	if (pSoldier->RetreatActive())
		DebugAI(AI_MSG_INFO, pSoldier, String("Retreating turns %d", pSoldier->ubTacticalTurns));
	if (pSoldier->RushAttackActive())
		DebugAI(AI_MSG_INFO, pSoldier, String("Rush attack turns %d", pSoldier->ubTacticalTurns));
	if (pSoldier->bTacticalOrder != TACTICAL_ORDER_NONE)
		DebugAI(AI_MSG_INFO, pSoldier, String("Tactical order %d spot %d level %d data %d turns %d", pSoldier->bTacticalOrder, pSoldier->sTacticalSpot, pSoldier->bTacticalLevel, pSoldier->sTacticalData, pSoldier->ubTacticalTurns));

	// show watched locations
	INT8	bLoop;	
	for ( bLoop = 0; bLoop < NUM_WATCHED_LOCS; bLoop++ )
	{		
		if (!TileIsOutOfBounds(gsWatchedLoc[ pSoldier->ubID ][ bLoop ]) )
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("Watched location %d level %d points %d", gsWatchedLoc[ pSoldier->ubID ][ bLoop ], gbWatchedLocLevel[ pSoldier->ubID ][ bLoop ], gubWatchedLocPoints[ pSoldier->ubID ][ bLoop ]));
		}
	}

	LogKnowledgeInfo(pSoldier);	

	DebugAI( AI_MSG_INFO, pSoldier, String("What I know %d", WhatIKnowThatPublicDont(pSoldier, FALSE)));
	DebugAI( AI_MSG_INFO, pSoldier, String("Has Gun %d, Short range weapon %d, Gun Range %d, Gun Ammo %d, Gun Scoped %d ", AICheckHasGun(pSoldier), AICheckShortWeaponRange(pSoldier), AIGunRange(pSoldier), AIGunAmmo(pSoldier), AIGunScoped(pSoldier)));
	DebugAI( AI_MSG_INFO, pSoldier, String("StopSeek %d RushAttack %d RetreatCounter %d", pSoldier->StopSeekValue(), pSoldier->RushAttackValue(), pSoldier->RetreatValue()) );
}

void LogKnowledgeInfo( SOLDIERTYPE *pSoldier )
{
	//CHAR8 str8[1024];
	//memset(str8, 0, 1024 * sizeof(char));

	// show public opponents
	for(UINT16 oppID = 0; oppID < MAX_NUM_SOLDIERS; oppID++)
	{
		if ( gbPublicOpplist[pSoldier->bTeam][oppID] != NOT_HEARD_OR_SEEN &&
			!MercPtrs[oppID]->aiData.bNeutral )
		{
			//wcstombs(str8, MercPtrs[oppID]->GetName(), wcslen(MercPtrs[oppID]->GetName())+1);
			//wcstombs(str8, MercPtrs[oppID]->GetName(), 1024 - 1);
			DebugAI( AI_MSG_INFO, pSoldier, String("public opponent [%d] knowledge %s gridno %d level %d", oppID, gStr8Knowledge[gbPublicOpplist[pSoldier->bTeam][oppID] - OLDEST_HEARD_VALUE], gsPublicLastKnownOppLoc[pSoldier->bTeam][oppID], gbPublicLastKnownOppLevel[pSoldier->bTeam][oppID]));
			//swprintf( pStrInfo, L"%s[%d] %s %s\n", pStrInfo, oppID, MercPtrs[oppID]->GetName(), SeenStr(gbPublicOpplist[pSoldier->bTeam][oppID]) );
		}
	}
	// show personal opponents
	for(UINT16 oppID = 0; oppID < MAX_NUM_SOLDIERS; oppID++)
	{
		if ( pSoldier->aiData.bOppList[oppID] != NOT_HEARD_OR_SEEN &&
			!MercPtrs[oppID]->aiData.bNeutral )
		{
			//wcstombs(str8, MercPtrs[oppID]->GetName(), wcslen(MercPtrs[oppID]->GetName())+1);
			//wcstombs(str8, MercPtrs[oppID]->GetName(), 1024 - 1);
			DebugAI(AI_MSG_INFO, pSoldier, String("personal opponent [%d] knowledge %s gridno %d level %d", oppID, gStr8Knowledge[pSoldier->aiData.bOppList[oppID] - OLDEST_HEARD_VALUE], gsLastKnownOppLoc[pSoldier->ubID][oppID], gbLastKnownOppLevel[pSoldier->ubID][oppID]));
			//swprintf( pStrInfo, L"%s[%d] %s %s\n", pStrInfo, oppID, MercPtrs[oppID]->GetName(), SeenStr(pSoldier->aiData.bOppList[oppID]) );
		}
	}
	// show watched locations
	for (UINT8 ubWatchLoc = 0; ubWatchLoc < NUM_WATCHED_LOCS; ubWatchLoc++)
	{
		if (!TileIsOutOfBounds(gsWatchedLoc[pSoldier->ubID][ubWatchLoc]))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("watched location: spot %d level %d", gsWatchedLoc[pSoldier->ubID][ubWatchLoc], gbWatchedLocLevel[pSoldier->ubID][ubWatchLoc]));
		}
	}

	DebugAI(AI_MSG_INFO, pSoldier, String("public noise volume %d spot %d level %d", gubPublicNoiseVolume[pSoldier->bTeam], gsPublicNoiseGridNo[pSoldier->bTeam], gbPublicNoiseLevel[pSoldier->bTeam]));
	DebugAI(AI_MSG_INFO, pSoldier, String("personal noise volume %d spot %d level %d", pSoldier->aiData.ubNoiseVolume, pSoldier->aiData.sNoiseGridno, pSoldier->bNoiseLevel));
}