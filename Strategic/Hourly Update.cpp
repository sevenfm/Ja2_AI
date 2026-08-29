#ifdef PRECOMPILEDHEADERS
	#include "Strategic All.h"
#else
	#include "Assignments.h"
	#include "Strategic Town Loyalty.h"
	#include "Strategic Merc Handler.h"
	#include "Strategic Mines.h"
	#include "Campaign.h"
	#include "Morale.h"
	#include "Quests.h"
	#include "Game Clock.h"
	#include "Overhead.h"
	#include "jascreens.h"
	#include "screenids.h"
	#include "Items.h"
	#include "Random.h"
	#include "finances.h"
	#include "history.h"
	#include "Dialogue Control.h"
	#include "Strategic AI.h"
	#include "Tactical Save.h"	// added by Flugente
	#include "SkillCheck.h"			// added by Flugente
	#include "Drugs And Alcohol.h"	
	#include "boxing.h"
#endif

#include "Luaglobal.h"
#include "LuaInitNPCs.h"
#include "SaveLoadGame.h"
#include "GameSettings.h"
#include "connect.h"
#include "Options Screen.h"
#include "SaveLoadScreen.h"
#include "Text.h"
// HEADROCK HAM 3.5: Add facility code for hourly update of detection levels
#include "Facilities.h"
#include "Food.h"	// added by Flugente
// anv: for hourly heli repair
#include "Vehicles.h"
#include "Map Screen Helicopter.h" 
// sevenfm
#include "NPC.h"

void HourlyQuestUpdate();
void HourlyLarryUpdate();
void HourlyStealUpdate();	// Flugente: certain characters might steal equipment (backgrounds)
void HourlySnitchUpdate();	// anv: decreasing cooldown after exposition

extern BOOLEAN fAutoStopTimeEachHour;

extern INT32 GetCurrentBalance( void );
extern void PayOffSkyriderDebtIfAny( );
#ifdef JA2UB
//no UB
#else
void HourlyCheckIfSlayAloneSoHeCanLeave();
#endif

#ifdef JA2UB
//no UB
#else
void HourlyHelicopterRepair();
#endif

void HourlyGatheringInformation();

void UpdateRegenCounters( void );

extern void DebugQuestInfo(STR szOutput);

void HandleMinuteUpdate()
{
}

// This function gets called every hour, on the hour.
// It spawns the handling of all sorts of stuff:
// Morale changes, assignment progress, town loyalties, etc.

void HandleHourlyUpdate()
{
CHAR16	zString[128]; 
	//if the game hasnt even started yet ( we havent arrived in the sector ) dont process this
	if ( DidGameJustStart() )
		return;
	if(is_networked)
		return;

	// hourly update of town loyalty
	HandleTownLoyalty();

	// sevenfm: auto refill canteens
	HourlyCanteenUpdate();

	// Flugente: food update
	if ( gGameOptions.fFoodSystem )
		HourlyFoodUpdate();

	// hourly update of team assignments
	UpdateAssignments();

	// HEADROCK HAM 3.5: Tells the rest of the program whether we've got mercs working on detecting enemy units.
	UpdateStrategicDetectionLevel();

	// HEADROCK HAM 3.5: Re-calculates a modifier to Skyrider's flight costs based on mercs working in specific facilities.
	UpdateSkyriderCostModifier();

	// hourly update of hated/liked mercs
	UpdateBuddyAndHatedCounters();

	HandleHourlyRisks();

	// update morale!
	HourlyMoraleUpdate();

	// degrade camouflage
	HourlyCamouflageUpdate();

	// check mines for monster infestation/extermination
	HourlyMinesUpdate();

	// check how well the player is doing right now
	HourlyProgressUpdate();

	// do any quest stuff necessary
	HourlyQuestUpdate();

	HourlyLarryUpdate();

	HourlyStealUpdate();

	HourlySnitchUpdate();

	HourlyGatheringInformation();

#ifdef JA2UB
// no UB
#else
	HourlyCheckIfSlayAloneSoHeCanLeave();
#endif
	// WDS - New AI
	HourlyCheckStrategicAI();

#ifdef JA2UB
// no UB
#else
	PayOffSkyriderDebtIfAny();

	HourlyHelicopterRepair();
#endif
	
	HourlyDrugUpdate();

	if ( GetWorldHour() % 6 == 0 ) // 4 times a day
	{		UpdateRegenCounters();
	}

	// WANNE: This check should avoid the resaving of a loaded auto-save game, when entering tactical
	BOOLEAN doAutoSave = TRUE;
	if (lastLoadedSaveGameDay == GetWorldDay() && lastLoadedSaveGameHour == GetWorldHour() )
	{
		doAutoSave = FALSE;
	}

	if (doAutoSave)
	{
		if ( AutoSaveToSlot[1] == FALSE && AutoSaveToSlot[2] == FALSE && AutoSaveToSlot[3] == FALSE && AutoSaveToSlot[4] == FALSE )
			AutoSaveToSlot[0] = TRUE;

		if ((gGameExternalOptions.autoSaveTime != 0) && (GetWorldHour() % gGameExternalOptions.autoSaveTime == 0)) 
		{
				if ( AutoSaveToSlot[0] == TRUE )
				{
					if( CanGameBeSaved() )
					{
						guiPreviousOptionScreen = guiCurrentScreen;
						swprintf( zString, L"%s%d",pMessageStrings[ MSG_SAVE_AUTOSAVE_TEXT ],SAVE__TIMED_AUTOSAVE_SLOT1);
						DoAutoSave(SAVE__TIMED_AUTOSAVE_SLOT1,zString);
					}
					
					AutoSaveToSlot[0] = FALSE;
					AutoSaveToSlot[1] = TRUE;
					AutoSaveToSlot[2] = FALSE;
					AutoSaveToSlot[3] = FALSE;
					AutoSaveToSlot[4] = FALSE;
				}
				else if ( AutoSaveToSlot[1] == TRUE )
				{
					if( CanGameBeSaved() )
					{
						guiPreviousOptionScreen = guiCurrentScreen;
						swprintf( zString, L"%s%d",pMessageStrings[ MSG_SAVE_AUTOSAVE_TEXT ],SAVE__TIMED_AUTOSAVE_SLOT2);
						DoAutoSave(SAVE__TIMED_AUTOSAVE_SLOT2,zString);
					}
					
					AutoSaveToSlot[0] = FALSE;
					AutoSaveToSlot[1] = FALSE;
					AutoSaveToSlot[2] = TRUE;
					AutoSaveToSlot[3] = FALSE;
					AutoSaveToSlot[4] = FALSE;
				}
				else if ( AutoSaveToSlot[2] == TRUE )
				{
					if( CanGameBeSaved() )
					{
						guiPreviousOptionScreen = guiCurrentScreen;
						swprintf( zString, L"%s%d",pMessageStrings[ MSG_SAVE_AUTOSAVE_TEXT ],SAVE__TIMED_AUTOSAVE_SLOT3);
						DoAutoSave(SAVE__TIMED_AUTOSAVE_SLOT3,zString);
					}
					
					AutoSaveToSlot[0] = FALSE;
					AutoSaveToSlot[1] = FALSE;
					AutoSaveToSlot[2] = FALSE;
					AutoSaveToSlot[3] = TRUE;
					AutoSaveToSlot[4] = FALSE;
				}
				else if ( AutoSaveToSlot[3] == TRUE )
				{
					if( CanGameBeSaved() )
					{
						guiPreviousOptionScreen = guiCurrentScreen;
						swprintf( zString, L"%s%d",pMessageStrings[ MSG_SAVE_AUTOSAVE_TEXT ],SAVE__TIMED_AUTOSAVE_SLOT4);
						DoAutoSave(SAVE__TIMED_AUTOSAVE_SLOT4,zString);
					}
					AutoSaveToSlot[0] = FALSE;
					AutoSaveToSlot[1] = FALSE;
					AutoSaveToSlot[2] = FALSE;
					AutoSaveToSlot[3] = FALSE;
					AutoSaveToSlot[4] = TRUE;
					
				}
				else if ( AutoSaveToSlot[4] == TRUE )
				{
					if( CanGameBeSaved() )
					{
						guiPreviousOptionScreen = guiCurrentScreen;
						swprintf( zString, L"%s%d",pMessageStrings[ MSG_SAVE_AUTOSAVE_TEXT ],SAVE__TIMED_AUTOSAVE_SLOT5);
						DoAutoSave(SAVE__TIMED_AUTOSAVE_SLOT5,zString);
					}
					AutoSaveToSlot[0] = TRUE;
					AutoSaveToSlot[1] = FALSE;
					AutoSaveToSlot[2] = FALSE;
					AutoSaveToSlot[3] = FALSE;
					AutoSaveToSlot[4] = FALSE;
					
				}
		}
	}

	// sevenfm: auto stop time compression
	if (fAutoStopTimeEachHour)
	{
		StopTimeCompression();
	}
}

void UpdateRegenCounters( void )
{
	UINT8	ubID;

	for ( ubID = gTacticalStatus.Team[ gbPlayerNum ].bFirstID; ubID <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubID++ )
	{
		if ( MercPtrs[ ubID ]->bRegenBoostersUsedToday > 0 )
		{
			MercPtrs[ ubID ]->bRegenBoostersUsedToday--;
		}
	}
}

void HandleQuarterHourUpdate()
{
	//if the game hasnt even started yet ( we havent arrived in the sector ) dont process this
	if ( DidGameJustStart() )
		return;

	DecayTacticalMoraleModifiers( );
}

void HourlyQuestUpdate()
{	
#ifdef LUA_HOURLY_QUEST_UPDATE
	LetLuaHourlyQuestUpdate(0);
#else
	UINT32 uiHour = GetWorldHour();

	DebugQuestInfo(String("HourlyQuestUpdate: hour %d", uiHour));

	// brothel
	if ( uiHour == 4 )
	{
		SetFactFalse( FACT_BROTHEL_OPEN );
	}
	else if ( uiHour == 20 )
	{
		SetFactTrue( FACT_BROTHEL_OPEN );
	}

	// sevenfm: restore boxer health every hour
	HealBoxers();

	// bar/nightclub
	if ( uiHour == 15 )
	{
		SetFactTrue( FACT_CLUB_OPEN );
		SetFactFalse( FACT_PAST_CLUB_CLOSING_AND_PLAYER_WARNED );						
	}
	else if ( uiHour == 2 )
	{
		SetFactFalse( FACT_CLUB_OPEN );
	
		if (gfBoxersResting)
		{
			gfBoxersResting = FALSE;
			gubBoxersRests++;
			DebugQuestInfo(String("Stop boxers resting, gfBoxersResting %d gubBoxersRests %d", gfBoxersResting, gubBoxersRests));
		}
		// set boxers to resting after every 3 fights
		else if (gubBoxingMatchesWon / 3 > gubBoxersRests)
		{
			// time for the boxers to rest!
			gfBoxersResting = TRUE;
			DebugQuestInfo(String("Start boxers resting after 3 fights, gfBoxersResting %d", gfBoxersResting));
		}

		// reset boxers fought when club is being closed
		for (UINT8 ubLoop = 0; ubLoop < NUM_BOXERS; ubLoop++)
		{
			gfBoxerFought[ubLoop] = FALSE;
			DebugQuestInfo(String("gfBoxerFought[%d] %d", ubLoop, gfBoxerFought[ubLoop]));
		}
	}	

	// museum
	if ( uiHour == 9 )
		SetFactTrue( FACT_MUSEUM_OPEN );
	else if ( uiHour == 18 )
		SetFactFalse( FACT_MUSEUM_OPEN );

	// sevenfm: reset day counter for Darren to allow starting boxing matches again
	MERCPROFILESTRUCT *pProfile = &(gMercProfiles[DARREN]);
	UINT32 uiDay = GetWorldDay();
	if (pProfile && pProfile->ubLastDateSpokenTo > 0 && uiDay > 1)
	{
		pProfile->ubLastDateSpokenTo = uiDay - 1;
		DebugQuestInfo(String("reset day counter for Darren to allow starting boxing matches again, ubLastDateSpokenTo %d", pProfile->ubLastDateSpokenTo = uiDay));
	}

#endif
}

#define BAR_TEMPTATION 4

// Flugente: abandoned the LarryItems for the new drug system
/*#define NUM_LARRY_ITEMS 6
UINT16	LarryItems[ NUM_LARRY_ITEMS ][ 3 ] =
{
	// item, temptation, points to use
	{ ADRENALINE_BOOSTER,		5, 100 },
	{	ALCOHOL,							BAR_TEMPTATION, 25	},
	{ MEDICKIT,							4, 10	},
	{	WINE,									3, 50	},
	{ REGEN_BOOSTER,				3, 100 },
	{	BEER,									2, 100 },
};*/

#define LARRY_FALLS_OFF_WAGON 8

void HourlyLarryUpdate()
{
	SOLDIERTYPE *				pSoldier = NULL;
	SOLDIERTYPE *				pOtherSoldier = NULL;
	INT8						bSlot = NO_SLOT, bBoozeSlot;
	//INT8						bLarryItemLoop;		// VR r2370
	UINT16						usTemptation = 0;
	UINT16						usCashAmount;
	BOOLEAN						fBar;
	OBJECTTYPE*					pObj = NULL;

	for( UINT32 cnt = gTacticalStatus.Team[ OUR_TEAM ].bFirstID; cnt <= gTacticalStatus.Team[ OUR_TEAM ].bLastID; ++cnt )
	{
		pSoldier = MercPtrs[ cnt ];

		if( pSoldier && 
			pSoldier->bAssignment < ON_DUTY && 
			!pSoldier->flags.fBetweenSectors && 
			pSoldier->bActive && 
			!pSoldier->flags.fMercAsleep &&
			pSoldier->bInSector && 
			!(gTacticalStatus.fEnemyInSector || guiCurrentScreen == GAME_SCREEN ) )
		{			
			if ( pSoldier->ubProfile == LARRY_NORMAL || pSoldier->ubProfile == LARRY_DRUNK || pSoldier->HasBackgroundFlag( BACKGROUND_DRUGUSE ) )
			{				
				// Flugente: reworked this for the new drug system. We now loop over our entire inventory
				usTemptation = 0;
				INT8 invsize = (INT8)pSoldier->inv.size();										// remember inventory size, so we don't call size() repeatedly
				for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop)									// ... for all items in our inventory ...
				{
					if ( pSoldier->inv[bLoop].exists() && Item[ pSoldier->inv[bLoop].usItem ].drugtype > 0 )
					{
						pObj = &(pSoldier->inv[bLoop]);
						usTemptation = 5;
						// any drug will do... I'm not going to create a new tag for sth minor like this
						break;
					}
				}

				// sevenfm: initialize for every soldier
				fBar = FALSE;

				// check to see if we're in a bar sector, if we are, we have access to alcohol
				// which may be better than anything we've got...
				if ( usTemptation < BAR_TEMPTATION && GetCurrentBalance() >= Item[ ALCOHOL ].usPrice )
				{
					// sevenfm: check facility
					if( pSoldier->bSectorZ == 0 )
					{
						for (UINT16 usFacilityType = 0; usFacilityType < NUM_FACILITY_TYPES; usFacilityType++)
						{
							// Is this facility here?
							if (gFacilityLocations[SECTOR(pSoldier->sSectorX, pSoldier->sSectorY)][usFacilityType].fFacilityHere)
							{
								for (UINT16 usFacilityAssignment = 0; usFacilityAssignment < NUM_FACILITY_ASSIGNMENTS; usFacilityAssignment++)
								{
									// is it a place with risk of getting drunk?
									if (gFacilityTypes[usFacilityType].AssignmentData[usFacilityAssignment].Risk[RISK_DRUNK].usChance > 0)
									{
										// Cool.
										fBar = TRUE;
										usTemptation = BAR_TEMPTATION;
										// sevenfm: stop searching
										break;
									}
								}
							}
						}
					}					

					/*if ( pSoldier->bSectorZ == 0 &&
								( ( pSoldier->sSectorX == 13 && pSoldier->sSectorY == MAP_ROW_D) ||
									( pSoldier->sSectorX == 13 && pSoldier->sSectorY == MAP_ROW_C) ||
									( pSoldier->sSectorX == 5 && pSoldier->sSectorY == MAP_ROW_C) ||
									( pSoldier->sSectorX == 6 && pSoldier->sSectorY == MAP_ROW_C) ||
									( pSoldier->sSectorX == 5 && pSoldier->sSectorY == MAP_ROW_D) ||
									( pSoldier->sSectorX == 2 && pSoldier->sSectorY == MAP_ROW_H)
								)
						)
					{
						// in a bar!
						fBar = TRUE;
						usTemptation = BAR_TEMPTATION;
					}*/
				}

				if ( usTemptation > 0 )
				{
					// anv: snitches stop mercs from getting wasted
					BOOLEAN fSnitchStoppedBehaviour = FALSE;
					UINT16 bPreventChance;

					for( INT32 cnt2 = gTacticalStatus.Team[ OUR_TEAM ].bFirstID; cnt2 <= gTacticalStatus.Team[ OUR_TEAM ].bLastID; ++cnt2 )
					{					
						pOtherSoldier = MercPtrs[ cnt2 ];
						// note - snitches stop others, but can get wasted themselves (if they have drug use specifically set in background...)
						if( pOtherSoldier && 
							!pOtherSoldier->flags.fBetweenSectors && 
							pOtherSoldier->bActive && 
							//!pOtherSoldier->flags.fMercAsleep && 
							pSoldier->ubProfile != pOtherSoldier->ubProfile )
						{
							// sevenfm: squadleaders also can prevent mercs from getting wasted
							if (ProfileHasSkillTrait(pOtherSoldier->ubProfile, SNITCH_NT) && !(pOtherSoldier->usSoldierFlagMask2 & SOLDIER_PREVENT_MISBEHAVIOUR_OFF) ||
								ProfileHasSkillTrait( pOtherSoldier->ubProfile, SQUADLEADER_NT ))
							{
								if( pSoldier->sSectorX == pOtherSoldier->sSectorX && 
									pSoldier->sSectorY == pOtherSoldier->sSectorY && 
									pSoldier->bSectorZ == pOtherSoldier->bSectorZ )
								{																		
									if( !pOtherSoldier->flags.fMercAsleep )
									{
										// 100% chance if snitch is not sleeping
										bPreventChance = 100;
									}
									else
									{
										// 50-100% base chance
										bPreventChance = 50 + ( EffectiveLeadership( pOtherSoldier ) / 2 + EffectiveExpLevel( pOtherSoldier, FALSE ) * 5 ) / 2;

										if (gGameOptions.fNewTraitSystem)
										{
											//bPreventChance += 25 * NUM_SKILL_TRAITS( pOtherSoldier, SQUADLEADER_NT );
											bPreventChance -= 25 * NUM_SKILL_TRAITS( pSoldier, STEALTHY_NT );
										}
										else
										{
											bPreventChance -= 25 * NUM_SKILL_TRAITS( pSoldier, STEALTHY_OT );
										}
									}									

									// apply min/max limits
									bPreventChance = max(0, bPreventChance);
									bPreventChance = min(100, bPreventChance);
									
									if( bPreventChance > PreRandom( 100 ) )
									{
										// merc is not amused by being prevented
										HandleMoraleEvent( pSoldier, MORALE_PREVENTED_MISBEHAVIOUR, pSoldier->sSectorX, pSoldier->sSectorX, pSoldier->bSectorZ );
										// also here would be a place for dynamic relationship decrease between them

										fSnitchStoppedBehaviour = TRUE;
										continue;
									}
								}
							}
						}
					}

					if ( fSnitchStoppedBehaviour )
						continue;

					if ( pSoldier->ubProfile == LARRY_DRUNK )
					{
						// NB store all drunkenness info in LARRY_NORMAL profile (to use same values)
						// so long as he keeps consuming, keep number above level at which he cracked
						gMercProfiles[ LARRY_NORMAL ].bNPCData = __max( gMercProfiles[ LARRY_NORMAL ].bNPCData, LARRY_FALLS_OFF_WAGON );
						gMercProfiles[ LARRY_NORMAL ].bNPCData += (INT8) Random( usTemptation );
						// allow value to keep going up to 24 (about 2 days since we subtract Random( 2 ) when he has no access )
						gMercProfiles[ LARRY_NORMAL ].bNPCData = __min( gMercProfiles[ LARRY_NORMAL ].bNPCData, 24 );
					}
					else
					{
						gMercProfiles[ pSoldier->ubProfile ].bNPCData += (INT8) Random( usTemptation );

						if ( gMercProfiles[ pSoldier->ubProfile ].bNPCData < LARRY_FALLS_OFF_WAGON )
							continue;
					}

					if ( fBar )
					{
						// take $ from player's account
						usCashAmount = Item[ ALCOHOL ].usPrice;
						AddTransactionToPlayersBook ( TRANSFER_FUNDS_TO_MERC, pSoldier->ubProfile, GetWorldTotalMin() , -( usCashAmount ) );
						// give Larry some booze and set slot etc values appropriately
						// CHRISL: Change final parameter to allow dynamic control of inventory slots
						bBoozeSlot = FindEmptySlotWithin( pSoldier, HANDPOS, NUM_INV_SLOTS );
						if ( bBoozeSlot != NO_SLOT )
						{
							// give Larry booze here
							CreateItem( ALCOHOL, 100, &(pSoldier->inv[bBoozeSlot]) );
						}
						bSlot = bBoozeSlot;

						// VR r2370
						/*bLarryItemLoop = 1;

						if ( pSoldier->ubProfile == LARRY_DRUNK )
						{
							SwapLarrysProfiles( pSoldier );
						}*/

						if ( bSlot != NO_SLOT )
						{
							// VR r2370
							//UseKitPoints( &(pSoldier->inv[ bSlot ]), 25/*LarryItems[ bLarryItemLoop ][ 2 ]*/, pSoldier );
							ApplyDrugs(pSoldier, &(pSoldier->inv[bSlot]));
						}
					}
					else
					{
						if (pObj)
						{
							// VR r2370
							//UseKitPoints(pObj, 100/*LarryItems[ bLarryItemLoop ][ 2 ]*/, pSoldier);
							ApplyDrugs(pSoldier, pObj);
						}
					}
				}
				else if ( pSoldier->ubProfile == LARRY_DRUNK )
				{
					gMercProfiles[ LARRY_NORMAL ].bNPCData -= (INT8) Random( 2 );
					if ( gMercProfiles[ LARRY_NORMAL ].bNPCData <= 0 )
					{
						// goes sober!
						SwapLarrysProfiles( pSoldier );
					}
				}
			}
		}
	}
}

// anv: decrease exposed snitch cooldown (for simplified exposition handling)
void HourlySnitchUpdate()
{
	for( INT32 cnt = 0; cnt < NUM_PROFILES; ++cnt )
	{
		if( gMercProfiles[cnt].ubSnitchExposedCooldown > 0 )
			gMercProfiles[cnt].ubSnitchExposedCooldown--;
	}
}

void HourlyStealUpdate()
{
	SOLDIERTYPE *	pSoldier = NULL;
	SOLDIERTYPE *	pOtherSoldier = NULL;
	INT8			bSlot = NO_SLOT;
	UINT16			usTemptation = 0;
	BOOLEAN			fBar = FALSE;
	OBJECTTYPE*		pObj = NULL;
	BOOLEAN			fMercInCurrentSector;
	UINT8			ubBaseChance;			

	for( INT32 cnt = gTacticalStatus.Team[ OUR_TEAM ].bFirstID; cnt <= gTacticalStatus.Team[ OUR_TEAM ].bLastID; ++cnt )
	{
		pSoldier = MercPtrs[ cnt ];

		fMercInCurrentSector = FALSE;
		if( pSoldier &&
			gWorldSectorX == pSoldier->sSectorX && 
			gWorldSectorY == pSoldier->sSectorY && 
			gbWorldSectorZ == pSoldier->bSectorZ )
		{
			fMercInCurrentSector = TRUE;
		}

		// merc must be alive, not traveling and awake. If he is in the currently loaded sector, we may not be in tactical (we would see an item suddenly disappearing) and not in combat
		if( pSoldier &&
			!pSoldier->flags.fBetweenSectors && 
			pSoldier->bActive && 
			!pSoldier->flags.fMercAsleep && 
			pSoldier->bAssignment != IN_TRANSIT && 
			pSoldier->bAssignment != ASSIGNMENT_POW &&
			!( fMercInCurrentSector && (gTacticalStatus.fEnemyInSector || guiCurrentScreen == GAME_SCREEN ))  )
		{
			if ( pSoldier->HasBackgroundFlag( BACKGROUND_SCROUNGING ) )
			{
				// 50% chance that we'll steal something
				ubBaseChance = 50;

				// sevenfm: much less chance if soldier is busy with something
				if( pSoldier->bAssignment >= ON_DUTY )
				{
					ubBaseChance = ubBaseChance / 10;
				}
				
				if ( Chance(ubBaseChance) )
				{
					// anv: snitches prevent scrounging in the same sector
					BOOLEAN fSnitchStoppedBehaviour = FALSE;
					for( INT32 cnt2 = gTacticalStatus.Team[ OUR_TEAM ].bFirstID; cnt2 <= gTacticalStatus.Team[ OUR_TEAM ].bLastID; ++cnt2 )
					{					
						pOtherSoldier = MercPtrs[ cnt2 ];
						// note - snitches stop others, but can scrounge themselves (if they have scrounging specifically set in background...)
						if( pOtherSoldier && 
							!pOtherSoldier->flags.fBetweenSectors && 
							pOtherSoldier->bAssignment != IN_TRANSIT &&
							pOtherSoldier->bAssignment != ASSIGNMENT_POW &&
							pOtherSoldier->bActive &&							
							pSoldier->ubProfile != pOtherSoldier->ubProfile &&
							pSoldier->sSectorX == pOtherSoldier->sSectorX && 
							pSoldier->sSectorY == pOtherSoldier->sSectorY && 
							pSoldier->bSectorZ == pOtherSoldier->bSectorZ )
						{
							if (ProfileHasSkillTrait(pOtherSoldier->ubProfile, SNITCH_NT) && !(pOtherSoldier->usSoldierFlagMask2 & SOLDIER_PREVENT_MISBEHAVIOUR_OFF) ||
								ProfileHasSkillTrait(pOtherSoldier->ubProfile, SQUADLEADER_NT))
							{
								UINT16 bPreventChance;

								// sevenfm: snitch always prevents misbehavior if he is not sleeping
								if( !pOtherSoldier->flags.fMercAsleep )
								{
									bPreventChance = 100;
								}
								else
								{
									// sevenfm: base chance is 50-100%
									bPreventChance = 50 + ( EffectiveLeadership( pOtherSoldier ) / 2 + EffectiveExpLevel( pOtherSoldier, FALSE ) * 5 ) / 2;
									if (gGameOptions.fNewTraitSystem)
									{
										//bPreventChance += 25 * NUM_SKILL_TRAITS( pOtherSoldier, SQUADLEADER_NT );
										bPreventChance -= 25 * NUM_SKILL_TRAITS( pSoldier, STEALTHY_NT );
									}
									else
									{
										bPreventChance -= 25 * NUM_SKILL_TRAITS( pSoldier, STEALTHY_OT );
									}
								}

								bPreventChance = max(0, bPreventChance);
								bPreventChance = min(100, bPreventChance);

								if( bPreventChance > Random( 100 ) )
								{
									// merc is not amused by being prevented
									HandleMoraleEvent( pSoldier, MORALE_PREVENTED_MISBEHAVIOUR, pSoldier->sSectorX, pSoldier->sSectorX, pSoldier->bSectorZ );
									// also here would be a place for dynamic relationship decrease between them

									fSnitchStoppedBehaviour = TRUE;

									continue;
								}
							}
						}
					}

					if ( fSnitchStoppedBehaviour )
						continue;

					// we loop over this sector's inventory and look for something shiny. We will pick it up if we have enough space in our inventory
					// open sector inv
					UINT32 uiTotalNumberOfRealItems = 0;
					std::vector<WORLDITEM> pWorldItem;//dnl ch75 271013
					BOOLEAN fReturn					= FALSE;

					if( fMercInCurrentSector )
					{
						uiTotalNumberOfRealItems = guiNumWorldItems;

						if ( uiTotalNumberOfRealItems > 0 )
						{
							pWorldItem = gWorldItems;
						}
					}
					else
					{
						// not loaded, load
						// get total number, visible and invisible
						fReturn = GetNumberOfWorldItemsFromTempItemFile( pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ, &uiTotalNumberOfRealItems, FALSE );
						Assert( fReturn );

						if( uiTotalNumberOfRealItems > 0 )
						{
							// allocate space for the list
							pWorldItem.resize(uiTotalNumberOfRealItems);//dnl ch75 271013							

							// now load into mem
							LoadWorldItemsFromTempItemFile(  pSoldier->sSectorX,  pSoldier->sSectorY, pSoldier->bSectorZ, pWorldItem );
						}
					}

					//FLOAT bestpriceweightratio = 50.0f;		// must have a value of at least 50$/100 gram
					OBJECTTYPE* pTargetObj = NULL;
					OBJECTTYPE* pObj;
					UINT16		usItem;
					UINT8		ubStealChance = 5;
					BOOLEAN		ubStealAttempts = 0;

					if( NightTime() )
					{
						ubStealChance = 20;
					}

					for( UINT32 uiCount = 0; uiCount < uiTotalNumberOfRealItems && ubStealAttempts < 3; ++uiCount )				// ... for all items in the world ...
					{
						if( pWorldItem[ uiCount ].fExists )										// ... if item exists ...
						{
							pObj = &(pWorldItem[ uiCount ].object);								// ... get pointer for this item ...							

							if ( pObj != NULL && pObj->exists() )								// ... if pointer is not obviously useless ...
							{
								usItem = pObj->usItem;

								// test whether item is reachable and usable by militia
								// sevenfm: allow stealing only invisible items
								if ( pWorldItem[ uiCount ].usFlags & WORLD_ITEM_REACHABLE &&
									 pWorldItem[ uiCount ].bVisible < 0 )
								{									
									// of course we prefer money above everything else
									// sevenfm: 10% chance to steal money
									if( Item[usItem].usItemClass == IC_MONEY )
									{
										ubStealAttempts++;

										if( Random(100) < ubStealChance )
										{
											pTargetObj = pObj;
											break;
										}										
									}
									// sevenfm: only allow stealing items of MISC type and value > 50$/100g
									else if( (Item[usItem].usItemClass == IC_MISC || Item[usItem].usItemClass == IC_MEDKIT || Item[usItem].usItemClass == IC_KIT || Item[usItem].usItemClass == IC_FACE ) &&
											Item[usItem].ubWeight > 0 && 
											Item[usItem].usPrice / Item[usItem].ubWeight >= 10 )
									{
										ubStealAttempts++;

										if( Random(100) < ubStealChance )
										{
											pTargetObj = pObj;
											break;
										}										
									}
								}
							}
						}
					}

					INT8 bTownId = GetTownIdForSector( pSoldier->sSectorX, pSoldier->sSectorY );
					INT16 sLoyaltyValue = 0;

					if ( pTargetObj )
					{
						if( Item[pTargetObj->usItem].usItemClass == IC_MONEY )
							sLoyaltyValue = (*pTargetObj)[0]->data.money.uiMoneyAmount;
						else
							sLoyaltyValue = Item[pTargetObj->usItem].usPrice;

						AutoPlaceObject( pSoldier, pTargetObj, TRUE );						
					}
					// not found money in sector, loyalty/10 chance to steal money in town sectors
					else if( bTownId != BLANK_SECTOR &&
							gfTownUsesLoyalty[ bTownId ] && 
							gTownLoyalty[ bTownId ].fStarted &&
							Random( 100 ) < (UINT32)(ubStealChance * gTownLoyalty[ bTownId ].ubRating / 100) )
					{
						UINT32 uiMoney = Random( 1000 );

						// less money in towns with low loyalty
						uiMoney = uiMoney * gTownLoyalty[ bTownId ].ubRating / 100;

						if( uiMoney >= 10 )
						{
							OBJECTTYPE TempObj;
							if( CreateMoney(uiMoney, &TempObj ) )
							{
								sLoyaltyValue = uiMoney;
								AutoPlaceObject(pSoldier, &TempObj, true);
							}							
						}
					}

					// sevenfm: penalty to town loyalty						
					if( bTownId != BLANK_SECTOR && 
						sLoyaltyValue > 100 &&
						gfTownUsesLoyalty[ bTownId ] && 
						gTownLoyalty[ bTownId ].fStarted )
					{
						DecrementTownLoyalty( bTownId, GAIN_PTS_PER_LOYALTY_PT * min(sLoyaltyValue, 1000) / 100 );
					}

					// for stealing something valuable - global penalty
					if( sLoyaltyValue >= 1000 )
					{
						HandleGlobalLoyaltyEvent( GLOBAL_LOYALTY_NATIVE_KILLED, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
					}

					// save the changed inventory
					// open sector inv
					// sevenfm: only if we used sector items
					if( uiTotalNumberOfRealItems > 0 )
					{
						if( fMercInCurrentSector )
						{
							guiNumWorldItems = uiTotalNumberOfRealItems;
							gWorldItems = pWorldItem;
						}
						else
						{
							//Save the Items to the the file
							SaveWorldItemsToTempItemFile( pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ, uiTotalNumberOfRealItems, pWorldItem );
						}
					}					
				}				
			}
		}
	}
}

#ifdef JA2UB
// no JA25 UB
#else 
void HourlyCheckIfSlayAloneSoHeCanLeave()
{
	// sevenfm: r8404
	if (gGameExternalOptions.fEnableSlayForever) 
	{
		return;
	}

	SOLDIERTYPE *pSoldier;
	pSoldier = FindSoldierByProfileID( SLAY, TRUE );
	if( !pSoldier )
	{
		return;
	}
	if( pSoldier->flags.fBetweenSectors )
	{
		return;
	}
	if( !pSoldier->bActive || !pSoldier->stats.bLife )
	{
		return;
	}
	if( PlayerMercsInSector( (UINT8)pSoldier->sSectorX, (UINT8)pSoldier->sSectorY, pSoldier->bSectorZ ) == 1 )
	{
		if( Chance( 15 ) )
		{
			pSoldier->ubLeaveHistoryCode = HISTORY_SLAY_MYSTERIOUSLY_LEFT;
			TacticalCharacterDialogueWithSpecialEvent( pSoldier, 0, DIALOGUE_SPECIAL_EVENT_CONTRACT_ENDING_NO_ASK_EQUIP, 0, 0 );
		}
	}
}
#endif

#ifdef JA2UB
// no JA25 UB
#else 
void HourlyHelicopterRepair()
{
	if( gubHelicopterHoursToRepair == 0 )
	{
		return;
	}
	MERCPROFILESTRUCT *pMechanic;
	pMechanic = &(gMercProfiles[ WALDO ]);
	if( !pMechanic )
	{
		return;
	}
	if( !pMechanic->bLife )
	{
		return;
	}
	// sevenfm: r8399
	if( pVehicleList[ iHelicopterVehicleId ].sSectorX == gMercProfiles[ WALDO ].sSectorX && pVehicleList[ iHelicopterVehicleId ].sSectorY == gMercProfiles[ WALDO ].sSectorY )
	{
		gubHelicopterHoursToRepair--;
	}
	if( gubHelicopterHoursToRepair == 0 )
	{
		FinishHelicopterRepair();
	}
}
#endif

void HourlyGatheringInformation()
{
	SOLDIERTYPE *pSoldier;
	INT32 cnt=0;

	// go through list of characters, find all snicthes gathering info
	for ( pSoldier = MercPtrs[ cnt ]; cnt <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID; cnt++,pSoldier++)
	{
		if( ( pSoldier->bActive ) && ( pSoldier->bAssignment == SNITCH_GATHER_RUMOURS || pSoldier->bAssignment == FACILITY_GATHER_RUMOURS ) )
		{
			HandleGatheringInformationBySoldier( pSoldier );
		}
	}
}

