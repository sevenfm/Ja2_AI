#ifdef PRECOMPILEDHEADERS
#include "AI All.h"
#else
#include "ai.h"
#include "Weapons.h"
#include "opplist.h"
#include "Points.h"
#include "PathAI.h"
#include "WorldMan.h"
#include "AIInternals.h"
#include "Items.h"
#include "message.h"
#include "los.h"
#include "assignments.h"
#include "Soldier Functions.h"
#include "Points.h"
#include "GameSettings.h"
#include "Buildings.h"
#include "Soldier macros.h"
#include "Render Fun.h"
#include "strategicmap.h"
#include "environment.h"
#include "lighting.h"
#include "Soldier Create.h"
#include "SkillCheck.h"		// added by SANDRO
#include "Vehicles.h"		// added by silversurfer

// sevenfm:
#include "Game Clock.h"
#include "Rotting Corpses.h"
#include "wcheck.h"
#include "Drugs And Alcohol.h"
#include "Sound Control.h"
#include "SmokeEffects.h"
#include "strategicmap.h"
#include "interface.h"
#include "Structure Wrap.h"
#include "Keys.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// SANDRO - In this file, all APBPConstants[AP_CROUCH] and APBPConstants[AP_PRONE] were changed to GetAPsCrouch() and GetAPsProne()
//			On the bottom here, there are these functions made
//////////////////////////////////////////////////////////////////////

//
// CJC's DG->JA2 conversion notes
//
// Commented out:
//
// InWaterOrGas - gas stuff
// RoamingRange - point patrol stuff

// sevenfm
extern UINT16 PickSoldierReadyAnimation( SOLDIERTYPE *pSoldier, BOOLEAN fEndReady, BOOLEAN fHipStance );
extern SECTOR_EXT_DATA	SectorExternalData[256][4];

std::map<std::pair<INT32, INT8>, UINT8, std::less<>> gSpotDangerCache;
std::map<std::pair<INT32, INT8>, UINT8, std::less<>> gPathCoverCache;
std::map<CoverCacheInfo, BOOLEAN, std::less<>> gCoverCache;

bool operator<(const CoverCacheInfo& first, const CoverCacheInfo& second)
{
	if (first.sSpot < second.sSpot)
		return true;
	else if (first.sSpot > second.sSpot)
		return false;

	if (first.bLevel < second.bLevel)
		return true;
	else if (first.bLevel > second.bLevel)
		return false;

	if (first.ubType < second.ubType)
		return true;
	else if (first.ubType > second.ubType)
		return false;

	if (first.fUnlimited < second.fUnlimited)
		return true;
	else if (first.fUnlimited > second.fUnlimited)
		return false;

	if (first.fAdjacent < second.fAdjacent)
		return true;
	else if (first.fAdjacent > second.fAdjacent)
		return false;

	return false;
}

//rain
extern INT8 gbCurrentRainIntensity;
extern BOOLEAN gfLightningInProgress;
extern BOOLEAN gfHaveSeenSomeone;
extern UINT8 ubRealAmbientLightLevel;
//end rain

UINT8 Urgency[NUM_STATUS_STATES][NUM_MORALE_STATES] =
{
	{URGENCY_LOW,  URGENCY_LOW,  URGENCY_LOW,  URGENCY_LOW,  URGENCY_LOW}, // green
	{URGENCY_HIGH, URGENCY_MED,  URGENCY_MED,  URGENCY_LOW,  URGENCY_LOW}, // yellow
	{URGENCY_HIGH, URGENCY_MED,  URGENCY_MED,  URGENCY_MED,  URGENCY_MED}, // red
	{URGENCY_HIGH, URGENCY_HIGH, URGENCY_HIGH, URGENCY_MED,  URGENCY_MED}  // black
};

UINT16 MovementMode[LAST_MOVEMENT_ACTION + 1][NUM_URGENCY_STATES] =
{
	{WALKING,	 WALKING,  WALKING }, // AI_ACTION_NONE

	{WALKING,  WALKING,  WALKING }, // AI_ACTION_RANDOM_PATROL
	{WALKING,  RUNNING,  RUNNING }, // AI_ACTION_SEEK_FRIEND
	{WALKING,  RUNNING,  RUNNING }, // AI_ACTION_SEEK_OPPONENT
	{RUNNING,  RUNNING,  RUNNING }, // AI_ACTION_TAKE_COVER
	{WALKING,  RUNNING,  RUNNING }, // AI_ACTION_GET_CLOSER

	{WALKING,  WALKING,  WALKING }, // AI_ACTION_POINT_PATROL,
	{WALKING,  RUNNING,  RUNNING }, // AI_ACTION_LEAVE_WATER_GAS,
	{WALKING,  SWATTING,  RUNNING }, // AI_ACTION_SEEK_NOISE,
	{RUNNING,  RUNNING,  RUNNING }, // AI_ACTION_ESCORTED_MOVE,
	{WALKING,  RUNNING,  RUNNING }, // AI_ACTION_RUN_AWAY,

	{RUNNING,  RUNNING,  RUNNING }, // AI_ACTION_KNIFE_MOVE
	{WALKING,  WALKING,  WALKING }, // AI_ACTION_APPROACH_MERC
	{RUNNING,  RUNNING,  RUNNING }, // AI_ACTION_TRACK
	{RUNNING,	 RUNNING,  RUNNING },	// AI_ACTION_EAT 
	{WALKING,	 RUNNING,  RUNNING},	// AI_ACTION_PICKUP_ITEM

	{WALKING,	 WALKING,  WALKING},	// AI_ACTION_SCHEDULE_MOVE
	{WALKING,	 WALKING,  WALKING},	// AI_ACTION_WALK
	{WALKING,	 RUNNING,  RUNNING},	// withdraw
	{RUNNING,	 RUNNING,  SWATTING},	// flank left
	{RUNNING,	 RUNNING,  SWATTING},	// flank right
	{RUNNING,	 RUNNING,  RUNNING},	// AI_ACTION_MOVE_TO_CLIMB
};

INT8 OKToAttack(SOLDIERTYPE * pSoldier, int target)
{
	DebugAI( AI_MSG_INFO, pSoldier, String("---OKToAttack [%d] target %d", pSoldier->ubID, target));

	// can't shoot yourself
	if (target == pSoldier->sGridNo)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_MYSELF"));
		return(NOSHOOT_MYSELF);
	}

	if (WaterTooDeepForAttacks(pSoldier->sGridNo, pSoldier->pathing.bLevel))
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_WATER"));
		return(NOSHOOT_WATER);
	}

	// make sure a weapon is in hand (FEB.8 ADDITION: tossable items are also OK)
	if (!WeaponInHand(pSoldier))
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---no weapon in hand! NOSHOOT_NOWEAPON"));
		return(NOSHOOT_NOWEAPON);
	}

	// JUST PUT THIS IN ON JULY 13 TO TRY AND FIX OUT-OF-AMMO SITUATIONS

	if ( Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_GUN)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---found gun in a hand"));

		if ( Item[pSoldier->inv[HANDPOS].usItem].cannon )
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("---cannon"));

			// look for another tank shell ELSEWHERE IN INVENTORY
			if ( FindNonSmokeLaunchable( pSoldier, pSoldier->inv[HANDPOS].usItem ) == NO_SLOT )
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_NOLOAD"));
				return(NOSHOOT_NOLOAD);
			}
		}
		else if (pSoldier->inv[HANDPOS][0]->data.gun.ubGunShotsLeft == 0 || 
				!(pSoldier->inv[HANDPOS][0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) ||
				(pSoldier->IsValidSecondHandShotForReloadingPurposes( ) && (pSoldier->inv[SECONDHANDPOS][0]->data.gun.ubGunShotsLeft == 0 || !(pSoldier->inv[SECONDHANDPOS][0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER))))
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_NOAMMO"));
			return(NOSHOOT_NOAMMO);
		}
	}
	else if (Item[pSoldier->inv[HANDPOS].usItem].usItemClass == IC_LAUNCHER)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---launcher"));

		if ( FindNonSmokeLaunchable( pSoldier, pSoldier->inv[HANDPOS].usItem ) == NO_SLOT )
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_NOLOAD"));
			return(NOSHOOT_NOLOAD);
		}
	}

	DebugAI( AI_MSG_INFO, pSoldier, String("---return TRUE"));
	return(TRUE);
}

BOOLEAN ConsiderCrouch(SOLDIERTYPE * pSoldier)
{
	INT32 sOpponentGridNo;
	INT8 bOpponentLevel;
	INT32 sClosestOpponent = ClosestKnownOpponent(pSoldier, &sOpponentGridNo, &bOpponentLevel);

	if (TileIsOutOfBounds(sOpponentGridNo))
		return FALSE;

	if (!AICheckHasGun(pSoldier))
		return FALSE;

	if (pSoldier->aiData.bUnderFire || CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE / 4))
		return TRUE;

	if (pSoldier->RushAttackAdvance() || pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE])
		return FALSE;

	if (!TileIsOutOfBounds(sOpponentGridNo) &&
		PythSpacesAway(pSoldier->sGridNo, sOpponentGridNo) > MAX_VISION_RANGE)
		return FALSE;

	if (SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) && 
		!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, FALSE))
		return FALSE;

	return TRUE;
}

BOOLEAN ConsiderProne( SOLDIERTYPE * pSoldier )
{
	INT32 sOpponentGridNo;
	INT8 bOpponentLevel;
	INT32 sClosestOpponent = ClosestKnownOpponent(pSoldier, &sOpponentGridNo, &bOpponentLevel);

	if (RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		(TileIsOutOfBounds(sOpponentGridNo) || PythSpacesAway(pSoldier->sGridNo, sOpponentGridNo) > DAY_VISION_RANGE / 4))
	{
		return TRUE;
	}

	// if have no weapon - no prone
	if ( !AICheckHasGun(pSoldier) )
	{
		return FALSE;
	}

	// don't go prone when under adrenaline
	if (pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] && !pSoldier->aiData.bUnderFire)
	{
		return FALSE;
	}

	if (pSoldier->ShockLevelPercent() < 5 * RangeChangeDesire(pSoldier) &&
		!pSoldier->aiData.bUnderFire &&
		CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE / 4) == 0 &&
		pSoldier->stats.bLife > 3*pSoldier->stats.bLifeMax/4 && 
		(	RangeChangeDesire(pSoldier) > 4 || 
			pSoldier->LastAttackHit() ||
			pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR ||
			pSoldier->ubSoldierClass == SOLDIER_CLASS_GREEN_MILITIA	)	)
	{
		return( FALSE );
	}

	// We don't want to go prone if there is a nearby enemy
	if (!TileIsOutOfBounds(sOpponentGridNo) && 
		PythSpacesAway(pSoldier->sGridNo, sOpponentGridNo) < TACTICAL_RANGE / 2)
	{
		return( FALSE );
	}

	return( TRUE );
}


UINT16 DetermineMovementMode(SOLDIERTYPE * pSoldier, INT8 bAction, BOOLEAN fReverse)
{
	// always return RUNNING for player mercs
	if( pSoldier->IsSoldierPC() && pSoldier->IsMercBodyType() )
	{
		return RUNNING;
	}

	// zombies always run if they know enemy location
	if (pSoldier->IsZombie() && pSoldier->IsMercBodyType())
	{
		INT32 sClosestThreat =	ClosestKnownOpponent( pSoldier, NULL, NULL );

		if( !TileIsOutOfBounds(sClosestThreat) )
		{
			return RUNNING;
		}
		else
		{
			return WALKING;
		}
	}

	if ( pSoldier->flags.fUIMovementFast )
	{
		return( RUNNING );
	}
	else if ( CREATURE_OR_BLOODCAT( pSoldier ) )
	{
		if (pSoldier->aiData.bAlertStatus == STATUS_GREEN)
		{
			return( WALKING );
		}
		else
		{
			return( RUNNING );
		}
	}
	else if (pSoldier->ubBodyType == COW || pSoldier->ubBodyType == CROW)
	{
		return( WALKING );
	}
	else
	{
		// sevenfm: movement mode when deciding reverse movement
		if (pSoldier->bReverse || fReverse)
		{
			switch (pSoldier->AnimEndHeight())
			{
			case ANIM_STAND:
				return WALKING;
			case ANIM_CROUCH:
				return SWATTING;
			case ANIM_PRONE:
				return CRAWLING;
			default: return WALKING;
			}
		}
		else if ( (pSoldier->aiData.ubAIFlags & AI_CAUTIOUS) )
		{
			// if soldier is already crouched/prone, use SWATTING
			if (pSoldier->IsMercBodyType() &&
				(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM) &&
				gAnimControl[ pSoldier->usAnimState ].ubEndHeight <= ANIM_CROUCH )
			{
				if (pSoldier->AnimEndHeight() == ANIM_PRONE &&
					CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0)
					return CRAWLING;

				return SWATTING;
			}
			// use WALKING instead of RUNNING
			if( MovementMode[bAction][Urgency[pSoldier->aiData.bAlertStatus][pSoldier->aiData.bAIMorale]] == RUNNING )
			{
				return( WALKING );
			}

			// if not RUNNING, use default movement mode
			return MovementMode[bAction][Urgency[pSoldier->aiData.bAlertStatus][pSoldier->aiData.bAIMorale]];
		}
		else if ( bAction == AI_ACTION_SEEK_NOISE && pSoldier->bTeam == CIV_TEAM && !IS_MERC_BODY_TYPE( pSoldier ) )
		{
			return( WALKING );
		}
		else if ( (pSoldier->ubBodyType == HATKIDCIV || pSoldier->ubBodyType == KIDCIV) && (pSoldier->aiData.bAlertStatus == STATUS_GREEN) && Random( 10 ) == 0 )
		{
			return( KID_SKIPPING );
		}
		else
		{
			// sevenfm: movement mode tweaks
			INT32 sClosestThreat =	ClosestKnownOpponent( pSoldier, NULL, NULL );			

			// use walking mode if no enemy known
			if( pSoldier->aiData.bAlertStatus < STATUS_RED &&
				TileIsOutOfBounds(sClosestThreat) &&
				!pSoldier->aiData.bUnderFire &&
				(bAction == AI_ACTION_SEEK_FRIEND || bAction == AI_ACTION_SEEK_NOISE || bAction == AI_ACTION_TAKE_COVER) )
			{
				return WALKING;
			}

			// use swatting when blinded
			if (pSoldier->IsMercBodyType() &&
				pSoldier->bBlindedCounter > 0)
			{
				if (pSoldier->AnimEndHeight() == ANIM_PRONE &&
					CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0)
					return CRAWLING;
				else
					return SWATTING;
			}

			if (pSoldier->IsMercBodyType() &&
				pSoldier->aiData.bAlertStatus >= STATUS_RED &&
				!InWaterOrGas( pSoldier, pSoldier->sGridNo ) &&
				!(pSoldier->IsBoxer()) &&
				!TileIsOutOfBounds(sClosestThreat) &&
				(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM))
			{
				// determine max visible distance
				//INT16 sDistanceVisible = DistanceVisible( pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, pSoldier->sGridNo, pSoldier->pathing.bLevel );
				INT16 sDistanceVisible = VISION_RANGE;
				BOOLEAN fSawEnemyRecently = GuySawEnemy(pSoldier);
				BOOLEAN fHeardEnemyRecently = GuyHeardEnemy(pSoldier);
				UINT8 ubFriendsNearby = CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4);
				UINT8 ubFriendsBlack = CountFriendsBlack(pSoldier);
				INT16 sClosestThreatDistance = PythSpacesAway(pSoldier->sGridNo, sClosestThreat);
				UINT8 ubStance = pSoldier->AnimEndHeight();
				INT32 iRCD = RangeChangeDesire(pSoldier);
				INT8 bAIMorale = pSoldier->aiData.bAIMorale;
				INT8 bAttitude = pSoldier->aiData.bAttitude;
				INT8 bOrders = pSoldier->aiData.bOrders;
				BOOLEAN fSuccessfulAttack = AICheckSuccessfulAttack(pSoldier, TRUE);
				BOOLEAN fWeOutnumberPublic = AICheckWeOutnumberPublic(pSoldier, sClosestThreat);
				BOOLEAN fUnderAttack = (pSoldier->ShockLevelPercent() > 25);
				BOOLEAN fUnderHeavyAttack = (pSoldier->ShockLevelPercent() > 50);
				BOOLEAN fUnderFire = pSoldier->aiData.bUnderFire;
				BOOLEAN fSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE);
				BOOLEAN fUnlimitedSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE);
				BOOLEAN fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
				BOOLEAN fUnlimitedProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE);
				BOOLEAN fCrouchedSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_CROUCH, FALSE);
				BOOLEAN fUnlimitedCrouchedSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_CROUCH, TRUE);
				UINT8	ubFriendsUnderAttack = CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE / 4);
				BOOLEAN fSeekEnemy = (pSoldier->aiData.bOrders == SEEKENEMY);
				BOOLEAN fCorpseWarning = CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);
				UINT8 ubCorpsesNearby = CountCorpses(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 2, FALSE);
				BOOLEAN fSectorAttack = SectorAttack(pSoldier->bTeam);
				BOOLEAN fAdrenaline = (pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] > 0);
				BOOLEAN fStationary = (bOrders == STATIONARY);
				BOOLEAN fRoom = InARoom(pSoldier->sGridNo, NULL) && fProneSightCover;
				BOOLEAN fOnRoof = (pSoldier->pathing.bLevel > 0);
				UINT8 ubTeamPercentKilled = TeamPercentKilled(pSoldier->bTeam);

				INT16 sStartAP, sEndAP;
				INT16 sCrawlMoveAP, sSwatMoveAP, sRunMoveAP;
				INT16 sCrawlTiles, sSwatTiles, sRunTiles;
				// check if moving in current stance is more effective than running
				if( ubStance == ANIM_PRONE )
				{
					// crawl
					sStartAP = 0;
					sEndAP = 0;
					sCrawlMoveAP = APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_CRAWL];
					sCrawlTiles = (pSoldier->bActionPoints - sStartAP - sEndAP) / sCrawlMoveAP;

					// swat
					sStartAP = GetAPsProne(pSoldier, TRUE);
					sEndAP = GetAPsProne(pSoldier, TRUE);
					sSwatMoveAP = APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_SWAT];
					sSwatTiles = (pSoldier->bActionPoints - sStartAP - sEndAP) / sSwatMoveAP;

					// run
					sStartAP = GetAPsProne(pSoldier, TRUE) + GetAPsCrouch(pSoldier, TRUE) + APBPConstants[AP_MODIFIER_RUN];
					sEndAP = GetAPsCrouch(pSoldier, TRUE);
					sRunMoveAP = APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_RUN];
					sRunTiles = (pSoldier->bActionPoints - sStartAP - sEndAP) / sRunMoveAP;

					if (sCrawlTiles > 0 && sCrawlTiles >= sSwatTiles && sCrawlTiles >= sRunTiles)
					{
						if (sClosestThreatDistance > TACTICAL_RANGE / 4)// && CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0)
							return CRAWLING;
						else
							return SWATTING;
					}
					if (sSwatTiles > 0 && sSwatTiles >= sCrawlTiles && sSwatTiles >= sRunTiles)
					{
						return SWATTING;
					}
				}
				else if ( ubStance == ANIM_CROUCH )
				{
					// swat
					sStartAP = 0;
					sEndAP = 0;
					sSwatMoveAP = APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_SWAT];
					sSwatTiles = (pSoldier->bActionPoints - sStartAP - sEndAP) / sSwatMoveAP;

					// run
					sStartAP = GetAPsCrouch(pSoldier, TRUE) + APBPConstants[AP_MODIFIER_RUN];
					sEndAP = GetAPsCrouch(pSoldier, TRUE);
					sRunMoveAP = APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_RUN];
					sRunTiles = (pSoldier->bActionPoints - sStartAP - sEndAP) / sRunMoveAP;

					if (sSwatTiles > 0 && sSwatTiles >= sRunTiles)
					{
						return SWATTING;
					}
				}

				// always use running when under adrenaline
				if (fAdrenaline &&
					!fStationary &&
					(bAction == AI_ACTION_SEEK_OPPONENT || 
					bAction == AI_ACTION_GET_CLOSER ||
					bAction == AI_ACTION_SEEK_FRIEND ||
					bAction == AI_ACTION_TAKE_COVER) &&
					pSoldier->bBreath > 25 &&
					ubStance > ANIM_PRONE)
				{
					return RUNNING;
				}

				// rush attack
				if( pSoldier->RushAttackAdvance() &&
					!fStationary &&
					(bAction == AI_ACTION_SEEK_OPPONENT || 
					bAction == AI_ACTION_GET_CLOSER ||
					bAction == AI_ACTION_TAKE_COVER) &&
					pSoldier->bBreath > 25 &&
					!pSoldier->TakenLargeHit() &&
					!fUnderHeavyAttack &&
					(ubStance > ANIM_PRONE || pSoldier->bActionPoints > APBPConstants[AP_MINIMUM]) )
				{
					return RUNNING;
				}

				// use crawling/swatting when on roof on in a room with STATIONARY/ONGUARD orders
				if ((bOrders == STATIONARY || bOrders == ONGUARD || AIRoleSniper(pSoldier) || bAIMorale < MORALE_FEARLESS && fProneSightCover) &&
					sClosestThreatDistance < TACTICAL_RANGE * 2 &&
					(bAction == AI_ACTION_SEEK_OPPONENT ||
					bAction == AI_ACTION_GET_CLOSER ||
					bAction == AI_ACTION_SEEK_FRIEND ||
					bAction == AI_ACTION_TAKE_COVER ||
					bAction == AI_ACTION_PICKUP_ITEM) &&
					(fUnderFire || fSawEnemyRecently || fHeardEnemyRecently || ubFriendsUnderAttack > 0 || fCorpseWarning) &&
					(fOnRoof || fRoom))
				{
					if (ubStance == ANIM_PRONE &&
						fProneSightCover &&
						sClosestThreatDistance > TACTICAL_RANGE / 8 &&
						(fOnRoof || CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0))
						return CRAWLING;
					else
						return SWATTING;
				}

				// use running when in light at night (moving slowly in light at night is dangerous)
				if (NightLight() &&
					InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
					!fStationary &&
					!fRoom &&
					!fOnRoof &&
					(bAction == AI_ACTION_SEEK_OPPONENT ||
					bAction == AI_ACTION_GET_CLOSER ||
					bAction == AI_ACTION_SEEK_FRIEND ||
					bAction == AI_ACTION_TAKE_COVER))
				{
					return RUNNING;
				}

				// use swatting\crawling when approaching enemy
				if ((NightLight() || 
					RainIntensity() > 0 || 
					ubCorpsesNearby > ubFriendsNearby ||
					ubTeamPercentKilled > 25 ||
					!TeamSawEnemy(pSoldier->bTeam, SEEN_3_TURNS_AGO) ||
					TerrainDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, 5, TRUE) >= 20) &&
					!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
					!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) && 
					!pSoldier->aiData.bUnderFire &&
					!AICheckUnderground() &&
					!InARoom(pSoldier->sGridNo, NULL) &&
					!pSoldier->RushAttackAdvance() &&
					!fAdrenaline &&
					(!fSectorAttack || !fWeOutnumberPublic) &&
					sClosestThreatDistance <= (INT16)(TACTICAL_RANGE * 2) &&
					fProneSightCover &&
					!fUnlimitedSightCover &&
					//!fSightCover &&
					pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					!GuySawEnemy(pSoldier, SEEN_THIS_TURN) &&
					ubFriendsBlack == 0 &&
					!fSuccessfulAttack &&
					(ubCorpsesNearby > 0 || ubTeamPercentKilled > 25 || bAIMorale < MORALE_FEARLESS || !TeamSawEnemy(pSoldier->bTeam, SEEN_3_TURNS_AGO)) &&
					(bAction == AI_ACTION_SEEK_OPPONENT ||
					bAction == AI_ACTION_GET_CLOSER ||
					bAction == AI_ACTION_TAKE_COVER))
				{
					if (fProneSightCover &&
						(bAIMorale < MORALE_FEARLESS || ubTeamPercentKilled > 50) &&
						sClosestThreatDistance > TACTICAL_RANGE / 4 &&
						(ubStance <= ANIM_CROUCH || !gfTurnBasedAI) &&
						(ubStance == ANIM_PRONE || IsValidStance(pSoldier, ANIM_PRONE)) &&
						(bAction == AI_ACTION_GET_CLOSER ||	bAction == AI_ACTION_TAKE_COVER) &&	// no crawling for SEEK_OPPONENT
						(bAction == AI_ACTION_TAKE_COVER || CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel)) == 0)
					{
						return CRAWLING;
					}
					return SWATTING;
				}

				// use swatting/crawling for covert approach
				// (more chance for AI to sneak unseen and get interrupt)
				if( !InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
					pSoldier->aiData.bAlertStatus == STATUS_RED &&
					!fSawEnemyRecently &&
					!pSoldier->aiData.bUnderFire &&					
					fProneSightCover &&
					!fUnlimitedSightCover &&
					!fAdrenaline &&
					!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
					!pSoldier->RushAttackAdvance() &&
					ubFriendsNearby < 3 &&
					ubFriendsUnderAttack == 0 &&
					(!fSectorAttack || !fWeOutnumberPublic) &&
					sClosestThreatDistance <= (INT16)(TACTICAL_RANGE * 2) &&
					ubFriendsBlack == 0 &&
					!fSuccessfulAttack &&
					bAIMorale < MORALE_FEARLESS &&
					pSoldier->ubSoldierClass != SOLDIER_CLASS_ADMINISTRATOR &&
					pSoldier->ubSoldierClass != SOLDIER_CLASS_GREEN_MILITIA &&
					(pSoldier->pathing.bLevel > 0 || 
					NightLight() || 
					pSoldier->bStealthMode || 
					ubTeamPercentKilled > 25 ||
					RainIntensity() > 0 ||
					pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_APPROACH ||
					TerrainDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, 5, TRUE) >= 20 ||
					pSoldier->HighWoodCamo() && TerrainJungleDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
					pSoldier->HighDesertCamo() && TerrainDesertDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
					pSoldier->HighUrbanCamo() && TerrainUrbanDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
					pSoldier->HighSnowCamo() && TerrainSnowDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
					pSoldier->HighWornStealth() && TerrainDarknessDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30) &&
					(bAction == AI_ACTION_SEEK_OPPONENT || bAction == AI_ACTION_TAKE_COVER) )
				{
					if (fProneSightCover &&	
						sClosestThreatDistance > TACTICAL_RANGE / 4 &&						
						(!InARoom( pSoldier->sGridNo, NULL ) || pSoldier->pathing.bLevel > 0) &&
						(ubStance == ANIM_PRONE || !gfTurnBasedAI) &&
						(ubStance == ANIM_PRONE || IsValidStance(pSoldier, ANIM_PRONE)) &&
						(bAction == AI_ACTION_GET_CLOSER || bAction == AI_ACTION_TAKE_COVER) &&	// no crawling for SEEK_OPPONENT
						(bAction == AI_ACTION_TAKE_COVER || CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0))
						//LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestThreat, pSoldier->pathing.bLevel, NO_DISTANCE_LIMIT))
					{
						return CRAWLING;
					}

					return SWATTING;
				}

				// use SWATTING/CRAWLING when under fire
				// (use suppression fire to slow down enemy)
				if( pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					fUnderAttack && 
					sClosestThreatDistance > TACTICAL_RANGE / 8 &&
					sClosestThreatDistance < 2 * sDistanceVisible &&
					ubStance <= ANIM_CROUCH &&
					(!fAdrenaline || fUnderHeavyAttack)  &&
					!pSoldier->LastAttackHit() &&
					( bAction == AI_ACTION_SEEK_OPPONENT || 
					bAction == AI_ACTION_GET_CLOSER ||
					bAction == AI_ACTION_SEEK_FRIEND ||
					bAction == AI_ACTION_TAKE_COVER ) )
				{
					if( fUnderHeavyAttack &&
						ubStance == ANIM_PRONE &&
						!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
						(bAction == AI_ACTION_TAKE_COVER || CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0) &&
						(!InARoom( pSoldier->sGridNo, NULL ) || pSoldier->pathing.bLevel > 0) &&
						(bAction == AI_ACTION_GET_CLOSER || bAction == AI_ACTION_TAKE_COVER) &&	// no crawling for SEEK_OPPONENT
						sClosestThreatDistance > TACTICAL_RANGE / 2 )
					{
						return CRAWLING;
					}

					return SWATTING;
				}

				// use SWATTING when in a room and seen enemy recently or under fire
				// (better cover for AI hiding behind windows)
				if ((fRoom || fOnRoof) &&
					(fStationary || AIRoleSniper(pSoldier)) &&
					sClosestThreatDistance > TACTICAL_RANGE / 8 &&
					(bAction == AI_ACTION_TAKE_COVER || bAction == AI_ACTION_GET_CLOSER))
				{
					if ((fUnderFire || fSawEnemyRecently || !fSightCover || ubFriendsUnderAttack > 0 || fCorpseWarning) &&
						ubStance <= ANIM_CROUCH &&
						!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
						(ubStance == ANIM_PRONE || IsValidStance(pSoldier, ANIM_PRONE)) &&
						fProneSightCover &&
						(bAction == AI_ACTION_TAKE_COVER || CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0) &&
						sClosestThreatDistance > TACTICAL_RANGE / 4)
					{
						return CRAWLING;
					}

					return SWATTING;
				}

				// use SWATTING when in a room and seen enemy recently or under fire
				// (better cover for AI hiding behind windows)
				if ((fRoom || fOnRoof) &&
					pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					(fProneSightCover || sClosestThreatDistance > TACTICAL_RANGE / 2) &&
					(bAIMorale < MORALE_FEARLESS && fSawEnemyRecently || pSoldier->aiData.bShock > 0 || ubFriendsUnderAttack > 0 || fCorpseWarning) &&
					sClosestThreatDistance > TACTICAL_RANGE / 8 &&
					(bAction == AI_ACTION_TAKE_COVER || bAction == AI_ACTION_GET_CLOSER || bAction == AI_ACTION_SEEK_OPPONENT && !fSuccessfulAttack || bAction == AI_ACTION_SEEK_FRIEND))
				{
					if ((fUnderFire || fSawEnemyRecently || !fSightCover) &&
						!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
						bAIMorale < MORALE_FEARLESS &&
						(ubStance == ANIM_PRONE || ubStance == ANIM_CROUCH && (pSoldier->aiData.bUnderFire || ubFriendsUnderAttack > 0)) &&
						(ubStance == ANIM_PRONE || IsValidStance(pSoldier, ANIM_PRONE)) &&
						(bAction == AI_ACTION_TAKE_COVER || CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0) &&
						(bAction == AI_ACTION_GET_CLOSER || bAction == AI_ACTION_TAKE_COVER) &&	// no crawling for SEEK_OPPONENT or SEEK_FRIEND
						sClosestThreatDistance > TACTICAL_RANGE / 4)
					{
						return CRAWLING;
					}

					return SWATTING;
				}

				// use running for taking cover when not under attack
				if (!fUnderFire &&
					bAction == AI_ACTION_TAKE_COVER &&
					pSoldier->bInitialActionPoints > APBPConstants[AP_MINIMUM] &&
					(!fRoom || sClosestThreatDistance > TACTICAL_RANGE * 2) &&
					bAIMorale > MORALE_NORMAL &&
					pSoldier->bBreath > 25 &&
					pSoldier->pathing.bLevel == 0 &&
					!fStationary &&
					(ubStance > ANIM_PRONE || pSoldier->bActionPoints > APBPConstants[AP_MINIMUM]))
				{
					return RUNNING;
				}

				// decide movement mode when getting closer
				if (bAction == AI_ACTION_GET_CLOSER)
				{
					if (ubStance == ANIM_STAND)
					{
						if (WeaponReady(pSoldier) && !pSoldier->aiData.bUnderFire && pSoldier->aiData.bAlertStatus >= STATUS_RED)
							return WALKING;
						else
							return RUNNING;
					}
					else if (ubStance == ANIM_CROUCH)
					{
						if (fUnderAttack && sClosestThreatDistance > TACTICAL_RANGE / 8)
							return SWATTING;
						else
							return RUNNING;
					}
					else if (ubStance == ANIM_PRONE)
					{
						if (fUnderAttack && sClosestThreatDistance > TACTICAL_RANGE / 4 &&
							CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0 &&
							!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel))
							return CRAWLING;
						else if (pSoldier->aiData.bUnderFire && !pSoldier->LastAttackHit() && sClosestThreatDistance > TACTICAL_RANGE / 8)
							return SWATTING;
						else
							return RUNNING;
					}
				}

				// use walking/swatting when flanking in realtime
				// (better cover at night, save BP in realtime at daytime)
				if(pSoldier->IsFlanking() && !gfTurnBasedAI)
				{
					if( !gfTurnBasedAI &&
						sClosestThreatDistance < TACTICAL_RANGE * 3 / 2 &&
						!InARoom( pSoldier->sGridNo, NULL ) &&
						!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
						ubFriendsNearby < 3 &&
						bAIMorale < MORALE_FEARLESS &&
						(ubStance == ANIM_PRONE || IsValidStance(pSoldier, ANIM_PRONE)) &&
						CountNearbyFences(pSoldier->sGridNo, pSoldier->pathing.bLevel) == 0 &&
						!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
						(bAction == AI_ACTION_FLANK_LEFT || bAction == AI_ACTION_FLANK_RIGHT || bAction == AI_ACTION_GET_CLOSER) &&	// no crawling for SEEK_OPPONENT
						(TerrainDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, 2, TRUE) >= 20 || 
						NightLight() ||
						pSoldier->HighWoodCamo() && TerrainJungleDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
						pSoldier->HighDesertCamo() && TerrainDesertDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
						pSoldier->HighUrbanCamo() && TerrainUrbanDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
						pSoldier->HighSnowCamo() && TerrainSnowDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30 ||
						pSoldier->HighWornStealth() && TerrainDarknessDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, TACTICAL_RANGE / 4) > 30))
					{
						return CRAWLING;
					}
					if( !gfTurnBasedAI &&
						(NightLight() || TerrainDensity(pSoldier->sGridNo, pSoldier->pathing.bLevel, 2, TRUE) >= 20) && 
						!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
						sClosestThreatDistance < TACTICAL_RANGE * 3 / 2)
					{
						return SWATTING;
					}
					if(pSoldier->bBreath < pSoldier->bBreathMax/2)
					{
						return WALKING;
					}					
				}
			}

			return( MovementMode[bAction][Urgency[pSoldier->aiData.bAlertStatus][pSoldier->aiData.bAIMorale]] );
		}
	}
}

BOOLEAN CheckPathForObstacles(SOLDIERTYPE *pSoldier)
{
	INT16 sLoop;
	INT32 sCheckGridNo = pSoldier->sGridNo;
	UINT8 ubDirection;
	UINT8 ubCost;

	for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
	{
		ubDirection = (UINT8)pSoldier->pathing.usPathingData[sLoop];
		sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc(ubDirection));

		ubCost = gubWorldMovementCosts[sCheckGridNo][ubDirection][pSoldier->pathing.bLevel];

		if (ubCost == TRAVELCOST_FENCE ||
			ubCost >= TRAVELCOST_SHORE && ubCost <= TRAVELCOST_DEEPWATER ||
			ubCost == TRAVELCOST_DOOR ||
			ubCost >= TRAVELCOST_DOOR_CLOSED_HERE && ubCost <= TRAVELCOST_DOOR_OPEN_NW_W)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN CheckPathForGas(SOLDIERTYPE *pSoldier)
{
	INT16 sLoop;
	INT32 sCheckGridNo = pSoldier->sGridNo;
	UINT8 ubDirection;

	for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
	{
		ubDirection = (UINT8)pSoldier->pathing.usPathingData[sLoop];
		sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc(ubDirection));

		if (!TileIsOutOfBounds(sCheckGridNo) && InGasSpot(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel, FALSE))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN CheckNextPathForObstacles(SOLDIERTYPE *pSoldier)
{
	INT32 sCheckGridNo = pSoldier->sGridNo;
	UINT8 ubDirection;
	UINT8 ubCost;

	ubDirection = (UINT8)pSoldier->pathing.usPathingData[pSoldier->pathing.usPathIndex];
	sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc(ubDirection));

	ubCost = gubWorldMovementCosts[sCheckGridNo][ubDirection][pSoldier->pathing.bLevel];

	if (ubCost == TRAVELCOST_FENCE ||
		ubCost >= TRAVELCOST_SHORE && ubCost <= TRAVELCOST_DEEPWATER ||
		ubCost == TRAVELCOST_DOOR ||
		ubCost >= TRAVELCOST_DOOR_CLOSED_HERE && ubCost <= TRAVELCOST_DOOR_OPEN_NW_W)
	{
		return TRUE;
	}

	return FALSE;
}

void NewDest(SOLDIERTYPE *pSoldier, INT32 usGridNo)
{	
	// sevenfm: always use DetermineMovementMode
	pSoldier->usMovementMode = DetermineMovementMode(pSoldier, pSoldier->aiData.bAction);
	// check for non merc bodytypes
	if ((pSoldier->usMovementMode == SWATTING || pSoldier->usMovementMode == SWATTING_WK) && !IS_MERC_BODY_TYPE(pSoldier))
	{
		pSoldier->usMovementMode = WALKING;
	}
	DebugAI(AI_MSG_INFO, pSoldier, String("NewDest: movement mode %d action %d", pSoldier->usMovementMode, pSoldier->aiData.bAction));

	// sevenfm: check AI crawling
	if (!pSoldier->IsSoldierPC() && 
		pSoldier->usMovementMode == CRAWLING &&
		CheckPathForObstacles(pSoldier))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("NewDest: found obstacle, switch movement mode to SWATTING"));
		pSoldier->usMovementMode = SWATTING;
	}

	// sevenfm: check path for gas spots
	if (SoldierAI(pSoldier) &&
		!pSoldier->WearsGasMask() && 
		pSoldier->HasGasMask() &&
		CheckPathForGas(pSoldier))
	{
		WearGasMaskIfAvailable(pSoldier);
	}

	//pSoldier->EVENT_GetNewSoldierPath( pSoldier->pathing.sDestination, pSoldier->usMovementMode );
	// ATE: Using this more versatile version
	// Last parameter says whether to re-start the soldier's animation
	// This should be done if buddy was paused for fNoApstofinishMove...
	pSoldier->EVENT_InternalGetNewSoldierPath(usGridNo, pSoldier->usMovementMode, FALSE, pSoldier->flags.fNoAPToFinishMove);
}


BOOLEAN IsActionAffordable(SOLDIERTYPE *pSoldier, INT8 bAction)
{
	INT16	bMinPointsNeeded = 0;
	// sevenfm: r7972 fix
	INT8 bAPForStandUp = 0;
	INT8 bAPToLookAtWall = 0;

	//NumMessage("AffordableAction - Guy#",pSoldier->ubID);

	if( bAction == AI_ACTION_NONE )
	{
		bAction = pSoldier->aiData.bAction;
	}

	switch (bAction)
	{
	case AI_ACTION_NONE:                  // maintain current position & facing		
		// no cost for doing nothing!
		bMinPointsNeeded = 0;
		break;
	case AI_ACTION_CHANGE_FACING:         // turn to face another direction
		bMinPointsNeeded = (INT8) GetAPsToLook( pSoldier );
		break;
	case AI_ACTION_RANDOM_PATROL:         // move towards a particular location
	case AI_ACTION_SEEK_FRIEND:           // move towards friend in trouble
	case AI_ACTION_SEEK_OPPONENT:         // move towards a reported opponent
	case AI_ACTION_TAKE_COVER:            // run for nearest cover from threat
	case AI_ACTION_GET_CLOSER:            // move closer to a strategic location
	case AI_ACTION_POINT_PATROL:          // move towards next patrol point
	case AI_ACTION_LEAVE_WATER_GAS:       // seek nearest spot of ungassed land
	case AI_ACTION_SEEK_NOISE:            // seek most important noise heard
	case AI_ACTION_ESCORTED_MOVE:         // go where told to by escortPlayer
	case AI_ACTION_RUN_AWAY:              // run away from nearby opponent(s)
	case AI_ACTION_APPROACH_MERC:
	case AI_ACTION_TRACK:
	case AI_ACTION_EAT:
	case AI_ACTION_SCHEDULE_MOVE:
	case AI_ACTION_WALK:
	case AI_ACTION_MOVE_TO_CLIMB:
		// for movement, must have enough APs to move at least 1 tile's worth
		bMinPointsNeeded = MinPtsToMove(pSoldier);
		break;
	case AI_ACTION_PICKUP_ITEM:           // grab things lying on the ground
		bMinPointsNeeded = max( MinPtsToMove( pSoldier ), GetBasicAPsToPickupItem( pSoldier ) ); // SANDRO
		break;
	case AI_ACTION_OPEN_OR_CLOSE_DOOR:
	case AI_ACTION_UNLOCK_DOOR:
	case AI_ACTION_LOCK_DOOR:
		bMinPointsNeeded = MinPtsToMove(pSoldier);
		break;
	case AI_ACTION_DROP_ITEM:
		bMinPointsNeeded = GetBasicAPsToPickupItem( pSoldier ); // SANDRO
		break;
	case AI_ACTION_FIRE_GUN:              // shoot at nearby opponent
	case AI_ACTION_TOSS_PROJECTILE:       // throw grenade at/near opponent(s)
	case AI_ACTION_KNIFE_MOVE:            // preparing to stab adjacent opponent
	case AI_ACTION_THROW_KNIFE:
		// only FIRE_GUN currently actually pays extra turning costs!
		bMinPointsNeeded = MinAPsToAttack(pSoldier,pSoldier->aiData.usActionData,ADDTURNCOST,pSoldier->aiData.bAimTime);
#ifdef BETAVERSION
		if (ptsNeeded > pSoldier->bActionPoints)
		{
			/*
			sprintf(tempstr,"AI ERROR: %s has insufficient points for attack action %d at grid %d",
			pSoldier->name,pSoldier->aiData.bAction,pSoldier->aiData.usActionData);
			PopMessage(tempstr);
			*/
		}
#endif
		break;
	case AI_ACTION_PULL_TRIGGER:          // activate an adjacent panic trigger
		bMinPointsNeeded = APBPConstants[AP_PULL_TRIGGER];
		break;
	case AI_ACTION_USE_DETONATOR:         // grab detonator and set off bomb(s)
		bMinPointsNeeded = APBPConstants[AP_USE_REMOTE];
		break;
	case AI_ACTION_YELLOW_ALERT:		// tell friends opponent(s) heard
	case AI_ACTION_RED_ALERT:			// tell friends opponent(s) seen
	case AI_ACTION_CREATURE_CALL:		// for now
		bMinPointsNeeded = APBPConstants[AP_RADIO];
		break;
	case AI_ACTION_CHANGE_STANCE:                // crouch
		bMinPointsNeeded = GetAPsCrouch(pSoldier, TRUE);
		break;
	case AI_ACTION_GIVE_AID:              // help injured/dying friend
		bMinPointsNeeded = 0;
		break;
	case AI_ACTION_CLIMB_ROOF:
		// sevenfm: r7972 fix
		bAPForStandUp = 0;
		bAPToLookAtWall = (FindDirectionForClimbing( pSoldier, pSoldier->sGridNo ) == pSoldier->ubDirection) ? 0 : GetAPsToLook( pSoldier );
		// SANDRO - improved this a bit
		if (pSoldier->pathing.bLevel == 0)
		{
			if( PTR_CROUCHED ) bAPForStandUp = (INT8)(GetAPsCrouch(pSoldier, TRUE));
			else if( PTR_PRONE ) bAPForStandUp = GetAPsCrouch(pSoldier, TRUE) + GetAPsProne(pSoldier, TRUE);
			bMinPointsNeeded = GetAPsToClimbRoof( pSoldier, FALSE ) + bAPForStandUp + bAPToLookAtWall;
		}
		else
		{
			if( !PTR_CROUCHED ) bAPForStandUp = (INT8)(GetAPsCrouch(pSoldier, TRUE));
			bMinPointsNeeded = GetAPsToClimbRoof( pSoldier, TRUE ) + bAPForStandUp + bAPToLookAtWall;
		}			
		break;
	case AI_ACTION_COWER:
	case AI_ACTION_STOP_COWERING:
	case AI_ACTION_LOWER_GUN:
	case AI_ACTION_END_COWER_AND_MOVE:
	case AI_ACTION_TRAVERSE_DOWN:
	case AI_ACTION_OFFER_SURRENDER:
		bMinPointsNeeded = 0;
		break;
	case AI_ACTION_STEAL_MOVE: // added by SANDRO
		//bMinPointsNeeded = GetAPsToStealItem( pSoldier, NULL, pSoldier->aiData.usActionData );;
		break;
	case AI_ACTION_JUMP_WINDOW:
		if (UsingNewInventorySystem() && pSoldier->inv[BPACKPOCKPOS].exists())
			bMinPointsNeeded = GetAPsToJumpThroughWindows(pSoldier, TRUE);
		else
			bMinPointsNeeded = GetAPsToJumpThroughWindows(pSoldier, FALSE);
			//bMinPointsNeeded = GetAPsToJumpFence( pSoldier, FALSE );
		break;
	case AI_ACTION_FREE_PRISONER:
		bMinPointsNeeded = APBPConstants[AP_HANDCUFF];
		break;
	case AI_ACTION_USE_SKILL:
		bMinPointsNeeded = GetAPsToUseSkill();
		break;
	case AI_ACTION_DOCTOR:
	case AI_ACTION_DOCTOR_SELF:
		bMinPointsNeeded = APBPConstants[AP_START_FIRST_AID];
		break;
	case AI_ACTION_SELFDETONATE:
		bMinPointsNeeded = GetAPsToSelfDetonate();
		break;
	case AI_ACTION_SWITCH_FLASHLIGHT:
		bMinPointsNeeded = GetAPsToUseFlashlight();
		break;
	case AI_ACTION_FAKE_ALERT:
		bMinPointsNeeded = APBPConstants[AP_RADIO];
		break;
	case AI_ACTION_STOP_MEDIC:
		bMinPointsNeeded = 0;
		break;
	case AI_ACTION_DRINK_CANTEEN:
		bMinPointsNeeded = APBPConstants[AP_DRINK];
		break;
	case AI_ACTION_USE_DRUG:
		bMinPointsNeeded = APBPConstants[AP_DRINK];
		break;
	case AI_ACTION_CANCEL_DRAG:
		bMinPointsNeeded = 0;
		break;
	case AI_ACTION_HANDLE_ITEM:
		bMinPointsNeeded = 0;
		break;
	case AI_ACTION_PLANT_BOMB:
		bMinPointsNeeded = APBPConstants[AP_INVENTORY_ARM] + APBPConstants[AP_DROP_BOMB];
		break;
	case AI_ACTION_STOP_FOCUS:
		bMinPointsNeeded = 0;
		break;
	default:
#ifdef BETAVERSION
		//NumMessage("AffordableAction - Illegal action type = ",pSoldier->aiData.bAction);
#endif
		break;
	}

	// check whether or not we can afford to do this action
	if (bMinPointsNeeded > pSoldier->bActionPoints)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

INT16 RandomFriendWithin(SOLDIERTYPE *pSoldier)
{
	UINT32				uiLoop;
	UINT16				usMaxDist;
	UINT8					ubFriendCount, ubFriendIDs[MAXMERCS], ubFriendID;
	UINT8				ubDirection;
	UINT8					ubDirsLeft;
	BOOLEAN				fDirChecked[8];
	BOOLEAN				fRangeRestricted = FALSE, fFound = FALSE;
	INT32				usDest, usOrigin;
	SOLDIERTYPE *	pFriend;

	// obtain maximum roaming distance from soldier's origin
	usMaxDist = RoamingRange(pSoldier,&usOrigin);

	// sevenfm: limit max search range to day vision range
	usMaxDist = min( usMaxDist, TACTICAL_RANGE );

	// if range is restricted, make sure origin is a legal gridno!
	if ((usOrigin < 0) || (usOrigin >= GRIDSIZE))
	{
#ifdef BETAVERSION
		NameMessage(pSoldier,"has illegal origin, but his roaming range is restricted!",1000);
#endif
		return(FALSE);
	}

	ubFriendCount = 0;

	// build a list of the guynums of all active, eligible friendly mercs

	// go through each soldier, looking for "friends" (soldiers on same side)
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pFriend = MercSlots[ uiLoop ];

		// if this merc is inactive, not in sector, or dead
		if (!pFriend)
		{
			continue;
		}

		// skip ourselves
		if (pFriend->ubID == pSoldier->ubID)
		{
			continue;
		}

		// if this man not neutral, but is on my side, OR if he is neutral, but
		// so am I, then he's a "friend" for the purposes of random visitations
		if ((!pFriend->aiData.bNeutral && (pSoldier->bSide == pFriend->bSide)) ||
			(pFriend->aiData.bNeutral && pSoldier->aiData.bNeutral))
		{
			// if we're not already neighbors
			if (SpacesAway(pSoldier->sGridNo,pFriend->sGridNo) > 1)
			{
				// remember his guynum, increment friend counter
				ubFriendIDs[ubFriendCount++] = pFriend->ubID;
			}
		}
	}

	while (ubFriendCount && !fFound)
	{
		// randomly select one of the remaining friends in the list
		ubFriendID = ubFriendIDs[PreRandom(ubFriendCount)];

		// if our movement range is NOT restricted, or this friend's within range
		// use distance - 1, because there must be at least 1 tile 1 space closer
		if ( SpacesAway(usOrigin,Menptr[ubFriendID].sGridNo) - 1 <= usMaxDist)
		{
			// should be close enough, try to find a legal->pathing.sDestination within 1 tile

			// clear dirChecked flag for all 8 directions
			for (ubDirection = 0; ubDirection < 8; ubDirection++)
			{
				fDirChecked[ubDirection] = FALSE;
			}

			ubDirsLeft = 8;

			// examine all 8 spots around 'ubFriendID'
			// keep looking while directions remain and a satisfactory one not found
			while ((ubDirsLeft--) && !fFound)
			{
				// randomly select a direction which hasn't been 'checked' yet
				do
				{
					ubDirection = (UINT8) Random(8);
				}
				while (fDirChecked[ubDirection]);

				fDirChecked[ubDirection] = TRUE;

				// determine the gridno 1 tile away from current friend in this direction
				usDest = NewGridNo(Menptr[ubFriendID].sGridNo,DirectionInc(ubDirection));

				// if that's out of bounds, ignore it & check next direction
				if (usDest == Menptr[ubFriendID].sGridNo)
				{
					continue;
				}

				if(SpacesAway(usOrigin, usDest) > usMaxDist)
				{
					continue;
				}

				if (SpotDangerLevel(pSoldier, usDest) > 0)
				{
					continue;
				}

				if (LegalNPCDestination(pSoldier, usDest, ENSURE_PATH, NOWATER, 0))
				{
					fFound = TRUE;			// found a spot
					pSoldier->aiData.usActionData = usDest;	// store this->pathing.sDestination
					pSoldier->pathing.bPathStored = TRUE;	// optimization - Ian
					break;					// stop checking in other directions
				}
			}
		}

		if (!fFound)
		{
			ubFriendCount--;

			// if we hadn't already picked the last friend currently in the list
			if (ubFriendCount != ubFriendID)
			{
				ubFriendIDs[ubFriendID] = ubFriendIDs[ubFriendCount];
			}
		}
	}

	return(fFound);
}


INT32 RandDestWithinRange(SOLDIERTYPE *pSoldier)
{
	INT32 sRandDest = NOWHERE;
	INT32 usOrigin, usMaxDist;
	UINT8	ubTriesLeft;
	BOOLEAN fLimited = FALSE, fFound = FALSE;
	INT16 sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXRange, sYRange, sXOffset, sYOffset;
	INT16 sOrigX, sOrigY;
	INT16 sX, sY;
	UINT16 usRoom = 0, usTempRoom;
	sOrigX = sOrigY = -1;
	sMaxLeft = sMaxRight = sMaxUp = sMaxDown = sXRange = sYRange = -1;

	// sevenfm:
	INT16 sFriendDist;

	// Try to find a random->pathing.sDestination that's no more than maxDist away from
	// the given gridno of origin

	if (gfTurnBasedAI)
	{
		ubTriesLeft = 10;
	}
	else
	{
		ubTriesLeft = 1;
	}

	usMaxDist = RoamingRange(pSoldier,&usOrigin);

	if ( pSoldier->aiData.bOrders <= CLOSEPATROL && (pSoldier->bTeam == CIV_TEAM || pSoldier->ubProfile != NO_PROFILE ) )
	{
		// any other combo uses the default of ubRoom == 0, set above
		if ( !InARoom( pSoldier->aiData.sPatrolGrid[0], &usRoom ) )
		{
			usRoom = 0;
		}
	}

	// sevenfm: limit max search range to day vision range
	//usMaxDist = min( usMaxDist, WORLD_COLS / 4 );
	usMaxDist = min( usMaxDist, DAY_VISION_RANGE );

	// if the maxDist is truly a restriction
	if (usMaxDist < (MAXCOL - 1))
	{
		fLimited = TRUE;

		// determine maximum horizontal limits
		sOrigX = usOrigin % MAXCOL;
		sOrigY = usOrigin / MAXCOL;

		sMaxLeft	= min(usMaxDist, sOrigX);
		sMaxRight = min(usMaxDist,MAXCOL - (sOrigX + 1));

		// determine maximum vertical limits
		sMaxUp	= min(usMaxDist, sOrigY);
		sMaxDown = min(usMaxDist,MAXROW - (sOrigY + 1));

		sXRange = sMaxLeft + sMaxRight + 1;
		sYRange = sMaxUp + sMaxDown + 1;
	}

	if (pSoldier->ubBodyType == LARVAE_MONSTER)
	{
		// only crawl 1 tile, within our roaming range
		while ((ubTriesLeft--) && !fFound)
		{
			sXOffset = (INT16) Random( 3 ) - 1; // generates -1 to +1
			sYOffset = (INT16) Random( 3 ) - 1;

			if (fLimited)
			{
				sX = pSoldier->sGridNo % MAXCOL + sXOffset;
				sY = pSoldier->sGridNo / MAXCOL + sYOffset;
				if (sX < sOrigX - sMaxLeft || sX > sOrigX + sMaxRight)
				{
					continue;
				}
				if (sY < sOrigY - sMaxUp || sY > sOrigY + sMaxDown)
				{
					continue;
				}
				sRandDest = usOrigin + sXOffset + (MAXCOL * sYOffset);
			}
			else
			{
				sRandDest = usOrigin + sXOffset + (MAXCOL * sYOffset);
			}

			if (!LegalNPCDestination(pSoldier,sRandDest,ENSURE_PATH,NOWATER,0))
			{
				sRandDest = NOWHERE;
				continue;					// try again!
			}

			// passed all the tests,->pathing.sDestination is acceptable
			fFound = TRUE;
			pSoldier->pathing.bPathStored = TRUE;	// optimization - Ian
		}
	}
	else
	{
		// keep rolling random->pathing.sDestinations until one's satisfactory or retries used
		while ((ubTriesLeft--) && !fFound)
		{
			if (fLimited)
			{
				UINT8 ubTriesLeft2 = 128;
				//dnl ch53 111009 This loop should increase search performance, but probably need some counter to prevent eventual endless loop
				do
				{
					sXOffset = ((INT16)Random(sXRange)) - sMaxLeft;
					sYOffset = ((INT16)Random(sYRange)) - sMaxUp;
					sRandDest = usOrigin + sXOffset + (MAXCOL * sYOffset);
				}while(!GridNoOnVisibleWorldTile(sRandDest) && --ubTriesLeft2);
#ifdef BETAVERSION
				if ((sRandDest < 0) || (sRandDest >= GRIDSIZE))
				{
					NumMessage("RandDestWithinRange: ERROR - Gridno out of range! = ",sRandDest);
					sRandDest = random(GRIDSIZE);
				}
#endif
			}
			else
			{
				UINT8 ubTriesLeft2 = 128;
				//dnl ch53 111009 This loop should increase search performance, but probably need some counter to prevent eventual endless loop
				do
				{
					sRandDest = PreRandom(GRIDSIZE);
				}while(!GridNoOnVisibleWorldTile(sRandDest) && --ubTriesLeft2);
			}

			if (usRoom && InARoom(sRandDest, &usTempRoom) && usTempRoom != usRoom)
			{
				// outside of room available for patrol!
				sRandDest = NOWHERE;
				continue;
			}

			if (SpotDangerLevel(pSoldier, sRandDest) > 0)
			{
				sRandDest = NOWHERE;
				continue;
			}

			sFriendDist = DistanceToClosestNotSeekEnemyFriend(pSoldier, sRandDest);
			// sevenfm: don't go too far from closest not SEEKENEMY friend if alert is not raised yet
			if( pSoldier->aiData.bAlertStatus < STATUS_RED &&
				(sFriendDist > DAY_VISION_RANGE || NightLight() && sFriendDist > DAY_VISION_RANGE / 2) )
			{
				sRandDest = NOWHERE;
				continue;
			}

			// sevenfm: limit going from/into room if alert is not raised
			/*if( pSoldier->aiData.bAlertStatus < STATUS_RED &&
				InARoom(pSoldier->sGridNo, NULL) != InARoom(sRandDest, NULL) )
			{
				sRandDest = NOWHERE;
				continue;
			}*/

			if (!LegalNPCDestination(pSoldier,sRandDest,ENSURE_PATH,NOWATER,0))
			{
				sRandDest = NOWHERE;
				continue;					// try again!
			}

			// passed all the tests,->pathing.sDestination is acceptable
			fFound = TRUE;
			pSoldier->pathing.bPathStored = TRUE;	// optimization - Ian
		}
	}

	return(sRandDest); // defaults to NOWHERE
}

INT32 ClosestReachableDisturbance(SOLDIERTYPE *pSoldier, BOOLEAN * pfChangeLevel, UINT8 &ubThreatID, INT32 &sThreatSpot, INT8 &bThreatLevel, UINT8 &ubSeekType)
{
	INT32		sGridNo = NOWHERE;
	INT8		bLevel, bClosestLevel = 0;
	BOOLEAN		fClimbingNecessary, fClosestClimbingNecessary = FALSE;
	INT32		iPathCost;	
	INT32		sClosestDisturbance = NOWHERE;
	INT32		sDisturbance = NOWHERE;
	UINT32		uiLoop;
	INT32		iShortestPath = 1000;
	UINT8		*pubNoiseVolume;
	INT32		*pusNoiseGridNo;
	INT8		*pbNoiseLevel;
	INT32		sClimbGridNo;
	SOLDIERTYPE *pOpponent;
	SOLDIERTYPE	*pClosestOpponent = NULL;
	BOOLEAN		fHasTNT = FALSE;
	BOOLEAN		fHeavyWeapon = FALSE;

	// sevenfm: safety check
	if ( pfChangeLevel )
	{
		*pfChangeLevel = FALSE;
	}

	ubThreatID = NOBODY;
	sThreatSpot = NOWHERE;
	bThreatLevel = 0;
	ubSeekType = SEEK_NORMAL;

	UINT8 ubCurrentSeekType;

	pubNoiseVolume = &gubPublicNoiseVolume[pSoldier->bTeam];
	pusNoiseGridNo = &gsPublicNoiseGridNo[pSoldier->bTeam];
	pbNoiseLevel = &gbPublicNoiseLevel[pSoldier->bTeam];

	// sevenfm: remember ID of closest known opponent
	UINT8 ubClosestKnownOpponentID;
	INT32 sClosestKnownOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL, &ubClosestKnownOpponentID);

	BOOLEAN fAllowComplexAI = AllowComplexAI(pSoldier);
	BOOLEAN fSawEnemy = GuySawEnemy(pSoldier, SEEN_LAST_TURN);

	if (FindTNT(pSoldier) != NO_SLOT)
	{
		fHasTNT = TRUE;
	}

	if (AIRoleGLOperator(pSoldier))
	{
		fHeavyWeapon = TRUE;
	}

	// reset closest reachable opponent if he is no longer valid
	pOpponent = NULL;
	if (pSoldier->ubClosestReachableOpponentID != NOBODY)
	{
		pOpponent = MercPtrs[pSoldier->ubClosestReachableOpponentID];
	}
	if (pOpponent &&
		(!ValidOpponent(pSoldier, pOpponent) || Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN || TileIsOutOfBounds(KnownLocation(pSoldier, pOpponent->ubID))))
	{
		pSoldier->ubClosestReachableOpponentID = NOBODY;
	}

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];
		ubCurrentSeekType = SEEK_NORMAL;

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		// sevenfm: speed optimization, skip all opponents except last found and closest known
		if ((!pSoldier->aiData.bUnderFire && !fSawEnemy || pSoldier->aiData.bAlertStatus == STATUS_BLACK || !fAllowComplexAI) &&
			pSoldier->ubClosestReachableOpponentID != NOBODY &&
			pOpponent->ubID != pSoldier->ubClosestReachableOpponentID &&
			pOpponent->ubID != ubClosestKnownOpponentID &&
			Knowledge(pSoldier, pSoldier->ubClosestReachableOpponentID) != HEARD_3_TURNS_AGO)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// if we are standing at that gridno (!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo && bLevel == pSoldier->pathing.bLevel)
		{
			continue;			// next merc
		}

		if (TileIsOutOfBounds(sGridNo))
		{
			// huh?
			continue;
		}

		// by default, we move to opponent location
		sDisturbance = sGridNo;

		// optimization for slow PC - ignore opponent if farther away than closest reachable
		if (!fAllowComplexAI &&
			pClosestOpponent &&
			pClosestOpponent->stats.bLife >= OKLIFE &&
			!pClosestOpponent->IsEmptyVehicle() &&
			!TileIsOutOfBounds(sClosestDisturbance) &&
			PythSpacesAway(pSoldier->sGridNo, sGridNo) > PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance))
		{
			continue;
		}

		// sevenfm: if soldier is zombie and he cannot climb, skip location
		if (pSoldier->IsZombie() && pSoldier->pathing.bLevel != bLevel && !gGameExternalOptions.fZombieCanClimb)
		{
			continue;
		}

		// sevenfm: zombies do not attack vehicles
		if (pSoldier->IsZombie() && (pOpponent->IsTank() || pOpponent->IsVehicle()))
		{
			continue;
		}

		// sevenfm: ignore dying opponent if closest opponent is not dying
		if (pClosestOpponent && pClosestOpponent->stats.bLife >= OKLIFE && pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		// sevenfm: ignore empty vehicles if closest opponent is not empty vehicle
		if (pClosestOpponent && !pClosestOpponent->IsEmptyVehicle() && pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// sevenfm: ignore tank if soldier has no heavy weapons
		if (pClosestOpponent && 
			!pClosestOpponent->IsTank() && 
			pOpponent->IsTank() && 
			!fHeavyWeapon)
		{
			continue;
		}

		// when in deep water, skip opponents in deep water
		if (DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel) && DeepWater(sGridNo, bLevel))
		{
			continue;
		}

		// sevenfm: ignore enemy if he is too far
		if (AICheckSpotTooFar(pSoldier, sGridNo))// || !AICheckRoamingRange(pSoldier, sGridNo))
		{
			continue;
		}

		iPathCost = 0;
		//DebugAI(AI_MSG_INFO, pSoldier, String("found opponent [%d] %s spot %d", pOpponent->ubID, pOpponent->GetName(), sGridNo));

		// check if we can blow up enemy from above or below
		if (pSoldier->bTeam == ENEMY_TEAM &&
			SoldierAI(pSoldier) &&
			fHasTNT &&
			pSoldier->aiData.bAttitude == SEEKENEMY &&
			!AISpecialRole(pSoldier) &&
			!AICheckUnderground() &&
			InARoom(sGridNo, NULL) &&
			IsLocationSittableExcludingPeople(sGridNo, bLevel ? 0 : 1) &&
			(CountCorpsesInRoom(pSoldier, RoomNo(sGridNo), bLevel) || TeamHighPercentKilled(pSoldier->bTeam)))
		{
			iPathCost = EstimatePathCostToLocation(pSoldier, sGridNo, bLevel ? 0 : 1, FALSE, &fClimbingNecessary, &sClimbGridNo);
			// move to opponent's location but on different level, use climb spot if necessary
			sDisturbance = fClimbingNecessary ? sClimbGridNo : sGridNo;
			ubCurrentSeekType = SEEK_PLANT_BOMB;
		}		

		// if we don't have a valid path yet
		if (iPathCost == 0)
		{
			iPathCost = EstimatePathCostToLocation(pSoldier, sGridNo, bLevel, FALSE, &fClimbingNecessary, &sClimbGridNo);
			sDisturbance = fClimbingNecessary ? sClimbGridNo : sGridNo;
			ubCurrentSeekType = SEEK_NORMAL;
		}

		// check if enemy is in a room and we can blow up wall
		if (pSoldier->bTeam == ENEMY_TEAM &&
			SoldierAI(pSoldier) &&
			fHasTNT &&
			pSoldier->pathing.bLevel == 0 &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			!pSoldier->aiData.bUnderFire &&
			pSoldier->aiData.bAttitude == SEEKENEMY &&
			!AISpecialRole(pSoldier) &&			
			!AICheckUnderground() &&
			InARoom(sGridNo) &&
			!SameRoom(sGridNo, pSoldier->sGridNo) &&
			CountFriendsInRoom(pSoldier, RoomNo(sGridNo)) == 0 &&
			(iPathCost == 0 || CountCorpsesInRoom(pSoldier, RoomNo(sGridNo), 0) || TeamHighPercentKilled(pSoldier->bTeam)))
		{
			//DebugAI(AI_MSG_INFO, pSoldier, String("opponent is in a room %d", RoomNo(sGridNo)));
			INT32	sNewSpot;
			UINT8	ubDirection;
			INT32	iNewPathCost = 0;

			// check all tiles around room
			for (INT32 cnt = 0; cnt < WORLD_MAX; cnt++)
			{
				//DebugAI(AI_MSG_INFO, pSoldier, String("8389: TileIsOutOfBounds %d GridNoOnVisibleWorldTile %d SameRoom %d IsLocationSittable %d CorpseWarning %d", TileIsOutOfBounds(cnt), GridNoOnVisibleWorldTile(cnt), SameRoom(cnt, sGridNo), IsLocationSittable(cnt, bLevel), CorpseWarning(pSoldier, cnt, bLevel)));

				// check only tiles in the same room, find wall to blow up
				if (TileIsOutOfBounds(cnt) ||
					!GridNoOnVisibleWorldTile(cnt) ||
					SpacesAway(cnt, sGridNo) > (INT16)(TACTICAL_RANGE / 2) ||
					!SameRoom(cnt, sGridNo) ||
					!IsLocationSittable(cnt, bLevel) ||
					CorpseWarning(pSoldier, cnt, bLevel) && iPathCost > 0)
				{
					continue;
				}

				//DebugAI(AI_MSG_INFO, pSoldier, String("check tile %d in a room %d", cnt, RoomNo(cnt)));
				// check tiles around this spot
				for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
				{
					sNewSpot = NewGridNo(cnt, DirectionInc(ubDirection));

					// check if we can blow up wall here
					if (sNewSpot != cnt &&
						!SameRoom(cnt, sNewSpot) &&
						//!InARoom(sNewSpot, NULL) &&
						//CheckWindow(cnt, ubDirection, gGameExternalOptions.fCanJumpThroughClosedWindows) &&
						gubWorldMovementCosts[sNewSpot][ubDirection][bLevel] >= TRAVELCOST_BLOCKED &&
						gpWorldLevelData[sNewSpot].ubHeight == gpWorldLevelData[cnt].ubHeight &&
						IsLocationSittable(sNewSpot, bLevel))
					{
						//DebugAI(AI_MSG_INFO, pSoldier, String("found obstacle spot %d in a room %d", sNewSpot, RoomNo(sNewSpot)));
						// fClimbingNecessary will be set to FALSE, sClimbGridNo will be set to NOWHERE as we search on the same level
						iNewPathCost = EstimatePathCostToLocation(pSoldier, sNewSpot, bLevel, FALSE, NULL, NULL);
						if (iNewPathCost > 0 && (iPathCost == 0 || iNewPathCost < iPathCost))
						{
							//DebugAI(AI_MSG_INFO, pSoldier, String("spot %d reachable pathcost %d", sNewSpot, iPathCost));
							iPathCost = iNewPathCost;
							// substitute disturbance spot - use tnt location
							sDisturbance = sNewSpot;
							// reset climbing information
							fClimbingNecessary = FALSE;
							sClimbGridNo = NOWHERE;
							ubCurrentSeekType = SEEK_PLANT_BOMB;
						}
					}
				}
			}
		}

		// check if enemy is in a room and we can jump into window
		if (pSoldier->bTeam == ENEMY_TEAM &&
			SoldierAI(pSoldier) &&
			gGameExternalOptions.fCanJumpThroughWindows &&
			!fHasTNT &&
			pSoldier->pathing.bLevel == 0 &&
			!AISpecialRole(pSoldier) &&
			pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
			pSoldier->aiData.bAttitude == SEEKENEMY &&
			(iPathCost == 0 || pSoldier->aiData.bAttitude == CUNNINGSOLO || pSoldier->aiData.bAttitude == CUNNINGAID) &&
			!AICheckUnderground() &&
			InARoom(sGridNo) &&
			(iPathCost == 0 || CountCorpsesInRoom(pSoldier, RoomNo(sGridNo), 0) || TeamHighPercentKilled(pSoldier->bTeam)))
		{
			//DebugAI(AI_MSG_INFO, pSoldier, String("opponent is in a room %d", RoomNo(sGridNo)));
			INT32	sNewSpot;
			UINT8	ubDirection;
			INT32	iWindowPathCost = 0;

			// check all tiles around room
			for (INT32 cnt = 0; cnt < WORLD_MAX; cnt++)
			{
				//DebugAI(AI_MSG_INFO, pSoldier, String("8389: TileIsOutOfBounds %d GridNoOnVisibleWorldTile %d SameRoom %d IsLocationSittable %d CorpseWarning %d", TileIsOutOfBounds(cnt), GridNoOnVisibleWorldTile(cnt), SameRoom(cnt, sGridNo), IsLocationSittable(cnt, bLevel), CorpseWarning(pSoldier, cnt, bLevel)));

				// check only tiles in the same room, find window to jump
				if (TileIsOutOfBounds(cnt) ||
					!GridNoOnVisibleWorldTile(cnt) ||
					SpacesAway(cnt, sGridNo) > (INT16)(TACTICAL_RANGE / 2) ||
					!SameRoom(cnt, sGridNo) ||
					!IsLocationSittable(cnt, bLevel) ||
					CorpseWarning(pSoldier, cnt, bLevel) && iPathCost > 0)
				{
					continue;
				}

				//DebugAI(AI_MSG_INFO, pSoldier, String("check tile %d in a room %d", cnt, RoomNo(cnt)));
				// check tiles around this spot
				for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
				{
					sNewSpot = NewGridNo(cnt, DirectionInc(ubDirection));

					// check if we can jump into this room here
					if (sNewSpot != cnt &&
						!InARoom(sNewSpot, NULL) &&
						CheckWindow(cnt, ubDirection, gGameExternalOptions.fCanJumpThroughClosedWindows) &&
						IsLocationSittable(sNewSpot, bLevel))
					{
						//DebugAI(AI_MSG_INFO, pSoldier, String("found jumpable spot %d in a room %d", sNewSpot, RoomNo(sNewSpot)));
						// fClimbingNecessary will be set to FALSE, sClimbGridNo will be set to NOWHERE as we search on the same level
						iWindowPathCost = EstimatePathCostToLocation(pSoldier, sNewSpot, bLevel, FALSE, NULL, NULL);
						if (iWindowPathCost > 0 && (iPathCost == 0 || iWindowPathCost < iPathCost))
						{
							//DebugAI(AI_MSG_INFO, pSoldier, String("spot %d reachable pathcost %d", sNewSpot, iPathCost));
							iPathCost = iWindowPathCost;
							// substitute disturbance spot - use jump location
							sDisturbance = sNewSpot;
							// reset climbing information
							fClimbingNecessary = FALSE;
							sClimbGridNo = NOWHERE;
							ubCurrentSeekType = SEEK_JUMP_WINDOW;
						}
					}
				}
			}
		}

		// if we can get there and it's first reachable enemy or closer than other known enemies
		// always switch from dying to not dying
		// always switch from empty vehicle to not empty vehicle
		if (iPathCost != 0 &&
			(TileIsOutOfBounds(sClosestDisturbance) || 
			iPathCost < iShortestPath ||	
			pClosestOpponent && 
			(pClosestOpponent->stats.bLife < OKLIFE || pClosestOpponent->IsEmptyVehicle() || pClosestOpponent->IsTank() && !fHeavyWeapon) && 
			(pOpponent->stats.bLife >= OKLIFE && !pOpponent->IsEmptyVehicle() && (!pOpponent->IsTank() || fHeavyWeapon)))
			)
		{
			sClosestDisturbance = sDisturbance;
			iShortestPath = iPathCost;
			fClosestClimbingNecessary = fClimbingNecessary;
			pClosestOpponent = pOpponent;
			ubThreatID = pOpponent->ubID;
			// store known opponent location as sGridNo and bLevel could be changed
			sThreatSpot = KnownLocation(pSoldier, pOpponent->ubID);
			bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);
			ubSeekType = ubCurrentSeekType;
		}
	}

	// remember closest reachable opponent
	pSoldier->ubClosestReachableOpponentID = ubThreatID;

	// if any "misc. noise" was also heard recently
	// sevenfm: skip noise location if too far
	if( !TileIsOutOfBounds(pSoldier->aiData.sNoiseGridno) &&
		pSoldier->aiData.sNoiseGridno != sClosestDisturbance &&
		!AICheckSpotTooFar(pSoldier, pSoldier->aiData.sNoiseGridno) &&
		!GuySawEnemy(pSoldier) &&
		(TileIsOutOfBounds(sClosestDisturbance) || PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.sNoiseGridno) < PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) || ubThreatID != NOBODY && Knowledge(pSoldier, ubThreatID) == HEARD_3_TURNS_AGO))
	{
		// test this gridno, too
		sGridNo = pSoldier->aiData.sNoiseGridno;
		bLevel = pSoldier->bNoiseLevel;

		// if we are there (at the noise gridno)
		if (sGridNo == pSoldier->sGridNo)
		{
			for(uiLoop=0; uiLoop<guiNumMercSlots; uiLoop++)//dnl ch58 160813
			{
				pOpponent = MercSlots[uiLoop];
				if(pOpponent && pSoldier->bSide == pOpponent->bSide && pSoldier->ubID != pOpponent->ubID && pSoldier->aiData.sNoiseGridno == pOpponent->aiData.sNoiseGridno)
				{
					pOpponent->aiData.sNoiseGridno = NOWHERE;// Erase for all from the same team as it not useful anymore, this will avoid others to check already tested location
					pOpponent->aiData.ubNoiseVolume = 0;
				}
			}
			pSoldier->aiData.sNoiseGridno = NOWHERE;		// wipe it out, not useful anymore
			pSoldier->aiData.ubNoiseVolume = 0;
		}
		else
		{
			BOOLEAN fFriendNoise = FALSE;
			UINT8 ubNoiseID = WhoIsThere2(sGridNo, bLevel);
			if( ubNoiseID != NOBODY && MercPtrs[ubNoiseID]->bSide == pSoldier->bSide )
			{
				fFriendNoise = TRUE;
			}

			// get the AP cost to get to the location of the noise
			iPathCost = EstimatePathCostToLocation( pSoldier, sGridNo, bLevel, FALSE, &fClimbingNecessary, &sClimbGridNo );
			// if we can get there
			// sevenfm: only if we don't know enemy location or noise source is close and we have not seen enemy recently
			if (iPathCost != 0 &&
				!fFriendNoise &&
				!pSoldier->IsFlanking() &&
				(TileIsOutOfBounds(sClosestDisturbance) || iPathCost < iShortestPath))
			{
				if (fClimbingNecessary)
					sClosestDisturbance = sClimbGridNo;
				else
					sClosestDisturbance = sGridNo;

				iShortestPath = iPathCost;
				fClosestClimbingNecessary = fClimbingNecessary;

				ubThreatID = ubNoiseID;
				sThreatSpot = sGridNo;
				bThreatLevel = bLevel;

				ubSeekType = SEEK_NORMAL;
			}
		}
	}

	// if any PUBLIC "misc. noise" was also heard recently
	// sevenfm: skip noise location if too far
	if( !TileIsOutOfBounds(*pusNoiseGridNo) && 
		*pusNoiseGridNo != sClosestDisturbance &&
		!AICheckSpotTooFar(pSoldier, pSoldier->aiData.sNoiseGridno) &&
		!GuySawEnemy(pSoldier) &&
		(TileIsOutOfBounds(sClosestDisturbance) || PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.sNoiseGridno) < PythSpacesAway(pSoldier->sGridNo, sClosestDisturbance) || ubThreatID != NOBODY && Knowledge(pSoldier, ubThreatID) == HEARD_3_TURNS_AGO))
	{
		// test this gridno, too
		sGridNo = *pusNoiseGridNo;
		bLevel = *pbNoiseLevel;

		// if we are not NEAR the noise gridno...
		if( pSoldier->pathing.bLevel != bLevel || 
			PythSpacesAway( pSoldier->sGridNo, sGridNo ) >= DAY_VISION_RANGE / 4 || 
			LOS_Virtual(pSoldier, sGridNo, bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) == 0)
		{
			BOOLEAN fFriendNoise = FALSE;
			UINT8 ubNoiseID = WhoIsThere2(sGridNo, bLevel);
			if( ubNoiseID != NOBODY && MercPtrs[ubNoiseID]->bSide == pSoldier->bSide )
			{
				fFriendNoise = TRUE;
			}

			// get the AP cost to get to the location of the noise
			iPathCost = EstimatePathCostToLocation( pSoldier, sGridNo, bLevel, FALSE, &fClimbingNecessary, &sClimbGridNo );
			// if we can get there
			// sevenfm: only if we don't know enemy location or noise source is close and we have not seen enemy recently
			//if (iPathCost != 0 )
			if (iPathCost != 0 &&
				!fFriendNoise &&
				!pSoldier->IsFlanking() &&
				(TileIsOutOfBounds(sClosestDisturbance) || iPathCost < iShortestPath))
			{
				if (fClimbingNecessary)
					sClosestDisturbance = sClimbGridNo;
				else
					sClosestDisturbance = sGridNo;

				iShortestPath = iPathCost;
				fClosestClimbingNecessary = fClimbingNecessary;

				ubThreatID = ubNoiseID;
				sThreatSpot = sGridNo;
				bThreatLevel = bLevel;
				ubSeekType = SEEK_NORMAL;
			}
		}
		else
		{
			// degrade our public noise a bit
			// sevenfm: we are at the same level as noise gridno, very close and have line of sight to it
			if(*pubNoiseVolume > 1)
			{
				(*pubNoiseVolume)--;
			}
		}
	}

	// sevenfm: also consider inspecting watched locations
	/*if(TileIsOutOfBounds(sClosestDisturbance))
	{
		for ( INT8 bLoop = 0; bLoop < NUM_WATCHED_LOCS; bLoop++ )
		{
			sGridNo = gsWatchedLoc[ pSoldier->ubID ][ bLoop ];
			bLevel = gbWatchedLocLevel[ pSoldier->ubID ][ bLoop ];

			// sevenfm: if soldier is zombie and he cannot climb, skip location
			if( pSoldier->IsZombie() && pSoldier->pathing.bLevel != bLevel && !gGameExternalOptions.fZombieCanClimb )
			{
				continue;
			}

			if (!TileIsOutOfBounds(sGridNo) )
			{
				// check if we are standing at this gridno
				if( sGridNo == pSoldier->sGridNo)
				{
					gsWatchedLoc[ pSoldier->ubID ][ bLoop ] =  NOWHERE;
				}
				else
				{
					iPathCost = EstimatePathCostToLocation( pSoldier, sGridNo, bLevel, FALSE, &fClimbingNecessary, &sClimbGridNo );

					// if we can get there and it's first reachable enemy or closer than other known enemies
					if( iPathCost != 0 &&
						(TileIsOutOfBounds(sClosestDisturbance) || iPathCost < iShortestPath) ) 
					{
						if (fClimbingNecessary)
						{
							sClosestDisturbance = sClimbGridNo;
						}
						else
						{
							sClosestDisturbance = sGridNo;
						}

						iShortestPath = iPathCost;
						fClosestClimbingNecessary = fClimbingNecessary;

						ubThreatID = NOBODY;
						sThreatSpot = sGridNo;
						bThreatLevel = bLevel;
					}
				}				
			}
		}
	}*/

#ifdef DEBUGDECISIONS
	if (!TileIsOutOfBounds(sClosestDisturbance))
	{
		AINumMessage("CLOSEST DISTURBANCE is at gridno ",sClosestDisturbance);
	}
#endif

	// sevenfm: safety check
	if ( pfChangeLevel )
	{
		*pfChangeLevel = fClosestClimbingNecessary;
	}

	return(sClosestDisturbance);
}

INT32 ClosestActiveOpponent(SOLDIERTYPE *pSoldier, INT32 *psGridNo, INT8 *pbLevel, UINT8 *pubOpponentID)
{
	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpponent;
	SOLDIERTYPE *pClosestOpponent = NULL;

	if (pubOpponentID)
	{
		*pubOpponentID = NOBODY;
	}

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sGridNo))
		{
			continue;
		}

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo && bLevel == pSoldier->pathing.bLevel)
		{
			continue;			// next merc
		}

		// ignore opponents who are not threat to us
		if (pOpponent->IsCowering() || pOpponent->IsUnconscious() || pOpponent->stats.bLife < OKLIFE || pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);

		if (sClosestOpponent == NOWHERE || iRange < iClosestRange)
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
			pClosestOpponent = pOpponent;
		}
	}

	if (psGridNo)
		*psGridNo = sClosestOpponent;

	if (pbLevel)
		*pbLevel = bClosestLevel;

	if (pubOpponentID && pClosestOpponent)
		*pubOpponentID = pClosestOpponent->ubID;

	return(sClosestOpponent);
}

INT32 ClosestKnownOpponent(SOLDIERTYPE *pSoldier, INT32 *psGridNo, INT8 *pbLevel, UINT8 *pubOpponentID)
{
	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpponent;
	SOLDIERTYPE *pClosestOpponent = NULL;
	BOOLEAN		fSameBuilding, fClosestSameBuilding = FALSE;

	if (pubOpponentID)
	{
		*pubOpponentID = NOBODY;
	}

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];
		fSameBuilding = FALSE;

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sGridNo))
		{
			continue;
		}

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo && bLevel == pSoldier->pathing.bLevel)
		{
			continue;			// next merc
		}

		// this function is used only for turning towards closest opponent or changing stance
		// as such, if they AI is in a building,
		// we should ignore people who are on the roof of the same building as the AI
		// sevenfm: check LOS to opponent
		if( bLevel != pSoldier->pathing.bLevel && 
			SameBuilding( pSoldier->sGridNo, sGridNo ) &&			
			!LOS_Virtual(pSoldier, sGridNo, bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
		{
			fSameBuilding = TRUE;
		}

		// avoid switching to empty vehicle/dying opponent/opponent on different level in the same building
		if (pClosestOpponent &&
			(fSameBuilding && !fClosestSameBuilding || 
			pOpponent->IsEmptyVehicle() && !pClosestOpponent->IsEmptyVehicle() ||
			pOpponent->stats.bLife < OKLIFE && !pOpponent->IsZombie() && pClosestOpponent->stats.bLife >= OKLIFE))
		{
			continue;
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (sClosestOpponent == NOWHERE || 
			iRange < iClosestRange || 
			pClosestOpponent && !pClosestOpponent->IsZombie() && pClosestOpponent->stats.bLife < OKLIFE && pOpponent->stats.bLife >= OKLIFE ||
			pClosestOpponent && pClosestOpponent->IsEmptyVehicle() && !pOpponent->IsEmptyVehicle())
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
			pClosestOpponent = pOpponent;
			fClosestSameBuilding = fSameBuilding;
		}
	}

#ifdef DEBUGDECISIONS	
	if (!TileIsOutOfBounds(sClosestOpponent))
	{
		AINumMessage("CLOSEST OPPONENT is at gridno ",sClosestOpponent);
	}
#endif

	if (psGridNo)
	{
		*psGridNo = sClosestOpponent;
	}
	if (pbLevel)
	{
		*pbLevel = bClosestLevel;
	}
	if (pubOpponentID && pClosestOpponent)
	{
		*pubOpponentID = pClosestOpponent->ubID;
	}
	return( sClosestOpponent );
}

UINT8 ClosestKnownOpponentID(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	UINT8 ubClosestOpponentID = NOBODY;

	INT32		sGridNo;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel;
	SOLDIERTYPE *pOpponent;

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if(TileIsOutOfBounds(sGridNo))
		{
			continue;
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		iRange = GetRangeInCellCoordsFromGridNoDiff(sSpot, sGridNo);

		if (ubClosestOpponentID == NOBODY ||
			iRange < iClosestRange ||
			ubClosestOpponentID != NOBODY && MercPtrs[ubClosestOpponentID] && !MercPtrs[ubClosestOpponentID]->IsZombie() && !pSoldier->IsBoxer() && MercPtrs[ubClosestOpponentID]->stats.bLife < OKLIFE && pOpponent->stats.bLife >= OKLIFE ||
			ubClosestOpponentID != NOBODY && MercPtrs[ubClosestOpponentID] && MercPtrs[ubClosestOpponentID]->IsEmptyVehicle() && !pOpponent->IsEmptyVehicle())
		{
			iClosestRange = iRange;
			ubClosestOpponentID = pOpponent->ubID;
		}
	}

	return ubClosestOpponentID;
}

INT16 DistanceToClosestActiveOpponent(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	INT32		sGridNo;
	UINT32		uiLoop;
	INT8		bLevel;
	SOLDIERTYPE *pOpponent;
	INT16		sDistance, sClosestDistance = -1;

	if(!pSoldier || TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if(pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		if(pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sGridNo))
		{
			continue;
		}

		sDistance = PythSpacesAway(sSpot, sGridNo);

		if (sClosestDistance < 0 ||
			sDistance < sClosestDistance)
		{
			sClosestDistance = sDistance;
		}
	}

	return sClosestDistance;
}

INT32 ClosestSeenOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel)
{
	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpponent;

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( PersonalKnowledge(pSoldier, pOpponent->ubID) != SEEN_CURRENTLY )
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownPersonalLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownPersonalLevel(pSoldier, pOpponent->ubID);

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo)
		{
			continue;			// next merc
		}

		// this function is used only for turning towards closest opponent or changing stance
		// as such, if they AI is in a building,
		// we should ignore people who are on the roof of the same building as the AI
		if ( bLevel != pSoldier->pathing.bLevel && 
			SameBuilding( pSoldier->sGridNo, sGridNo ) &&
			!LOS_Virtual(pSoldier, sGridNo, bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
			//!LOS(pSoldier, pOpp, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
		{
			continue;
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (iRange < iClosestRange)
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
		}
	}

#ifdef DEBUGDECISIONS	
	if (!TileIsOutOfBounds(sClosestOpponent))
	{
		AINumMessage("CLOSEST OPPONENT is at gridno ",sClosestOpponent);
	}
#endif

	if (psGridNo)
	{
		*psGridNo = sClosestOpponent;
	}
	if (pbLevel)
	{
		*pbLevel = bClosestLevel;
	}
	return( sClosestOpponent );
}

INT32 ClosestPC( SOLDIERTYPE *pSoldier, INT32 * psDistance )
{
	// used by NPCs... find the closest PC

	// NOTE: skips EPCs!

	UINT8 ubLoop;
	SOLDIERTYPE		*pTargetSoldier;
	INT32					sMinDist = WORLD_MAX;
	INT32					sDist;
	INT32					sGridNo = NOWHERE;

	// Loop through all mercs on player team
	for (ubLoop = gTacticalStatus.Team[gbPlayerNum].bFirstID; ubLoop <= gTacticalStatus.Team[gbPlayerNum].bLastID; ubLoop++)
	{
		pTargetSoldier = Menptr + ubLoop;

		if (!pTargetSoldier->bActive || !pTargetSoldier->bInSector)
		{
			continue;
		}

		// if not conscious, skip him
		if (pTargetSoldier->stats.bLife < OKLIFE)
		{
			continue;
		}

		if ( AM_AN_EPC( pTargetSoldier ) )
		{
			continue;
		}

		sDist = PythSpacesAway(pSoldier->sGridNo,pTargetSoldier->sGridNo);

		// if this PC is not visible to the soldier, then add a penalty to the distance
		// so that we weight in favour of visible mercs
		if ( pTargetSoldier->bTeam != pSoldier->bTeam && pSoldier->aiData.bOppList[ ubLoop ] != SEEN_CURRENTLY )
		{
			sDist += 10;
		}

		if (sDist < sMinDist)
		{
			sMinDist = sDist;
			sGridNo = pTargetSoldier->sGridNo;
		}
	}

	if ( psDistance )
	{
		*psDistance = sMinDist;
	}

	return( sGridNo );
}

// Flugente: like ClosestPC(...), but does not account for covert or not visible mercs
INT32 ClosestUnDisguisedPC( SOLDIERTYPE *pSoldier, INT32 * psDistance )
{
	// used by NPCs... find the closest PC
	// NOTE: skips EPCs!

	UINT8 ubLoop;
	SOLDIERTYPE		*pTargetSoldier;
	INT32					sMinDist = WORLD_MAX;
	INT32					sDist;
	INT32					sGridNo = NOWHERE;

	// Loop through all mercs on player team
	ubLoop = gTacticalStatus.Team[ gbPlayerNum ].bFirstID;
	for ( ; ubLoop <= gTacticalStatus.Team[ gbPlayerNum ].bLastID; ubLoop++)
	{
		pTargetSoldier = Menptr + ubLoop;

		if (!pTargetSoldier->bActive || !pTargetSoldier->bInSector)
			continue;

		// if not conscious, skip him
		if (pTargetSoldier->stats.bLife < OKLIFE)
			continue;

		if ( AM_AN_EPC( pTargetSoldier ) )
			continue;

		if ( pTargetSoldier->usSoldierFlagMask & (SOLDIER_COVERT_CIV|SOLDIER_COVERT_SOLDIER) )
			continue;

		sDist = PythSpacesAway(pSoldier->sGridNo,pTargetSoldier->sGridNo);

		// if this PC is not visible to the soldier, then add a penalty to the distance
		// so that we weight in favour of visible mercs
		if ( pTargetSoldier->bTeam != pSoldier->bTeam && pSoldier->aiData.bOppList[ ubLoop ] != SEEN_CURRENTLY )
			continue;

		if (sDist < sMinDist)
		{
			sMinDist = sDist;
			sGridNo = pTargetSoldier->sGridNo;
		}
	}

	if ( psDistance )
	{
		*psDistance = sMinDist;
	}

	return( sGridNo );
}

INT32 FindClosestClimbPointAvailableToAI( SOLDIERTYPE * pSoldier, INT32 sStartGridNo, INT32 sDesiredGridNo, BOOLEAN fClimbUp )
{
	//INT32	sGridNo;
	//INT32	sRoamingOrigin;
	//INT16	sRoamingRange;

	// sevenfm: safety check
	if (!pSoldier)
	{
		return NOWHERE;
	}

	// sevenfm: don't check roaming range
	return FindClosestBuildingClimbPoint(pSoldier, sStartGridNo, sDesiredGridNo, fClimbUp);

	/*if ( pSoldier->flags.uiStatusFlags & SOLDIER_PC )
	{
		sRoamingOrigin = pSoldier->sGridNo;
		sRoamingRange = MAX_ROAMING_RANGE;
	}
	else
	{
		sRoamingRange = RoamingRange( pSoldier, &sRoamingOrigin );
	}

	// since climbing necessary involves going an extra tile, we compare against 1 less than the roam range...
	// or add 1 to the distance to the climb point

	sGridNo = FindClosestBuildingClimbPoint( pSoldier, sStartGridNo, sDesiredGridNo, fClimbUp );

	if ( PythSpacesAway( sRoamingOrigin, sGridNo ) + 1 > sRoamingRange )
	{
		return( NOWHERE );
	}
	else
	{
		return( sGridNo );
	}*/
}

BOOLEAN ClimbingNecessary( SOLDIERTYPE * pSoldier, INT32 sDestGridNo, INT8 bDestLevel )
{
	if (pSoldier->pathing.bLevel == bDestLevel)
	{
		if ( (pSoldier->pathing.bLevel == 0) || ( gubBuildingInfo[ pSoldier->sGridNo ] == gubBuildingInfo[ sDestGridNo ] ) )
		{
			return( FALSE );
		}
		else // different buildings!
		{
			return( TRUE );
		}
	}
	else
	{
		return( TRUE );
	}
}

INT32 GetInterveningClimbingLocation( SOLDIERTYPE * pSoldier, INT32 sDestGridNo, INT8 bDestLevel, BOOLEAN * pfClimbingNecessary )
{
	if (pSoldier->pathing.bLevel == bDestLevel)
	{
		if ( (pSoldier->pathing.bLevel == 0) || ( gubBuildingInfo[ pSoldier->sGridNo ] == gubBuildingInfo[ sDestGridNo ] ) )
		{
			// on ground or same building... normal!
			*pfClimbingNecessary = FALSE;
			return( NOWHERE );
		}
		else
		{
			// different buildings!
			// yes, pass in same gridno twice... want closest climb-down spot for building we are on!
			*pfClimbingNecessary = TRUE;
			return( FindClosestClimbPointAvailableToAI( pSoldier, pSoldier->sGridNo, pSoldier->sGridNo, FALSE ) );
		}
	}
	else
	{
		*pfClimbingNecessary = TRUE;
		// different levels
		if (pSoldier->pathing.bLevel == 0)
		{
			// got to go UP onto building
			return( FindClosestClimbPointAvailableToAI( pSoldier, pSoldier->sGridNo, sDestGridNo, TRUE ) );
		}
		else
		{
			// got to go DOWN off building
			return( FindClosestClimbPointAvailableToAI( pSoldier, pSoldier->sGridNo, pSoldier->sGridNo, FALSE ) );
		}
	}
}

INT16 EstimatePathCostToLocation( SOLDIERTYPE * pSoldier, INT32 sDestGridNo, INT8 bDestLevel, BOOLEAN fAddCostAfterClimbingUp, BOOLEAN * pfClimbingNecessary, INT32 * psClimbGridNo )
{
	INT16	sPathCost;
	INT32 sClimbGridNo;

	if (pSoldier->pathing.bLevel == bDestLevel)
	{
		if ( (pSoldier->pathing.bLevel == 0) || ( gubBuildingInfo[ pSoldier->sGridNo ] == gubBuildingInfo[ sDestGridNo ] ) )
		{
			// on ground or same building... normal!
			sPathCost = EstimatePlotPath( pSoldier, sDestGridNo, FALSE, FALSE, FALSE, WALKING, FALSE, FALSE, 0);
			if (pfClimbingNecessary)
				*pfClimbingNecessary = FALSE;
			if (psClimbGridNo)
				*psClimbGridNo = NOWHERE;
		}
		else
		{
			// different buildings!
			// yes, pass in same gridno twice... want closest climb-down spot for building we are on!
			sClimbGridNo = FindClosestClimbPointAvailableToAI( pSoldier, sDestGridNo, pSoldier->sGridNo, FALSE );			
			if (TileIsOutOfBounds(sClimbGridNo))
			{
				sPathCost = 0;
			}
			else
			{
				sPathCost = PlotPath( pSoldier, sClimbGridNo, FALSE, FALSE, FALSE, WALKING, FALSE, FALSE, 0 );
				// sevenfm: check if we are already standing at climb gridno
				if (sPathCost != 0 || pSoldier->sGridNo == sClimbGridNo)
				{
					// add in cost of climbing down
					if (fAddCostAfterClimbingUp)
					{
						// add in cost of later climbing up, too
						sPathCost += APBPConstants[AP_CLIMBOFFROOF] + APBPConstants[AP_CLIMBROOF];
						// add in an estimate of getting there after climbing down
						sPathCost += (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK]) * PythSpacesAway( sClimbGridNo, sDestGridNo );
					}
					else
					{
						sPathCost += APBPConstants[AP_CLIMBOFFROOF];
						// add in an estimate of getting there after climbing down, *but not on top of roof*
						sPathCost += (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK]) * PythSpacesAway( sClimbGridNo, sDestGridNo ) / 2;
					}
					if(pfClimbingNecessary)
						*pfClimbingNecessary = TRUE;
					if(psClimbGridNo)
						*psClimbGridNo = sClimbGridNo;
				}
			}
		}
	}
	else
	{
		// sevenfm: check if zombie cannot climb
		if( pSoldier->IsZombie() && !gGameExternalOptions.fZombieCanClimb )
		{
			return 0;
		}

		// different levels
		if (pSoldier->pathing.bLevel == 0)
		{
			//got to go UP onto building
			sClimbGridNo = FindClosestClimbPointAvailableToAI(pSoldier, pSoldier->sGridNo, sDestGridNo, TRUE);
		}
		else
		{
			// got to go DOWN off building
			sClimbGridNo = FindClosestClimbPointAvailableToAI(pSoldier, sDestGridNo, pSoldier->sGridNo, FALSE);
		}

		if (TileIsOutOfBounds(sClimbGridNo))
		{
			sPathCost = 0;
		}
		else
		{
			sPathCost = PlotPath( pSoldier, sClimbGridNo, FALSE, FALSE, FALSE, WALKING, FALSE, FALSE, 0);
			// sevenfm: check if we are already standing at climb gridno
			if (sPathCost != 0 || pSoldier->sGridNo == sClimbGridNo)
			{
				// add in the cost of climbing up or down
				if (pSoldier->pathing.bLevel == 0)
				{
					// must climb up
					sPathCost += APBPConstants[AP_CLIMBROOF];
					if (fAddCostAfterClimbingUp)
					{
						// add to path a rough estimate of how far to go from the climb gridno to the friend
						// estimate walk cost
						sPathCost += (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK]) * PythSpacesAway( sClimbGridNo, sDestGridNo );
					}
				}
				else
				{
					// must climb down
					sPathCost += APBPConstants[AP_CLIMBOFFROOF];
					// add to path a rough estimate of how far to go from the climb gridno to the friend
					// estimate walk cost
					sPathCost += (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK]) * PythSpacesAway( sClimbGridNo, sDestGridNo );
				}
				if(pfClimbingNecessary)
					*pfClimbingNecessary = TRUE;
				if(psClimbGridNo)
					*psClimbGridNo = sClimbGridNo;
			}
		}
	}

	return( sPathCost );
}

BOOLEAN TeamSawEnemyPublic( INT8 bTeam, UINT8 ubMax )
{
	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if( !ValidTeamOpponent(bTeam, pOpponent) )
		{
			continue;
		}

		// check public knowledge
		if( PublicKnowledge(bTeam, pOpponent->ubID) >= SEEN_CURRENTLY &&
			PublicKnowledge(bTeam, pOpponent->ubID) <= ubMax )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

BOOLEAN TeamHeardEnemyPublic(INT8 bTeam, UINT8 ubMax)
{
	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if (!ValidTeamOpponent(bTeam, pOpponent))
		{
			continue;
		}

		// check public knowledge
		if (PublicKnowledge(bTeam, pOpponent->ubID) <= HEARD_THIS_TURN &&
			PublicKnowledge(bTeam, pOpponent->ubID) >= ubMax)
		{
			return(TRUE);
		}
	}

	return(FALSE);
}

BOOLEAN GuySawEnemy( SOLDIERTYPE * pSoldier, UINT8 ubMax )
{
	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this guy SAW an enemy recently...
		if( PersonalKnowledge(pSoldier, pOpponent->ubID) >= SEEN_CURRENTLY &&
			PersonalKnowledge(pSoldier, pOpponent->ubID) <= ubMax )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

BOOLEAN GuyHeardEnemy( SOLDIERTYPE * pSoldier, UINT8 ubMax )
{
	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this guy heard an enemy recently...
		if( PersonalKnowledge(pSoldier, pOpponent->ubID) <= HEARD_THIS_TURN &&
			PersonalKnowledge(pSoldier, pOpponent->ubID) >= ubMax )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

INT32 ClosestReachableFriendInTrouble(SOLDIERTYPE *pSoldier, BOOLEAN * pfClimbingNecessary)
{
	UINT32 uiLoop;
	INT32 sPathCost, sClosestFriend = NOWHERE, sShortestPath = 1000, sClimbGridNo;
	BOOLEAN fClimbingNecessary, fClosestClimbingNecessary = FALSE;
	SOLDIERTYPE *pFriend;

	UINT8 ubSeenEnemies;
	UINT8 ubKnownEnemiesNearby;
	UINT8 ubFriendsNearby;
	UINT8 ubCovertOpponentsNearby;
	//UINT8 ubSoldierDifficulty;
	INT32 sClosestKnownOpponent;
	INT32 sClosestOpponentDistance;
	INT32 sClosestKnownOpponentSoldier;
	INT32 sClosestOpponentDistanceSoldier;
	BOOLEAN fRadio;
	BOOLEAN fCallHelp;
	UINT8 ubClosestFriendID = NOBODY;

	// civilians don't really have any "friends", so they don't bother with this
	if (PTR_CIVILIAN)
	{
		return(NOWHERE);
	}

	BOOLEAN fAllowComplexAI = AllowComplexAI(pSoldier);

	// find new closest friend if this friend is dying, unconscious or became POW
	if (pSoldier->ubClosestReachableFriendID != NOBODY && MercPtrs[pSoldier->ubClosestReachableFriendID])
	{
		pFriend = MercPtrs[pSoldier->ubClosestReachableFriendID];
		if (pFriend->stats.bLife <= 0 || pFriend->IsPOW() || pFriend->IsUnconscious())
			pSoldier->ubClosestReachableFriendID = NOBODY;
	}

	// consider every friend of this soldier (locations assumed to be known)
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pFriend = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pFriend)
		{
			continue;			// next merc
		}

		// speed optimization - check only last found closest friend
		if (!fAllowComplexAI &&
			pSoldier->ubClosestReachableFriendID != NOBODY &&
			pFriend->ubID != pSoldier->ubClosestReachableFriendID)
		{
			continue;
		}

		// if this merc is neutral or NOT on the same side, he's not a friend
		if (pFriend->aiData.bNeutral || (pSoldier->bSide != pFriend->bSide))
		{
			continue;			// next merc
		}

		// if this "friend" is actually US
		if (pFriend->ubID == pSoldier->ubID)
		{
			continue;			// next merc
		}

		// check if he is captured
		if (pFriend->IsPOW())
		{
			continue;
		}

		// check that he is able to call help
		if (pFriend->IsUnconscious())
		{
			continue;
		}

		// if we're already neighbors
		if (SpacesAway(pSoldier->sGridNo, pFriend->sGridNo) == 1)
		{
			continue;			// next merc
		}

		// optimization for slow PC - ignore friend if farther away than closest reachable
		if (!fAllowComplexAI &&
			ubClosestFriendID != NOBODY &&
			MercPtrs[ubClosestFriendID] &&
			PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) > PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubClosestFriendID]->sGridNo))
		{
			continue;
		}

		// check if friend is too far
		if (AICheckSpotTooFar(pSoldier, pFriend->sGridNo) || !gfTurnBasedAI && PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) > TACTICAL_RANGE)
		{
			continue;
		}

		// check roaming range
		if (!AICheckRoamingRange(pSoldier, pFriend->sGridNo))
		{
			continue;
		}

		// medics only help wounded friends
		if (AIRoleMedic(pSoldier) &&
			!(pFriend->iHealableInjury >= MIN_AI_HEALABLE_INJURY || pFriend->bBleeding > MIN_BLEEDING_THRESHOLD && pFriend->iHealableInjury >= MIN_HEALABLE_INJURY))
		{
			continue;
		}

		// help last soldier who raised alert
		// help if friend has more recently seen opponents than friends nearby
		// help if friend is under fire
		// check only enemies that are known public
		ubSeenEnemies = CountSeenEnemiesLastTurn(pFriend, TRUE);
		ubFriendsNearby = CountNearbyFriends(pFriend, pFriend->sGridNo, TACTICAL_RANGE / 4);
		ubCovertOpponentsNearby = CountNearbyKnownCovertOpponents(pFriend);
		ubKnownEnemiesNearby = CountPublicKnownEnemies(pFriend, pFriend->sGridNo, TACTICAL_RANGE / 2);
		//ubSoldierDifficulty = SoldierDifficultyLevel(pFriend);
		sClosestKnownOpponent = ClosestKnownOpponent(pFriend, NULL, NULL);
		sClosestOpponentDistance = 0;
		if (!TileIsOutOfBounds(sClosestKnownOpponent))
		{
			sClosestOpponentDistance = PythSpacesAway(pFriend->sGridNo, sClosestKnownOpponent);
		}
		sClosestKnownOpponentSoldier = ClosestKnownOpponent(pSoldier, NULL, NULL);
		sClosestOpponentDistanceSoldier = 0;
		if (!TileIsOutOfBounds(sClosestKnownOpponentSoldier))
		{
			sClosestOpponentDistanceSoldier = PythSpacesAway(pSoldier->sGridNo, sClosestKnownOpponentSoldier);
		}

		fRadio = FALSE;
		fCallHelp = FALSE;

		if (pFriend->RadioCallValue() > 0)
			fRadio = TRUE;

		// don't allow helping from a building to outside
		if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
			InARoom(pSoldier->sGridNo, NULL) &&
			!InARoom(pFriend->sGridNo, NULL) &&
			!AIRoleMedic(pSoldier) &&
			(CountSeenEnemiesLastTurn(pSoldier, TRUE) > 0 || !TileIsOutOfBounds(sClosestKnownOpponentSoldier) && sClosestOpponentDistanceSoldier < DAY_VISION_RANGE * 2))
		{
			continue;
		}

		// don't allow helping if too many enemies nearby
		if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
			!AIRoleMedic(pSoldier) &&
			CountPublicKnownEnemies(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE) > CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE))
		{
			continue;
		}

		// allow help if more publicly known enemies than friends nearby
		if (ubSeenEnemies > ubFriendsNearby ||
			ubKnownEnemiesNearby > ubFriendsNearby)
		{
			fCallHelp = TRUE;
		}

		// medic can help wounded friends
		if (AIRoleMedic(pSoldier) &&
			(pFriend->iHealableInjury >= MIN_AI_HEALABLE_INJURY || pFriend->bBleeding > MIN_BLEEDING_THRESHOLD && pFriend->iHealableInjury >= MIN_HEALABLE_INJURY))
		{
			fCallHelp = TRUE;
		}

		// allow help if friend is in a building and has no friends nearby
		if (pSoldier->bTeam == ENEMY_TEAM &&
			!AIRoleMedic(pSoldier) &&
			!AICheckShortWeaponRange(pSoldier) &&
			InARoom(pFriend->sGridNo, NULL) &&
			!InARoom(pSoldier->sGridNo, NULL) &&
			pFriend->pathing.bLevel == 0 &&
			pSoldier->pathing.bLevel == 0 &&
			ubFriendsNearby < 1 &&
			!CorpseWarning(pFriend, pFriend->sGridNo, pFriend->pathing.bLevel) &&
			!TileIsOutOfBounds(sClosestKnownOpponent) &&
			sClosestOpponentDistance < TACTICAL_RANGE)
		{
			fCallHelp = TRUE;
		}

		// allow snipers and machinegunners to help friends on roof
		/*if (pSoldier->bTeam == ENEMY_TEAM &&
		(AICheckIsSniper(pSoldier) || AICheckIsMachinegunner(pSoldier)) &&
		!AICheckIsOfficer(pSoldier) &&
		!AICheckIsMortarOperator(pSoldier) &&
		!InARoom(pSoldier->sGridNo, NULL) &&
		pFriend->pathing.bLevel > 0 &&
		pSoldier->pathing.bLevel == 0 &&
		ubFriendsNearby < 1 &&
		!CorpseWarning(pFriend, pFriend->sGridNo, pFriend->pathing.bLevel) &&
		!TileIsOutOfBounds(sClosestKnownOpponent) &&
		sClosestOpponentDistance < DAY_VISION_RANGE * 2)
		{
		fCallHelp = TRUE;
		}*/

		// zombies always call for help if they know enemy position
		if (pSoldier->IsZombie() && pFriend->IsZombie() && !TileIsOutOfBounds(sClosestKnownOpponent))
		{
			fCallHelp = TRUE;
		}

		// medics should help wounded friends
		/*if (pSoldier->bTeam == ENEMY_TEAM &&
			AICheckIsMedic(pSoldier) &&
			!AICheckIsMedic(pFriend) &&
			pFriend->bTeam == pSoldier->bTeam &&
			pFriend->pathing.bLevel == pSoldier->pathing.bLevel &&
			!pFriend->aiData.bUnderFire &&
			pFriend->bBleeding > MIN_BLEEDING_THRESHOLD &&
			GetClosestMedicSoldierID(pFriend, TACTICAL_RANGE, pFriend->bTeam) == NOBODY)
		{
			fCallHelp = TRUE;
		}*/

		// officers should help groups of friendly soldiers
		/*if (pSoldier->bTeam == ENEMY_TEAM &&
			AICheckIsOfficer(pSoldier) &&
			!AICheckIsOfficer(pFriend) &&
			pFriend->bTeam == pSoldier->bTeam &&
			GuySawEnemy(pFriend) &&
			CountNearbyFriends(pFriend, pFriend->sGridNo, TACTICAL_RANGE / 2) > 2 &&
			GetClosestOfficerSoldierID(pSoldier, TACTICAL_RANGE, pSoldier->bTeam) != NOBODY &&
			GetClosestOfficerSoldierID(pFriend, TACTICAL_RANGE, pFriend->bTeam) == NOBODY)
		{
			fCallHelp = TRUE;
		}*/

		// help friends who see covert opponents
		if (pSoldier->bTeam == ENEMY_TEAM &&
			!AIRoleMedic(pSoldier) &&
			(pFriend->aiData.bAlertStatus >= STATUS_RED || SectorCurfew(FALSE) || NightLight()) &&
			ubCovertOpponentsNearby > 0 &&
			ubFriendsNearby < ubCovertOpponentsNearby)
		{
			fCallHelp = TRUE;
		}

		// help friend who called for help (fake alert)
		if (pSoldier->bTeam == ENEMY_TEAM &&
			!AIRoleMedic(pSoldier) &&
			fRadio &&
			(pSoldier->aiData.bAlertStatus < STATUS_RED || !gfTurnBasedAI && TileIsOutOfBounds(sClosestKnownOpponent) && TileIsOutOfBounds(sClosestKnownOpponentSoldier)) &&
			ubFriendsNearby < 1 + gGameOptions.fNewTraitSystem ? NUM_SKILL_TRAITS(pFriend, SQUADLEADER_NT) : 0)
		{
			fCallHelp = TRUE;
		}

		// help if friend is alone at night, underground or in restricted sector, or after the alert was raised
		if (pSoldier->bTeam == ENEMY_TEAM &&
			(pSoldier->aiData.bAlertStatus < STATUS_RED || !gfTurnBasedAI && TileIsOutOfBounds(sClosestKnownOpponent) && TileIsOutOfBounds(sClosestKnownOpponentSoldier)) &&
			!AIRoleMedic(pSoldier) &&
			(NightTime() || AICheckUnderground() || SectorCurfew(FALSE) || gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition) &&
			ubFriendsNearby < max(1, SectorCurfew(FALSE)))
		{
			fCallHelp = TRUE;
		}

		if (!fCallHelp)
		{
			continue;			// next merc
		}		

		// get the AP cost to go to this friend's gridno
		sPathCost = EstimatePathCostToLocation(pSoldier, pFriend->sGridNo, pFriend->pathing.bLevel, FALSE, &fClimbingNecessary, &sClimbGridNo);

		// if we can get there
		if (sPathCost != 0)
		{
			//sprintf(tempstr,"Path cost to friend %s's location is %d",pFriend->name,pathCost);
			//PopMessage(tempstr);

			if (sPathCost < sShortestPath || ubClosestFriendID == NOBODY)
			{
				if (fClimbingNecessary)
				{
					sClosestFriend = sClimbGridNo;
				}
				else
				{
					sClosestFriend = pFriend->sGridNo;
				}

				sShortestPath = sPathCost;
				fClosestClimbingNecessary = fClimbingNecessary;
				ubClosestFriendID = pFriend->ubID;
			}
		}
	}
	
	// remember closest reachable friend
	pSoldier->ubClosestReachableFriendID = ubClosestFriendID;

#ifdef DEBUGDECISIONS	
	if (!TileIsOutOfBounds(sClosestFriend))
	{
		AINumMessage("CLOSEST FRIEND is at gridno ",sClosestFriend);
	}
#endif

	*pfClimbingNecessary = fClosestClimbingNecessary;
	return(sClosestFriend);
}

INT16 DistanceToClosestFriend( SOLDIERTYPE * pSoldier )
{
	// find the distance to the closest person on the same team
	UINT8 ubLoop;
	SOLDIERTYPE		*pTargetSoldier;
	INT16					sMinDist = 1000;
	INT16					sDist;

	// Loop through all mercs on player team
	ubLoop = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID;

	for ( ; ubLoop <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID; ubLoop++ )
	{
		if (ubLoop == pSoldier->ubID)
		{
			// same guy - continue!
			continue;
		}

		pTargetSoldier = Menptr + ubLoop;

		if ( pSoldier->bActive && pSoldier->bInSector )
		{
			if (!pTargetSoldier->bActive || !pTargetSoldier->bInSector)
			{
				continue;
			}
			// if not conscious, skip him
			else if (pTargetSoldier->stats.bLife < OKLIFE)
			{
				continue;
			}
		}
		else
		{
			// compare sector #s
			if ( (pSoldier->sSectorX != pTargetSoldier->sSectorX) ||
				(pSoldier->sSectorY != pTargetSoldier->sSectorY) ||
				(pSoldier->bSectorZ != pTargetSoldier->bSectorZ) )
			{
				continue;
			}
			else if (pTargetSoldier->stats.bLife < OKLIFE)
			{
				continue;
			}
			else
			{
				// well there's someone who could be near
				return( 1 );
			}
		}

		sDist = SpacesAway(pSoldier->sGridNo,pTargetSoldier->sGridNo);

		if (sDist < sMinDist)
		{
			sMinDist = sDist;
		}
	}

	return( sMinDist );
}

BOOLEAN InGasOrSmoke( SOLDIERTYPE *pSoldier, INT32 sGridNo )
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sGridNo) )
		return FALSE;

	// smoke
	if (gpWorldLevelData[sGridNo].usExtFlags[ pSoldier->pathing.bLevel ] & (MAPELEMENT_EXT_SMOKE|MAPELEMENT_EXT_SIGNAL_SMOKE) )
		return TRUE;

	return InGas(pSoldier,sGridNo);
}

INT16 InWaterOrGas(SOLDIERTYPE *pSoldier, INT32 sGridNo)
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sGridNo) )
		return FALSE;

	if (WaterTooDeepForAttacks( sGridNo, pSoldier->pathing.bLevel ))
	{
		return(TRUE);
	}

	return (INT16)InGas( pSoldier, sGridNo );
}

BOOLEAN InGasSpot( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT8 bLevel, BOOLEAN fCheckMask )
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sGridNo) )
		return FALSE;

	// tear gas
	if ((gpWorldLevelData[sGridNo].usExtFlags[bLevel] & MAPELEMENT_EXT_TEARGAS) && 
		(!fCheckMask || !pSoldier->WearsGasMask() && !pSoldier->HasGasMask()))
	{
		return TRUE;
	}
	// fire/creature/mustard gas
	// sevenfm: avoid mustard gas even when wearing gas mask
	if( gpWorldLevelData[sGridNo].usExtFlags[bLevel] & (MAPELEMENT_EXT_BURNABLEGAS|MAPELEMENT_EXT_CREATUREGAS|MAPELEMENT_EXT_MUSTARDGAS))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN InGas( SOLDIERTYPE *pSoldier, INT32 sGridNo )
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sGridNo) )
		return FALSE;

	if( InGasSpot(pSoldier, sGridNo, pSoldier->pathing.bLevel) )
	{
		return TRUE;
	}

	//WarmSteel - One square away from gas is still considered in gas, because it could expand any moment.
	//Note: this only works for gas that expands with one tile, but hey it's better than nothing!
	int iNeighbourGridNo;
	for(int iDir = 0; iDir < NUM_WORLD_DIRECTIONS; ++iDir)
	{
		iNeighbourGridNo = sGridNo + DirectionInc(iDir);

		if( !TileIsOutOfBounds(iNeighbourGridNo) && InGasSpot(pSoldier, iNeighbourGridNo, pSoldier->pathing.bLevel) )
		{
			return TRUE;
		}
	}

	// sevenfm: check if soldier is in spot withing smoke effect's radius
	if( CheckSmokeEffectWithRadiusAndRoomCheck(sGridNo, pSoldier->pathing.bLevel, MUSTARDGAS_SMOKE_EFFECT) || 
		CheckSmokeEffectWithRadiusAndRoomCheck(sGridNo, pSoldier->pathing.bLevel, CREATURE_SMOKE_EFFECT) ||
		CheckSmokeEffectWithRadiusAndRoomCheck(sGridNo, pSoldier->pathing.bLevel, BURNABLEGAS_SMOKE_EFFECT) )
	{
		return TRUE;
	}

	return(FALSE);
}

BOOLEAN WearGasMaskIfAvailable( SOLDIERTYPE * pSoldier )
{
	INT8		bSlot, bNewSlot;

	bSlot = FindGasMask( pSoldier );
	if ( bSlot == NO_SLOT )
	{
		return( FALSE );
	}
	if ( bSlot == HEAD1POS || bSlot == HEAD2POS || bSlot == HELMETPOS )
	{
		return( FALSE );
	}
	if ( pSoldier->inv[ HEAD1POS ].exists() == false )
	{
		bNewSlot = HEAD1POS;
	}
	else if ( pSoldier->inv[ HEAD2POS ].exists() == false )
	{
		bNewSlot = HEAD2POS;
	}
	else
	{
		// screw it, going in position 1 anyhow
		bNewSlot = HEAD1POS;
	}

	RearrangePocket( pSoldier, bSlot, bNewSlot, TRUE );

	// sevenfm: r8315
	if ( pSoldier->bTeam == gbPlayerNum )
	{
		pSoldier->DeleteSoldierLight( );
		pSoldier->PositionSoldierLight( );
	}

	return( TRUE );
}

BOOLEAN RemoveGasMask(SOLDIERTYPE * pSoldier)
{
	INT8		bSlot;

	bSlot = FindGasMask(pSoldier);
	if (bSlot == NO_SLOT)
	{
		return(FALSE);
	}

	if (bSlot == HEAD1POS || bSlot == HEAD2POS)
	{
		// put gas mask back into inventory
		// Make a copy of item
		gTempObject = pSoldier->inv[bSlot];
		if (!AutoPlaceObject(pSoldier, &gTempObject, TRUE))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("failed to put gas mask into inventory, try dropping it"));
			AddItemToPool(pSoldier->sGridNo, &gTempObject, pSoldier->bVisible, pSoldier->pathing.bLevel, 0, -1);
		}
		else
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("successfully put gas mask into inventory"));
		}
		DeleteObj(&pSoldier->inv[bSlot]);
		
		/*if (AutoPlaceObject(pSoldier, &(pSoldier->inv[bSlot]), TRUE, HANDPOS))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("successfully put gas mask into inventory"));
		}
		else
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("failed to put gas mask into inventory"));
			AddItemToPool(pSoldier->sGridNo, &(pSoldier->inv[bSlot]), pSoldier->bVisible, pSoldier->pathing.bLevel, 0, -1);
			return FALSE;
		}*/
	}

	// sevenfm: r8315
	if (pSoldier->bTeam == gbPlayerNum)
	{
		pSoldier->DeleteSoldierLight();
		pSoldier->PositionSoldierLight();
	}

	return TRUE;
}

BOOLEAN InLightAtNight(INT32 sGridNo, INT8 bLevel)
{
	UINT8 ubBackgroundLightLevel;

	// do not consider us to be "in light" if we're in an underground sector
	if (gbWorldSectorZ > 0)
	{
		return(FALSE);
	}

	ubBackgroundLightLevel = GetTimeOfDayAmbientLightLevel();

	if (ubBackgroundLightLevel < NORMAL_LIGHTLEVEL_DAY + 2)
	{
		// don't consider it nighttime, too close to daylight levels
		return(FALSE);
	}

	// could've been placed here, ignore the light
	// sevenfm: always check light in a room
	/*if (InARoom(sGridNo, NULL))
	{
		return(FALSE);
	}*/

	// NB light levels are backwards, so a lower light level means the
	// spot in question is BRIGHTER

	if (LightTrueLevel(sGridNo, bLevel) < ubBackgroundLightLevel)
	{
		return(TRUE);
	}

	return(FALSE);
}

INT32 CalcManThreatValue( SOLDIERTYPE *pEnemy, INT32 sMyGrid, UINT8 ubReduceForCover, SOLDIERTYPE *pMe )
{
	INT32	iThreatValue = 0;
	INT32	sDist;

	// If man is inactive, at base, on assignment, dead, unconscious
	if (!pEnemy->bActive || !pEnemy->bInSector || !pEnemy->stats.bLife)
	{
		// he's no threat at all, return a negative number
		iThreatValue = -999;
		return(iThreatValue);
	}

	// in boxing mode, let only a boxer be considered a threat.
	if ( (gTacticalStatus.bBoxingState == BOXING) && !(pEnemy->IsBoxer()) )
	{
		iThreatValue = -999;
		return( iThreatValue );
	}

	if (CREATURE_OR_BLOODCAT(pEnemy))
	{
		// health (1-100)
		iThreatValue += pEnemy->stats.bLife;
		// bleeding (more attractive!) (1-100)
		iThreatValue += pEnemy->bBleeding;
		// decrease according to distance
		if(!TileIsOutOfBounds(sMyGrid))
		{
			iThreatValue = (iThreatValue * 10) / (10 + PythSpacesAway(sMyGrid, pEnemy->sGridNo));
		}
	}
	else if (pEnemy->IsZombie())
	{
		iThreatValue += pEnemy->stats.bLife;
		iThreatValue += EffectiveStrength(pEnemy, FALSE);
		if (gGameOptions.fNewTraitSystem)
		{
			iThreatValue += iThreatValue * NUM_SKILL_TRAITS(pEnemy, MARTIAL_ARTS_NT) * 25 / 100;;
			iThreatValue += iThreatValue * NUM_SKILL_TRAITS(pEnemy, ATHLETICS_NT) * 25 / 100;
		}
	}
	else
	{
		// ADD twice the man's level (2-20)
		//iThreatValue += EffectiveExpLevel(pEnemy, TRUE); // SANDRO - find precise effective exp level
		iThreatValue += 2 * pEnemy->stats.bExpLevel;

		// ADD man's total action points (10-35)
		// sevenfm: r7810 fix
		//iThreatValue += pEnemy->CalcActionPoints();
		//iThreatValue += 25 * pEnemy->CalcActionPoints() / APBPConstants[AP_MAXIMUM];		
		//iThreatValue += 25 * AICalcActionPoints(pEnemy) / APBPConstants[AP_MAXIMUM];
		iThreatValue += 25 * pEnemy->bInitialActionPoints / APBPConstants[AP_MAXIMUM];

		// ADD 1/2 of man's current action points (4-17)
		// sevenfm: r7810 fix
		//iThreatValue += (pEnemy->bActionPoints / 2);
		iThreatValue += 25 * pEnemy->bActionPoints / (APBPConstants[AP_MAXIMUM] * 2);

		// ADD 1/10 of man's current health (0-10)
		iThreatValue += (pEnemy->stats.bLife / 10);

		if (pEnemy->bAssignment < ON_DUTY )
		{
			// ADD 1/4 of man's protection percentage (0-25)
			iThreatValue += ArmourPercent( pEnemy ) / 4;

			// ADD 1/5 of man's marksmanship skill (0-20)
			iThreatValue += (pEnemy->stats.bMarksmanship / 5);

			// ADD the deadliness of the item(weapon) he's holding (0-50)
			// sevenfm: search for weapon if not in hand, take into account status, increase threat if scope used
			iThreatValue += (INT32)(10 + AIGunScopeMagFactor(pEnemy)) * AIGunDeadliness(pEnemy) * (100 + AIGunStatus(pEnemy)) / (20 * 200);
		}

		// SUBTRACT 1/5 of man's bleeding (0-20)
		iThreatValue -= (pEnemy->bBleeding / 5);

		// SUBTRACT 1/10 of man's breath deficiency (0-10)
		iThreatValue -= ((100 - pEnemy->bBreath) / 10);

		// SUBTRACT man's current shock value
		iThreatValue -= pEnemy->aiData.bWoundShock / 2;
		// SUBTRACT suppression shock
		iThreatValue -= pEnemy->ShockLevelPercent() / 4;
	}

	// if I have a specifically defined spot where I'm at (sometime I don't!)	
	if (!TileIsOutOfBounds(sMyGrid))
	{
		// ADD 10% if man's already been shooting at me
		if (pEnemy->sLastTarget == sMyGrid)
		{
			iThreatValue += iThreatValue / 10;
		}
		else
		{
			// ADD 5% if man's already facing me
			//if (pEnemy->ubDirection == atan8(CenterX(pEnemy->sGridNo),CenterY(pEnemy->sGridNo),CenterX(sMyGrid),CenterY(sMyGrid)))
			if (pEnemy->ubDirection == AIDirection(pEnemy->sGridNo, sMyGrid))
			{
				iThreatValue += iThreatValue / 20;
			}
		}

		// 10% penalty if I have cover from enemy
		if (!pEnemy->IsZombie() &&
			AnyCoverFromSpot(sMyGrid, pMe->pathing.bLevel, pEnemy->sGridNo, pEnemy->pathing.bLevel))
		{
			iThreatValue -= iThreatValue * min(TACTICAL_RANGE, PythSpacesAway(sMyGrid, pEnemy->sGridNo)) / (TACTICAL_RANGE * 10);
		}

		// sevenfm: reduce threat value for cowering enemy
		if (pEnemy->IsCowering())
		{
			iThreatValue /= 2;
		}

		// add more threat to tanks
		if(pEnemy->IsTank())
		{
			iThreatValue += 100;
		}
	}

	// if this man is conscious
	if (pEnemy->stats.bLife >= OKLIFE)
	{
		// and we were told to reduce threat for my cover		
		if (ubReduceForCover && (!TileIsOutOfBounds(sMyGrid) && !TileIsOutOfBounds(pEnemy->sGridNo) ))
		{
			if (pEnemy->IsZombie())
			{
				sDist = PythSpacesAway(sMyGrid, pEnemy->sGridNo);
				if( sDist < DAY_VISION_RANGE / 2 )
					iThreatValue = iThreatValue * sDist / (2 * DAY_VISION_RANGE);
				else
					iThreatValue = iThreatValue * sDist * sDist / (DAY_VISION_RANGE * DAY_VISION_RANGE);
			}
			else
			{
				// Reduce iThreatValue to same % as the chance HE has shoot through at ME
				//iThreatValue = (iThreatValue * ChanceToGetThrough( pEnemy, myGrid, FAKE, ACTUAL, TESTWALLS, 9999, M9PISTOL, NOT_FOR_LOS)) / 100;
				//iThreatValue = (iThreatValue * SoldierTo3DLocationChanceToGetThrough( pEnemy, myGrid, FAKE, ACTUAL, TESTWALLS, 9999, M9PISTOL, NOT_FOR_LOS)) / 100;
				iThreatValue = (iThreatValue * SoldierToLocationChanceToGetThrough( pEnemy, sMyGrid, pMe->pathing.bLevel, 1, pMe->ubID ) ) / 100;
			}			
		}
	}
	else
	{
		// if he's still something of a threat
		if (iThreatValue > 0)
		{
			// drastically reduce his threat value (divide by 5 to 18)
			iThreatValue /= (4 + (OKLIFE - pEnemy->stats.bLife));
		}
	}	

	// threat value of any opponent can never drop below 1
	if (iThreatValue < 1)
	{
		iThreatValue = 1;
	}

	//sprintf(tempstr,"%s's iThreatValue = ",pEnemy->name);
	//NumMessage(tempstr,iThreatValue);

#ifdef BETAVERSION	// unnecessary for real release
	// NOTE: maximum is about 200 for a healthy Mike type with a mortar!
	if (iThreatValue > 250)
	{
		sprintf(tempstr,"CalcManThreatValue: WARNING - %d has a very high threat value of %d",pEnemy->ubID,iThreatValue);

#ifdef TESTVERSION
		PopMessage(tempstr);
#endif

	}
#endif

	return iThreatValue;
}

// sevenfm: ONGUARD, POINTPATROL, RNDPTPATROL - max roaming if seen enemy recently or under fire
// STATIONARY/SNIPER - 5 tiles roaming
// other orders - max roaming if enemy position is known
INT16 RoamingRange(SOLDIERTYPE *pSoldier, INT32 * pusFromGridNo)
{
	BOOLEAN fOppPosKnown = FALSE;
	BOOLEAN fInCombat = FALSE;
	BOOLEAN fRedAlert = FALSE;
	BOOLEAN fPublicNoise = FALSE;
	//BOOLEAN fFriendsNeedHelp = FALSE;

	// sevenfm: in case we want to call this for player mercs
	if ( pSoldier->flags.uiStatusFlags & SOLDIER_PC )
	{
		*pusFromGridNo = pSoldier->sGridNo;
		return MAX_ROAMING_RANGE;
	}

	// sevenfm: for zombies, allow max range
	if( pSoldier->IsZombie() )
	{
		*pusFromGridNo = pSoldier->sGridNo;
		return MAX_ROAMING_RANGE;
	}

	if ( CREATURE_OR_BLOODCAT( pSoldier ) )
	{
		//if ( pSoldier->aiData.bAlertStatus == STATUS_BLACK )
		if (pSoldier->aiData.bAlertStatus >= STATUS_RED)
		{
			*pusFromGridNo = pSoldier->sGridNo; // from current position!
			return(MAX_ROAMING_RANGE);
		}
	}
	if ( pSoldier->aiData.bOrders == POINTPATROL || pSoldier->aiData.bOrders == RNDPTPATROL )
	{
		// roam near NEXT PATROL POINT, not from where merc starts out
		*pusFromGridNo = pSoldier->aiData.sPatrolGrid[pSoldier->aiData.bNextPatrolPnt];
	}
	else
	{
		// roam around where mercs started
		//*pusFromGridNo = pSoldier->sInitialGridNo;
		*pusFromGridNo = pSoldier->aiData.sPatrolGrid[0];
	}

	if( GuyKnowsEnemyPosition(pSoldier) )
	{
		fOppPosKnown = TRUE;
	}
	if( pSoldier->aiData.bAlertStatus >= STATUS_RED )
	{
		fRedAlert = TRUE;
	}
	if (pSoldier->aiData.bAlertStatus == STATUS_YELLOW &&
		gubPublicNoiseVolume[pSoldier->bTeam] != 0 &&
		gsPublicNoiseGridNo[pSoldier->bTeam] != NOWHERE)
	{
		fPublicNoise = TRUE;
	}
	if (pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier, SEEN_3_TURNS_AGO) || fPublicNoise)
	{
		fInCombat = TRUE;
	}

	switch (pSoldier->aiData.bOrders)
	{
	// JA2 GOLD: give non-NPCs a 5 tile roam range for cover in combat when being shot at
	case STATIONARY:	if (pSoldier->ubProfile != NO_PROFILE || !fInCombat)
							return 0;
						else
							return 5;

	case ONGUARD:		if (!fRedAlert)
							return 5;
						else
							return MAX_ROAMING_RANGE;

	case CLOSEPATROL:	if (!fRedAlert)
							return 15;
						else
							return MAX_ROAMING_RANGE;

	case POINTPATROL:	if (!fRedAlert)
							return 10;	// from nextPatrolGrid, not whereIWas
						else
							return MAX_ROAMING_RANGE;

	case RNDPTPATROL:	if (!fRedAlert)
							return 10;	// from nextPatrolGrid, not whereIWas
						else
							return MAX_ROAMING_RANGE;

	case FARPATROL:		if (!fRedAlert && !fPublicNoise)
							return 25;
						else
							return MAX_ROAMING_RANGE;

	case ONCALL:		if (!fRedAlert && !fPublicNoise)
							return 10;
						else
							return MAX_ROAMING_RANGE;

	case SEEKENEMY:		*pusFromGridNo = pSoldier->sGridNo; // from current position!
						return MAX_ROAMING_RANGE;
	default:
#ifdef BETAVERSION
		sprintf(tempstr,"%s has invalid orders = %d",pSoldier->GetName(),pSoldier->aiData.bOrders);
		PopMessage(tempstr);
#endif
		return(0);
	}
}

void RearrangePocket(SOLDIERTYPE *pSoldier, INT8 bPocket1, INT8 bPocket2, UINT8 bPermanent)
{
	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"RearrangePocket");
	// NB there's no such thing as a temporary swap for now...
	// 0verhaul:  There is now!  If not permanent, don't lose weapon ready status because the
	// weapon will be restored after the trial situation is finished.
	//SwapObjs( &(pSoldier->inv[bPocket1]), &(pSoldier->inv[bPocket2]) );
	SwapObjs( pSoldier, bPocket1, bPocket2, bPermanent );

	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"RearrangePocket done");
}

BOOLEAN FindBetterSpotForItem( SOLDIERTYPE * pSoldier, INT8 bSlot )
{
	// looks for a place in the slots to put an item in a hand or armour
	// position, and moves it there.
	if (bSlot >= BIGPOCKSTART)
	{
		return( FALSE );
	}
	if (pSoldier->inv[bSlot].exists() == false)
	{
		// well that's just fine then!
		return( TRUE );
	}

	if( FitsInSmallPocket(&pSoldier->inv[bSlot]) )
	{
		// try a small pocket first
		bSlot = FindEmptySlotWithin( pSoldier, SMALLPOCKSTART, NUM_INV_SLOTS );
		if (bSlot == NO_SLOT)
		{
			bSlot = FindEmptySlotWithin( pSoldier, BIGPOCKSTART, MEDPOCKFINAL );
		}
	}
	else
	{		
		// then we're looking for a big pocket
		bSlot = FindEmptySlotWithin( pSoldier, BIGPOCKSTART, MEDPOCKFINAL );
	}

	if (bSlot == NO_SLOT)
	{
		return( FALSE );
	}

	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"findbetterspotforitem: swapping items");
	RearrangePocket(pSoldier, HANDPOS, bSlot, FOREVER );		
	return( TRUE );
}

UINT8 GetTraversalQuoteActionID( INT8 bDirection )
{
	switch( bDirection )
	{
	case NORTHEAST: // east
		return( QUOTE_ACTION_ID_TRAVERSE_EAST );

	case SOUTHEAST: // south
		return( QUOTE_ACTION_ID_TRAVERSE_SOUTH );

	case SOUTHWEST: // west
		return( QUOTE_ACTION_ID_TRAVERSE_WEST );

	case NORTHWEST: // north
		return( QUOTE_ACTION_ID_TRAVERSE_NORTH );

	default:
		return( 0 );
	}
}

UINT8 SoldierDifficultyLevel( SOLDIERTYPE * pSoldier )
{
	INT8 bDifficultyBase;
	INT8 bDifficulty;

	DebugMsg(TOPIC_JA2AI,DBG_LEVEL_3,String("SoldierDifficultyLevel"));
	// difficulty modifier ranges from 0 to 100
	// and we want to end up with a number between 0 and 4 (4=hardest)
	// to a base of 1, divide by 34 to get a range from 1 to 3
	bDifficultyBase = 1 + ( CalcDifficultyModifier( pSoldier->ubSoldierClass ) / 34 );

	switch( pSoldier->ubSoldierClass )
	{
	case SOLDIER_CLASS_ADMINISTRATOR:
		bDifficulty = bDifficultyBase - 1;
		break;

	case SOLDIER_CLASS_ARMY:
		bDifficulty = bDifficultyBase;
		break;

	case SOLDIER_CLASS_ELITE:
		bDifficulty = bDifficultyBase + 1;
		break;

		// hard code militia;
	case SOLDIER_CLASS_GREEN_MILITIA:
		bDifficulty = 2;
		break;

	case SOLDIER_CLASS_REG_MILITIA:
		bDifficulty = 3;
		break;

	case SOLDIER_CLASS_ELITE_MILITIA:
		bDifficulty = 4;
		break;

	case SOLDIER_CLASS_ZOMBIE:
		bDifficulty = bDifficultyBase;
		break;

	default:
		if (pSoldier->bTeam == CREATURE_TEAM)
		{
			bDifficulty = bDifficultyBase + pSoldier->pathing.bLevel / 4;
		}
		else // civ...
		{
			bDifficulty = (bDifficultyBase + pSoldier->pathing.bLevel / 4) - 1;
		}
		break;

	}

	bDifficulty = max( bDifficulty, 0 );
	bDifficulty = min( bDifficulty, 4 );

	return( (UINT8) bDifficulty );
}

BOOLEAN ValidCreatureTurn( SOLDIERTYPE * pCreature, INT8 bNewDirection )
{
	INT8	bDirChange;
	INT8	bTempDir;
	INT8	bLoop;
	BOOLEAN	fFound;

	bDirChange = (INT8) QuickestDirection( pCreature->ubDirection, bNewDirection );

	for( bLoop = 0; bLoop < 2; bLoop++ )
	{
		fFound = TRUE;

		bTempDir = pCreature->ubDirection;

		do
		{

			bTempDir += bDirChange;
			if (bTempDir < NORTH)
			{
				bTempDir = NORTHWEST;
			}
			else if (bTempDir > NORTHWEST)
			{
				bTempDir = NORTH;
			}
			if (!pCreature->InternalIsValidStance( bTempDir, ANIM_STAND ))
			{
				fFound = FALSE;
				break;
			}

		} while ( bTempDir != bNewDirection );

		if ( fFound )
		{
			break;
		}
		else if ( bLoop > 0 )
		{
			// can't find a dir!
			return( FALSE );
		}
		else
		{
			// try the other direction
			bDirChange = bDirChange * -1;
		}
	}

	return( TRUE );
}

INT32 RangeChangeDesire( SOLDIERTYPE * pSoldier )
{
	INT32 iRangeFactorMultiplier;

	iRangeFactorMultiplier = max(pSoldier->aiData.bAIMorale - 1, 0);

	// sevenfm: retreat
	if( pSoldier->RetreatActive())
	{
		return 0;
	}

	// civilians only run away
	if( pSoldier->aiData.bNeutral )
	{
		return 0;
	}

	INT8 bBonus = 0;
	// sevenfm: if we have no weapons, try to get closer to enemy
	if ( !AICheckHasGun(pSoldier) || pSoldier->RushAttackAdvance() )
	{
		bBonus = 2;
	}
	// bonus if weapon range is short
	else if (GuySawEnemy(pSoldier, SEEN_LAST_TURN) && AICheckShortWeaponRange(pSoldier) && !AISpecialRole(pSoldier) || pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE])
	{
		bBonus = 1;
	}

	switch (pSoldier->aiData.bAttitude)
	{
	//case DEFENSIVE:		iRangeFactorMultiplier +=	max(-1, bBonus); break;
	case DEFENSIVE:		iRangeFactorMultiplier +=	max(0, bBonus); break;
	case BRAVESOLO:		iRangeFactorMultiplier +=	max(2, bBonus); break;
	case BRAVEAID:		iRangeFactorMultiplier +=	max(2, bBonus); break;
	case CUNNINGSOLO:	iRangeFactorMultiplier +=	max(0, bBonus); break;
	case CUNNINGAID:	iRangeFactorMultiplier +=	max(0, bBonus); break;
	case ATTACKSLAYONLY:
	case AGGRESSIVE:	iRangeFactorMultiplier +=	max(1, bBonus); break;
	}

	if ( (pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->bTeam != ENEMY_TEAM) &&
		gTacticalStatus.bConsNumTurnsWeHaventSeenButEnemyDoes > 0 )
	{
		iRangeFactorMultiplier += gTacticalStatus.bConsNumTurnsWeHaventSeenButEnemyDoes;
	}
	
	return iRangeFactorMultiplier;
}

BOOLEAN ArmySeesOpponents( void )
{
	INT32				cnt;
	SOLDIERTYPE *		pSoldier;

	for ( cnt = gTacticalStatus.Team[ ENEMY_TEAM ].bFirstID; cnt <= gTacticalStatus.Team[ ENEMY_TEAM ].bLastID; cnt++ )
	{
		pSoldier = MercPtrs[ cnt ];

		// sevenfm: check only valid opponents
		if( pSoldier->bActive && 
			pSoldier->bInSector && 
			pSoldier->stats.bLife >= OKLIFE && 
			//pSoldier->aiData.bOppCnt > 0 &&
			GuySawEnemy(pSoldier, SEEN_CURRENTLY) )
		{
			return( TRUE );
		}
	}

	return( FALSE );
}

#ifdef DEBUGDECISIONS
void AIPopMessage ( STR16 str )
{
	DebugAI(str);
}

void AIPopMessage ( const STR8	str )
{
	STR tempstr;
	sprintf( tempstr,"%s", str);
	DebugAI(tempstr);
}

void AINumMessage(const STR8	str, INT32 num)
{
	STR tempstr;
	sprintf( tempstr,"%s %d", str, num);
	DebugAI(tempstr);
}

void AINameMessage(SOLDIERTYPE * pSoldier,const STR8	str,INT32 num)
{
	STR tempstr;
	sprintf( tempstr,"%d %s %d",pSoldier->GetName() , str, num);
	DebugAI( tempstr );
}
#endif

// Flugente: get the id of the closest soldier with a specific flag that we can currently see
UINT8 GetClosestFlaggedSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam, UINT32 aFlag, BOOLEAN fCheckSight )
{
	UINT8			id = NOBODY;
	UINT32			uiLoop;
	SOLDIERTYPE *	pFriend;
	INT16			sClosestDistance = aRange;
	INT16			sDistance;

	// go through each soldier, looking for "friends" (soldiers on same team)
	for (uiLoop = gTacticalStatus.Team[ auTeam ].bFirstID; uiLoop <= gTacticalStatus.Team[ auTeam ].bLastID; ++uiLoop)
	{
		pFriend = MercPtrs[ uiLoop ];

		// if this merc is inactive, not in sector, or dead
		if (!pFriend)
			continue;

		// skip ourselves
		if (pFriend->ubID == pSoldier->ubID)
			continue;

		// must be on the same level
		if ( pFriend->pathing.bLevel != pSoldier->pathing.bLevel )
			continue;

		// this is not for tanks
		if ( TANK( pFriend ) )
			continue;

		// skip if this guy is dead
		if ( pFriend->stats.bLife < OKLIFE )
			continue;

		// check for flag
		if ( !(pFriend->usSoldierFlagMask & aFlag) )
			continue;

		// sevenfm: ignore unknown soldiers
		if( pSoldier->bTeam != pFriend->bTeam &&
			pSoldier->bSide != pFriend->bSide &&
			PersonalKnowledge(pSoldier, pFriend->ubID) != SEEN_CURRENTLY &&
			PersonalKnowledge(pSoldier, pFriend->ubID) != SEEN_THIS_TURN &&
			PersonalKnowledge(pSoldier, pFriend->ubID) != HEARD_THIS_TURN )
		{
			continue;
		}

		sDistance = PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo);

		if (id != NOBODY && sDistance > sClosestDistance)
			continue;

		// are we close enough?
		if (PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) < aRange)
		{
			// can we see this guy?
			if ( !fCheckSight || LOS_Soldier(pSoldier, pFriend, LOS_NORMAL, CALC_FROM_WANTED_DIR) )
			{
				id = pFriend->ubID;
				sDistance = sClosestDistance;
			}
		}
	}

	return id;
}

UINT8 GetClosestSeenDruggedFriendID(SOLDIERTYPE * pSoldier)
{
	UINT8			id = NOBODY;
	UINT32			uiLoop;
	SOLDIERTYPE *	pFriend;
	INT16			sClosestDistance = 1000;
	INT16			sDistance;

	// go through each soldier, looking for "friends" (soldiers on same team)
	for (uiLoop = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; uiLoop <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; ++uiLoop)
	{
		pFriend = MercPtrs[uiLoop];

		// if this merc is inactive, not in sector, or dead
		if (!pFriend)
			continue;

		// skip ourselves
		if (pFriend->ubID == pSoldier->ubID)
			continue;

		// this is not for tanks
		if (TANK(pFriend))
			continue;

		// skip if this guy is dead
		if (pFriend->stats.bLife < OKLIFE)
			continue;

		// check for flag
		if (!MercUnderTheInfluence(pFriend, DRUG_TYPE_BLIND) && 
			!MercUnderTheInfluence(pFriend, DRUG_TYPE_KNOCKOUT) && 
			!MercUnderTheInfluence(pFriend, DRUG_TYPE_LIFEDAMAGE) && 
			!MercUnderTheInfluence(pFriend, DRUG_TYPE_STUNANDKILL))
			continue;

		sDistance = PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo);

		if (id != NOBODY && sDistance > sClosestDistance)
			continue;

		// can we see this guy?
		if (LOS_Soldier(pSoldier, pFriend, LOS_NORMAL, CALC_FROM_WANTED_DIR))
		{
			id = pFriend->ubID;
			sClosestDistance = sDistance;
		}
	}

	return id;
}

// get the id of the closest soldier (closer than x tiles) of a specific team that is wounded that we can currently see
UINT8 GetClosestWoundedSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam )
{
	UINT8			id = NOBODY;
	UINT32			uiLoop;
	SOLDIERTYPE *	pFriend;
	INT16			range = aRange;

	//INT32			sPathCost;
	//INT32			sClimbGridNo;
	BOOLEAN			fClimbingNecessary = FALSE;
	//INT32			sClosestOpponent;

	if( !pSoldier )
	{
		return NOBODY;
	}
	if( auTeam >= MAXTEAMS )
	{
		return NOBODY;
	}

	// go through each soldier, looking for "friends" (soldiers on same team)
	for (uiLoop = gTacticalStatus.Team[ auTeam ].bFirstID; uiLoop <= gTacticalStatus.Team[ auTeam ].bLastID; ++uiLoop)
	{
		pFriend = MercPtrs[ uiLoop ];

		// if this merc is inactive, not in sector, or dead
		if (!pFriend)
			continue;

		// skip ourselves (allow healing)
		//if (pFriend->ubID == pSoldier->ubID)
			//continue;

		// must be on the same level
		if ( pFriend->pathing.bLevel != pSoldier->pathing.bLevel )
			continue;

		// only for merc bodytypes
		if (!pFriend->IsMercBodyType())
			continue;

		// skip if this guy is dead, or not wounded (enough)
		// sevenfm: doctor can heal wounds and bleeding on other soldiers, without doctor skill and bag can only bandage own wounds if has first aid kit
		if (pFriend->stats.bLife <= 0 || 
			!(pFriend->iHealableInjury >= MIN_AI_HEALABLE_INJURY && pSoldier->CanMedicAI() ||
			pFriend->bBleeding > MIN_BLEEDING_THRESHOLD && pFriend->iHealableInjury >= MIN_HEALABLE_INJURY && 
			(pSoldier->CanMedicAI() || pFriend->ubID == pSoldier->ubID && pSoldier->CanFirstAidAI())))
			continue;

		// check if he is captured
		if( pFriend->IsPOW() )
			continue;

		// skip friends in water or gas
		if (Water(pFriend->sGridNo, pFriend->pathing.bLevel) || InGas(pFriend, pFriend->sGridNo))
			continue;

		// sevenfm: don't approach soldiers who are under fire or too close to enemy
		if (pFriend->aiData.bUnderFire)
			continue;

		// without doctor skill, only allow self healing
		//if (pFriend->ubID != pSoldier->ubID && !AIRoleMedic(pSoldier))
			//continue;

		// sevenfm: skip if not safe for healing and not already close to medic
		if (PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) > 1 && !SafeMedicSpot(pFriend, pFriend->sGridNo))
			continue;

		/*sClosestOpponent = ClosestKnownOpponent(pFriend, NULL, NULL);
		if (!TileIsOutOfBounds(sClosestOpponent) && 
			PythSpacesAway(pFriend->sGridNo, sClosestOpponent) < TACTICAL_RANGE / 2 &&
			PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) > 1)
		{
			continue;
		}*/

		// are we close enough?
		INT16 friendrange = PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo);
		if (friendrange < range)
		{
			// get the AP cost to go to this friend's gridno
			//sPathCost = EstimatePathCostToLocation( pSoldier, pFriend->sGridNo, pFriend->pathing.bLevel, TRUE, &fClimbingNecessary, &sClimbGridNo );

			//if( sPathCost != 0 && pSoldier->sGridNo != pFriend->sGridNo)
			{
				range = friendrange;
				id = pFriend->ubID;
			}
		}
	}

	return id;
}

// get the id of the closest medic (closer than x tiles) of a specific team
UINT8 GetClosestMedicSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam )
{
	UINT8			id = NOBODY;
	UINT32			uiLoop;
	SOLDIERTYPE *	pFriend;
	INT16			range = aRange;

	//INT32			sPathCost;
	//INT32			sClimbGridNo;
	BOOLEAN			fClimbingNecessary = FALSE;
	//INT32			sClosestOpponent;

	if( !pSoldier )
	{
		return NOBODY;
	}
	if( auTeam >= MAXTEAMS )
	{
		return NOBODY;
	}

	// go through each soldier, looking for "friends" (soldiers on same team)
	for (uiLoop = gTacticalStatus.Team[ auTeam ].bFirstID; uiLoop <= gTacticalStatus.Team[ auTeam ].bLastID; ++uiLoop)
	{
		pFriend = MercPtrs[ uiLoop ];

		// if this merc is inactive, not in sector, or dead
		if (!pFriend)
			continue;

		// skip ourselves (we seek OTHER people)
		if ( pFriend->ubID == pSoldier->ubID)
			continue;

		// must be on the same level
		if ( pFriend->pathing.bLevel != pSoldier->pathing.bLevel )
			continue;

		// this is not for tanks
		if ( TANK(pFriend) )
			continue;

		// skip this guy if he is dead or unconscious
		if ( pFriend->stats.bLife < OKLIFE )
			continue;

		// check if he is captured
		if( pFriend->IsPOW() )
		{
			continue;
		}

		// check if he is collapsed
		if( pFriend->bCollapsed || pFriend->bBreathCollapsed )
		{
			continue;
		}

		// skip friends in water or gas
		if (Water(pFriend->sGridNo, pFriend->pathing.bLevel) || InGas(pFriend, pFriend->sGridNo))
		{
			continue;
		}

		// skip if this guy if he is no medic
		if (!AIRoleMedic(pFriend))
			continue;

		// sevenfm: don't approach soldiers who are under fire or too close to enemy
		if (pFriend->aiData.bUnderFire)
		{
			continue;
		}

		/*sClosestOpponent = ClosestKnownOpponent(pFriend, NULL, NULL);
		if (!TileIsOutOfBounds(sClosestOpponent) && PythSpacesAway(pFriend->sGridNo, sClosestOpponent) < DAY_VISION_RANGE / 2)
		{
			continue;
		}*/

		// sevenfm: check that medic position is safe for healing
		if (!SafeMedicSpot(pFriend, pFriend->sGridNo))
		{
			continue;
		}		

		// are we close enough?
		INT16 friendrange = PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo);
		if (friendrange < range)
		{
			// get the AP cost to go to this friend's gridno
			//sPathCost = EstimatePathCostToLocation( pSoldier, pFriend->sGridNo, pFriend->pathing.bLevel, TRUE, &fClimbingNecessary, &sClimbGridNo );

			//if( sPathCost != 0 )
			{
				range = friendrange;
				id = pFriend->ubID;
			}
		}
	}

	return id;
}

UINT8 GetClosestOfficerSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam )
{
	UINT8			id = NOBODY;
	UINT32			uiLoop;
	SOLDIERTYPE *	pFriend;
	INT16			range = aRange;

	INT32			sPathCost;
	INT32			sClimbGridNo;
	BOOLEAN			fClimbingNecessary = FALSE;
	//INT32			sClosestOpponent;

	// go through each soldier, looking for "friends" (soldiers on same team)
	for (uiLoop = gTacticalStatus.Team[ auTeam ].bFirstID; uiLoop <= gTacticalStatus.Team[ auTeam ].bLastID; ++uiLoop)
	{
		pFriend = MercPtrs[ uiLoop ];

		// if this merc is inactive, not in sector, or dead
		if (!pFriend)
			continue;

		// skip ourselves (we seek OTHER people)
		if ( pFriend->ubID == pSoldier->ubID)
			continue;

		// must be on the same level
		if ( pFriend->pathing.bLevel != pSoldier->pathing.bLevel )
			continue;

		// this is not for tanks
		if ( TANK(pFriend) )
			continue;

		// skip this guy if he is dead or unconscious
		if ( pFriend->stats.bLife < OKLIFE )
			continue;

		// check if he is captured
		if( pFriend->IsPOW() )
		{
			continue;
		}

		// check if he is collapsed
		if( pFriend->bCollapsed || pFriend->bBreathCollapsed )
		{
			continue;
		}

		// skip if this guy if he is no medic
		if( !AIRoleOfficer(pFriend) )
			continue;

		// are we close enough?
		INT16 friendrange = PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo);
		if ( friendrange < range)
		{
			// can we see this guy?
			//if ( LOS( pSoldier, pFriend->sGridNo, pSoldier->pathing.bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_WANTED_DIR ) )
			//{

			// get the AP cost to go to this friend's gridno
			sPathCost = EstimatePathCostToLocation( pSoldier, pFriend->sGridNo, pFriend->pathing.bLevel, TRUE, &fClimbingNecessary, &sClimbGridNo );

			if( sPathCost != 0 )
			{
				range = friendrange;
				id = pFriend->ubID;
			}			
		}
	}

	return id;
}

INT16 MaxNormalVisionDistance( void )
{
	if( NightTime() )
	{
		return gGameExternalOptions.ubStraightSightRange * STRAIGHT_RATIO;
	}
	return gGameExternalOptions.ubStraightSightRange * 2 * STRAIGHT_RATIO;
}

// sevenfm: count nearby friend soldiers
UINT8 CountNearbyFriends( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// safety check
	if( !pSoldier )
		return 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];
		// Make sure that character is alive, not too shocked, and conscious, and of higher experience level
		// than the character being suppressed.
		if( pFriend &&
			pFriend != pSoldier && 
			pFriend->bActive && 
			pFriend->stats.bLife >= OKLIFE &&
			PythSpacesAway( sGridNo, pFriend->sGridNo ) <= sDistance )
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

// sevenfm: find leader with highest skills and experience level
UINT8 SectorLeader(INT8 bTeam)
{
	if (gGameOptions.fNewTraitSystem)
		return NOBODY;

	SOLDIERTYPE * pFriend;
	UINT8 ubSectorLeader = NOBODY;
	UINT8 ubLeaderValue = 0;
	UINT8 ubValue = 0;

	// safety check
	if (bTeam >= MAXTEAMS)
		return NOBODY;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			//!pFriend->IsCowering() &&
			//!pFriend->bCollapsed &&
			HAS_SKILL_TRAIT(pFriend, SQUADLEADER_NT))
		{
			ubValue = NUM_SKILL_TRAITS(pFriend, SQUADLEADER_NT) * 10 + pFriend->stats.bExpLevel;
			if (ubSectorLeader == NOBODY || ubValue > ubLeaderValue)
			{
				ubLeaderValue = ubValue;
				ubSectorLeader = pFriend->ubID;
			}
		}
	}

	return ubSectorLeader;
}

BOOLEAN WeAttack(INT8 bTeam)
{
	if (bTeam >= MAXTEAMS)
	{
		return FALSE;
	}

	if(bTeam != ENEMY_TEAM)
	{
		return FALSE;
	}

	// check that every soldier has SEEKENEMY order
	SOLDIERTYPE * pFriend;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->aiData.bOrders != SEEKENEMY)
		{
			return FALSE;
		}
	}

	return TRUE;
}

// first 5 turns of attacking sector
BOOLEAN SectorAttack(INT8 bTeam)
{
	if (bTeam == ENEMY_TEAM && WeAttack(bTeam) && gTacticalStatus.Team[bTeam].bAwareOfOpposition <= 10)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN GarrisonDefense(SOLDIERTYPE *pSoldier)
{
	if (pSoldier->bTeam == ENEMY_TEAM && AICheckTown() && !WeAttack(pSoldier->bTeam))
	{
		return TRUE;
	}

	return FALSE;
}

// count neutral civilians
UINT8 CountNearbyNeutrals( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// safety check
	if( !pSoldier )
		return 0;

	for ( UINT32 uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pFriend = MercSlots[ uiLoop ];

		if( pFriend &&
			pFriend != pSoldier && 
			pFriend->bActive && 
			pFriend->stats.bLife >= OKLIFE &&
			CONSIDERED_NEUTRAL(pSoldier, pFriend) &&
			PythSpacesAway( sGridNo, pFriend->sGridNo ) <= sDistance )
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

UINT8 CountNearbyFriendsOnRoof( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// safety check
	if( !pSoldier )
		return 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];
		// Make sure that character is alive, not too shocked, and conscious, and of higher experience level
		// than the character being suppressed.
		if( pFriend &&
			pFriend != pSoldier && 
			pFriend->bActive && 
			pFriend->stats.bLife >= OKLIFE &&
			PythSpacesAway( sGridNo, pFriend->sGridNo ) <= sDistance &&
			pFriend->pathing.bLevel > 0)
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

// sevenfm: new AI morale calculation
INT8 CalcMorale(SOLDIERTYPE *pSoldier)
{
	UINT32	uiLoop, uiLoop2;
	INT32	iOurTotalThreat = 0, iTheirTotalThreat = 0;
	INT16	sOppThreatValue, sFrndThreatValue, sMorale;
	INT32	iPercent;
	INT8	bMostRecentOpplistValue;
	INT8	bMoraleCategory;
	UINT8	*pSeenOpp; //,*friendOlPtr;
	INT8	*pbPersOL, *pbPublOL;
	SOLDIERTYPE *pOpponent,*pFriend;
	BOOLEAN fWeAttack = WeAttack(pSoldier->bTeam);
	BOOLEAN fGarrisonDefense = GarrisonDefense(pSoldier);

	// zombies always have high morale
	if (pSoldier->IsZombie())
	{
		return MORALE_FEARLESS;
	}	

	// run away when in fire
	if (gpWorldLevelData[pSoldier->sGridNo].usExtFlags[pSoldier->pathing.bLevel] & (MAPELEMENT_EXT_BURNABLEGAS) || pSoldier->flags.fHitByGasFlags & HIT_BY_BURNABLEGAS)
	{
		return MORALE_HOPELESS;
	}

	// sevenfm: retreat, also when blinded
	if (pSoldier->RetreatActive() || pSoldier->bBlindedCounter > 0)
	{
		return MORALE_WORRIED;
	}

	// if army guy has NO weapons left then panic!
	if ( pSoldier->bTeam == ENEMY_TEAM || !pSoldier->aiData.bNeutral )
	{
		if ( !AICheckHasGun(pSoldier) )
		{
			// sevenfm: instead of leaving sector, try to attack with hands/knife
			return( MORALE_FEARLESS );
			//return( MORALE_HOPELESS );
		}
	}

	// sevenfm: neutrals always have low AI morale even if they have weapons (so they run from enemy)
	if( pSoldier->aiData.bNeutral )
	{
		return( MORALE_WORRIED );
	}

	// hang pointers to my personal opplist, my team's public opplist, and my
	// list of previously seen opponents
	pSeenOpp	= (UINT8 *) &(gbSeenOpponents[pSoldier->ubID][0]);

	// loop through every one of my possible opponents
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if( !pOpponent )
			continue;			// next merc

		if( !ValidOpponent(pSoldier, pOpponent) )
			continue;

		pbPersOL = pSoldier->aiData.bOppList + pOpponent->ubID;
		pbPublOL = gbPublicOpplist[pSoldier->bTeam] + pOpponent->ubID;
		pSeenOpp = (UINT8 *)gbSeenOpponents[pSoldier->ubID] + pOpponent->ubID;

		// if this opponent is unknown to me personally AND unknown to my team, too
		if ((*pbPersOL == NOT_HEARD_OR_SEEN) && (*pbPublOL == NOT_HEARD_OR_SEEN))
		{
			// if I have never seen him before anywhere in this sector, either
			if ((*pSeenOpp) == NEVER_SEEN)
				continue;		// next merc

			// have seen him in the past, so he remains something of a threat
			bMostRecentOpplistValue = 0;		// uses the free slot for 0 opplist
		}
		else		 // decide which opplist is more current
		{
			// if personal knowledge is more up to date or at least equal
			if ((gubKnowledgeValue[*pbPublOL - OLDEST_HEARD_VALUE][*pbPersOL - OLDEST_HEARD_VALUE] > 0) || (*pbPersOL == *pbPublOL))
				bMostRecentOpplistValue = *pbPersOL;		// use personal
			else
				bMostRecentOpplistValue = *pbPublOL;		// use public
		}

		iPercent = ThreatPercent[bMostRecentOpplistValue - OLDEST_HEARD_VALUE];

		sOppThreatValue = (iPercent * CalcManThreatValue(pOpponent,pSoldier->sGridNo,FALSE,pSoldier)) / 100;

		//sprintf(tempstr,"Known opponent %s, opplist status %d, percent %d, threat = %d",
		//			ExtMen[pOpponent->ubID].name,ubMostRecentOpplistValue,ubPercent,sOppThreatValue);
		//PopMessage(tempstr);

		// ADD this to their running total threatValue (decreases my MORALE)
		iTheirTotalThreat += sOppThreatValue;
		//NumMessage("Their TOTAL threat now = ",sTheirTotalThreat);

		// NOW THE FUN PART: SINCE THIS OPPONENT IS KNOWN TO ME IN SOME WAY,
		// ANY FRIENDS OF MINE THAT KNOW ABOUT HIM BOOST MY MORALE.	SO, LET'S GO
		// THROUGH THEIR PERSONAL OPPLISTS AND CHECK WHICH OF MY FRIENDS KNOW
		// SOMETHING ABOUT HIM AND WHAT THEIR THREAT VALUE TO HIM IS.

		for (uiLoop2 = 0; uiLoop2 < guiNumMercSlots; uiLoop2++)
		{
			pFriend = MercSlots[ uiLoop2 ];

			// if this merc is inactive, at base, on assignment, dead, unconscious
			if (!pFriend || (pFriend->stats.bLife < OKLIFE))
				continue;		// next merc

			// if this merc is not on my side, then he's NOT one of my friends

			// WE CAN'T AFFORD TO CONSIDER THE ENEMY OF MY ENEMY MY FRIEND, HERE!
			// ONLY IF WE ARE ACTUALLY OFFICIALLY CO-OPERATING TOGETHER (SAME SIDE)
			if ( pFriend->aiData.bNeutral && !( pSoldier->ubCivilianGroup != NON_CIV_GROUP && pSoldier->ubCivilianGroup == pFriend->ubCivilianGroup ) )
			{
				continue;		// next merc
			}

			if ( pSoldier->bSide != pFriend->bSide )
				continue;		// next merc

			// THIS TEST IS INVALID IF A COMPUTER-TEAM IS PLAYING CO-OPERATIVELY
			// WITH A NON-COMPUTER TEAM SINCE THE OPPLISTS INVOLVED ARE NOT
			// UP-TO-DATE.	THIS SITUATION IS CURRENTLY NOT POSSIBLE IN HTH/DG.

			// ALSO NOTE THAT WE COUNT US AS OUR (BEST) FRIEND FOR THESE CALCULATIONS

			// subtract HEARD_2_TURNS_AGO (which is negative) to make values start at 0 and
			// be positive otherwise
			iPercent = ThreatPercent[pFriend->aiData.bOppList[pOpponent->ubID] - OLDEST_HEARD_VALUE];

			// reduce the percentage value based on how far away they are from the enemy, if they only hear him
			if ( pFriend->aiData.bOppList[ pOpponent->ubID ] <= HEARD_LAST_TURN )
			{
				iPercent -= PythSpacesAway( pSoldier->sGridNo, pFriend->sGridNo ) * 2;
				if ( iPercent <= 0 )
				{
					//ignore!
					continue;
				}
			}

			sFrndThreatValue = (iPercent * CalcManThreatValue(pFriend,pOpponent->sGridNo,FALSE,pSoldier)) / 100;

			//sprintf(tempstr,"Known by friend %s, opplist status %d, percent %d, threat = %d",
			//		 ExtMen[pFriend->ubID].name,pFriend->aiData.bOppList[pOpponent->ubID],ubPercent,sFrndThreatValue);
			//PopMessage(tempstr);

			// ADD this to our running total threatValue (increases my MORALE)
			// We multiply by sOppThreatValue to PRO-RATE this based on opponent's
			// threat value to ME personally.	Divide later by sum of them all.
			iOurTotalThreat += sOppThreatValue * sFrndThreatValue;
		}

		// this could get slow if I have a lot of friends...
		//KeepInterfaceGoing();
	}


	// if they are no threat whatsoever
	if (!iTheirTotalThreat)
		sMorale = 500;		// our morale is just incredible
	else
	{
		// now divide sOutTotalThreat by sTheirTotalThreat to get the REAL value
		iOurTotalThreat /= iTheirTotalThreat;

		// calculate the morale (100 is even, < 100 is us losing, > 100 is good)
		sMorale = (INT16) ((100 * iOurTotalThreat) / iTheirTotalThreat);
	}

	if (sMorale <= 25)				// odds 1:4 or worse
		bMoraleCategory = MORALE_HOPELESS;
	else if (sMorale <= 50)		 // odds between 1:4 and 1:2
		bMoraleCategory = MORALE_WORRIED;
	else if (sMorale <= 150)		// odds between 1:2 and 3:2
		bMoraleCategory = MORALE_NORMAL;
	else if (sMorale <= 300)		// odds between 3:2 and 3:1
		bMoraleCategory = MORALE_CONFIDENT;
	else							// odds better than 3:1
		bMoraleCategory = MORALE_FEARLESS;

	// make idiot administrators more aggressive
	// sevenfm: also make civilians more aggressive
	if (pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral)
	{
		bMoraleCategory += 2;
	}

	// SEEKENEMY soldiers are more aggressive
	if (pSoldier->aiData.bOrders == SEEKENEMY && !fGarrisonDefense)
	{
		bMoraleCategory++;
	}

	// if have good health
	if (pSoldier->stats.bLife > 75 && !fGarrisonDefense)
	{
		bMoraleCategory++;
	}
	// bad health
	else if (pSoldier->stats.bLife < 40)
	{
		bMoraleCategory--;
	}

	// good breath
	if (pSoldier->bBreath > 75 && !fGarrisonDefense)
	{
		bMoraleCategory++;
	}
	// bad breath
	else if (pSoldier->bBreath < 30)
	{
		bMoraleCategory--;
	}

	// if not under fire - attack
	if (!pSoldier->aiData.bUnderFire && !fGarrisonDefense)
	{
		bMoraleCategory++;
	}

	UINT8 ubClosestOpponentID;
	INT32 sClosestThreat = ClosestKnownOpponent(pSoldier, NULL, NULL, &ubClosestOpponentID);
	INT32 sClosestSeenThreat = ClosestSeenLastTurnOpponent(pSoldier, NULL, NULL);

	if (CountFriendsFlankSameSpot(pSoldier) > 0 &&
		SafeSpot(pSoldier, pSoldier->sGridNo) &&
		!TileIsOutOfBounds(sClosestThreat) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestThreat) <= AIGunRange(pSoldier) &&
		!pSoldier->RushAttackAdvance() &&
		AggressiveAI(pSoldier))
	{
		bMoraleCategory--;
	}
	
	// if last attack of this soldier hit enemy - increase morale
	// bonus if last target is unconscious or suppressed
	if (pSoldier->LastAttackHit() || pSoldier->LastTargetCollapsed() || pSoldier->LastTargetSuppressed() || CountFriendsLastAttackHit(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE) > 0)
	{
		bMoraleCategory++;
	}

	// bonus if out of weapon range
	if( AICheckHasGun(pSoldier) &&
		!fGarrisonDefense &&
		!TileIsOutOfBounds(sClosestThreat) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestThreat) > AIGunRange(pSoldier) &&
		(GuySawEnemy(pSoldier, SEEN_LAST_TURN) || pSoldier->aiData.bUnderFire))
	{
		bMoraleCategory++;
	}

	// bonus if soldier is under adrenaline
	if (pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE])
	{
		bMoraleCategory ++;
	}
	/*else if (pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE])
	{
		bMoraleCategory --;
	}*/

	// rush attack
	if (pSoldier->RushAttackAdvance())
	{
		bMoraleCategory += 2;
	}	

	// find effective morale
	INT8 bMorale = pSoldier->aiData.bMorale;
	//if (pSoldier->aiData.bOrders == SEEKENEMY)
		//bMorale += 20;
	UINT8 ubSquadLeaders = min(2, GetSquadleadersCountInVicinity(pSoldier, FALSE, FALSE));
	UINT8 ubSoldierDifficulty = SoldierDifficultyLevel(pSoldier);
	INT8 bEffectiveMorale = bMorale * 100 / (100 + pSoldier->ShockLevelPercent() + pSoldier->aiData.bWoundShock);
	//UINT8 ubEffectiveShock = max(pSoldier->ShockLevelPercent(), pSoldier->aiData.bWoundShock);

	// limit AI morale depending on morale and shock level
	if (!pSoldier->RushAttackAdvance() && 
		!pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] && 
		//(pSoldier->ShockLevelPercent() > 0 || pSoldier->aiData.bWoundShock > 0) &&
		(pSoldier->ShockLevelPercent() / 15 >= ubSoldierDifficulty + ubSquadLeaders || pSoldier->aiData.bWoundShock > ubSoldierDifficulty * 5))
		bMoraleCategory = min(bMoraleCategory, max(MORALE_WORRIED, bEffectiveMorale / 20));

	// limit AI morale when soldier is under heavy fire
	/*if (pSoldier->ShockLevelPercent() > 75)
		bMoraleCategory = min(bMoraleCategory, MORALE_CONFIDENT);
	else if (pSoldier->ShockLevelPercent() > 50)
		bMoraleCategory = min(bMoraleCategory, MORALE_NORMAL);*/

	// limit AI morale if no successful attack
	if (pSoldier->bTeam == ENEMY_TEAM &&
		!TileIsOutOfBounds(sClosestThreat) &&
		(pSoldier->aiData.bOrders != SEEKENEMY || AISpecialRole(pSoldier) || !fWeAttack) &&
		AICheckHasGun(pSoldier) &&
		(!AICheckShortWeaponRange(pSoldier) || AISpecialRole(pSoldier)) &&
		!pSoldier->RushAttackAdvance() &&
		AggressiveAI(pSoldier) &&
		!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
		//!AICheckSuccessfulAttack(pSoldier, TRUE) &&
		(!CountFriendsBlack(pSoldier, sClosestThreat) || AISpecialRole(pSoldier)) &&
		(!AICheckWeOutnumber(pSoldier, sClosestThreat) || AISpecialRole(pSoldier)) &&
		(SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || AnyCoverAtSpot(pSoldier, pSoldier->sGridNo) && SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE)) &&
		(GuySawEnemy(pSoldier) && !TileIsOutOfBounds(sClosestSeenThreat) && PythSpacesAway(pSoldier->sGridNo, sClosestSeenThreat) <= AIGunRange(pSoldier) ||
		CanSeeEnemyAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND) && PythSpacesAway(pSoldier->sGridNo, sClosestThreat) <= AIGunRange(pSoldier)) &&
		(CountFriendsFlankSameSpot(pSoldier) > 0 || InARoom(pSoldier->sGridNo, NULL) || AISpecialRole(pSoldier)))
	{
		bMoraleCategory = min(bMoraleCategory, MORALE_CONFIDENT);
	}

	// limit AI morale when cannot advance
	if (pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->StopSeekValue() > 0 &&
		CountFriendsBlack(pSoldier) == 0 &&
		SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE) &&
		(InARoom(pSoldier->sGridNo, NULL) || AnyCoverAtSpot(pSoldier, pSoldier->sGridNo) || SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE)))
	{
		bMoraleCategory = min(bMoraleCategory, MORALE_CONFIDENT);
	}

	// limit AI morale for AI special role and medics if there are friends nearby
	if (pSoldier->bTeam == ENEMY_TEAM &&
		(AIRoleMedic(pSoldier) || AISpecialRole(pSoldier)) &&
		!pSoldier->RushAttackAdvance() &&
		!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
		CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE) > 0 &&
		!TileIsOutOfBounds(sClosestThreat) && 
		PythSpacesAway(pSoldier->sGridNo, sClosestThreat) <= AIGunRange(pSoldier))
	{
		bMoraleCategory = min(bMoraleCategory, MORALE_CONFIDENT);
	}

	// limit AI morale when retreating
	if (pSoldier->RetreatActive())
	{
		bMoraleCategory = min(bMoraleCategory, MORALE_NORMAL);
	}

	// limit AI morale when bleeding and can bandage
	if (bMoraleCategory < MORALE_FEARLESS &&
		!pSoldier->RushAttackAdvance() &&
		!MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE) &&
		!AICheckSuccessfulAttack(pSoldier, FALSE) &&
		(!AICheckSuccessfulAttack(pSoldier, TRUE) || pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2) &&
		pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD &&
		pSoldier->iHealableInjury >= MIN_HEALABLE_INJURY &&
		!TileIsOutOfBounds(sClosestThreat) &&
		ubClosestOpponentID != NOBODY &&
		MercPtrs[ubClosestOpponentID] &&
		!MercPtrs[ubClosestOpponentID]->IsUnconscious() &&
		!MercPtrs[ubClosestOpponentID]->IsCowering() &&
		MercPtrs[ubClosestOpponentID]->stats.bLife >= OKLIFE &&
		(pSoldier->CanFirstAidAI() || GetClosestMedicSoldierID(pSoldier, TACTICAL_RANGE, pSoldier->bTeam) != NOBODY))
	{
		bMoraleCategory = min(bMoraleCategory, MORALE_WORRIED);
	}

	// prevent hopeless morale when not under attack
	if (bMoraleCategory == MORALE_HOPELESS && !pSoldier->aiData.bUnderFire && !pSoldier->RetreatActive())
	{
		bMoraleCategory = MORALE_WORRIED;
	}

	// check limits
	bMoraleCategory = max(bMoraleCategory, MORALE_HOPELESS);
	bMoraleCategory = min(bMoraleCategory, MORALE_FEARLESS);

	return(bMoraleCategory);
}

BOOLEAN AISpecialRole(SOLDIERTYPE *pSoldier)
{
	if (AIRoleSniper(pSoldier) || 
		AIRoleMachinegunner(pSoldier) || 
		AIRoleMortarOperator(pSoldier) || 
		AIRoleRadioOperator(pSoldier) || 
		AIRoleCommander(pSoldier) || 
		AIRoleGLOperator(pSoldier) ||
		AIRoleDoctor(pSoldier))
		return TRUE;

	return FALSE;
}

BOOLEAN AggressiveAI(SOLDIERTYPE *pSoldier)
{
	if (pSoldier->aiData.bOrders == STATIONARY ||
		AISpecialRole(pSoldier) ||
		AIGunRange(pSoldier) > TACTICAL_RANGE ||
		AICheckUnderground() ||
		InARoom(pSoldier->sGridNo) && pSoldier->aiData.bOrders != SEEKENEMY ||
		pSoldier->RushAttackActive())
		return FALSE;

	return FALSE;
}

extern INT32 giSmokeTestSpot;
extern INT8 gbSmokeTestLevel;

BOOLEAN gfTacticalLOS[256][256];
UINT8 gubTacticalCTGT[256][256];
UINT8 gubTacticalCTH[256][256];
INT32 giTacticalThreat[256];
INT32 giTacticalBestThreat[256][256];

INT32 CalcSituationValue(SOLDIERTYPE *pSoldier, INT32 sSmokeSpot, INT8 bSmokeLevel, INT16 sDistance)
{
	CHECKF(pSoldier);

	INT32 iBestValue;
	INT32 iMyTotalValue = 0;
	INT32 iEnemyTotalValue = 0;
	INT32 iThreat;
	SOLDIERTYPE * pFriend;
	SOLDIERTYPE *pOpponent;
	INT32 sEnemySpot;
	INT8 bEnemyLevel;
	INT32 iTotal;
	UINT8 ubCnt;
	UINT8 ubCTH;
	UINT8 ubCTGT;
	BOOLEAN fLOS;
	UINT8 ubBestTarget;

	if (TileIsOutOfBounds(sSmokeSpot))
	{
		memset(gfTacticalLOS, 0, sizeof(gfTacticalLOS));
		memset(gubTacticalCTGT, 0, sizeof(gubTacticalCTGT));
		memset(giTacticalThreat, 0, sizeof(giTacticalThreat));
		memset(gubTacticalCTH, 0, sizeof(gubTacticalCTH));
		memset(giTacticalBestThreat, 0, sizeof(giTacticalBestThreat));
	}

	// check all friends within range (including this soldier)
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) <= sDistance)
		{
			// find best attack value for this friend
			iBestValue = 0;
			iTotal = 0;
			ubCnt = 0;
			ubBestTarget = NOBODY;

			// check all known public opponents
			for (UINT8 uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
			{
				pOpponent = MercSlots[uiLoop];

				// if this merc is inactive, at base, on assignment, or dead
				if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
				{
					continue;
				}

				if (!ValidOpponent(pFriend, pOpponent))
				{
					continue;
				}

				// check that this opponent is known publicly
				if (PublicKnowledge(pFriend->bTeam, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
				{
					continue;
				}

				sEnemySpot = KnownPublicLocation(pFriend->bTeam, pOpponent->ubID);
				bEnemyLevel = KnownPublicLevel(pFriend->bTeam, pOpponent->ubID);

				// unknown location or too far
				if (TileIsOutOfBounds(sEnemySpot) || PythSpacesAway(pFriend->sGridNo, sEnemySpot) > TACTICAL_RANGE * 2)
				{
					continue;
				}

				// my threat against enemy
				if (TileIsOutOfBounds(sSmokeSpot))
				{
					iThreat = CalcManThreatValue(pFriend, NOWHERE, FALSE, pOpponent);
					if (iThreat == -999) iThreat = 0;
					giTacticalThreat[pFriend->ubID] = iThreat;
				}
				else
				{
					iThreat = giTacticalThreat[pFriend->ubID];
				}

				// reduce from cover
				//iMyThreat -= iMyThreat * SoldierToLocationChanceToGetThrough(pFriend, sEnemySpot, bEnemyLevel, 0, pOpponent->ubID) / 200;
				//iThreat = iThreat * CalcAverageCTGTForPosition(pFriend, pOpponent->ubID, sEnemySpot, bEnemyLevel, APBPConstants[AP_MAXIMUM]) / 100;								
				if (TileIsOutOfBounds(sSmokeSpot))
				{
					ubCTGT = SoldierToLocationChanceToGetThrough(pFriend, sEnemySpot, bEnemyLevel, 0, pOpponent->ubID);
					gubTacticalCTGT[pFriend->ubID][pOpponent->ubID] = ubCTGT;
				}
				else
				{
					ubCTGT = gubTacticalCTGT[pFriend->ubID][pOpponent->ubID];
				}
				
				iThreat = iThreat * (ubCTGT + 100) / 200;

				// reduce from sight
				fLOS = FALSE;
				// prepare or has original sight
				if (TileIsOutOfBounds(sSmokeSpot) || gfTacticalLOS[pFriend->ubID][pOpponent->ubID])
				{
					if (TileIsOutOfBounds(sSmokeSpot))
					{
						// initial test
						if (PersonalKnowledge(pFriend, pOpponent->ubID) == SEEN_CURRENTLY)
						{
							fLOS = TRUE;
						}
						else
						{
							giSmokeTestSpot = NOWHERE;
							gbSmokeTestLevel = 0;
							fLOS = LOS_Location(pFriend->sGridNo, pFriend->pathing.bLevel, sEnemySpot, bEnemyLevel, MAX_VISION_RANGE) > 0;
						}
					}
					/*else if (pFriend->pathing.bLevel == bEnemyLevel && pFriend->pathing.bLevel == bSmokeLevel ||
							pFriend->pathing.bLevel == bSmokeLevel && PythSpacesAway(pFriend->sGridNo, sSmokeSpot) <= PythSpacesAway(sSmokeSpot, sEnemySpot) ||
							bEnemyLevel == bSmokeLevel && PythSpacesAway(pFriend->sGridNo, sSmokeSpot) > PythSpacesAway(sSmokeSpot, sEnemySpot))
					{
						// use distance check
						if (CheckSpotBetween(pFriend->sGridNo, sEnemySpot, sSmokeSpot))
							fLOS = FALSE;
						else
							fLOS = gfTacticalLOS[pFriend->ubID][pOpponent->ubID];
					}*/
					else
					{
						// positive test, but friend and enemy are on different levels, do full test
						giSmokeTestSpot = sSmokeSpot;
						gbSmokeTestLevel = bSmokeLevel;
						fLOS = LOS_Location(pFriend->sGridNo, pFriend->pathing.bLevel, sEnemySpot, bEnemyLevel, MAX_VISION_RANGE) > 0;
						giSmokeTestSpot = NOWHERE;
						gbSmokeTestLevel = 0;
					}
				}

				if (fLOS)
				{
					gfTacticalLOS[pFriend->ubID][pOpponent->ubID] = TRUE;					
				}
				else
				{
					iThreat = iThreat / 2;
				}
				
				// estimate CTH
				if (TileIsOutOfBounds(sSmokeSpot))
				{
					ubCTH = AIEstimateCTH(pFriend, pOpponent, pFriend->sGridNo, sEnemySpot, FALSE);
					gubTacticalCTH[pFriend->ubID][pOpponent->ubID] = ubCTH;
				}
				else
				{
					ubCTH = gubTacticalCTH[pFriend->ubID][pOpponent->ubID];
				}
				
				iThreat = iThreat * ubCTH / 100;				

				if (iThreat > iBestValue)
				{
					iBestValue = iThreat;
					ubBestTarget = pOpponent->ubID;					
				}
				ubCnt++;
				//iTotal += iThreat * iThreat;
			}
			// add best value
			iMyTotalValue += iBestValue;

			// remember best threat value for each soldier
			if (TileIsOutOfBounds(sSmokeSpot) && ubBestTarget != NOBODY)
			{
				giTacticalBestThreat[pFriend->ubID][ubBestTarget] = iBestValue;
			}
			/*if (ubCnt > 0)
			{
				iMyTotalValue += (INT32)sqrt(iTotal / ubCnt);
			}*/
		}
	}

	// check all known public opponents
	for (UINT8 uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check that this opponent is known publicly
		if (PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sEnemySpot = KnownPublicLocation(pSoldier->bTeam, pOpponent->ubID);
		bEnemyLevel = KnownPublicLevel(pSoldier->bTeam, pOpponent->ubID);

		if (TileIsOutOfBounds(sEnemySpot))
		{
			continue;
		}

		// find best attack value for this opponent
		iBestValue = 0;
		iTotal = 0;
		ubCnt = 0;

		// check all friends within range (including this soldier)
		for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
		{
			pFriend = MercPtrs[iCounter];

			if (pFriend &&
				pFriend->bActive &&
				pFriend->bInSector &&
				pFriend->stats.bLife >= OKLIFE &&
				PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) <= sDistance)
			{
				// check if this friend is too far from known enemy
				if (PythSpacesAway(pFriend->sGridNo, sEnemySpot) > TACTICAL_RANGE * 2)
				{
					continue;
				}

				// enemy threat against friend
				if (TileIsOutOfBounds(sSmokeSpot))
				{
					iThreat = CalcManThreatValue(pOpponent, NOWHERE, FALSE, pFriend);
					if (iThreat == -999) iThreat = 0;
					giTacticalThreat[pOpponent->ubID] = iThreat;
				}
				else
				{
					iThreat = giTacticalThreat[pOpponent->ubID];
				}

				// reduce from cover
				//iEnemyThreat -= iEnemyThreat * SoldierToLocationChanceToGetThrough(pOpponent, pFriend->sGridNo, pFriend->pathing.bLevel, 0, pFriend->ubID) / 100;
				//iThreat = iThreat * CalcAverageCTGTForPosition(pOpponent, pFriend->ubID, pFriend->sGridNo, pFriend->pathing.bLevel, APBPConstants[AP_MAXIMUM]) / 100;				
				if (TileIsOutOfBounds(sSmokeSpot))
				{
					ubCTGT = SoldierToLocationChanceToGetThrough(pOpponent, pFriend->sGridNo, pFriend->pathing.bLevel, 0, pFriend->ubID);
					gubTacticalCTGT[pOpponent->ubID][pFriend->ubID] = ubCTGT;
				}
				else
				{
					ubCTGT = gubTacticalCTGT[pOpponent->ubID][pFriend->ubID];
				}

				iThreat = iThreat * (ubCTGT + 100) / 200;

				// reduce from sight
				fLOS = FALSE;
				// prepare or has original sight
				if (TileIsOutOfBounds(sSmokeSpot) || gfTacticalLOS[pOpponent->ubID][pFriend->ubID])
				{
					if (TileIsOutOfBounds(sSmokeSpot))
					{
						// prepare
						if (PersonalKnowledge(pOpponent, pFriend->ubID) == SEEN_CURRENTLY)
						{
							fLOS = TRUE;
						}
						else
						{
							giSmokeTestSpot = NOWHERE;
							gbSmokeTestLevel = 0;
							fLOS = LOS_Location(sEnemySpot, bEnemyLevel, pFriend->sGridNo, pFriend->pathing.bLevel, MAX_VISION_RANGE) > 0;
						}
					}
					/*else if (pFriend->pathing.bLevel == bEnemyLevel && pFriend->pathing.bLevel == bSmokeLevel ||
							pFriend->pathing.bLevel == bSmokeLevel && PythSpacesAway(pFriend->sGridNo, sSmokeSpot) <= PythSpacesAway(sSmokeSpot, sEnemySpot) ||
							bEnemyLevel == bSmokeLevel && PythSpacesAway(pFriend->sGridNo, sSmokeSpot) > PythSpacesAway(sSmokeSpot, sEnemySpot))
					{
						// use distance check
						if (CheckSpotBetween(pFriend->sGridNo, sEnemySpot, sSmokeSpot))
							fLOS = FALSE;
						else
							fLOS = gfTacticalLOS[pOpponent->ubID][pFriend->ubID];
					}*/
					else
					{
						// do full test
						giSmokeTestSpot = sSmokeSpot;
						gbSmokeTestLevel = bSmokeLevel;
						fLOS = LOS_Location(sEnemySpot, bEnemyLevel, pFriend->sGridNo, pFriend->pathing.bLevel, MAX_VISION_RANGE) > 0;
						giSmokeTestSpot = NOWHERE;
						gbSmokeTestLevel = 0;
					}
				}				

				if(fLOS)
				{
					gfTacticalLOS[pOpponent->ubID][pFriend->ubID] = TRUE;
				}
				else
				{
					iThreat = iThreat / 2;
				}

				// estimate CTH				
				if (TileIsOutOfBounds(sSmokeSpot))
				{
					ubCTH = AIEstimateCTH(pOpponent, pFriend, sEnemySpot, pFriend->sGridNo, FALSE);
					gubTacticalCTH[pOpponent->ubID][pFriend->ubID] = ubCTH;
				}
				else
				{
					ubCTH = gubTacticalCTH[pOpponent->ubID][pFriend->ubID];
				}

				iThreat = iThreat * ubCTH / 100;

				if (iThreat > iBestValue)
				{
					iBestValue = iThreat;
					ubBestTarget = pFriend->ubID;
				}
				ubCnt++;
				//iTotal += iThreat * iThreat;
			}			
		}
		// add best value for this enemy
		iEnemyTotalValue += iBestValue;

		// remember best threat value for each soldier
		if (TileIsOutOfBounds(sSmokeSpot) && ubBestTarget != NOBODY)
		{
			giTacticalBestThreat[pFriend->ubID][ubBestTarget] = iBestValue;
		}
		/*if (ubCnt > 0)
		{
			iEnemyTotalValue += (INT32)sqrt(iTotal / ubCnt);
		}*/
	}
	
	//ScreenMsg(FONT_MCOLOR_LTRED, MSG_INTERFACE, L"my %d enemy %d", iMyTotalValue, iEnemyTotalValue);
	return 100 * iMyTotalValue / max(1, iMyTotalValue + iEnemyTotalValue);
}

// check if spot is between some friend and opponent
BOOLEAN CheckBetween(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel, INT16 sDistance, BOOLEAN fBestChance)
{
	CHECKF(pSoldier);

	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	SOLDIERTYPE *pFriend;
	SOLDIERTYPE *pOpponent;
	INT32 sEnemySpot;
	INT8 bEnemyLevel;

	// check all friends within range (including this soldier)
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) <= sDistance)
		{
			// check all known public opponents
			for (UINT8 uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
			{
				pOpponent = MercSlots[uiLoop];

				// if this merc is inactive, at base, on assignment, or dead
				if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
				{
					continue;
				}

				if (!ValidOpponent(pFriend, pOpponent))
				{
					continue;
				}

				// check that this opponent is known publicly
				if (PublicKnowledge(pFriend->bTeam, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
				{
					continue;
				}

				sEnemySpot = KnownPublicLocation(pFriend->bTeam, pOpponent->ubID);
				bEnemyLevel = KnownPublicLevel(pFriend->bTeam, pOpponent->ubID);

				// unknown location or too far
				if (TileIsOutOfBounds(sEnemySpot) || PythSpacesAway(pFriend->sGridNo, sEnemySpot) > TACTICAL_RANGE * 2)
				{
					continue;
				}

				// spot is between friend and opponent
				if (CheckSpotBetween(pFriend->sGridNo, sEnemySpot, sSpot) &&
					(!fBestChance || giTacticalBestThreat[pFriend->ubID][pOpponent->ubID] || giTacticalBestThreat[pOpponent->ubID][pFriend->ubID]))
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

BOOLEAN CheckSpotBetween(INT32 sSpot1, INT32 sSpot2, INT32 sCheckSpot)
{
	CHECKF(!TileIsOutOfBounds(sSpot1));
	CHECKF(!TileIsOutOfBounds(sSpot2));
	CHECKF(!TileIsOutOfBounds(sCheckSpot));

	if ((AIDirection(sSpot1, sCheckSpot) == AIDirection(sSpot1, sSpot2) || AIDirection(sSpot1, sCheckSpot) == gOneCDirection[AIDirection(sSpot1, sSpot2)] || AIDirection(sSpot1, sCheckSpot) == gOneCCDirection[AIDirection(sSpot1, sSpot2)]) &&
		(AIDirection(sSpot2, sCheckSpot) == AIDirection(sSpot2, sSpot1) || AIDirection(sSpot2, sCheckSpot) == gOneCDirection[AIDirection(sSpot2, sSpot1)] || AIDirection(sSpot2, sCheckSpot) == gOneCCDirection[AIDirection(sSpot2, sSpot1)]) &&
		GetRangeInCellCoordsFromGridNoDiff(sSpot1, sCheckSpot) + GetRangeInCellCoordsFromGridNoDiff(sSpot2, sCheckSpot) <= GetRangeInCellCoordsFromGridNoDiff(sSpot1, sSpot2) + CELL_X_SIZE * 2)
		return TRUE;

	return FALSE;
}

void SetTeamHalfAP( INT8 bTeam )
{
	if( bTeam >= MAXTEAMS )
	{
		return;
	}

	SOLDIERTYPE * pFriend;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if (pFriend &&
			pFriend->bActive && 
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE )
		{
			pFriend->bActionPoints = 0;
			pFriend->usSoldierFlagMask |= SOLDIER_HALF_AP;
		}
	}
}

UINT8 CountFriendsFlankSameSpot( SOLDIERTYPE *pSoldier, INT32 sSpot )
{
	CHECKF(pSoldier);

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	UINT8 ubFlankLeft = 0;
	UINT8 ubFlankRight = 0;

	if( TileIsOutOfBounds(sSpot) )
	{
		sSpot = ClosestKnownOpponent( pSoldier, NULL, NULL );
	}

	if(TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if (pFriend &&
			pFriend != pSoldier && 
			pFriend->bActive && 
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->aiData.bAlertStatus == STATUS_RED &&
			pFriend->aiData.bOrders > ONGUARD)
		{
			// check if this friend flanks around the same spot
			if (pFriend->IsFlanking() &&
				PythSpacesAway(pFriend->sTacticalSpot, sSpot) < (INT16)(TACTICAL_RANGE / 2))
			{
				if (pFriend->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT)
				{
					ubFlankLeft++;
				}
				else
				{
					ubFlankRight++;
				}
			}
		}
	}

	return ubFlankLeft + ubFlankRight;
}

UINT8 CountRushAttackFriends(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance)
{
	CHECKF(pSoldier);

	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->RushAttackActive() &&
			PythSpacesAway(sSpot, pFriend->sGridNo) <= sDistance)
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

UINT8 CountRushAttackTeam(INT8 bTeam)
{
	if (bTeam < 0 || bTeam >= MAXTEAMS)
	{
		return 0;
	}

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->RushAttackActive())
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

UINT8 CountFriendsLastAttackHit( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance )
{
	CHECKF(pSoldier);

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->aiData.bOrders > ONGUARD &&
			PythSpacesAway( sGridNo, pFriend->sGridNo ) <= sDistance &&
			(pFriend->LastAttackHit() != NOWHERE || pFriend->usSoldierFlagMask2 & SOLDIER_SUCCESSFUL_ATTACK || pFriend->LastTargetSuppressed()))
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

UINT8 CountFriendsShortRangeWeapon(SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance)
{
	CHECKF(pSoldier);

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->aiData.bOrders > ONGUARD &&
			PythSpacesAway(sGridNo, pFriend->sGridNo) <= sDistance &&
			AICheckShortWeaponRange(pFriend))
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

void StartRushAttackTeam(INT8 bTeam, INT32 sSpot)
{
	SOLDIERTYPE * pFriend;
	//	INT16 iPathCost;
	//	BOOLEAN fClimbingNecessary;
	//	INT32 sClimbGridNo;

	// safety check
	if (TileIsOutOfBounds(sSpot))
		return;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->pathing.bLevel == 0 &&
			CanRushAttack(pFriend))
		{
			pFriend->RushAttackStart();
		}
	}
}

UINT8 CountFriendsSeenEnemy( SOLDIERTYPE *pSoldier, INT16 sDistance, UINT8 ubMax )
{
	CHECKF(pSoldier);

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			PythSpacesAway( pSoldier->sGridNo, pFriend->sGridNo ) <= sDistance &&
			GuySawEnemy(pFriend, ubMax))
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

UINT8 CountSeenEnemiesLastTurn( SOLDIERTYPE *pSoldier, BOOLEAN fOnlyPublicKnown )
{
	CHECKF(pSoldier);

	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;
	UINT8		cnt = 0;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this guy SAW an enemy recently...
		if( PersonalKnowledge(pSoldier, pOpponent->ubID) >= SEEN_CURRENTLY &&
			PersonalKnowledge(pSoldier, pOpponent->ubID) <= SEEN_LAST_TURN &&
			(!fOnlyPublicKnown || PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) != NOT_HEARD_OR_SEEN) )
		{
			cnt++;
		}
	}

	return cnt;
}

UINT8 CountEngagedEnemiesLastTurn(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;
	UINT8		cnt = 0;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check if we saw enemy or attacked him or he saw us or attacked us
		/*if (PersonalKnowledge(pSoldier, pOpponent->ubID) >= SEEN_CURRENTLY && PersonalKnowledge(pSoldier, pOpponent->ubID) <= SEEN_LAST_TURN ||
			PersonalKnowledge(pOpponent, pSoldier->ubID) >= SEEN_CURRENTLY && PersonalKnowledge(pOpponent, pSoldier->ubID) <= SEEN_LAST_TURN ||
			pOpponent->ubID == pSoldier->ubPreviousAttackerID ||
			pSoldier->ubID == pOpponent->ubPreviousAttackerID )
		{
			cnt++;
		}*/

		// check that we saw or attacked enemy last turn
		if (PersonalKnowledge(pSoldier, pOpponent->ubID) >= SEEN_CURRENTLY && PersonalKnowledge(pSoldier, pOpponent->ubID) <= SEEN_LAST_TURN ||
			pOpponent->ubID == pSoldier->ubPreviousAttackerID)
		{
			cnt++;
		}
	}

	return cnt;
}

UINT8 CanAttackEnemy(SOLDIERTYPE *pSoldier, INT8 bStance, BOOLEAN fExactSight)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT8		bKnowledge;
	INT32		sThreatLoc;
	INT8		bThreatLevel;
	BOOLEAN		fStandingLOS;
	BOOLEAN		fMaxLOS;
	BOOLEAN		fUnlimitedLOS;
	BOOLEAN		fSeenRecently;
	UINT8		ubCTGT;
	INT16		sDistance;
	UINT16		usTrueState;

	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;

	if(!AICheckHasGun(pSoldier))
	{
		return FALSE;
	}	

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);

		// check knowledge
		if (bKnowledge == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		sDistance = PythSpacesAway(pSoldier->sGridNo, sThreatLoc);

		/*if (sDistance > MAX_VISION_RANGE)
		{
			continue;
		}*/

		// check sight from standing stance
		usTrueState = pSoldier->usAnimState;
		pSoldier->usAnimState = STANDING;

		fStandingLOS = LOS_Virtual(pSoldier, sThreatLoc, bThreatLevel, bStance, LOS_READY, CALC_FROM_ALL_DIRS);
		// also check adjacent tiles
		if (!fStandingLOS && !fExactSight)
		{
			for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
			{
				sTempGridNo = NewGridNo(sThreatLoc, DirectionInc(ubDirection));

				if (sTempGridNo != sThreatLoc)
				{
					ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bThreatLevel];
					if (ubMovementCost < TRAVELCOST_BLOCKED)
					{
						fStandingLOS = LOS_Virtual(pSoldier, sTempGridNo, bThreatLevel, bStance, LOS_NORMAL, CALC_FROM_ALL_DIRS);
						if (fStandingLOS)
							break;
					}
				}
			}
		}

		fMaxLOS = LOS_Virtual(pSoldier, sThreatLoc, bThreatLevel, bStance, LOS_NORMAL, MAX_VISION_RANGE);
		// also check adjacent tiles
		if (!fMaxLOS && !fExactSight)
		{
			for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
			{
				sTempGridNo = NewGridNo(sThreatLoc, DirectionInc(ubDirection));

				if (sTempGridNo != sThreatLoc)
				{
					ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bThreatLevel];
					if (ubMovementCost < TRAVELCOST_BLOCKED)
					{
						fMaxLOS = LOS_Virtual(pSoldier, sTempGridNo, bThreatLevel, bStance, LOS_NORMAL, MAX_VISION_RANGE);
						if (fMaxLOS)
							break;
					}
				}
			}
		}
		pSoldier->usAnimState = usTrueState;

		fUnlimitedLOS = LOS_Virtual(pSoldier, sThreatLoc, bThreatLevel, bStance, LOS_NORMAL, NO_DISTANCE_LIMIT);
		// also check adjacent tiles
		if (!fUnlimitedLOS && !fExactSight)
		{
			for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
			{
				sTempGridNo = NewGridNo(sThreatLoc, DirectionInc(ubDirection));

				if (sTempGridNo != sThreatLoc)
				{
					ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bThreatLevel];
					if (ubMovementCost < TRAVELCOST_BLOCKED)
					{
						fUnlimitedLOS = LOS_Virtual(pSoldier, sTempGridNo, bThreatLevel, bStance, LOS_NORMAL, NO_DISTANCE_LIMIT);
						if (fUnlimitedLOS)
							break;
					}
				}
			}
		}

		// AI CTGT is checked from standing stance
		ubCTGT = AISoldierToLocationChanceToGetThrough(pSoldier, sThreatLoc, bThreatLevel, 3);

		fSeenRecently = FALSE;
		if (bKnowledge >= SEEN_CURRENTLY && bKnowledge <= SEEN_3_TURNS_AGO)
		{
			fSeenRecently = TRUE;
		}

		// check if we can attack this enemy
		if ((fStandingLOS || fSeenRecently) && AIGunRange(pSoldier) >= sDistance ||
			(fMaxLOS || fSeenRecently || ubCTGT > 25 || fUnlimitedLOS && sDistance <= (INT16)MAX_VISION_RANGE) && AIGunAutofireCapable(pSoldier) && (INT16)(AIGunRange(pSoldier) + DAY_VISION_RANGE / 2) >= sDistance ||
			ubCTGT > 0 && (pOpponent->ubPreviousAttackerID == pSoldier->ubID || pOpponent->ubNextToPreviousAttackerID == pSoldier->ubID) && (INT16)AIGunRange(pSoldier) >= sDistance)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// sevenfm: check friendly soldiers between me and noise gridno
// count only friends that are active and not stationary/onguard/sniper
UINT8 CountFriendsBetweenMeAndSpotFromSpot( SOLDIERTYPE *pSoldier, INT32 sTargetGridNo )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendDir, ubMyDir;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sTargetGridNo) )
	{
		return 0;
	}

	ubMyDir = AIDirection(sTargetGridNo, pSoldier->sGridNo);

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if( !pFriend )
		{
			continue;
		}

		ubFriendDir = AIDirection(sTargetGridNo, pFriend->sGridNo);

		if (pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->stats.bLife >= pFriend->stats.bLifeMax/2 &&
			pFriend->aiData.bOrders > ONGUARD &&
			(ubFriendDir == ubMyDir || ubFriendDir == gOneCDirection[ubMyDir] || ubFriendDir == gOneCCDirection[ubMyDir]) &&
			PythSpacesAway( sTargetGridNo, pFriend->sGridNo) < PythSpacesAway(sTargetGridNo, pSoldier->sGridNo) )
		{
			ubFriends++;
		}
	}

	return ubFriends;
}

UINT8 CountFriendsInDirection( SOLDIERTYPE *pSoldier, UINT8 ubDirection, INT16 sDistance, BOOLEAN fCheckSight )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);

	if( ubDirection == DIRECTION_IRRELEVANT )
	{
		return 0;
	}

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			AIDirection(pSoldier->sGridNo, pFriend->sGridNo) == ubDirection &&
			PythSpacesAway( pSoldier->sGridNo, pFriend->sGridNo) <= sDistance &&
			(!fCheckSight || LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, pFriend->sGridNo, pFriend->pathing.bLevel, MAX_VISION_RANGE)) )
		{
			ubFriends++;
		}
	}

	return ubFriends;
}

UINT8 CountFriendsInDirectionFromSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubDirection, INT16 sDistance)
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);

	if (ubDirection == DIRECTION_IRRELEVANT)
	{
		return 0;
	}

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			AIDirection(sSpot, pFriend->sGridNo) == ubDirection &&
			PythSpacesAway(sSpot, pFriend->sGridNo) <= sDistance)
		{
			ubFriends++;
		}
	}

	return ubFriends;
}

// find friends who don't see but can possibly see opponent if we flashlight him
UINT8 CountFriendsPossiblySeeFlashlightOpponent(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel, UINT8 ubID)
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);

	if (ubID == NOBODY)
	{
		return 0;
	}

	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->bActionPoints >= APBPConstants[AP_MINIMUM] &&
			AICheckHasGun(pFriend) &&			
			PythSpacesAway(pFriend->sGridNo, sSpot) <= DAY_VISION_RANGE &&
			AIGunRange(pFriend) >= PythSpacesAway(pFriend->sGridNo, sSpot) &&
			PersonalKnowledge(pFriend, ubID) != SEEN_CURRENTLY &&
			LOS_Location(pFriend->sGridNo, pFriend->pathing.bLevel, sSpot, bLevel, NO_DISTANCE_LIMIT))
		{
			ubFriends++;
		}
	}

	return ubFriends;
}

UINT8 CountFriendsCoverDirection( SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent, INT32 sHisGridNo, UINT8 ubCheckDir )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendDir;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);
	CHECKF(pOpponent);

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if(!pFriend)
		{
			continue;
		}

		ubFriendDir = AIDirection(sHisGridNo, pFriend->sGridNo);

		// check that friend is in contact with this enemy and has same direction from him as specified gridno
		if (pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->aiData.bOppList[pOpponent->ubID] >= SEEN_CURRENTLY &&
			pFriend->aiData.bOppList[pOpponent->ubID] <= SEEN_LAST_TURN &&			
			ubFriendDir == ubCheckDir )
		{
			ubFriends++;
		}
	}

	return ubFriends;
}

BOOLEAN AIRoleSniper(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
	{
		return FALSE;
	}

	if ((AIGunRange(pSoldier) > DAY_VISION_RANGE || AICheckHasWeaponOfType(pSoldier, GUN_SN_RIFLE)) &&
		AIGunScoped(pSoldier) &&
		pSoldier->stats.bMarksmanship > 90 &&
		gGameOptions.fNewTraitSystem &&
		HAS_SKILL_TRAIT(pSoldier, SNIPER_NT))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleMarksman(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
	{
		return FALSE;
	}

	if( !AICheckHasGun(pSoldier) )
	{
		return FALSE;
	}

	if( AIGunType(pSoldier) != GUN_SN_RIFLE && 
		AIGunType(pSoldier) != GUN_RIFLE &&
		AIGunType(pSoldier) != GUN_AS_RIFLE )
	{
		return FALSE;
	}

	if( AIGunRange(pSoldier) > DAY_VISION_RANGE * 3 / 4 &&
		(AIGunScoped(pSoldier) || 
		pSoldier->stats.bMarksmanship > 90 || 
		gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SNIPER_NT) ||
		AIGunType(pSoldier) == GUN_RIFLE && gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, RANGER_NT)))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleRadioOperator(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
	{
		return FALSE;
	}

	if( pSoldier->CanUseRadio(FALSE) )
		return TRUE;

	return FALSE;
}

BOOLEAN AIRoleMedic(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
	{
		return FALSE;
	}

	if (gGameOptions.fNewTraitSystem && 
		HAS_SKILL_TRAIT(pSoldier, DOCTOR_NT) && 
		pSoldier->CanMedicAI())
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleDoctor(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if (!SoldierAI(pSoldier))
	{
		return FALSE;
	}

	if (gGameOptions.fNewTraitSystem && NUM_SKILL_TRAITS(pSoldier, DOCTOR_NT) > 1 && pSoldier->CanMedicAI())
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleRPGOperator(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	INT8 bPocket = FindRocketLauncher(pSoldier);
	if (bPocket != NO_SLOT && (EnoughAmmo(pSoldier, FALSE, bPocket) || FindLaunchable(pSoldier, bPocket) != NO_SLOT))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleMortarOperator(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	if ( pSoldier->HasMortar(TRUE) )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleGLOperator(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	// find GL
	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_LAUNCHER);

	if(bWeaponIn != NO_SLOT && 
		(EnoughAmmo(pSoldier, FALSE, bWeaponIn) || FindAmmoToReload(pSoldier, bWeaponIn, NO_SLOT) != NO_SLOT))
	{
		return TRUE;
	}

	// check for attached GL
	INT8 bGunSlot = FindAIUsableObjClass(pSoldier, IC_GUN);
	INT8 bRealWeaponMode = pSoldier->bWeaponMode;
	pSoldier->bWeaponMode = WM_ATTACHED_GL;		// So that EnoughAmmo will check for a grenade not a bullet
	if (bGunSlot != NO_SLOT &&
		IsGrenadeLauncherAttached(&pSoldier->inv[bGunSlot]) &&
		(EnoughAmmo(pSoldier, FALSE, bGunSlot) || FindAmmoToReload(pSoldier, bGunSlot, NO_SLOT) != NO_SLOT))
	{
		pSoldier->bWeaponMode = bRealWeaponMode;
		return TRUE;
	}
	pSoldier->bWeaponMode = bRealWeaponMode;

	return FALSE;
}

BOOLEAN AIRoleOfficer(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SQUADLEADER_NT))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleCommander(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	if (gGameOptions.fNewTraitSystem && NUM_SKILL_TRAITS(pSoldier, SQUADLEADER_NT) > 1)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleMachinegunner(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	if (AIGunType(pSoldier) == GUN_LMG && 
		AIGunRange(pSoldier) >= TACTICAL_RANGE)
	{
		return TRUE;
	}
	return FALSE;
}

BOOLEAN AIRoleSuppress(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	if (AISpecialRole(pSoldier))
	{
		return FALSE;
	}

	if ((AIGunType(pSoldier) == GUN_SMG || AIGunType(pSoldier) == GUN_AS_RIFLE) &&
		AIGunMagSize(pSoldier) >= 30 &&
		AIGunRange(pSoldier) >= TACTICAL_RANGE / 2 &&
		AIGunExtraAmmo(pSoldier))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AIRoleCQB(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(SoldierAI(pSoldier));

	if (AISpecialRole(pSoldier))
	{
		return FALSE;
	}

	if ((AIGunType(pSoldier) == GUN_SMG) && AIGunRange(pSoldier) < TACTICAL_RANGE / 2 || 
		AIGunType(pSoldier) == GUN_M_PISTOL ||
		AIGunType(pSoldier) == GUN_PISTOL ||
		AIGunType(pSoldier) == GUN_SHOTGUN)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckHasKey(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	// do we carry a key?
	for (int ubLoop = 0; ubLoop < NUM_KEYS; ++ubLoop)
	{
		if (SoldierHasKey(pSoldier, ubLoop))
			return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckHasTNT(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if (FindTNT(pSoldier) != NO_SLOT)
		return TRUE;

	return FALSE;
}

BOOLEAN AIGunInHandScoped(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if( UsingNewCTHSystem() == false && IsScoped(&pSoldier->inv[HANDPOS]) )
	{
		return TRUE;
	}

	if( UsingNewCTHSystem() == true && NCTHIsScoped(&pSoldier->inv[HANDPOS]) )
	{
		return TRUE;
	}
	return FALSE;
}

BOOLEAN AIGunExtraAmmo(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);
	if (bWeaponIn != NO_SLOT && 
		pSoldier->inv[bWeaponIn].exists() &&
		FindAmmoToReload(pSoldier, bWeaponIn, NO_SLOT) != NO_SLOT)
	{
		return TRUE;
	}

	return FALSE;
}

// check if gun that AI can use is scoped
BOOLEAN AIGunScoped(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		if( UsingNewCTHSystem() )
		{
			return NCTHIsScoped(&pSoldier->inv[bWeaponIn]);
		}
		else
		{
			return IsScoped(&pSoldier->inv[bWeaponIn]);
		}
	}

	return FALSE;
}

// return range for the AI gun in tiles
UINT16 AIGunRange(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return GunRange(&pSoldier->inv[bWeaponIn], pSoldier) /  CELL_X_SIZE;
	}

	return 0;
}

UINT16 AIGunMagSize(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return GetMagSize(&pSoldier->inv[bWeaponIn]);
	}

	return 0;
}

UINT16 AIGunClass(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return Weapon[pSoldier->inv[bWeaponIn].usItem].ubWeaponClass;
	}

	return 0;
}

UINT16 AIGunType(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return Weapon[pSoldier->inv[bWeaponIn].usItem].ubWeaponType;
	}

	return 0;
}


// use soldier AI - merc bodytype, no robots/tanks/boxers/etc
BOOLEAN SoldierAI( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	//if( pSoldier->bTeam != ENEMY_TEAM && pSoldier->bTeam != MILITIA_TEAM )
	//return FALSE;

	if (pSoldier->IsCivilian())
		return FALSE;

	if (!pSoldier->IsMercBodyType())
		return FALSE;

	if (pSoldier->aiData.bNeutral)
		return FALSE;

	if (pSoldier->IsBoxer())
		return FALSE;

	if (pSoldier->IsTank())
		return FALSE;

	if (pSoldier->IsVehicle())
		return FALSE;

	if (pSoldier->IsRobot())
		return FALSE;

	/*if ((PTR_CIVILIAN && (pSoldier->ubCivilianGroup == NON_CIV_GROUP ||
		(pSoldier->aiData.bNeutral && gTacticalStatus.fCivGroupHostile[pSoldier->ubCivilianGroup] == CIV_GROUP_NEUTRAL) ||
		(pSoldier->ubBodyType >= FATCIV && pSoldier->ubBodyType <= CRIPPLECIV))))
		return FALSE;*/

	return TRUE;
}

BOOLEAN VeteranAI( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
		return FALSE;

	if( pSoldier->bTeam != ENEMY_TEAM )
		return FALSE;

	return TRUE;

	/*switch ( pSoldier->ubSoldierClass )
	{	
	case SOLDIER_CLASS_REG_MILITIA: 
	case SOLDIER_CLASS_ELITE_MILITIA:
	case SOLDIER_CLASS_ARMY:
	case SOLDIER_CLASS_ELITE:           
		return TRUE;
	case SOLDIER_CLASS_GREEN_MILITIA:
	case SOLDIER_CLASS_ADMINISTRATOR:
		return FALSE;
	}

	return FALSE;*/
}

BOOLEAN EliteAI( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
		return FALSE;

	if( pSoldier->bTeam != ENEMY_TEAM )
		return FALSE;

	if (pSoldier->ubSoldierClass != SOLDIER_CLASS_ELITE)
		return FALSE;

	return TRUE;

	/*switch ( pSoldier->ubSoldierClass )
	{	
	case SOLDIER_CLASS_ELITE_MILITIA:	
	case SOLDIER_CLASS_ELITE:           
		return TRUE;
	case SOLDIER_CLASS_GREEN_MILITIA:
	case SOLDIER_CLASS_REG_MILITIA: 
	case SOLDIER_CLASS_ADMINISTRATOR:
	case SOLDIER_CLASS_ARMY:
		return FALSE;
	}

	return TRUE;*/
}

BOOLEAN CanClimbFlank(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	if(TileIsOutOfBounds(pSoldier->sTacticalSpot))
	{
		return FALSE;
	}

	// get direction of position to flank from soldier's position
	//UINT8 ubDir = GetDirectionFromGridNo ( sPos, pSoldier) ;
	UINT8 ubDir = AIDirection(pSoldier->sGridNo, pSoldier->sTacticalSpot);
	UINT8 ubDesiredDir;

	// determine desired direction
	if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT)
		ubDesiredDir = gTwoCCDirection[ubDir];
	else
		ubDesiredDir = gTwoCDirection[ubDir];

	UINT8 ubClimbDir = FindDirectionForClimbing( pSoldier, pSoldier->sGridNo);

	// allow two additional direction
	if( ubClimbDir == ubDesiredDir ||
		ubClimbDir == gOneCDirection[ubDesiredDir] ||
		ubClimbDir == gOneCCDirection[ubDesiredDir])
	{
		return TRUE;
	}

	return FALSE;
}

// return TRUE if there are soldiers in team who see enemy currently
BOOLEAN TeamSawEnemy( INT8 bTeam, UINT8 ubMax  )
{
	SOLDIERTYPE * pFriend;

	if( bTeam >= MAXTEAMS )
	{
		return FALSE;
	}

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];		

		// Make sure that character is alive, not too shocked, and conscious
		if (pFriend &&
			pFriend->bActive &&	
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsCowering() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			GuySawEnemy(pFriend, ubMax) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

// count mobile friends that are in BLACK state and not in a dangerous place or have 3/4 APs or hit enemy recently
// this is mostly used to check if we can cross dangerous area (in light at night or fresh corpses)
UINT8 CountFriendsBlack( SOLDIERTYPE *pSoldier, INT32 sClosestOpponent )
{
	CHECKF(pSoldier);

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;
	INT32 sFriendClosestOpponent;

	// by default, use closest known opponent
	if( sClosestOpponent == NOWHERE )
	{
		sClosestOpponent = ClosestKnownOpponent( pSoldier, NULL, NULL );
	}

	if(TileIsOutOfBounds(sClosestOpponent))
	{
		return 0;
	}

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];		

		// Make sure that character is alive, not too shocked, and conscious
		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&	
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsCowering() &&
			!pFriend->IsUnconscious())
		{
			//sFriendClosestOpponent = ClosestKnownOpponent( pFriend, NULL, NULL );
			sFriendClosestOpponent = ClosestSeenOpponent( pFriend, NULL, NULL );
			if(!TileIsOutOfBounds(sFriendClosestOpponent) &&
				PythSpacesAway( sClosestOpponent, sFriendClosestOpponent ) < DAY_VISION_RANGE / 2 &&
				pFriend->aiData.bAlertStatus == STATUS_BLACK &&
				pFriend->stats.bLife > pFriend->stats.bLifeMax / 2 &&
				pFriend->bInitialActionPoints > APBPConstants[AP_MINIMUM] &&
				(	pFriend->bActionPoints == pFriend->bInitialActionPoints ||
					pFriend->LastAttackHit() ||
					pFriend->usSoldierFlagMask2 & SOLDIER_SUCCESSFUL_ATTACK ||
					pFriend->LastTargetSuppressed() ||
					//GetNearestRottingCorpseAIWarning( pFriend->sGridNo ) == 0 &&
					!CorpseWarning(pFriend, pFriend->sGridNo, pFriend->pathing.bLevel) && !InLightAtNight(pFriend->sGridNo, pFriend->pathing.bLevel ) && !pFriend->aiData.bUnderFire )
				)
			{
				ubFriendCount++;
			}
		}
	}

	return ubFriendCount;
}

UINT8 CountFriendsUsedRadio(SOLDIERTYPE *pSoldier, INT16 sDistance)
{
	CHECKF(pSoldier);
	SOLDIERTYPE *pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		// Make sure that character is alive, not too shocked, and conscious
		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->RaisedAlertThisTurn())
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

// check all known enemies
BOOLEAN CanSeeEnemyAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubEnemyStance)
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sSpot) )
	{
		return FALSE;
	}

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		// obtain opponent's location and level
		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// check that our knowledge is correct
		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		// check line of sight
		if (LOS_SoldierPos(pSoldier, sSpot, pSoldier->pathing.bLevel, ANIM_STAND, pOpponent, sThreatLoc, iThreatLevel, ubEnemyStance, LOS_READY, CALC_FROM_ALL_DIRS))
		{
			return TRUE;
		}
	}

	return FALSE;
}

INT16 AIMaxDistanceVisible(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	INT32 iDistanceVisible;
	INT32 iNoScopeDistanceVisible;
	INT32 iScopeDistanceVisible = 0;

	if (pSoldier)
	{
		iDistanceVisible = pSoldier->GetMaxDistanceVisible(sSpot, bLevel, CALC_FROM_ALL_DIRS);

		gbForceWeaponNotReady = true;
		iNoScopeDistanceVisible = pSoldier->GetMaxDistanceVisible(sSpot, bLevel, CALC_FROM_ALL_DIRS);
		gbForceWeaponNotReady = false;

		gbForceWeaponReady = true;
		iScopeDistanceVisible = pSoldier->GetMaxDistanceVisible(sSpot, bLevel, CALC_FROM_ALL_DIRS);
		gbForceWeaponReady = false;

		// increase normal vision in case opponent uses scope or moves
		iDistanceVisible = max(iNoScopeDistanceVisible + TACTICAL_RANGE / 4, max(iDistanceVisible, iScopeDistanceVisible));
	}
	else
	{
		iDistanceVisible = MaxNormalDistanceVisible();
		INT8 bLightLevel = LightTrueLevel(sSpot, bLevel);

		iDistanceVisible = iDistanceVisible * gGameExternalOptions.ubBrightnessVisionMod[bLightLevel] / 100;
	}

	// use day vision range in case enemy uses break light at night
	iDistanceVisible = max(iDistanceVisible, DAY_VISION_RANGE);

	// double vision distance in case opponent uses scope, moves or uses break light etc
	//iDistanceVisible = max(iDistanceVisible * 2, iScopeDistanceVisible);

	return iDistanceVisible;
}

INT16 AIDistanceVisible(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	INT32 iDistanceVisible;
	INT32 iScopeDistanceVisible = 0;
	INT32 iNoScopeDistanceVisible = 0;

	if (pSoldier)
	{
		iDistanceVisible = pSoldier->GetMaxDistanceVisible(sSpot, bLevel, CALC_FROM_ALL_DIRS);

		gbForceWeaponReady = true;
		iScopeDistanceVisible = pSoldier->GetMaxDistanceVisible(sSpot, bLevel, CALC_FROM_ALL_DIRS);
		gbForceWeaponReady = false;

		gbForceWeaponNotReady = true;
		iNoScopeDistanceVisible = pSoldier->GetMaxDistanceVisible(sSpot, bLevel, CALC_FROM_ALL_DIRS);
		gbForceWeaponNotReady = false;

		iDistanceVisible = max(iNoScopeDistanceVisible + TACTICAL_RANGE / 4, iDistanceVisible);
	}
	else
	{
		iDistanceVisible = MaxNormalDistanceVisible();
		INT8 bLightLevel = LightTrueLevel(sSpot, bLevel);

		iDistanceVisible = iDistanceVisible * gGameExternalOptions.ubBrightnessVisionMod[bLightLevel] / 100;
	}

	return iDistanceVisible;
}

BOOLEAN TerrainCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	CHECKF(pSoldier);

	if(TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	if (pSoldier->wornCamo >= 65 && TerrainJungle(sSpot, bLevel) ||
		pSoldier->wornDesertCamo >= 65 && TerrainDesert(sSpot, bLevel) ||
		pSoldier->wornUrbanCamo >= 65 && TerrainUrban(sSpot, bLevel) ||
		//pSoldier->wornSnowCamo >= 65 && TerrainSnow(sSpot, bLevel) ||
		GetWornStealth(pSoldier) >= 65 && TerrainDark(sSpot, bLevel))
	{
		return TRUE;
	}

	return FALSE;
}

// check if we have a sight cover from known enemies at spot
BOOLEAN SightCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubStance, BOOLEAN fUnlimited, BOOLEAN fAdjacent)
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sSpot) )
	{
		return FALSE;
	}

	if (!AllowComplexAI(pSoldier))
	{
		fAdjacent = FALSE;
	}

	CoverCacheInfo coverInfo;
	coverInfo.sSpot = sSpot;
	coverInfo.bLevel = pSoldier->pathing.bLevel;
	if (ubStance == ANIM_PRONE)
		coverInfo.ubType = COVER_TYPE_PRONE;
	else if (ubStance == ANIM_CROUCH)
		coverInfo.ubType = COVER_TYPE_CROUCHED;
	else
		coverInfo.ubType = COVER_TYPE_STANDING;
	coverInfo.fUnlimited = fUnlimited;
	coverInfo.fAdjacent = fAdjacent;

	std::map<CoverCacheInfo, BOOLEAN, std::less<>>::iterator cover_iter = gCoverCache.find(coverInfo);

	if (cover_iter != gCoverCache.end())
	{
		return cover_iter->second;
	}

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT32		sThreatLoc;
	INT8		bThreatLevel;
	INT32		iDistanceVisible;
	INT8		bKnowledge;
	UINT8		ubForceReady;
	UINT8		ubOpponentStance;

	// for adjacent tiles check
	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;

	INT16 sDistance;
	INT16 sMaxDistanceVisible = MAX_VISION_RANGE;
	//std::map<INT16, UINT8, std::less<>> sorted_map;
	std::vector<std::pair<INT16, UINT8>> vector;
	std::vector<std::pair<INT16, UINT8>>::iterator iter;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);

		// if this opponent is unknown personally and publicly
		if (bKnowledge == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// check that our knowledge is correct
		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		// sevenfm: store ID in map
		sDistance = PythSpacesAway(sThreatLoc, sSpot);
		vector.push_back(std::make_pair(sDistance, pOpponent->ubID));
	}

	if (vector.size() > 0)
	{
		std::sort(vector.begin(), vector.end());

		for (iter = vector.begin(); iter != vector.end(); iter++)
		{
			pOpponent = MercPtrs[iter->second];

			bKnowledge = Knowledge(pSoldier, pOpponent->ubID);
			sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
			bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);			

			sDistance = PythSpacesAway(sThreatLoc, sSpot);

			// find opponent's vision at this spot
			if (fUnlimited)
			{
				iDistanceVisible = NO_DISTANCE_LIMIT;
				ubForceReady = LOS_NORMAL;
				ubOpponentStance = ANIM_STAND;
			}
			else
			{
				sMaxDistanceVisible = AIMaxDistanceVisible(pOpponent, sSpot, pSoldier->pathing.bLevel);
				
				iDistanceVisible = AIDistanceVisible(pOpponent, sSpot, pSoldier->pathing.bLevel);
				//iDistanceVisible = CALC_FROM_ALL_DIRS;
				//iDistanceVisible = sMaxDistanceVisible;
				ubForceReady = LOS_NORMAL;
				ubOpponentStance = ANIM_STAND;
			}

			if (fUnlimited && LOS_LocationStance(sThreatLoc, bThreatLevel, ubOpponentStance, sSpot, pSoldier->pathing.bLevel, ubStance, NO_DISTANCE_LIMIT) ||
				!fUnlimited && sDistance <= sMaxDistanceVisible && LOS_SoldierPos(pOpponent, sThreatLoc, bThreatLevel, ubOpponentStance, pSoldier, sSpot, pSoldier->pathing.bLevel, ubStance, ubForceReady, iDistanceVisible))
			{
				gCoverCache[coverInfo] = FALSE;
				return FALSE;
			}

			// check adjacent reachable tiles
			if (fAdjacent)
			{
				for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
				{
					sTempGridNo = NewGridNo(sThreatLoc, DirectionInc(ubDirection));

					if (sTempGridNo != sThreatLoc)
					{
						ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bThreatLevel];
						sDistance = PythSpacesAway(sTempGridNo, sSpot);

						if (ubMovementCost < TRAVELCOST_BLOCKED &&
							NewOKDestination(pOpponent, sTempGridNo, FALSE, bThreatLevel) &&
							(fUnlimited && LOS_LocationStance(sTempGridNo, bThreatLevel, ubOpponentStance, sSpot, pSoldier->pathing.bLevel, ubStance, NO_DISTANCE_LIMIT) ||
							!fUnlimited && sDistance <= sMaxDistanceVisible && LOS_SoldierPos(pOpponent, sTempGridNo, bThreatLevel, ubOpponentStance, pSoldier, sSpot, pSoldier->pathing.bLevel, ubStance, ubForceReady, iDistanceVisible)))
						{
							gCoverCache[coverInfo] = FALSE;
							return(FALSE);
						}
					}
				}
			}
		}
	}

	gCoverCache[coverInfo] = TRUE;
	return TRUE;
}

/*INT32	RandomizeLocation(INT32 sSpot, INT8 bLevel, UINT8 ubTimes, SOLDIERTYPE *pSightSoldier)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return NOWHERE;
	}

	UINT8 ubDirection;
	UINT8 ubMovementCost;
	INT32 sTempSpot;
	INT32 sSpotArray[NUM_WORLD_DIRECTIONS + 1];
	UINT8 ubSpots;

	for (UINT8 ubCnt = 0; ubCnt < ubTimes; ubCnt++)
	{
		// store original location
		ubSpots = 1;
		sSpotArray[0] = sSpot;

		// find adjacent locations
		for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
		{
			sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

			if (sTempSpot != sSpot)
			{
				ubMovementCost = gubWorldMovementCosts[sTempSpot][ubDirection][bLevel];

				if (ubMovementCost < TRAVELCOST_BLOCKED &&
					IsLocationSittableExcludingPeople(sTempSpot, bLevel) &&
					(!pSightSoldier || LOS_Virtual(pSightSoldier, sTempSpot, bLevel, ANIM_STAND, LOS_NORMAL, NO_DISTANCE_LIMIT)))
				{
					sSpotArray[ubSpots] = sTempSpot;
					ubSpots++;
				}
			}
		}
		// find random location
		sSpot = sSpotArray[Random(ubSpots)];
		// stop if could not find any adjacent spot
		if (ubSpots < 2)
		{
			break;
		}
	}

	return sSpot;
}*/

INT32	RandomizeLocation(INT32 sSpot, INT8 bLevel, UINT8 ubTimes, SOLDIERTYPE *pSightSoldier)
{
	if( TileIsOutOfBounds(sSpot) )
	{
		return NOWHERE;
	}

	//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"RandomizeLocation spot %d times %d", sSpot, ubTimes);

	UINT8 ubDirection;
	UINT8 ubMovementCost;
	INT32 sTempSpot;

	std::vector<INT32> spots;
	std::vector<INT32> spots_edge;
	std::vector<INT32> spots_new;
	std::vector<INT32>::iterator iter;

	// store original location
	spots.push_back(sSpot);
	spots_edge.push_back(sSpot);

	// find all spots reachable within ubTimes steps
	for (UINT8 ubCnt = 0; ubCnt < ubTimes; ubCnt++)
	{
		//ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"ubCnt %d size %d edge size %d", ubCnt, spots.size(), spots_edge.size());
		spots_new.clear();

		for (iter = spots_edge.begin(); iter != spots_edge.end(); iter++)
		{
			// find adjacent locations
			for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
			{
				sTempSpot = NewGridNo(*iter, DirectionInc(ubDirection));

				if (sTempSpot != *iter)
				{
					ubMovementCost = gubWorldMovementCosts[sTempSpot][ubDirection][bLevel];

					if (ubMovementCost < TRAVELCOST_BLOCKED &&
						std::find(spots.begin(), spots.end(), sTempSpot) == spots.end() &&						
						IsLocationSittableExcludingPeople(sTempSpot, bLevel) &&						
						(!pSightSoldier || LOS_Virtual(pSightSoldier, sTempSpot, bLevel, ANIM_STAND, LOS_NORMAL, NO_DISTANCE_LIMIT)))
					{
						spots_new.push_back(sTempSpot);
						spots.push_back(sTempSpot);
					}
				}
			}
		}

		spots_edge.clear();
		spots_edge = spots_new;
	}

	//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"finished size %d", spots.size());
	return spots[Random(spots.size())];
}

INT32	RandomizeOpponentLocation(INT32 sSpot, SOLDIERTYPE *pOpponent, INT16 sMaxDistance)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return NOWHERE;
	}

	INT8 bXOffset, bYOffset;
	INT32 sRandomSpot;

	if (sMaxDistance > 0)
	{
		for (INT cnt = 0; cnt < min(sMaxDistance * 2, 100); cnt++)
		{
			bXOffset = Random(sMaxDistance * 2 + 1) - sMaxDistance;
			bYOffset = Random(sMaxDistance * 2 + 1) - sMaxDistance;

			sRandomSpot = sSpot + bXOffset + (MAXCOL * bYOffset);

			if (!TileIsOutOfBounds(sRandomSpot) &&
				PythSpacesAway(sSpot, sRandomSpot) < sMaxDistance &&
				NewOKDestination(pOpponent, sRandomSpot, FALSE, pOpponent->pathing.bLevel))
			{
				return sRandomSpot;
			}
		}
	}

	return sSpot;
}

UINT8 ClosestOpponentInContactID(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fCheckSight)
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	UINT8 ubClosestOpponentID = NOBODY;
	INT32 sClosestDistance = 0;
	INT32 sDistance = 0;

	INT32		sThreatLoc;
	INT8		bThreatLevel;

	// find closest opponent who can possibly see us
	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// check that our knowledge is correct
		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		// skip if opponent too far
		/*if( PythSpacesAway(sThreatLoc, sSpot) > DAY_VISION_RANGE * 2 )
		{
		continue;
		}*/

		sDistance = PythSpacesAway(sSpot, sThreatLoc);
		if (ubClosestOpponentID == NOBODY || sDistance < sClosestDistance)
		{
			// check that these soldiers are in contact
			if (PersonalKnowledge(pSoldier, pOpponent->ubID) == SEEN_CURRENTLY ||
				PersonalKnowledge(pSoldier, pOpponent->ubID) == SEEN_THIS_TURN ||
				PersonalKnowledge(pSoldier, pOpponent->ubID) == SEEN_LAST_TURN ||
				pSoldier->sLastTarget != NOWHERE && (pSoldier->sLastTarget == sThreatLoc || pOpponent->ubPreviousAttackerID == pSoldier->ubID || pOpponent->ubNextToPreviousAttackerID == pSoldier->ubID) ||
				pOpponent->sLastTarget != NOWHERE && (pOpponent->sLastTarget == pSoldier->sGridNo || pSoldier->ubPreviousAttackerID == pOpponent->ubID || pSoldier->ubNextToPreviousAttackerID == pOpponent->ubID) ||
				PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == SEEN_CURRENTLY && (PersonalKnowledge(pOpponent, pSoldier->ubID) == SEEN_CURRENTLY || PersonalKnowledge(pOpponent, pSoldier->ubID) == SEEN_THIS_TURN) ||
				fCheckSight && LOS_Location(sThreatLoc, bThreatLevel, sSpot, pSoldier->pathing.bLevel, NO_DISTANCE_LIMIT))
			{
				ubClosestOpponentID = pOpponent->ubID;
				sClosestDistance = sDistance;
			}
		}
	}

	return ubClosestOpponentID;
}

// check if we have any sight cover from known enemies at spot
BOOLEAN AnyCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fAdjacent)
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sSpot) )
	{
		return FALSE;
	}

	if (!AllowComplexAI(pSoldier))
	{
		fAdjacent = FALSE;
	}

	CoverCacheInfo coverInfo;
	coverInfo.sSpot = sSpot;
	coverInfo.bLevel = pSoldier->pathing.bLevel;
	coverInfo.ubType = COVER_TYPE_ANY;
	coverInfo.fUnlimited = FALSE;
	coverInfo.fAdjacent = fAdjacent;

	std::map<CoverCacheInfo, BOOLEAN, std::less<>>::iterator cover_iter = gCoverCache.find(coverInfo);

	if (cover_iter != gCoverCache.end())
	{
		return cover_iter->second;
	}

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT32		sThreatLoc;
	INT8		bThreatLevel;
	INT16		sDistanceVisible;
	UINT8		ubThreatDir;

	INT32 sOpponentSpot;
	INT8 bOpponentLevel;
	//INT32 sClosestOpponent = ClosestKnownOpponent(pSoldier, &sOpponentSpot, &bOpponentLevel);
	INT32 sClosestOpponent = ClosestActiveOpponent(pSoldier, &sOpponentSpot, &bOpponentLevel);

	if (TileIsOutOfBounds(sClosestOpponent))
		return FALSE;

	// always check cover from closest known opponent
	if (!AnyCoverFromSpot(sSpot, pSoldier->pathing.bLevel, sOpponentSpot, bOpponentLevel))
	{
		gCoverCache[coverInfo] = FALSE;
		return FALSE;
	}

	INT16 sDistance;
	std::vector<std::pair<INT16, UINT8>> vector;
	std::vector<std::pair<INT16, UINT8>>::iterator iter;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		// obtain opponent's location and level
		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// check that our knowledge is correct
		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		// ignore opponents who are not threat to us
		if (pOpponent->IsCowering() || pOpponent->IsUnconscious() || pOpponent->stats.bLife < OKLIFE || pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// skip if opponent too far
		if( PythSpacesAway(sThreatLoc, sSpot) > DAY_VISION_RANGE * 2 )
		{
			continue;
		}

		// sevenfm: store ID in map
		sDistance = PythSpacesAway(sThreatLoc, sSpot);
		vector.push_back(std::make_pair(sDistance, pOpponent->ubID));
	}

	if (vector.size() > 0)
	{
		std::sort(vector.begin(), vector.end());

		for (iter = vector.begin(); iter != vector.end(); iter++)
		{
			pOpponent = MercPtrs[iter->second];

			//bKnowledge = Knowledge(pSoldier, pOpponent->ubID);
			sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
			bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

			sDistanceVisible = AIMaxDistanceVisible(pOpponent, sSpot, pSoldier->pathing.bLevel);
			ubThreatDir = AIDirection(sSpot, sThreatLoc);

			if (PythSpacesAway(sSpot, sThreatLoc) <= MAX_VISION_RANGE &&
				!AnyCoverFromSpot(sSpot, pSoldier->pathing.bLevel, sThreatLoc, bThreatLevel) &&
				//!AnyCoverFromDirection(sSpot, pSoldier->pathing.bLevel, ubThreatDir, TRUE, TRUE) &&
				!AnyCoverFromSpot2(sSpot, pSoldier->pathing.bLevel, sThreatLoc, bThreatLevel) &&
				(PythSpacesAway(sSpot, sThreatLoc) < TACTICAL_RANGE || !AnyCoverFromDirection(sSpot, pSoldier->pathing.bLevel, gOneCDirection[ubThreatDir]) || AIRoleSniper(pOpponent) || AIRoleMarksman(pOpponent) || LOS_LocationStance(sThreatLoc, bThreatLevel, ANIM_STAND, sSpot, pSoldier->pathing.bLevel, ANIM_PRONE, DAY_VISION_RANGE)) &&
				(PythSpacesAway(sSpot, sThreatLoc) < TACTICAL_RANGE || !AnyCoverFromDirection(sSpot, pSoldier->pathing.bLevel, gOneCCDirection[ubThreatDir]) || AIRoleSniper(pOpponent) || AIRoleMarksman(pOpponent) || LOS_LocationStance(sThreatLoc, bThreatLevel, ANIM_STAND, sSpot, pSoldier->pathing.bLevel, ANIM_PRONE, DAY_VISION_RANGE)) &&
				LOS_LocationStance(sThreatLoc, bThreatLevel, ANIM_STAND, sSpot, pSoldier->pathing.bLevel, ANIM_PRONE, sDistanceVisible))
			{
				gCoverCache[coverInfo] = FALSE;
				return FALSE;
			}

			// check adjacent reachable tiles
			if (fAdjacent)
			{
				INT32 sTempGridNo;
				UINT8 ubMovementCost;
				UINT8 ubMoveDirection;

				for (ubMoveDirection = 0; ubMoveDirection < NUM_WORLD_DIRECTIONS; ubMoveDirection++)
				{
					sTempGridNo = NewGridNo(sThreatLoc, DirectionInc(ubMoveDirection));

					if (sTempGridNo != sThreatLoc && sTempGridNo != sSpot)
					{
						ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubMoveDirection][bThreatLevel];

						if (ubMovementCost < TRAVELCOST_BLOCKED &&
							//IsLocationSittable(sTempGridNo, bThreatLevel) &&
							NewOKDestination(pOpponent, sTempGridNo, FALSE, bThreatLevel))
						{
							ubThreatDir = AIDirection(sSpot, sTempGridNo);

							if (PythSpacesAway(sSpot, sTempGridNo) <= MAX_VISION_RANGE &&
								!AnyCoverFromSpot(sSpot, pSoldier->pathing.bLevel, sTempGridNo, bThreatLevel) &&
								!AnyCoverFromSpot2(sSpot, pSoldier->pathing.bLevel, sTempGridNo, bThreatLevel) &&
								(PythSpacesAway(sSpot, sTempGridNo) < TACTICAL_RANGE || !AnyCoverFromDirection(sSpot, pSoldier->pathing.bLevel, gOneCDirection[ubThreatDir]) || AIRoleSniper(pOpponent) || AIRoleMarksman(pOpponent) || LOS_LocationStance(sTempGridNo, bThreatLevel, ANIM_STAND, sSpot, pSoldier->pathing.bLevel, ANIM_PRONE, DAY_VISION_RANGE)) &&
								(PythSpacesAway(sSpot, sTempGridNo) < TACTICAL_RANGE || !AnyCoverFromDirection(sSpot, pSoldier->pathing.bLevel, gOneCCDirection[ubThreatDir]) || AIRoleSniper(pOpponent) || AIRoleMarksman(pOpponent) || LOS_LocationStance(sTempGridNo, bThreatLevel, ANIM_STAND, sSpot, pSoldier->pathing.bLevel, ANIM_PRONE, DAY_VISION_RANGE)) &&
								LOS_LocationStance(sTempGridNo, bThreatLevel, ANIM_STAND, sSpot, pSoldier->pathing.bLevel, ANIM_PRONE, sDistanceVisible))
							{
								gCoverCache[coverInfo] = FALSE;
								return FALSE;
							}
						}
					}
				}
			}
		}
	}

	gCoverCache[coverInfo] = TRUE;
	return TRUE;
}

// quick check if has cover from closest active opponent
BOOLEAN PathCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	CHECKF(pSoldier);

	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	if (bLevel > 0)
		return FALSE;

	std::pair<INT32, INT8> spotInfo;
	spotInfo.first = sSpot;
	spotInfo.second = bLevel;

	std::map<std::pair<INT32, INT8>, UINT8, std::less<>>::iterator cover_iter = gPathCoverCache.find(spotInfo);

	if (cover_iter != gPathCoverCache.end())
	{
		return cover_iter->second;
	}

	INT32 sOpponentSpot;
	INT8 bOpponentLevel;
	INT32 sClosestOpponent = ClosestActiveOpponent(pSoldier, &sOpponentSpot, &bOpponentLevel);

	if (TileIsOutOfBounds(sClosestOpponent))
		return FALSE;

	// always check cover from closest known opponent
	if (!AnyCoverFromSpot(sSpot, pSoldier->pathing.bLevel, sOpponentSpot, bOpponentLevel, TRUE, TRUE))
	{
		gPathCoverCache[spotInfo] = FALSE;
		return FALSE;
	}

	gPathCoverCache[spotInfo] = TRUE;
	return TRUE;
}

BOOLEAN AnyCoverFromSpot(INT32 sSpot, INT8 bLevel, INT32 sThreatLoc, INT8 bThreatLevel, BOOLEAN fAllowGrass, BOOLEAN fAllowSmoke)
{
	UINT8	ubDirection;

	if( TileIsOutOfBounds( sSpot ) || TileIsOutOfBounds(sThreatLoc) )
	{
		return FALSE;
	}

	ubDirection = AIDirection(sSpot, sThreatLoc);

	return AnyCoverFromDirection(sSpot, bLevel, ubDirection, fAllowGrass, fAllowSmoke);
}

BOOLEAN AnyCoverFromDirection(INT32 sSpot, INT8 bLevel, UINT8 ubDirection, BOOLEAN fAllowGrass, BOOLEAN fAllowSmoke)
{
	INT32	sCoverSpot;
	INT8	bCoverHeight;
	UINT8	ubMovementCost;
	BOOLEAN fWallFound = FALSE;

	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	sCoverSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

	// out of bounds check
	if (sCoverSpot == sSpot)
	{
		return FALSE;
	}

	// don't consider people as cover
	/*if (WhoIsThere2(sCoverSpot, bLevel) != NOBODY)
	{
		return FALSE;
	}*/

	// no cover if can move there
	/*if (IsLocationSittableExcludingPeople(sCoverSpot, bLevel))
	{
		return FALSE;
	}*/

	// explosive structure cannot provide cover!
	if (FindStructFlag(sCoverSpot, bLevel, STRUCTURE_EXPLOSIVE))
	{
		return FALSE;
	}

	// avoid places next to water or gas/fire
	if (Water(sCoverSpot, bLevel) ||
		gpWorldLevelData[sCoverSpot].usExtFlags[bLevel] & (MAPELEMENT_EXT_BURNABLEGAS | MAPELEMENT_EXT_CREATUREGAS | MAPELEMENT_EXT_MUSTARDGAS))
	{
		return FALSE;
	}

	// cover on the edge of smoke
	if (fAllowSmoke &&
		gpWorldLevelData[sCoverSpot].usExtFlags[bLevel] & (MAPELEMENT_EXT_SMOKE) && 
		!(gpWorldLevelData[sSpot].usExtFlags[bLevel] & (MAPELEMENT_EXT_SMOKE)))
	{
		return TRUE;
	}

	// check wall/door
	ubMovementCost = gubWorldMovementCosts[sCoverSpot][ubDirection][bLevel];
	if ((ubMovementCost >= TRAVELCOST_BLOCKED || IS_TRAVELCOST_CLOSED_DOOR(ubMovementCost)))
		//&& !LOS_LocationStance(sSpot, bLevel, ANIM_PRONE, sCoverSpot, bLevel, ANIM_PRONE, NO_DISTANCE_LIMIT))
	{
		return(TRUE);
	}
	
	// check structure at spot
	bCoverHeight = GetTallestStructureHeight(sCoverSpot, bLevel);
	if (bCoverHeight > 0 && 
		StructureDensity(sCoverSpot, bLevel) > 15 &&	// >= 25
		WhoIsThere2(sCoverSpot, bLevel) == NOBODY) 
	{
		return TRUE;
	}

	// check corpse at spot
	if (gGameSettings.fOptions[TOPTION_BULLET_HIT_CORPSE])
	{
		ROTTING_CORPSE *pCorpse = GetCorpseAtGridNo(sCoverSpot, bLevel);
		if (pCorpse &&
			WhoIsThere2(sCoverSpot, bLevel) == NOBODY &&
			!Water(sCoverSpot, bLevel))
		{
			return TRUE;
		}
	}

	// allow grass as cover
	if (fAllowGrass &&
		StructureArmour(sCoverSpot, bLevel) == MATERIAL_LIGHT_VEGETATION)	// light vegetation
	{
		return TRUE;
	}

	// check if there is wall there
	/*INT32 sWindowGridNo = sSpot;
	if (ubDirection == NORTH || ubDirection == WEST)
		sWindowGridNo = sCoverSpot;

	STRUCTURE * pStructure = FindStructure(sWindowGridNo, STRUCTURE_WALLNWINDOW | STRUCTURE_WALL | STRUCTURE_FENCE);
	if (pStructure && !(pStructure->fFlags & STRUCTURE_OPEN))
	{
		fWallFound = TRUE;
	}*/

	return FALSE;
}

BOOLEAN AnyCoverFromSpot2( INT32 sSpot, INT8 bLevel, INT32 sThreatLoc, INT8 bThreatLevel, BOOLEAN fAllowGrass, BOOLEAN fAllowSmoke )
{
	UINT8	ubDirection;
	INT32	sFirstCoverSpot;
	INT32	sCoverSpot;
	INT8	bCoverHeight;
	UINT8	ubMovementCost;

	if( TileIsOutOfBounds( sSpot ) || TileIsOutOfBounds(sThreatLoc) )
	{
		return FALSE;
	}

	ubDirection = atan8(CenterX(sSpot), CenterY(sSpot), CenterX(sThreatLoc), CenterY(sThreatLoc));
	sFirstCoverSpot = NewGridNo( sSpot, DirectionInc( ubDirection ) );

	// out of bounds check
	if ( sFirstCoverSpot == sSpot)
	{
		return FALSE;
	}

	sCoverSpot = NewGridNo( sFirstCoverSpot, DirectionInc( ubDirection ) );

	// out of bounds check
	if ( sCoverSpot == sFirstCoverSpot )
	{
		return FALSE;
	}

	// explosive structure cannot provide cover!
	if (FindStructFlag(sCoverSpot, bLevel, STRUCTURE_EXPLOSIVE))
	{
		return FALSE;
	}

	// avoid places next to water or gas/fire
	if (Water(sCoverSpot, bLevel) ||
		gpWorldLevelData[sCoverSpot].usExtFlags[bLevel] & (MAPELEMENT_EXT_BURNABLEGAS | MAPELEMENT_EXT_CREATUREGAS | MAPELEMENT_EXT_MUSTARDGAS))
	{
		return FALSE;
	}

	// cover on the edge of smoke
	if (fAllowSmoke &&
		gpWorldLevelData[sCoverSpot].usExtFlags[bLevel] & (MAPELEMENT_EXT_SMOKE) &&
		!(gpWorldLevelData[sSpot].usExtFlags[bLevel] & (MAPELEMENT_EXT_SMOKE)))
	{
		return TRUE;
	}

	// check wall/door
	ubMovementCost = gubWorldMovementCosts[sCoverSpot][ubDirection][bLevel];
	if (ubMovementCost >= TRAVELCOST_BLOCKED && !LOS_LocationStance(sSpot, bLevel, ANIM_PRONE, sCoverSpot, bLevel, ANIM_PRONE, NO_DISTANCE_LIMIT))
	{
		return(TRUE);
	}	

	// no cover if can move there
	/*if (IsLocationSittableExcludingPeople(sCoverSpot, bLevel))
	{
		return FALSE;
	}*/
	
	// check structure at spot
	bCoverHeight = GetTallestStructureHeight(sCoverSpot, bLevel);
	if (bCoverHeight > 0 &&
		StructureDensity(sCoverSpot, bLevel) > 15 &&	// >= 25
		//!FindCorpseAtGridNo(sCoverSpot, bLevel) &&
		//!GetCorpseAtGridNo(sCoverSpot, bLevel) &&
		WhoIsThere2(sCoverSpot, bLevel) == NOBODY)
	{
		return TRUE;
	}

	// check corpse at spot
	if (gGameSettings.fOptions[TOPTION_BULLET_HIT_CORPSE])
	{
		ROTTING_CORPSE *pCorpse = GetCorpseAtGridNo(sCoverSpot, bLevel);
		if (pCorpse &&
			WhoIsThere2(sCoverSpot, bLevel) == NOBODY &&
			!Water(sCoverSpot, bLevel))
		{
			return TRUE;
		}
	}

	// allow grass as cover
	if (fAllowGrass &&
		StructureArmour(sCoverSpot, bLevel) == MATERIAL_LIGHT_VEGETATION)	// light vegetation
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN FindObstacleNearSpot(INT32 sSpot, INT8 bLevel)
{
	if( TileIsOutOfBounds(sSpot) )
	{
		return FALSE;
	}
	/*if( !IsLocationSittableExcludingPeople(sSpot, bLevel) )
	{
		return FALSE;
	}*/

	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo( sSpot, DirectionInc( ubDirection ) );

		if( sTempGridNo != sSpot )
		{
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ bLevel ];

			if( ubMovementCost >= TRAVELCOST_BLOCKED || !IsLocationSittableExcludingPeople(sTempGridNo, bLevel) )
			{
				return( TRUE );
			}
		}
	}

	return FALSE;
}

UINT8 CountObstaclesNearSpot(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}
	/*if (!IsLocationSittableExcludingPeople(sSpot, bLevel))
	{
		return FALSE;
	}*/

	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;
	UINT8	ubCount = 0;

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo(sSpot, DirectionInc(ubDirection));

		if (sTempGridNo != sSpot)
		{
			ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

			if (ubMovementCost >= TRAVELCOST_BLOCKED || !IsLocationSittableExcludingPeople(sTempGridNo, bLevel))
			{
				ubCount++;
			}
		}
	}

	return ubCount;
}

BOOLEAN FindShadowAtSpot(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	INT32 sNewGridNo = NewGridNo(sSpot, (UINT16)DirectionInc(NORTH));
	if (sNewGridNo != sSpot)
	{
		STRUCTURE* pStructure = gpWorldLevelData[sNewGridNo].pStructureHead;

		if (pStructure != NULL && StructureHeight(pStructure) > 1)
		{
			LEVELNODE* pShadowNode = NULL;

			if (!(pStructure->fFlags & STRUCTURE_BASE_TILE))
			{
				STRUCTURE* pBaseStructure = FindBaseStructure(pStructure);
				if (pBaseStructure != NULL)
				{
					pShadowNode = gpWorldLevelData[pBaseStructure->sGridNo].pShadowHead;
				}
			}
			else
			{
				pShadowNode = gpWorldLevelData[sNewGridNo].pShadowHead;
			}

			if (pShadowNode != NULL)
				return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN NorthSpot(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	if (bLevel > 0)
		return FALSE;

	//INT32 sNextSpot = NewGridNo(sSpot, DirectionInc(NORTHWEST));

	if (gubWorldMovementCosts[sSpot + DirectionInc(NORTHWEST)][NORTHWEST][0] == TRAVELCOST_OFF_MAP)
		//gubWorldMovementCosts[sNextSpot + DirectionInc(NORTHWEST)][NORTHWEST][0] == TRAVELCOST_OFF_MAP)
	{
		return TRUE;
	}

	return FALSE;
}

// find closest unseen this turn opponent that can be possibly seen
INT32 ClosestPossiblySeenOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel)
{
	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpponent;
	SOLDIERTYPE *pClosestOpponent = NULL;

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		// sevenfm: ignore recently seen opponent
		/*if ( *pbPersOL == SEEN_CURRENTLY || *pbPersOL == SEEN_THIS_TURN )
		{
			continue;			// next merc
		}*/

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// sevenfm: safety check
		if(TileIsOutOfBounds(sGridNo))
		{
			continue;			// next merc
		}

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo)
		{
			continue;			// next merc
		}
		
		// sevenfm: ignore opponents that we cannot see after turning
		if (LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sGridNo, bLevel, CALC_FROM_ALL_DIRS) == 0 ||
			LOS_Soldier(pSoldier, pOpponent, LOS_NORMAL, CALC_FROM_ALL_DIRS) == 0)
		{
			continue;			// next merc
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (sClosestOpponent == NOWHERE || 
			iRange < iClosestRange || 
			pClosestOpponent && 
			(pClosestOpponent->stats.bLife < OKLIFE && pOpponent->stats.bLife >= OKLIFE || 
			pClosestOpponent->IsCowering() && !pOpponent->IsCowering() || 
			pClosestOpponent->IsUnconscious() && !pOpponent->IsUnconscious()))
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
			pClosestOpponent = pOpponent;
		}
	}

	if (psGridNo)
	{
		*psGridNo = sClosestOpponent;
	}
	if (pbLevel)
	{
		*pbLevel = bClosestLevel;
	}
	return( sClosestOpponent );
}

INT32 ClosestUnseenOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel)
{
	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpponent;

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}		

		// ignore dying, cowering or unconscious
		if (pOpponent->stats.bLife < OKLIFE || pOpponent->IsCowering() || pOpponent->IsUnconscious())
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		// sevenfm: ignore seen opponents
		if( PersonalKnowledge(pSoldier, pOpponent->ubID) == SEEN_CURRENTLY || 
			PersonalKnowledge(pSoldier, pOpponent->ubID) == SEEN_THIS_TURN || 
			PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == SEEN_CURRENTLY )
		{
			continue;			// next merc
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// sevenfm: safety check
		if(TileIsOutOfBounds(sGridNo))
		{
			continue;			// next merc
		}

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo)
		{
			continue;			// next merc
		}		

		// sevenfm: ignore opponents that we cannot see after turning
		if( LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sGridNo, bLevel, CALC_FROM_ALL_DIRS) == 0 ||
			LOS_Soldier(pSoldier, pOpponent, LOS_NORMAL, CALC_FROM_ALL_DIRS) == 0)
		{
			continue;			// next merc
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (sClosestOpponent == NOWHERE || iRange < iClosestRange)
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
		}
	}

	if (psGridNo)
	{
		*psGridNo = sClosestOpponent;
	}
	if (pbLevel)
	{
		*pbLevel = bClosestLevel;
	}
	return( sClosestOpponent );
}

INT32 ClosestSeenThisTurnOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel)
{
	CHECKF(pSoldier);

	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpp;


	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpp = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpp)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpp) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( PersonalKnowledge(pSoldier, pOpp->ubID) != SEEN_CURRENTLY &&
			PersonalKnowledge(pSoldier, pOpp->ubID) != SEEN_THIS_TURN )
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownPersonalLocation(pSoldier, pOpp->ubID);
		bLevel = KnownPersonalLevel(pSoldier, pOpp->ubID);

		// check that knowledge is correct
		if( TileIsOutOfBounds(sGridNo) )
		{
			continue;
		}

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo)
		{
			continue;			// next merc
		}

		// this function is used only for turning towards closest opponent or changing stance
		// as such, if they AI is in a building,
		// we should ignore people who are on the roof of the same building as the AI
		// sevenfm: check LOS
		if( bLevel != pSoldier->pathing.bLevel && 
			SameBuilding( pSoldier->sGridNo, sGridNo ) &&
			!LOS_Virtual(pSoldier, sGridNo, bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
		{
			continue;
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (iRange < iClosestRange)
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
		}
	}

	if (psGridNo)
	{
		*psGridNo = sClosestOpponent;
	}
	if (pbLevel)
	{
		*pbLevel = bClosestLevel;
	}
	return( sClosestOpponent );
}

INT32 ClosestSeenLastTurnOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel)
{
	CHECKF(pSoldier);

	INT32		sGridNo, sClosestOpponent = NOWHERE;
	UINT32		uiLoop;
	INT32		iRange, iClosestRange = 1500;
	INT8		bLevel, bClosestLevel = 0;
	SOLDIERTYPE *pOpp;

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpp = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpp)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpp) )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( PersonalKnowledge(pSoldier, pOpp->ubID) != SEEN_CURRENTLY &&
			PersonalKnowledge(pSoldier, pOpp->ubID) != SEEN_THIS_TURN &&
			PersonalKnowledge(pSoldier, pOpp->ubID) != SEEN_LAST_TURN )
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownPersonalLocation(pSoldier, pOpp->ubID);
		bLevel = KnownPersonalLevel(pSoldier, pOpp->ubID);

		// check that knowledge is correct
		if( TileIsOutOfBounds(sGridNo) )
		{
			continue;
		}

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo)
		{
			continue;			// next merc
		}

		// this function is used only for turning towards closest opponent or changing stance
		// as such, if they AI is in a building,
		// we should ignore people who are on the roof of the same building as the AI
		if( bLevel != pSoldier->pathing.bLevel && 
			SameBuilding( pSoldier->sGridNo, sGridNo ) &&
			!LOS_Virtual(pSoldier, sGridNo, bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
		{
			continue;
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (iRange < iClosestRange)
		{
			iClosestRange = iRange;
			sClosestOpponent = sGridNo;
			bClosestLevel = bLevel;
		}
	}

	if (psGridNo)
	{
		*psGridNo = sClosestOpponent;
	}
	if (pbLevel)
	{
		*pbLevel = bClosestLevel;
	}
	return( sClosestOpponent );
}

INT16 AIGunMinAPsToShoot(SOLDIERTYPE *pSoldier, BOOLEAN fRaiseCost)
{
	CHECKF(pSoldier);

	INT16 sShootAP = 0;
	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);	

	if (bWeaponIn != NO_SLOT)
	{
		// temporarily move to handpos to call MinAPsToAttack
		if (bWeaponIn != HANDPOS)
		{
			RearrangePocket(pSoldier, HANDPOS, bWeaponIn, TEMPORARILY);
		}

		//sShootAP = MinAPsToAttack(pSoldier, pSoldier->sLastTarget, ADDTURNCOST, 0, bWeaponIn != HANDPOS ? TRUE: FALSE);
		sShootAP = MinAPsToAttack(pSoldier, pSoldier->sLastTarget, ADDTURNCOST, 0, fRaiseCost);

		// return to original position
		if (bWeaponIn != HANDPOS)
		{
			RearrangePocket(pSoldier, HANDPOS, bWeaponIn, TEMPORARILY);
		}
	}	

	return sShootAP;
}

UINT8 AIEstimateCTH(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pTarget, INT32 sMySpot, INT32 sHisSpot, BOOLEAN fMovementPenalty)
{
	INT16 sCTH;

	CHECKF(pSoldier);
	CHECKF(pTarget);

	if (TileIsOutOfBounds(sMySpot) || TileIsOutOfBounds(sHisSpot))
	{
		return 0;
	}

	UINT16 usGunRange = max(100, AIGunRange(pSoldier) * CELL_X_SIZE);
	INT16 sDistance = PythSpacesAway(sMySpot, sHisSpot);
	INT32 iRange = GetRangeInCellCoordsFromGridNoDiff(sMySpot, sHisSpot);
	FLOAT fScopeMagFactor = AIGunScopeMagFactor(pSoldier);	
	//UINT16 usSightDistance = usDistance - usDistance * fScopeMagFactor / 20;	
	UINT16 usSightDistance = (UINT16)(sDistance / fScopeMagFactor);
	INT16 sDistanceVisible = AIMaxDistanceVisible(pSoldier, sHisSpot, pTarget->pathing.bLevel);

	// basic CTH is roughly equal to soldier's effective marksmanship
	sCTH = EffectiveMarksmanship(pSoldier);
	// modify chance to hit by morale
	sCTH += GetMoraleModifier( pSoldier );
	// penalize marksmanship for fatigue
	sCTH -= GetSkillCheckPenaltyForFatigue( pSoldier, sCTH );

	// add bonus for same target
	if (sHisSpot == pSoldier->sLastTarget)
	{
		sCTH += AIM_BONUS_SAME_TARGET;
	}

	if (pSoldier->pathing.bLevel > pTarget->pathing.bLevel)
	{
		sCTH += 10;
	}
	else if (pTarget->pathing.bLevel > pSoldier->pathing.bLevel)
	{
		sCTH -= 10;
	}

	// add bonus from skill
	if( gGameOptions.fNewTraitSystem )
	{
		sCTH += NUM_SKILL_TRAITS( pSoldier, SNIPER_NT ) * 10;
	}

	// bonus/penalty from effective sight distance different from NORMAL_RANGE - 3% per tile
	//if( usSightDistance > NORMAL_RANGE )
	//{
	sCTH -= (INT16)(gGameExternalOptions.dBaseTilePenalty * (usSightDistance - NORMAL_RANGE));
	//}

	// base range penalty 1%/tile
	if (iRange > gGameExternalOptions.usNormalRange)
	{
		sCTH -= min(100, (iRange - gGameExternalOptions.usNormalRange) / CELL_X_SIZE);
	}

	// out of weapon range penalty
	if( iRange > usGunRange )
	{
		sCTH -= min(50, 2 * (iRange - usGunRange) / CELL_X_SIZE);
		// use square law
		sCTH = sCTH * usGunRange * usGunRange / (iRange * iRange);
	}

	// visibility penalty
	if (sDistance > sDistanceVisible && sDistanceVisible > 0)
	{
		sCTH -= min(gGameExternalOptions.iPenaltyShootUnSeen, gGameExternalOptions.iPenaltyShootUnSeen * (sDistance - sDistanceVisible) / sDistanceVisible);
	}

	// movement penalty
	if (!UsingNewCTHSystem() && 
		gGameExternalOptions.fDirectionalPenalty && 
		fMovementPenalty)
	{
		INT16 sDistanceMoved = 0;
		FLOAT dPenaltyCoeff = 0.0;
		UINT8 ubMoveDist;
		INT32 iPenalty = 0;
		INT32 iLongRange = CELL_X_SIZE * TACTICAL_RANGE;

		MovementPenaltyDir(pSoldier, pTarget, sDistanceMoved, dPenaltyCoeff);

		//ubMoveDist = min(sDistanceMoved, pTarget->bTilesMoved / 2);
		ubMoveDist = (UINT8)sDistanceMoved;

		if (ubMoveDist > 0)
		{
			// calculate base penalty from distance moved
			for (UINT8 cnt = 1; cnt <= ubMoveDist; cnt++)
			{
				// added penalty value is reduced when the distance from the starting point increases
				iPenalty += max(1, 32 / (1 + cnt));
			}

			//sevenfm: apply directional penalty				
			iPenalty = (INT32)(iPenalty * dPenaltyCoeff);

			// sevenfm: reduce penalty at close range
			if (iRange < iLongRange)
			{
				iPenalty = iPenalty * iRange / iLongRange;
			}
			else if (iRange > iLongRange)
			{
				iPenalty = iPenalty * iLongRange / iRange;
			}

			sCTH -= iPenalty;
		}
	}

	// check limits
	sCTH = max(1, sCTH);
	sCTH = min(100, sCTH);

	return (UINT8)sCTH;
}

// try to estimate possible suppression value from attack
UINT8 AIEstimateSuppression( SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent, INT16 sAP )
{
	CHECKF(pSoldier);
	CHECKF(pOpponent);

	// no suppression for zombies!
	if(pOpponent->IsZombie())
	{
		return 0;
	}
	
	// no suppression for creatures, tanks, vehicles and robots
	if( pOpponent->bTeam == CREATURE_TEAM ||
		TANK( pOpponent ) ||
		AM_A_ROBOT( pOpponent ) ||
		pOpponent->IsVehicle() )
	{
		return 0;
	}

	INT16 sMinAPsToAttack = (UINT8)MinAPsToAttack( pSoldier, pOpponent->sGridNo, ADDTURNCOST, 0, TRUE);

	// no suppression if there is no line of fire
	if( !LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, pOpponent->sGridNo, pOpponent->pathing.bLevel, NO_DISTANCE_LIMIT) )
	{
		return 0;
	}

	if( sMinAPsToAttack >= sAP )
	{
		return 0;
	}

	if(!AIGunAutofireCapable(pSoldier))
	{
		return 0;
	}

	UINT16 usGunRange = AIGunRange(pSoldier) * CELL_X_SIZE;

	if( usGunRange == 0 )
	{
		return 0;
	}
	UINT16 usRange = PythSpacesAway(pSoldier->sGridNo, pOpponent->sGridNo) * CELL_X_SIZE;

	// estimate possible suppression points as remaining APs
	// take into account possible 100 AP system
	UINT8 ubSuppression = min( AIGunAmmo(pSoldier), 25 * (sAP - sMinAPsToAttack) / APBPConstants[AP_MAXIMUM] );

	// modify by suppression effectiveness value
	ubSuppression = ubSuppression * AISuppressionEffectiveness() / 100;

	// suppression effectiveness is reduced with distance
	if( usRange > usGunRange )
	{
		ubSuppression = ubSuppression * usGunRange * usGunRange / (usRange * usRange);
	}

	// suppression effectiveness is reduced with distance
	if( usRange > 100 )
	{
		ubSuppression = ubSuppression * 100 / usRange;
	}

	// modify by opponent's shock
	ubSuppression = ubSuppression * 100 / (100 + pOpponent->ShockLevelPercent());

	return ubSuppression;
}

INT32 AICalcTotalEnemyThreatAtSpot( SOLDIERTYPE *pSoldier, INT32 sSpot )
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT32		iThreatCertainty;
	INT8		iThreatLevel;

	INT32		iTotalThreat = 0;
	INT32		iThreat;
	UINT8		ubSeenEnemiesLastTurn;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		if( pOpponent->bCollapsed || pOpponent->bBreathCollapsed )
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);
		iThreatCertainty = ThreatPercent[Knowledge(pSoldier, pOpponent->ubID) - OLDEST_HEARD_VALUE];

		// safety check
		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		iThreat = iThreatCertainty * CalcManThreatValue(pOpponent, sSpot, TRUE, pSoldier) / 100;

		// estimate CTH
		iThreat = iThreat * AIEstimateCTH(pOpponent, pSoldier, sThreatLoc, sSpot, FALSE) / 100;

		// sevenfm: reduce for number of enemies this opponent seen this/last turn
		// so total threat value is divided between potential targets

		ubSeenEnemiesLastTurn = CountSeenEnemiesLastTurn(pOpponent);
		if( ubSeenEnemiesLastTurn > 0 )
		{
			iThreat = iThreat / ubSeenEnemiesLastTurn;
		}			

		iTotalThreat += iThreat;
	}

	return iTotalThreat;
}

// check that enemy can see and attack at spot
BOOLEAN EnemyCanAttackSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT8		bPersonalKnowledge, bPublicKnowledge, bKnowledge;

	INT32		sThreatLoc;
	INT32		iThreatCertainty;
	INT8		iThreatLevel;

	UINT8		ubSight;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if(pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);
		iThreatCertainty = ThreatPercent[Knowledge(pSoldier, pOpponent->ubID) - OLDEST_HEARD_VALUE];

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);
		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);

		// safety check
		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		// standard interrupt conditions
		if( (bPersonalKnowledge == SEEN_CURRENTLY || bPersonalKnowledge == SEEN_THIS_TURN || bPersonalKnowledge == HEARD_THIS_TURN ||
			bPublicKnowledge == SEEN_CURRENTLY || bPublicKnowledge == SEEN_THIS_TURN || bPublicKnowledge == HEARD_THIS_TURN) )
		{
			if(!pOpponent->CanInterrupt())			
			{
				continue;
			}			
		}

		// ignore opponents without weapons
		if( !AICheckHasGun(pOpponent) &&
			PythSpacesAway(sThreatLoc, sSpot) > TACTICAL_RANGE / 2 )
		{
			continue;
		}

		// sevenfm: use real location/level
		//sThreatLoc = pOpponent->sGridNo;
		//iThreatLevel = pOpponent->pathing.bLevel;

		// if we have actual information, and this opponent has enough APs to attack
		//if( AIGunDeadliness(pOpponent) > 0 && 
		//	pOpponent->bActionPoints >= AIGunMinAPsToShoot(pOpponent) )

		ubSight = LOS_UNREADY;

		if (bKnowledge == SEEN_CURRENTLY)
		{
			ubSight = LOS_NORMAL;
		}

		if(pSoldier->aiData.bAIMorale < MORALE_CONFIDENT)
		{
			ubSight = LOS_READY;
		}

		// if enemy recently attacked this spot
		if (AICheckSpotUnderAttack(pSoldier, sSpot, FALSE))
		{
			return TRUE;
		}

		// check if enemy is focusing at spot
		if ((bKnowledge == SEEN_CURRENTLY || bKnowledge == SEEN_THIS_TURN || bKnowledge == HEARD_THIS_TURN) &&
			pOpponent->IsFocusing() &&
			!TileIsOutOfBounds(pOpponent->sFocusGridNo) &&
			SpacesAway(sSpot, pOpponent->sFocusGridNo) < 2 &&
			bLevel == pOpponent->bFocusLevel)
		{
			return TRUE;
		}

		// sevenfm: check only sight from enemy location to checked location
		if (//LOS_Location(sThreatLoc, iThreatLevel, sSpot, bLevel, MAX_VISION_RANGE) &&
			LOS_SoldierPos(pOpponent, sThreatLoc, iThreatLevel, ANIM_STAND, pSoldier, sSpot, bLevel, ANIM_STAND, ubSight, CALC_FROM_ALL_DIRS) &&
			AIGunRange(pOpponent) * 3 / 2 >= PythSpacesAway(sThreatLoc, sSpot))
		{
			return TRUE;
		}
	}

	return FALSE;
}

UINT8 CalcSpotDanger(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	CHECKF(pSoldier);

	std::pair<INT32, INT8> spotInfo;
	spotInfo.first = sSpot;
	spotInfo.second = bLevel;
	
	std::map<std::pair<INT32, INT8>, UINT8, std::less<>> ::iterator danger_iter = gSpotDangerCache.find(spotInfo);

	if (danger_iter != gSpotDangerCache.end())
	{
		return danger_iter->second;
	}

	UINT32	uiLoop;
	SOLDIERTYPE *pOpponent;
	INT8	bPersonalKnowledge, bPublicKnowledge, bKnowledge;

	INT32	sThreatLoc;
	INT32	iThreatCertainty;
	INT8	bThreatLevel;

	UINT8	ubTotalDanger = 0;
	UINT8	ubDanger;
	UINT8	ubHisCTH;
	UINT8	ubHisCTGT;
	INT32	sNewDistance;
	INT32	iHisSight;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if (pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);
		iThreatCertainty = ThreatPercent[Knowledge(pSoldier, pOpponent->ubID) - OLDEST_HEARD_VALUE];

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);
		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);

		// safety check
		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		sNewDistance = PythSpacesAway(sSpot, sThreatLoc);

		if (sNewDistance <= AIDistanceVisible(pOpponent, sSpot, bLevel))
			iHisSight = LOS_SoldierPos(pOpponent, sThreatLoc, bThreatLevel, ANIM_STAND, pSoldier, sSpot, bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS);
		else
			iHisSight = 0;

		// calculate CTGT and CTH for melee enemies
		if (pOpponent->IsZombie() || !AICheckHasGun(pOpponent))
		{
			if (sNewDistance < TACTICAL_RANGE / 2)
				ubHisCTGT = 100 - 150 * sNewDistance / TACTICAL_RANGE;	// 100 - 25
			else
				ubHisCTGT = 100 * TACTICAL_RANGE * TACTICAL_RANGE / (16 * sNewDistance * sNewDistance);	// 25 - 0

			ubHisCTH = 100;
		}
		else
		{
			//ubHisCTGT = AISoldierToLocationChanceToGetThrough(pOpponent, sSpot, bLevel, 1);
			if (sNewDistance <= max(AIGunRange(pOpponent) + TACTICAL_RANGE / 4, TACTICAL_RANGE))
			{
				ubHisCTGT = CTGT(pOpponent, sThreatLoc, bThreatLevel, ANIM_STAND, sSpot, bLevel, 1, pSoldier->ubID);
				ubHisCTH = AIEstimateCTH(pOpponent, pSoldier, sThreatLoc, sSpot, FALSE);

				// take into account close distance attack
				if (sNewDistance < TACTICAL_RANGE)
				{
					ubHisCTGT = max(ubHisCTGT, 50 - 50 * sNewDistance * sNewDistance / (TACTICAL_RANGE * TACTICAL_RANGE));
				}
			}
			else
			{
				ubHisCTGT = 0;
				ubHisCTH = 0;
			}
		}

		if (iHisSight == 0)
		{
			ubHisCTH = ubHisCTH / 4;
		}

		ubDanger = ubHisCTGT * ubHisCTH / 100;

		ubTotalDanger += (100 - ubTotalDanger) * ubDanger / 100;

		if (ubTotalDanger >= 100)
		{
			break;
		}
	}

	gSpotDangerCache[spotInfo] = ubTotalDanger;

	return ubTotalDanger;
}

FLOAT AIGunScopeMagFactor(SOLDIERTYPE *pSoldier)
{
	if( !pSoldier )
	{
		return 1.0f;
	}
	
	OBJECTTYPE *pObj;
	FLOAT BestFactor = 1.0;
	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT)
	{
		pObj = &pSoldier->inv[bWeaponIn];

		if (pObj->exists())
		{
			BestFactor = Item[pObj->usItem].scopemagfactor;

			for (attachmentList::iterator iter = (*pObj)[0]->attachments.begin(); iter != (*pObj)[0]->attachments.end(); ++iter)
			{
				if (iter->exists())
				{
					BestFactor = max(BestFactor, Item[iter->usItem].scopemagfactor);
				}
			}
		}
	}

	return( BestFactor );
}

UINT16 AIGunAmmo(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass( pSoldier, IC_GUN );

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return pSoldier->inv[bWeaponIn][0]->data.gun.ubGunShotsLeft;
	}

	return 0;
}

BOOLEAN AIGunAutofireCapable(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass( pSoldier, IC_GUN );

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return IsGunAutofireCapable( &pSoldier->inv[bWeaponIn] );
	}

	return FALSE;
}

INT32 AICalcThreatTolerance(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	return (60 + 10 * pSoldier->aiData.bAIMorale) / (1 + SoldierDifficultyLevel(pSoldier));
}

BOOLEAN InFire(INT32 sGridNo, INT8 bLevel)
{
	if (TileIsOutOfBounds(sGridNo))
	{
		return FALSE;
	}

	if (gpWorldLevelData[sGridNo].usExtFlags[bLevel] & (MAPELEMENT_EXT_BURNABLEGAS))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN InSmoke( INT32 sGridNo, INT8 bLevel, BOOLEAN fCheckNearbySource )
{
	if (TileIsOutOfBounds(sGridNo))
	{
		return FALSE;
	}

	if ( gpWorldLevelData[sGridNo].usExtFlags[ bLevel ] & (MAPELEMENT_EXT_SMOKE) )
	{
		return TRUE;
	}

	if( fCheckNearbySource && CheckSmokeEffectWithRadiusAndRoomCheck( sGridNo, bLevel, NORMAL_SMOKE_EFFECT) )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN InSmokeNearby(INT32 sGridNo, INT8 bLevel)
{
	if (TileIsOutOfBounds(sGridNo))
	{
		return FALSE;
	}

	if (gpWorldLevelData[sGridNo].usExtFlags[bLevel] & (MAPELEMENT_EXT_SMOKE))
	{
		return TRUE;
	}

	UINT8 ubDirection;
	INT32 sTempGridNo;
	UINT8 ubMovementCost;

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo(sGridNo, DirectionInc(ubDirection));

		if (sTempGridNo != sGridNo)
		{
			ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

			if (ubMovementCost < TRAVELCOST_BLOCKED &&
				gpWorldLevelData[sTempGridNo].usExtFlags[bLevel] & (MAPELEMENT_EXT_SMOKE))
			{
				return(TRUE);
			}
		}
	}
	return FALSE;
}

UINT8 AIGunDeadliness(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return Weapon[ pSoldier->inv[bWeaponIn].usItem ].ubDeadliness;
	}

	return 0;
}

INT16 AIGunStatus(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bWeaponIn != NO_SLOT && pSoldier->inv[bWeaponIn].exists())
	{
		return pSoldier->inv[bWeaponIn][0]->data.objectStatus;
	}

	return 0;
}

BOOLEAN NightLight( void )
{
	//if( GetTimeOfDayAmbientLightLevel() >= NORMAL_LIGHTLEVEL_DAY + 2 )
	if( gubEnvLightValue >= NORMAL_LIGHTLEVEL_NIGHT - 3 )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN DuskLight(void)
{
	// average between day and night
	if (gubEnvLightValue >= (NORMAL_LIGHTLEVEL_NIGHT+NORMAL_LIGHTLEVEL_DAY) / 2)
	{
		return TRUE;
	}

	return FALSE;
}

UINT8 CountTeamSeeSoldier( INT8 bTeam, SOLDIERTYPE *pSoldier )
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);

	if( bTeam >= MAXTEAMS )
	{
		return 0;
	}

	for ( cnt = gTacticalStatus.Team[ bTeam ].bFirstID; cnt <= gTacticalStatus.Team[ bTeam ].bLastID; cnt++ )
	{
		pFriend = MercPtrs[ cnt ];

		if( pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed )
		{
			if (pFriend->aiData.bOppList[ pSoldier->ubID ] == SEEN_CURRENTLY )
				//pFriend->aiData.bOppList[ pSoldier->ubID ] == SEEN_THIS_TURN )
			{
				ubFriends++;
			}
		}
	}	

	return ubFriends;
}

UINT8 CountFriendsSeeOpponent(SOLDIERTYPE *pSoldier, UINT8 ubOpponentID)
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;
	UINT8 ubFriends = 0;

	CHECKF(pSoldier);

	for (cnt = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; cnt <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; cnt++)
	{
		pFriend = MercPtrs[cnt];

		if (pFriend->bActive &&
			pFriend->bInSector &&
			pFriend != pSoldier &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed)
		{
			if (pFriend->aiData.bOppList[ubOpponentID] == SEEN_CURRENTLY)
			{
				ubFriends++;
			}
		}
	}

	return ubFriends;
}

UINT8 CountTeamNearSpot( INT8 bTeam, INT32 sSpot, UINT8 ubDistance )
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;
	UINT8 ubFriends = 0;

	if (bTeam >= MAXTEAMS || TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	for ( cnt = gTacticalStatus.Team[ bTeam ].bFirstID; cnt <= gTacticalStatus.Team[ bTeam ].bLastID; cnt++ )
	{
		pFriend = MercPtrs[ cnt ];

		if( pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE)
		{
			if ( !TileIsOutOfBounds(pFriend->sGridNo) && PythSpacesAway(pFriend->sGridNo, sSpot) <= ubDistance )
			{
				ubFriends++;
			}
		}
	}	

	return ubFriends;
}

UINT8 CountTeamUnconsciousNearSpot(INT8 bTeam, INT32 sSpot, UINT8 ubDistance)
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;
	UINT8 ubFriends = 0;

	if (bTeam >= MAXTEAMS || TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	for (cnt = gTacticalStatus.Team[bTeam].bFirstID; cnt <= gTacticalStatus.Team[bTeam].bLastID; cnt++)
	{
		pFriend = MercPtrs[cnt];

		if (pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			(pFriend->bCollapsed || pFriend->bBreathCollapsed))
		{
			if (!TileIsOutOfBounds(pFriend->sGridNo) && PythSpacesAway(pFriend->sGridNo, sSpot) <= ubDistance)
			{
				ubFriends++;
			}
		}
	}

	return ubFriends;
}

BOOLEAN TeamSeesGridNo( INT8 bTeam, INT32 sGridNo, INT8 bLevel, INT8 bStance )
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;
	UINT8 ubFriends = 0;

	if( bTeam >= MAXTEAMS )
	{
		return 0;
	}

	for ( cnt = gTacticalStatus.Team[ bTeam ].bFirstID; cnt <= gTacticalStatus.Team[ bTeam ].bLastID; cnt++ )
	{
		pFriend = MercPtrs[ cnt ];

		if( pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed )
		{
			if( (PythSpacesAway(pFriend->sGridNo, sGridNo) < (INT16)MAX_VISION_RANGE || pFriend->ubDirection == AIDirection(pFriend->sGridNo, sGridNo) && (WeaponReady(pFriend) || pFriend->IsSpotting())) &&
				LOS_Virtual(pFriend, sGridNo, bLevel, bStance, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
			{
				return TRUE;
			}
		}
	}	

	return FALSE;
}

BOOLEAN TeamAtGridno( INT8 bTeam, INT32 sGridNo, INT8 bLevel)
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;
	UINT8 ubFriends = 0;

	if( bTeam >= MAXTEAMS )
	{
		return 0;
	}

	for ( cnt = gTacticalStatus.Team[ bTeam ].bFirstID; cnt <= gTacticalStatus.Team[ bTeam ].bLastID; cnt++ )
	{
		pFriend = MercPtrs[ cnt ];

		if( pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			pFriend->sGridNo == sGridNo &&
			pFriend->pathing.bLevel == bLevel )
		{
			return TRUE;
		}
	}	

	return FALSE;
}

BOOLEAN TeamKnowsSoldier( INT8 bTeam, UINT8 ubID )
{
	SOLDIERTYPE *pFriend;
	UINT16 cnt;

	if( bTeam >= MAXTEAMS || ubID == NOBODY )
	{
		return FALSE;
	}

	if( gbPublicOpplist[bTeam][ubID] != NOT_HEARD_OR_SEEN )
	{
		return TRUE;
	}

	for ( cnt = gTacticalStatus.Team[ bTeam ].bFirstID; cnt <= gTacticalStatus.Team[ bTeam ].bLastID; cnt++ )
	{
		pFriend = MercPtrs[ cnt ];

		if( pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			pFriend->aiData.bOppList[ ubID ] != NOT_HEARD_OR_SEEN )
		{
			return TRUE;
		}
	}	

	return FALSE;
}

BOOLEAN SeenThisTurnEnemyCanSeeMe( SOLDIERTYPE *pSoldier )
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT8		bPersonalKnowledge, bPublicKnowledge;

	CHECKF( pSoldier );

	//loop through all the enemies and determine the cover
	for (uiLoop = 0; uiLoop<guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if opponent is collapsed/breath collapsed
		if( pOpponent->bCollapsed || pOpponent->bBreathCollapsed )
		{
			continue;
		}

		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);

		// check that we see opponent and he can see us
		if( (bPersonalKnowledge == SEEN_CURRENTLY ||
			bPersonalKnowledge == SEEN_THIS_TURN ||
			bPublicKnowledge == SEEN_CURRENTLY) &&
			PersonalKnowledge(pOpponent, pSoldier->ubID) == SEEN_CURRENTLY )
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN SeenEnemyCanSeeMe( SOLDIERTYPE *pSoldier )
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT8		bPersonalKnowledge, bPublicKnowledge;

	CHECKF( pSoldier );

	//loop through all the enemies and determine the cover
	for (uiLoop = 0; uiLoop<guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if opponent is collapsed/breath collapsed
		if( pOpponent->bCollapsed || pOpponent->bBreathCollapsed )
		{
			continue;
		}

		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);

		// check that we see opponent and he can see us
		if( (bPersonalKnowledge == SEEN_CURRENTLY ||
			bPublicKnowledge == SEEN_CURRENTLY) &&
			PersonalKnowledge(pOpponent, pSoldier->ubID) == SEEN_CURRENTLY )
		{
			return TRUE;
		}
	}

	return FALSE;
}

// find alerted opponent
BOOLEAN EnemyAlerted(SOLDIERTYPE *pSoldier)
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	CHECKF(pSoldier);

	//loop through all the enemies and determine the cover
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// if opponent is collapsed/breath collapsed
		if (pOpponent->IsUnconscious())
		{
			continue;
		}

		if (pOpponent->aiData.bAlertStatus >= STATUS_RED)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// find alerted opponent
BOOLEAN TeamEnemyAlerted(INT8 bTeam)
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	//loop through all the enemies and determine the cover
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidTeamOpponent(bTeam, pOpponent))
		{
			continue;
		}

		// if opponent is collapsed/breath collapsed
		if (pOpponent->IsUnconscious())
		{
			continue;
		}

		if (pOpponent->aiData.bAlertStatus >= STATUS_RED)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void RaiseSuspicionVictim(SOLDIERTYPE *pVictim, INT16 sAP)
{
	UINT8	ubIDLoop;
	UINT32	uiValue;
	SOLDIERTYPE *pCovert;

	if(!pVictim)
	{
		return;
	}

	// check player spies
	for (ubIDLoop = gTacticalStatus.Team[OUR_TEAM].bFirstID; ubIDLoop <= gTacticalStatus.Team[OUR_TEAM].bLastID; ubIDLoop++)
	{
		pCovert = MercPtrs[ubIDLoop];
		// check that opponent is covert
		if (pCovert &&
			pCovert->bActive &&
			pCovert->bInSector &&
			pCovert->stats.bLife >= OKLIFE &&
			pCovert->IsCovert())
		{
			// add suspicion from all soldiers who see this soldier taking damage and recently seen/heard this spy
			uiValue = CountSuspicionValue(pCovert, pVictim);
			// use ubCombinedLoss as AP for counting suspicion value
			pCovert->AddSuspicion(uiValue * sAP);
		}
	}
}

UINT32 CountSuspicionValue( SOLDIERTYPE *pCovert, SOLDIERTYPE *pVictim )
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	UINT32		uiValue;
	UINT32		uiTotalValue = 0;	

	CHECKF( pCovert );

	if( !pCovert->bActive || !pCovert->bInSector )
	{
		return 0;
	}

	// only new skill system
	if( !gGameOptions.fNewTraitSystem )
	{
		return 0;
	}

	// no suspicion for assassins
	if( pCovert->IsAssassin() )
	{
		return 0;
	}

	// no suspicion if soldier is not covert
	if(!pCovert->IsCovert())
	{
		return 0;
	}

	// no suspicion for cowering civilian spy, except if someone is attacked
	if( pCovert->IsCovertCiv() && pCovert->IsCowering() && !pVictim )
	{
		return 0;
	}

	//loop through all the enemies
	for (uiLoop = 0; uiLoop<guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pCovert, pOpponent))
		{
			continue;
		}

		if(pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// creatures should not raise suspicion counter even if they are hostile to player
		if( pOpponent->bTeam == CREATURE_TEAM )
		{
			continue;
		}

		// if opponent is collapsed/breath collapsed
		if( pOpponent->bCollapsed || pOpponent->bBreathCollapsed )
		{
			continue;
		}

		// if victim soldier is specified, skip opponent if:
		// he is not a friend of victim or
		// he is not victim himself, he cannot see victim, he is too far to hear victim or he is deafened
		if( pVictim != NULL &&
			(pOpponent->bSide != pVictim->bSide ||
			pVictim != pOpponent &&
			!LOS_Soldier(pOpponent, pVictim, LOS_NORMAL, CALC_FROM_WANTED_DIR) &&
			(PythSpacesAway(pOpponent->sGridNo, pVictim->sGridNo) > DAY_VISION_RANGE || 
				pOpponent->bDeafenedCounter > 0 ||
				!LOS_Virtual(pOpponent, pVictim->sGridNo, pVictim->pathing.bLevel, ANIM_STAND, LOS_NORMAL, CALC_FROM_ALL_DIRS))))
		{
			continue;
		}

		// check that this opponent sees us or seen/heard recently (if alert is raised)
		if(pOpponent->aiData.bOppList[ pCovert->ubID ] == SEEN_CURRENTLY ||
			pOpponent->aiData.bAlertStatus >= STATUS_RED && 
			(pOpponent->aiData.bOppList[ pCovert->ubID ] == SEEN_THIS_TURN || pOpponent->aiData.bOppList[ pCovert->ubID ] == HEARD_THIS_TURN) &&
			LOS_Soldier(pOpponent, pCovert, LOS_NORMAL, CALC_FROM_ALL_DIRS))	// for seen/heard this turn, check if spy can be seen
		{
			UINT8	ubCovertLevel = NUM_SKILL_TRAITS(pCovert, COVERT_NT);
			INT16	sDistance = PythSpacesAway(pCovert->sGridNo, pOpponent->sGridNo);
			INT8	bTownId = GetTownIdForSector( gWorldSectorX, gWorldSectorY );
			UINT8	ubSectorId = SECTOR(gWorldSectorX, gWorldSectorY);
			UINT8	ubSectorData = 0;
			UINT8	ubKnownEnemiesNearby = CountPublicKnownEnemies(pOpponent, pCovert->sGridNo, TACTICAL_RANGE);			
			INT16	sCloseDistance = DAY_VISION_RANGE / 4;

			ubSectorData = SectorExternalData[ubSectorId][gbWorldSectorZ].usCurfewValue;

			if ( NightLight() )			// suspicious at night
				ubSectorData = max( ubSectorData, 1 );
			if ( gbWorldSectorZ > 0 )	// underground we are always suspicious				
				ubSectorData = max( ubSectorData, 2 );

			// -----------------------------------------------------------------------------------------------------
			// calculate basic value 

			uiValue = 1 + SoldierDifficultyLevel( pOpponent );
			// add bonus for squad leader
			if (HAS_SKILL_TRAIT( pOpponent, SQUADLEADER_NT ) )
			{
				uiValue += NUM_SKILL_TRAITS( pOpponent, SQUADLEADER_NT );
			}
			// bonus when using flashlight
			if ( pCovert->GetBestEquippedFlashLightRange() > 0 )
			{
				uiValue++;
			}
			// bonus if bleeding
			if ( pCovert->bBleeding > 0 )
			{
				uiValue++;
			}
			// bonus for soldier state
			if ( MercUnderTheInfluence( pCovert ) ||
				GetDrunkLevel( pCovert ) != SOBER )
			{
				uiValue += 1;
			}
			// bonus in capital
			if ( bTownId == MEDUNA )	
			{
				uiValue += 2;
			}
			// wearing or carrying backpack is suspicious
			if ( UsingNewInventorySystem() && FindBackpackOnSoldier( pCovert ) )
			{
				uiValue += 2;
			}
			// bonus if spotting
			if( pCovert->IsSpotting() )
			{
				uiValue += 2;
			}
			// bonus if soldier is carrying item with sight bonus in main hand
			if( pCovert->inv[HANDPOS].exists() &&
				Item[ pCovert->inv[HANDPOS].usItem ].dayvisionrangebonus > 0 || 
				Item[ pCovert->inv[HANDPOS].usItem ].brightlightvisionrangebonus > 0 || 
				Item[ pCovert->inv[HANDPOS].usItem ].nightvisionrangebonus > 0 || 
				Item[ pCovert->inv[HANDPOS].usItem ].cavevisionrangebonus > 0 )
			{
				uiValue += 2;
			}
			// bonus if there are friends under attack nearby
			uiValue += CountTeamUnderAttack(pOpponent->bTeam, pOpponent->sGridNo, DAY_VISION_RANGE / 2);
			// bonus if observing soldier sees more than one covert soldier
			uiValue += CountSeenCovertOpponents(pOpponent);
			// bonus for enemies in contact
			uiValue += CountSeenEnemiesLastTurn(pOpponent, FALSE);
			// add number of known enemies
			uiValue += (UINT32)sqrt(CountPublicKnownEnemies(pOpponent, pCovert->sGridNo, DAY_VISION_RANGE));
			// add number of friends killed
			if (pOpponent->bTeam == ENEMY_TEAM)
			{
				uiValue += (UINT32)sqrt(gTacticalStatus.ubArmyGuysKilled);
			}
			// add number of corpses nearby
			uiValue += CountCorpses(pOpponent, pCovert->sGridNo, DAY_VISION_RANGE / 2, TRUE);

			// -----------------------------------------------------------------------------------------------------
			// multipliers						

			// bonus if opponent previously saw this spy uncovered
			if (pOpponent->aiData.bOppList[pCovert->ubID] == SEEN_CURRENTLY &&
				gbSeenOpponents[pOpponent->ubID][pCovert->ubID] == SEEN_UNCOVERED)
			{
				uiValue = uiValue * 4;
			}

			// bonus for suspicious movement mode
			if ( pCovert->bStealthMode || 
				gAnimControl[ pCovert->usAnimState ].ubEndHeight != ANIM_STAND )
			{
				uiValue = uiValue * 2;
			}

			// bonus when running
			if(pCovert->AnimMovementMode() == RUNNING)
			{
				uiValue = uiValue * 2;
			}

			// soldier spies cause more suspicion (this can be compensated by skill)
			if( pCovert->IsCovertSoldier() )
			{
				uiValue = uiValue * 2;
			}

			// bonus if alert is raised
			if( pOpponent->aiData.bAlertStatus >= STATUS_RED )
			{
				uiValue = uiValue * 2;

				// increase if watching soldier is in combat situation
				if (GuySawEnemy(pOpponent, SEEN_LAST_TURN) || GuyHeardEnemy(pOpponent, HEARD_LAST_TURN))
				{
					uiValue = uiValue * 2;

					// more penalty for civilian spy in combat situation
					if (pCovert->IsCovertCiv())
					{
						uiValue = uiValue * 2;
					}
				}

				if (pOpponent->aiData.bUnderFire)
				{
					uiValue = uiValue * 2;

					// more penalty for civilian spy in combat situation
					if (pCovert->IsCovertCiv())
					{
						uiValue = uiValue * 2;
					}
				}

				// bonus if there are known enemies nearby
				uiValue += uiValue * (100 - 100 / (1 + ubKnownEnemiesNearby)) / 100;
			}			
			
			// bonus if weapon raised
			if( WeaponReady(pCovert) )
			{
				uiValue = uiValue * 2;
			}

			// bonus from uniform type (admin - 1, regular - 2, elite - 3)
			uiValue = uiValue * pCovert->UniformLevel();

			// -----------------------------------------------------------------------------------------------------
			// some modifiers can reduce suspicion level

			// less suspicion from hearing
			if( pOpponent->aiData.bOppList[ pCovert->ubID ] != SEEN_CURRENTLY )
			{
				uiValue = uiValue / 2;
			}

			// reduce 2-4 times if soldier has covert trait
			if ( HAS_SKILL_TRAIT( pCovert, COVERT_NT ) )
			{
				uiValue = uiValue / (2 * NUM_SKILL_TRAITS( pCovert, COVERT_NT ));
			}
			// for special NPCs even without covert trait
			else if( pCovert->IsCovertCivSpecial() )
			{
				uiValue = uiValue / 2;
			}
			// if observer is drunk, he is less suspicious
			if( GetDrunkLevel(pOpponent) > SOBER && GetDrunkLevel(pOpponent) < HUNGOVER )
			{
				uiValue -= uiValue * GetDrunkLevel(pOpponent) * 25 / 100;
			}
			// high level spy can lower suspicion by 50%
			uiValue -= uiValue * (EffectiveWisdom(pCovert) + pCovert->stats.bExpLevel * 10) / 400;
			
			// finally reduce according to the distance, 4 times at max day vision range
			if( sDistance > sCloseDistance )
			{
				uiValue = uiValue * sCloseDistance / sDistance;
			}
			// bonus to value if very close and alert is raised
			else if( pOpponent->aiData.bAlertStatus >= STATUS_RED  )
			{
				uiValue += uiValue * (sCloseDistance - sDistance) / sCloseDistance;
			}			

			// -----------------------------------------------------------------------------------------------------

			uiTotalValue += uiValue;
		}
	}

	return uiTotalValue;
}

BOOLEAN EnemySeenSoldierRecently( SOLDIERTYPE *pSoldier, INT8 bMax, BOOLEAN fOnlyAlerted )
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE || pOpponent->IsUnconscious())
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		if( fOnlyAlerted && pOpponent->aiData.bAlertStatus < STATUS_RED )
		{
			continue;
		}

		// check that this opponent have seen us recently
		if( PersonalKnowledge(pOpponent, pSoldier->ubID) >= SEEN_CURRENTLY && PersonalKnowledge(pOpponent, pSoldier->ubID) <= bMax ||
			PublicKnowledge(pOpponent->bTeam, pSoldier->ubID) >= SEEN_CURRENTLY && PublicKnowledge(pOpponent->bTeam, pSoldier->ubID) <= bMax )
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN EnemyHeardSoldierRecently( SOLDIERTYPE *pSoldier, INT8 bMax, BOOLEAN fOnlyAlerted )
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE || pOpponent->IsUnconscious())
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if( fOnlyAlerted && pOpponent->aiData.bAlertStatus < STATUS_RED )
		{
			continue;
		}

		// check that this opponent have seen us recently
		if( PersonalKnowledge(pOpponent, pSoldier->ubID) <= HEARD_THIS_TURN && PersonalKnowledge(pOpponent, pSoldier->ubID) >= bMax ||
			PublicKnowledge(pOpponent->bTeam, pSoldier->ubID) <= HEARD_THIS_TURN &&	PublicKnowledge(pOpponent->bTeam, pSoldier->ubID) >= bMax )
		{
			return TRUE;
		}
	}

	return FALSE;
}

// count friends in black state or those who seen enemy last turn
UINT8 CountTeamCombat( INT8 bTeam )
{
	if(bTeam >= MAXTEAMS)
	{
		return 0;
	}

	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];		

		// Make sure that character is alive, not too shocked, and conscious
		if( pFriend &&
			pFriend->bActive &&
			pFriend->bInSector &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			!pFriend->IsPOW() )
		{
			if(	pFriend->aiData.bAlertStatus == STATUS_BLACK ||
				GuySawEnemy(pFriend, SEEN_LAST_TURN) )
			{
				ubFriendCount++;
			}
		}
	}

	return ubFriendCount;
}

BOOLEAN GuyKnowsEnemyPosition( SOLDIERTYPE * pSoldier )
{
	CHECKF(pSoldier);

	UINT8		uiLoop;
	SOLDIERTYPE *pOpponent;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector)
		{
			continue;
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this guy knows something about this enemy
		if( Knowledge(pSoldier, pOpponent->ubID) != NOT_HEARD_OR_SEEN )
		{
			return( TRUE );
		}
	}

	/*INT32 sGridNo;
	INT8 bLevel;

	// sevenfm: also consider watched locations
	for ( INT8 bLoop = 0; bLoop < NUM_WATCHED_LOCS; bLoop++ )
	{
		sGridNo = gsWatchedLoc[ pSoldier->ubID ][ bLoop ];
		bLevel = gbWatchedLocLevel[ pSoldier->ubID ][ bLoop ];

		if ( !TileIsOutOfBounds(sGridNo) )
		{
			return TRUE;
		}
	}*/

	return( FALSE );
}

void PlayLNL( SOLDIERTYPE* pSoldier )
{
	CHECKV(pSoldier);

	// check main hand
	if( pSoldier->inv[ HANDPOS ].exists() &&
		Item[ pSoldier->inv[ HANDPOS ].usItem ].usItemClass == IC_GUN )
	{
		UINT16	usItem = pSoldier->inv[ HANDPOS ].usItem;
		UINT16	usWeapon = Item[ usItem ].ubClassIndex;		

		pSoldier->PlayLNL(usWeapon);
	}
}

BOOLEAN CheckDoorAtGridno( UINT32 usGridNo )
{
	STRUCTURE *pStructure;

	pStructure = FindStructure( usGridNo, STRUCTURE_ANYDOOR );
	if ( pStructure != NULL)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN CheckDoorNearGridno( UINT32 usGridNo )
{
	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;

	if( CheckDoorAtGridno(usGridNo) )
	{
		return TRUE;
	}

	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo( usGridNo, DirectionInc( ubDirection ) );

		if( sTempGridNo != usGridNo )
		{
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ 0 ];
			if ( IS_TRAVELCOST_DOOR( ubMovementCost ) )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

BOOLEAN FindBombNearby( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance )
{
	UINT32	uiBombIndex;
	INT32 sBombSpot;
	INT8 bBombLevel;

	// search all bombs that we can see
	for (uiBombIndex = 0; uiBombIndex < guiNumWorldBombs; uiBombIndex++)
	{
		if (gWorldBombs[uiBombIndex].fExists)
		{
			sBombSpot = gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].sGridNo;
			bBombLevel = gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].ubLevel;

			if (!TileIsOutOfBounds(sBombSpot) &&
				PythSpacesAway(sGridNo, sBombSpot) <= sDistance &&
				//bBombLevel == pSoldier->pathing.bLevel &&
				(gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].bVisible == VISIBLE || !pSoldier->IsSoldierPC() && gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].bVisible == INVISIBLE) &&
				gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].usFlags & WORLD_ITEM_ARMED_BOMB)
			{
				// only enemy team needs to check LOS
				if (!(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral) ||
					gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].usFlags & WORLD_ITEM_SEEN_BOMB ||
					sBombSpot == pSoldier->sGridNo ||
					PythSpacesAway(pSoldier->sGridNo, sBombSpot) <= (INT16)MAX_VISION_RANGE &&
					LOS_Virtual(pSoldier, sBombSpot, bBombLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_WANTED_DIR))
				{
					// set bomb as seen for enemy team and not neutral soldiers from civ team
					if (pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral)
					{
						gWorldItems[gWorldBombs[uiBombIndex].iItemIndex].usFlags |= WORLD_ITEM_SEEN_BOMB;
					}
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

BOOLEAN AllowDeepWater( SOLDIERTYPE *pSoldier )
{
	// check if soldier is non swimmer or has high carried weight
	if (pSoldier->ubProfile != NO_PROFILE && gMercProfiles[pSoldier->ubProfile].bDisability == NONSWIMMER || 
		pSoldier->sWeightCarriedAtTurnStart > 70 ||
		pSoldier->bBreath < 50)
	{
		return FALSE;
	}

	return TRUE;
}

INT16 DistanceToClosestNotSeekEnemyFriend( SOLDIERTYPE *pSoldier, INT32 sGridNo )
{
	CHECKF(pSoldier);

	INT16 sDistance = 0;
	SOLDIERTYPE * pFriend;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];		

		// Make sure that character is alive, not too shocked, and conscious
		if( pFriend &&
			pFriend->bActive &&
			pFriend->bInSector &&
			pFriend != pSoldier &&
			pFriend->aiData.bOrders != SEEKENEMY &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			!pFriend->IsPOW() )
		{
			if(	sDistance == 0 || PythSpacesAway(sGridNo, pFriend->sGridNo) < sDistance )
			{
				sDistance = PythSpacesAway(sGridNo, pFriend->sGridNo);
			}
		}
	}

	return sDistance;
}

BOOLEAN SightCoverNearSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubStance, BOOLEAN fUnlimited)
{
	CHECKF(pSoldier);

	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo( sSpot, DirectionInc( ubDirection ) );

		if( sTempGridNo != sSpot )
		{
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ pSoldier->pathing.bLevel ];

			if ( ubMovementCost < TRAVELCOST_BLOCKED &&
				NewOKDestination(pSoldier, sTempGridNo, FALSE, pSoldier->pathing.bLevel) &&
				SightCoverAtSpot(pSoldier, sTempGridNo, ubStance, fUnlimited))
			{
				return( TRUE );
			}
		}
	}

	return( FALSE );
}

BOOLEAN AnyCoverNearSpot(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	CHECKF(pSoldier);

	UINT8	ubMovementCost;
	INT32	sTempGridNo;
	UINT8	ubDirection;

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo(sSpot, DirectionInc(ubDirection));

		if (sTempGridNo != sSpot)
		{
			ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][pSoldier->pathing.bLevel];

			if (ubMovementCost < TRAVELCOST_BLOCKED &&
				NewOKDestination(pSoldier, sTempGridNo, FALSE, pSoldier->pathing.bLevel) &&
				AnyCoverAtSpot(pSoldier, sTempGridNo))
			{
				return(TRUE);
			}
		}
	}

	return(FALSE);
}

// count all soldiers that can possibly participate in combat on any side
UINT8 CountNotNeutral(void)
{
	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	UINT8 ubNum = 0;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		if (!pOpponent || !pOpponent->bActive || !pOpponent->bInSector || pOpponent->stats.bLife < OKLIFE || pOpponent->aiData.bNeutral)
			continue;

		ubNum++;
	}

	return ubNum;
}

// only allow complex AI calculations if number of possible combatants is no more than 64
BOOLEAN AllowComplexAI(SOLDIERTYPE *pSoldier)
{
	//if (pSoldier->ubSoldierClass != SOLDIER_CLASS_ELITE && pSoldier->ubSoldierClass != SOLDIER_CLASS_ARMY)
	if (pSoldier->bTeam != ENEMY_TEAM)
		return FALSE;

	if (!gfTurnBasedAI)
		return FALSE;

	//if (CountNotNeutral() > 42)
		//return FALSE;

	return TRUE;
}

UINT8 CountPublicKnownEnemies( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance )
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	UINT8		ubNum = 0;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// check public knowledge
		if( PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		sThreatLoc = KnownPublicLocation(pSoldier->bTeam, pOpponent->ubID);
		iThreatLevel = KnownPublicLevel(pSoldier->bTeam, pOpponent->ubID);

		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		// check distance
		if( PythSpacesAway(sThreatLoc, sGridNo ) > sDistance )
		{
			continue;
		}

		ubNum++;
	}

	return ubNum;
}

UINT8 CountPublicKnownEnemies(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	UINT8		ubNum = 0;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check public knowledge
		if (PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownPublicLocation(pSoldier->bTeam, pOpponent->ubID);
		iThreatLevel = KnownPublicLevel(pSoldier->bTeam, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		ubNum++;
	}

	return ubNum;
}

UINT8 CountFriendsInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoom)
{
	CHECKF(pSoldier);

	SOLDIERTYPE *pFriend;
	UINT8		 ubFriendCount = 0;
	UINT16		usRoomNo;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			InARoom(pFriend->sGridNo, &usRoomNo) &&
			usRoomNo == usRoom)
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

UINT16 SumDistanceToKnownEnemiesInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoom, INT32 sSpot)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	UINT8		ubNum = 0;
	UINT16		usSum = 0;
	UINT16		usRoomNo;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check public knowledge
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		// check room
		if (!InARoom(sThreatLoc, &usRoomNo))
		{
			continue;
		}

		if (usRoomNo != usRoom)
		{
			continue;
		}

		ubNum++;
		usSum += PythSpacesAway(sThreatLoc, sSpot);
	}

	return usSum;
}

UINT8 CountKnownEnemiesInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoom)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	UINT8		ubNum = 0;
	UINT16		usRoomNo;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check public knowledge
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		// check room
		if (!InARoom(sThreatLoc, &usRoomNo))
		{
			continue;
		}

		if( usRoomNo != usRoom )
		{
			continue;
		}

		ubNum++;
	}

	return ubNum;
}

UINT8 CountKnownEnemies(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, INT8 bLevel)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		bThreatLevel;

	UINT8		ubNum = 0;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check knowledge
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		if (PythSpacesAway(sSpot, sThreatLoc) > sDistance)
		{
			continue;
		}

		if (bLevel >= 0 && bThreatLevel != bLevel)
		{
			continue;
		}

		ubNum++;
	}

	return ubNum;
}

UINT8 CountKnownEnemiesInDirection(SOLDIERTYPE *pSoldier, UINT8 ubDirection, INT16 sDistance, BOOLEAN fAdjacent)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	UINT8		ubNum = 0;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check knowledge
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		if (PythSpacesAway(pSoldier->sGridNo, sThreatLoc) > sDistance)
		{
			continue;
		}

		if (AIDirection(pSoldier->sGridNo, sThreatLoc) != ubDirection &&
			(!fAdjacent || AIDirection(pSoldier->sGridNo, sThreatLoc) != gOneCDirection[ubDirection] &&	AIDirection(pSoldier->sGridNo, sThreatLoc) != gOneCCDirection[ubDirection]))
		{
			continue;
		}

		ubNum++;
	}

	return ubNum;
}

UINT8 CorpseWarning( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT8 bLevel)
{
	CHECKF(pSoldier);

	INT32			cnt;
	ROTTING_CORPSE *pCorpse;
	UINT8			ubDistance = CORPSE_WARNING_DIST;
	UINT8			ubWarning = 0;

	for ( cnt = 0; cnt < giNumRottingCorpse; ++cnt )
	{
		pCorpse = &(gRottingCorpse[ cnt ] );

		if( pCorpse &&
			pCorpse->fActivated &&
			pCorpse->def.ubType < ROTTING_STAGE2 &&
			pCorpse->def.ubBodyType <= REGFEMALE &&
			pCorpse->def.ubAIWarningValue > ubWarning &&
			pCorpse->def.bLevel == bLevel &&
			!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
			PythSpacesAway(sGridNo, pCorpse->def.sGridNo) <= ubDistance &&
			(pSoldier->bTeam == ENEMY_TEAM && CorpseEnemyTeam(pCorpse) || pSoldier->bTeam == MILITIA_TEAM && CorpseMilitiaTeam(pCorpse) || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral))
		{
			ubWarning = pCorpse->def.ubAIWarningValue;
		}
	}

	return ubWarning;
}

UINT16 CorpseMoralePenalty(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	CHECKF(pSoldier->ubProfile != NO_PROFILE);

	ROTTING_CORPSE *pCorpse;
	UINT16	usPenalty = 0;
	//UINT8	ubCorpseDirection;
	UINT8	ubSoldierDirection;
	BOOLEAN	fMalicious = FALSE;
	BOOLEAN	fPrimitive = FALSE;
	BOOLEAN fPacifist = FALSE;
	INT32 sNewSpot;
	INT32 sGridNo = pSoldier->sGridNo;
	INT8 bLevel = pSoldier->pathing.bLevel;

	if (pSoldier->AnimEndHeight() > ANIM_PRONE)
		return 0;

	if (gMercProfiles[pSoldier->ubProfile].bCharacterTrait == CHAR_TRAIT_MALICIOUS)
		fMalicious = TRUE;

	if (gMercProfiles[pSoldier->ubProfile].bCharacterTrait == CHAR_TRAIT_PRIMITIVE)
		fPrimitive = TRUE;

	if (gMercProfiles[pSoldier->ubProfile].bCharacterTrait == CHAR_TRAIT_PACIFIST)
		fPacifist = TRUE;
	
	// check tile under merc
	pCorpse = GetCorpseAtGridNo(sGridNo, bLevel);
	if (pCorpse &&
		pCorpse->fActivated &&
		(pCorpse->def.ubBodyType <= REGFEMALE || pCorpse->def.ubBodyType >= FATCIV && pCorpse->def.ubBodyType <= CRIPPLECIV) &&
		!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
		pCorpse->def.sGridNo == sGridNo)
	{
		if (pCorpse->def.ubType >= ROTTING_STAGE2)
			usPenalty = max(usPenalty, 40);
		else if (fMalicious || fPrimitive)
			usPenalty = max(usPenalty, 10);
		else
			usPenalty = max(usPenalty, 20);
	}

	// check tiles around merc
	for (UINT8 ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sNewSpot = NewGridNo(sGridNo, DirectionInc(ubDirection));

		if (sNewSpot != sGridNo)
		{
			pCorpse = GetCorpseAtGridNo(sNewSpot, bLevel);

			if (pCorpse &&
				pCorpse->fActivated &&
				(pCorpse->def.ubBodyType <= REGFEMALE || pCorpse->def.ubBodyType >= FATCIV && pCorpse->def.ubBodyType <= CRIPPLECIV))
			{
				ubSoldierDirection = pSoldier->ubDirection;
				// check if corpse is in the same tile or in front of merc
				if (ubDirection == ubSoldierDirection || ubDirection == gOneCDirection[ubSoldierDirection] || ubDirection == gOneCCDirection[ubSoldierDirection])
				{
					if (pCorpse->def.ubType >= ROTTING_STAGE2)
						usPenalty = max(usPenalty, 40);
					else if (fMalicious || fPrimitive)
						usPenalty = max(usPenalty, 10);
					else
						usPenalty = max(usPenalty, 20);
				}
			}
		}
	}

	if (fPacifist)
		usPenalty = usPenalty * 2;

	return min(90, usPenalty);
}

INT32	CountCorpses(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, BOOLEAN fCheckSight)
{
	CHECKF(pSoldier);

	ROTTING_CORPSE *pCorpse;
	INT32	iCount = 0;

	for (INT32 cnt = 0; cnt < giNumRottingCorpse; ++cnt)
	{
		pCorpse = &(gRottingCorpse[cnt]);

		if (pCorpse &&
			pCorpse->fActivated &&
			pCorpse->def.ubType < ROTTING_STAGE2 &&
			pCorpse->def.ubBodyType <= REGFEMALE &&
			pCorpse->def.ubAIWarningValue > 0 &&
			pCorpse->def.bLevel == pSoldier->pathing.bLevel &&
			!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
			PythSpacesAway(sSpot, pCorpse->def.sGridNo) <= sDistance &&
			(!fCheckSight || LOS_Virtual(pSoldier, pCorpse->def.sGridNo, pCorpse->def.bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS)))
		{
			// check whether corpse was one of soldier's allies
			if (pSoldier->bTeam == ENEMY_TEAM && CorpseEnemyTeam(pCorpse) || pSoldier->bTeam == MILITIA_TEAM && CorpseMilitiaTeam(pCorpse) || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral)
			{
				iCount++;
			}
		}
	}

	return iCount;
}

INT32	CountCorpsesInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoomNo, INT8 bLevel)
{
	CHECKF(pSoldier);

	ROTTING_CORPSE *pCorpse;
	INT32	iCount = 0;

	for (INT32 cnt = 0; cnt < giNumRottingCorpse; ++cnt)
	{
		pCorpse = &(gRottingCorpse[cnt]);

		if (pCorpse &&
			pCorpse->fActivated &&
			pCorpse->def.ubType < ROTTING_STAGE2 &&
			pCorpse->def.ubBodyType <= REGFEMALE &&
			pCorpse->def.ubAIWarningValue > 0 &&
			pCorpse->def.bLevel == bLevel &&
			!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
			RoomNo(pCorpse->def.sGridNo) == usRoomNo &&
			(pSoldier->bTeam == ENEMY_TEAM && CorpseEnemyTeam(pCorpse) || pSoldier->bTeam == MILITIA_TEAM && CorpseMilitiaTeam(pCorpse) || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral))
		{
			iCount++;
		}
	}

	return iCount;
}

INT32	CountCorpsesInDirection(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubDirection, INT16 sDistance, BOOLEAN fCheckSight, BOOLEAN fFresh)
{
	INT32			cnt;
	ROTTING_CORPSE *pCorpse;
	UINT16			usNum = 0;

	for (cnt = 0; cnt < giNumRottingCorpse; ++cnt)
	{
		pCorpse = &(gRottingCorpse[cnt]);

		if (pCorpse &&
			pCorpse->fActivated &&
			pCorpse->def.ubType < ROTTING_STAGE2 &&
			pCorpse->def.ubBodyType <= REGFEMALE &&
			(!fFresh || pCorpse->def.ubAIWarningValue > 0) &&
			!TileIsOutOfBounds(pCorpse->def.sGridNo) &&
			PythSpacesAway(sSpot, pCorpse->def.sGridNo) <= sDistance &&
			(pSoldier->bTeam == ENEMY_TEAM && CorpseEnemyTeam(pCorpse) || pSoldier->bTeam == MILITIA_TEAM && CorpseMilitiaTeam(pCorpse) || pSoldier->bTeam != ENEMY_TEAM && pSoldier->bTeam != MILITIA_TEAM) &&
			(!fCheckSight || SoldierToVirtualSoldierLineOfSightTest(pSoldier, pCorpse->def.sGridNo, pCorpse->def.bLevel, ANIM_PRONE, TRUE, CALC_FROM_ALL_DIRS)))
		{
			usNum++;
		}
	}

	return usNum;
}

// count friends under fire or with shock
UINT8 CountTeamUnderAttack( INT8 bTeam, INT32 sGridNo, INT16 sDistance )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;

	// safety check
	if( bTeam >= MAXTEAMS )
		return 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if( pFriend &&
			pFriend->bActive && 
			pFriend->stats.bLife >= OKLIFE &&
			PythSpacesAway( sGridNo, pFriend->sGridNo ) <= sDistance &&
			(pFriend->aiData.bUnderFire || pFriend->aiData.bShock > 0) )
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

BOOLEAN TeamHasActiveRadioOperator(INT8 bTeam)
{
	SOLDIERTYPE * pFriend;

	// safety check
	if( bTeam >= MAXTEAMS )
		return 0;

	// Run through each friendly.
	for ( UINT8 iCounter = gTacticalStatus.Team[ bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ bTeam ].bLastID ; iCounter ++ )
	{
		pFriend = MercPtrs[ iCounter ];

		if( pFriend &&
			pFriend->bActive && 
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->IsUnconscious() &&
			pFriend->CanUseRadio( FALSE ) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN TeamHasScanningRadioOperator(INT8 bTeam)
{
	SOLDIERTYPE * pFriend;

	// safety check
	if( bTeam >= MAXTEAMS )
		return 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			pFriend->CanUseRadio(FALSE) &&
			pFriend->IsScanning())
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN TeamHasListeningRadioOperator(INT8 bTeam)
{
	SOLDIERTYPE * pFriend;

	// safety check
	if (bTeam >= MAXTEAMS)
		return FALSE;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			pFriend->CanUseRadio(FALSE) &&
			pFriend->IsListening())
		{
			return TRUE;
		}
	}

	return FALSE;
}

UINT16 MaxTeamListenValue(INT8 bTeam)
{
	SOLDIERTYPE * pFriend;
	UINT16 usMaxValue = 0;

	// safety check
	if (bTeam >= MAXTEAMS)
		return 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			pFriend->CanUseRadio(FALSE) &&
			pFriend->IsListening())
		{
			usMaxValue = max(usMaxValue, pFriend->usSkillCounter[SOLDIER_COUNTER_LISTEN_TURNS]);
		}
	}

	return usMaxValue;
}

BOOLEAN TeamHasJammingRadioOperator(INT8 bTeam)
{
	SOLDIERTYPE * pFriend;

	// safety check
	if (bTeam >= MAXTEAMS)
		return 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			!pFriend->IsPOW() &&
			!pFriend->bCollapsed &&
			!pFriend->bBreathCollapsed &&
			pFriend->CanUseRadio(FALSE) &&
			pFriend->IsJamming())
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN TeamCanOrderArtilleryStrike(INT8 bTeam)
{
	if (!gSkillTraitValues.fROAllowArtillery)
		return FALSE;

	if (gbWorldSectorZ > 0)
		return FALSE;

	// if we are AI-controlled, we have to wait for our timer to run out
	//if (this->bTeam != gbPlayerNum && this->usSkillCounter[SOLDIER_COUNTER_RADIO_ARTILLERY])
		//return FALSE;

	// check whether we can call artillery from the 4 adjacent sectors
	for (UINT8 i = 0; i < 4; ++i)
	{
		INT16 loopX = gWorldSectorX;
		INT16 loopY = gWorldSectorY;

		if (i == 0)		++loopY;
		else if (i == 1)	++loopX;
		else if (i == 2)	--loopY;
		else if (i == 3)	--loopX;

		if (loopX < 1 || loopX >= MAP_WORLD_X - 1 || loopY < 1 || loopY >= MAP_WORLD_Y - 1)
			continue;

		// as the player team can order artillery from the militia, we have to check that too.
		if (IsValidArtilleryOrderSector(loopX, loopY, gbWorldSectorZ, bTeam) || (bTeam == gbPlayerNum && IsValidArtilleryOrderSector(loopX, loopY, gbWorldSectorZ, MILITIA_TEAM)))
		{
			//*pSectorID = (UINT32)SECTOR(loopX, loopY);
			return TRUE;
		}
	}

	return FALSE;
}

// check if soldier should advance using sight cover 
BOOLEAN UseSightCoverAdvance( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if( !SoldierAI(pSoldier) )
	{
		return FALSE;
	}

	if( pSoldier->bTeam != ENEMY_TEAM )
	{
		return FALSE;
	}

	if (pSoldier->aiData.bOrders == STATIONARY)
	{
		return FALSE;
	}

	if ((pSoldier->aiData.bOrders == SEEKENEMY || pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR) &&
		!pSoldier->aiData.bUnderFire &&
		!GuySawEnemy(pSoldier) && 
		!TeamHighPercentKilled(pSoldier->bTeam) &&
		!AISpecialRole(pSoldier) &&
		pSoldier->ubSoldierClass != SOLDIER_CLASS_ELITE &&
		(SectorAttack(pSoldier->bTeam) || pSoldier->ubSoldierClass == SOLDIER_CLASS_ADMINISTRATOR))
	{
		return FALSE;
	}

	switch( pSoldier->ubSoldierClass )
	{
	case SOLDIER_CLASS_ELITE:
			return TRUE;
		break;
	case SOLDIER_CLASS_ARMY:
		if( pSoldier->aiData.bUnderFire || 
			pSoldier->aiData.bShock > 0 ||
			AICheckDefense(pSoldier) ||
			CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) || 			
			CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE) > 0 ||
			CountCorpses(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE, TRUE) > 0)
		{
			return TRUE;
		}
		break;
	case SOLDIER_CLASS_ADMINISTRATOR:
		if( pSoldier->aiData.bUnderFire || 
			pSoldier->aiData.bShock > 0 ||
			CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) ||
			CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, TACTICAL_RANGE) > 0 ||
			CountCorpses(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE, TRUE) > 0)
		{
			return TRUE;
		}
		break;
	}

	if( pSoldier->aiData.bAttitude == DEFENSIVE || 
		pSoldier->aiData.bAttitude == CUNNINGSOLO ||
		pSoldier->aiData.bAttitude == CUNNINGAID )
	{
		return TRUE;
	}

	if( TeamHighPercentKilled(pSoldier->bTeam) )
	{
		return TRUE;
	}

	return FALSE;
}

// sevenfm: check if suppression is possible (count friends in the fire direction)
BOOLEAN CheckSuppressionDirection( SOLDIERTYPE *pSoldier, INT32 sTargetGridNo, INT8 bTargetLevel )
{
	SOLDIERTYPE * pFriend;
	UINT8 ubShootingDir;
	UINT32 uiLoop;

	CHECKF(pSoldier);
	CHECKF(!TileIsOutOfBounds(sTargetGridNo));

	ubShootingDir = AIDirection(pSoldier->sGridNo, sTargetGridNo);	

	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pFriend = MercSlots[ uiLoop ];

		if( pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			(pFriend->bSide == pSoldier->bSide || CONSIDERED_NEUTRAL(pSoldier, pFriend)) &&
			(pFriend->bVisible == TRUE || !pFriend->aiData.bNeutral) &&
			(pFriend->pathing.bLevel == pSoldier->pathing.bLevel || pFriend->pathing.bLevel == bTargetLevel) &&
			ubShootingDir == AIDirection(pSoldier->sGridNo, pFriend->sGridNo) &&
			PythSpacesAway(pSoldier->sGridNo, pFriend->sGridNo) > 1 &&
			PythSpacesAway( pSoldier->sGridNo, pFriend->sGridNo) < 2 * DAY_VISION_RANGE &&
			gAnimControl[ pFriend->usAnimState ].ubHeight >= gAnimControl[ pSoldier->usAnimState ].ubHeight &&
			gAnimControl[ pFriend->usAnimState ].ubHeight != ANIM_PRONE &&
			!pFriend->IsCowering() &&
			AISoldierToSoldierChanceToGetThrough(pSoldier, pFriend) > 25)
			//LOS(pSoldier->sGridNo, pSoldier->pathing.bLevel, pFriend->sGridNo, pFriend->pathing.bLevel, NO_DISTANCE_LIMIT))
		{
			return FALSE;
		}
	}

	return TRUE;
}

UINT8 AISuppressionEffectiveness(void)
{
	if (gGameExternalOptions.ubMaxSuppressionShock == 0 || gGameExternalOptions.usSuppressionShockEffect == 0)
		return 0;

	return min(255, gGameExternalOptions.sSuppressionEffectiveness);
}

INT16 AICalcActionPoints( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);
	
	INT16 sAP;
	INT16 sPoints;
	INT16 sMinPoints;
	INT16 sCarriedPoints;

	sPoints = pSoldier->CalcActionPoints();
	sMinPoints = min(sPoints, pSoldier->CalcMinActionPoints(sPoints));
	sCarriedPoints = min(pSoldier->bActionPoints, APBPConstants[MAX_AP_CARRIED]);

	sAP = max(sCarriedPoints + sPoints, sMinPoints);
	sAP = max(sAP, APBPConstants[AP_MIN_LIMIT]);

	return sAP;
}

BOOLEAN AICheckNVG( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	return pSoldier->WearsNVG();

	/*if( Item[pSoldier->inv[HEAD1POS].usItem].nightvisionrangebonus > 0 ||
		Item[pSoldier->inv[HEAD1POS].usItem].cavevisionrangebonus > 0 ||
		Item[pSoldier->inv[HEAD2POS].usItem].nightvisionrangebonus > 0 ||
		Item[pSoldier->inv[HEAD2POS].usItem].cavevisionrangebonus > 0 )
	{
		return TRUE;
	}

	return FALSE;*/
}

INT8 RainIntensity()
{
	if (gbWorldSectorZ > 0)
	{
		return 0;
	}

	if (guiEnvWeather & (WEATHER_FORECAST_SHOWERS | WEATHER_FORECAST_THUNDERSHOWERS))
	{
		return gbCurrentRainIntensity;
	}

	return 0;
}

INT16 VisionRange( void )
{
	INT16 sDist = MaxNormalDistanceVisible();
	INT8 bLightLevel = GetTimeOfDayAmbientLightLevel();
	
	sDist = sDist * gGameExternalOptions.ubBrightnessVisionMod[ bLightLevel ] / 100;

	return sDist;
}

INT16 DayVisionRange( void )
{
	INT16 sDist = MaxNormalDistanceVisible();
	INT8 bLightLevel = NORMAL_LIGHTLEVEL_DAY;

	sDist = sDist * gGameExternalOptions.ubBrightnessVisionMod[ bLightLevel ] / 100;

	return sDist;
}

INT16 NightVisionRange( void )
{
	INT16 sDist = MaxNormalDistanceVisible();
	INT8 bLightLevel = NORMAL_LIGHTLEVEL_NIGHT;

	sDist = sDist * gGameExternalOptions.ubBrightnessVisionMod[ bLightLevel ] / 100;

	return sDist;
}

BOOLEAN AICheckUnderAttack(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fCheckSuccessfull)
{
	CHECKF(pSoldier);

	if( TileIsOutOfBounds(sSpot) )
	{
		sSpot = pSoldier->sGridNo;
	}

	if( pSoldier->aiData.bUnderFire &&
		pSoldier->ubPreviousAttackerID != NOBODY &&
		MercPtrs[pSoldier->ubPreviousAttackerID]->exists() &&
		MercPtrs[pSoldier->ubPreviousAttackerID]->stats.bLife >= OKLIFE &&
		MercPtrs[pSoldier->ubPreviousAttackerID]->sLastTarget == sSpot &&
		(!fCheckSuccessfull || MercPtrs[pSoldier->ubPreviousAttackerID]->LastAttackHit()))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckSpotUnderAttack(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fCheckSuccessfull, BOOLEAN fAdjacent)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	if (TileIsOutOfBounds(sSpot))
	{
		sSpot = pSoldier->sGridNo;
	}

	// loop through all publicly known enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// check public knowledge
		if (PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == NOT_HEARD_OR_SEEN && PersonalKnowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		if (!TileIsOutOfBounds(pOpponent->sLastTarget) &&
			SpacesAway(sSpot, pOpponent->sLastTarget) < 2 &&
			//pOpponent->sLastTarget == sSpot && 
			(!fCheckSuccessfull || pOpponent->aiData.bLastAttackHit))
		{
			return TRUE;
		}
	}

	// possibly check adjacent spots
	if (fAdjacent)
	{
		INT32 sNewSpot;

		// check tiles around merc
		for (UINT8 ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
		{
			sNewSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

			if (sNewSpot != sSpot && gubWorldMovementCosts[sNewSpot][ubDirection][pSoldier->pathing.bLevel] < TRAVELCOST_BLOCKED)
			{
				return AICheckSpotUnderAttack(pSoldier, sSpot, fCheckSuccessfull, FALSE);
			}
		}
	}

	return FALSE;
}

// danger percent based on distance to closest smoke effect
UINT8 RedSmokeDanger( INT32 sGridNo, INT8 bLevel )
{
	UINT32	uiCnt;
	INT32	sDist;
	INT32	sClosestDist;
	INT32	sMaxDist = min(gSkillTraitValues.usVOMortarRadius, DAY_VISION_RANGE);
	INT32	sClosestSmoke = NOWHERE;
	UINT8	ubDangerPercent = 0;

	if( TileIsOutOfBounds(sGridNo) )
	{
		return 0;
	}

	if ( !gSkillTraitValues.fROAllowArtillery )
	{
		return 0;
	}

	// no artillery strike danger underground
	if ( gbWorldSectorZ > 0 )
	{
		return 0;
	}

	// check if artillery strike was ordered by any team
	if( !CheckArtilleryStrike() )
	{
		return 0;
	}

	// no danger when in a building
	if (bLevel == 0 && CheckRoof(sGridNo))
	{
		return 0;
	}

	// deep water should be safe
	if (DeepWater(sGridNo, bLevel))
	{
		return 0;
	}

	// try to hide in water from artillery strike
	if (Water(sGridNo, bLevel) && !FindNotWaterNearby(sGridNo, bLevel))
	{
		return 0;
	}

	// no danger when in dense terrain
	if(bLevel == 0 && TerrainDensity(sGridNo, bLevel, 2, FALSE) >= 75)
	{
		return 0;
	}

	//loop through all red smoke effects and find closest
	for( uiCnt=0; uiCnt < guiNumSmokeEffects; uiCnt++)
	{
		if( gSmokeEffectData[ uiCnt ].fAllocated &&
			gSmokeEffectData[ uiCnt ].bType == SIGNAL_SMOKE_EFFECT &&
			!TileIsOutOfBounds(gSmokeEffectData[ uiCnt ].sGridNo) )
		{
			sDist = PythSpacesAway(gSmokeEffectData[ uiCnt ].sGridNo, sGridNo);

			if( sClosestSmoke == NOWHERE || sDist < sClosestDist )
			{
				sClosestDist = sDist;
				sClosestSmoke = gSmokeEffectData[ uiCnt ].sGridNo;
			}
		}
	}

	// if we found red smoke, calculate danger percent based on distance
	// 0% at DAY_VISION_RANGE/2, 100% at zero range
	if( sClosestSmoke != NOWHERE )
	{
		ubDangerPercent = 100 * (sMaxDist - min(sMaxDist, sClosestDist)) / sMaxDist;		
	}

	return ubDangerPercent;
}

BOOLEAN	RedSmokeOnMap(void)
{
	UINT32	uiCnt;

	//loop through all red smoke effects
	for (uiCnt = 0; uiCnt < guiNumSmokeEffects; uiCnt++)
	{
		if (gSmokeEffectData[uiCnt].fAllocated &&
			gSmokeEffectData[uiCnt].bType == SIGNAL_SMOKE_EFFECT &&
			!TileIsOutOfBounds(gSmokeEffectData[uiCnt].sGridNo))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN FindClosestVisibleSmoke(SOLDIERTYPE *pSoldier, INT32 &sSpot, INT8 &bLevel, BOOLEAN fOnlyGas, INT32 sMaxDistance)
{
	CHECKF(pSoldier);

	UINT32	uiCnt;
	INT32	sDist, sClosestDist;
	INT32	sCheckSpot;
	INT8	bCheckLevel;

	sSpot = NOWHERE;
	bLevel = 0;

	//loop through all smoke effects and find closest visible
	for( uiCnt=0; uiCnt < guiNumSmokeEffects; uiCnt++)
	{
		if (gSmokeEffectData[uiCnt].fAllocated &&
			!TileIsOutOfBounds(gSmokeEffectData[uiCnt].sGridNo) &&
			(sMaxDistance == -1 || PythSpacesAway(gSmokeEffectData[uiCnt].sGridNo, pSoldier->sGridNo) < sMaxDistance))
		{
			// ignore smoke if not dangerous
			if (fOnlyGas && 
				gSmokeEffectData[ uiCnt ].bType != TEARGAS_SMOKE_EFFECT &&
				gSmokeEffectData[ uiCnt ].bType != MUSTARDGAS_SMOKE_EFFECT &&
				gSmokeEffectData[uiCnt].bType != CREATURE_SMOKE_EFFECT &&
				gSmokeEffectData[uiCnt].bType != SIGNAL_SMOKE_EFFECT)
			{
				continue;
			}

			sCheckSpot = gSmokeEffectData[ uiCnt ].sGridNo;

			if( gSmokeEffectData[ uiCnt ].bFlags & SMOKE_EFFECT_ON_ROOF )
				bCheckLevel = 1;
			else
				bCheckLevel = 0;

			sDist = PythSpacesAway(sCheckSpot, pSoldier->sGridNo);

			if( sDist < DAY_VISION_RANGE &&
				LOS_Virtual(pSoldier, sCheckSpot, bCheckLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS ) &&
				(sSpot == NOWHERE || sDist < sClosestDist) )
			{
				sClosestDist = sDist;
				sSpot = sCheckSpot;
				bLevel = bCheckLevel;
			}
		}
	}

	if( !TileIsOutOfBounds(sSpot) )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN FindClosestGas(SOLDIERTYPE *pSoldier, INT32 &sSpot, INT8 &bLevel, INT32 sMaxDistance)
{
	CHECKF(pSoldier);

	UINT32	uiCnt;
	INT32	sDist, sClosestDist;
	INT32	sCheckSpot;
	INT8	bCheckLevel;

	sSpot = NOWHERE;
	bLevel = 0;

	//loop through all smoke effects and find closest visible
	for (uiCnt = 0; uiCnt < guiNumSmokeEffects; uiCnt++)
	{
		if (gSmokeEffectData[uiCnt].fAllocated &&
			!TileIsOutOfBounds(gSmokeEffectData[uiCnt].sGridNo) &&
			(sMaxDistance == -1 || PythSpacesAway(gSmokeEffectData[uiCnt].sGridNo, pSoldier->sGridNo) < sMaxDistance))
		{
			// ignore smoke if not dangerous
			if (gSmokeEffectData[uiCnt].bType != TEARGAS_SMOKE_EFFECT &&
				gSmokeEffectData[uiCnt].bType != MUSTARDGAS_SMOKE_EFFECT &&
				gSmokeEffectData[uiCnt].bType != CREATURE_SMOKE_EFFECT &&
				gSmokeEffectData[uiCnt].bType != SIGNAL_SMOKE_EFFECT)
			{
				continue;
			}

			sCheckSpot = gSmokeEffectData[uiCnt].sGridNo;

			if (gSmokeEffectData[uiCnt].bFlags & SMOKE_EFFECT_ON_ROOF)
				bCheckLevel = 1;
			else
				bCheckLevel = 0;

			sDist = PythSpacesAway(sCheckSpot, pSoldier->sGridNo);

			if (sDist < DAY_VISION_RANGE &&
				//LOS_Virtual(pSoldier, sCheckSpot, bCheckLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) &&
				(sSpot == NOWHERE || sDist < sClosestDist))
			{
				sClosestDist = sDist;
				sSpot = sCheckSpot;
				bLevel = bCheckLevel;
			}
		}
	}

	if (!TileIsOutOfBounds(sSpot))
	{
		return TRUE;
	}

	return FALSE;
}

UINT16 CountVisibleSmokeEffects(SOLDIERTYPE *pSoldier, INT16 sDistance, INT8 bType)
{
	CHECKF(pSoldier);

	UINT32	uiCnt;
	INT32	sDist;
	INT32	sCheckSpot;
	INT8	bCheckLevel;
	UINT16	usNum = 0;

	//loop through all smoke effects and find closest visible
	for (uiCnt = 0; uiCnt < guiNumSmokeEffects; uiCnt++)
	{
		if (gSmokeEffectData[uiCnt].fAllocated &&
			!TileIsOutOfBounds(gSmokeEffectData[uiCnt].sGridNo))
		{
			// check smoke type
			if (gSmokeEffectData[uiCnt].bType != bType)
			{
				continue;
			}

			sCheckSpot = gSmokeEffectData[uiCnt].sGridNo;

			if (gSmokeEffectData[uiCnt].bFlags & SMOKE_EFFECT_ON_ROOF)
				bCheckLevel = 1;
			else
				bCheckLevel = 0;

			sDist = PythSpacesAway(sCheckSpot, pSoldier->sGridNo);

			if (sDist <= sDistance &&
				LOS_Virtual(pSoldier, sCheckSpot, bCheckLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS))
			{
				usNum++;
			}
		}
	}

	return usNum;
}

BOOLEAN CheckRoof( INT32 sGridNo )
{
	if ( FindStructure( sGridNo, STRUCTURE_ROOF ) != NULL )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN CheckWindow(INT32 sSpot, UINT8 ubDirection, BOOLEAN fAllowClosed)
{
	CHECKF(!TileIsOutOfBounds(sSpot));

	// find window spot
	INT32 sWindowSpot = sSpot;
	if (ubDirection == NORTH || ubDirection == WEST)
		sWindowSpot = NewGridNo(sSpot, (UINT16)DirectionInc(ubDirection));

	//if (IsJumpableWindowPresentAtGridNo(sWindowSpot, ubDirection, gGameExternalOptions.fCanJumpThroughClosedWindows))
	if (IsJumpableWindowPresentAtGridNo(sWindowSpot, ubDirection, fAllowClosed))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN CheckClosedWindow(INT32 sSpot, UINT8 ubDirection)
{
	CHECKF(!TileIsOutOfBounds(sSpot));

	// find window spot
	INT32 sWindowSpot = sSpot;
	if (ubDirection == NORTH || ubDirection == WEST)
		sWindowSpot = NewGridNo(sSpot, (UINT16)DirectionInc(ubDirection));

	// check closed window
	if (IsJumpableWindowPresentAtGridNo(sWindowSpot, ubDirection, TRUE) && !IsJumpableWindowPresentAtGridNo(sWindowSpot, ubDirection, FALSE))
	{
		STRUCTURE * pStructure = FindStructure(sWindowSpot, STRUCTURE_WALLNWINDOW);
		if (pStructure && !(pStructure->fFlags & STRUCTURE_OPEN))
		{
			return TRUE;
		}
	}

	return FALSE;
}

// check if artillery strike was ordered by any team
BOOLEAN CheckArtilleryStrike( void )
{
	UINT32	uiBombIndex;
	OBJECTTYPE *pObj;

	// search all bombs
	for (uiBombIndex = 0; uiBombIndex < guiNumWorldBombs; uiBombIndex++)
	{
		if (gWorldBombs[ uiBombIndex ].fExists &&
			gWorldItems[ gWorldBombs[ uiBombIndex ].iItemIndex ].usFlags & WORLD_ITEM_ARMED_BOMB )
		{
			pObj = &( gWorldItems[ gWorldBombs[uiBombIndex].iItemIndex ].object );

			if( pObj && pObj->exists() && (*pObj)[0]->data.ubWireNetworkFlag & ANY_ARTILLERY_FLAG )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

BOOLEAN AICheckIsBandaged( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if( pSoldier->ubServiceCount > 0 )
	{
		return TRUE;
	}

	// check closest medic
	// disabled because of bug
	/*UINT8 ubMedic = GetClosestMedicSoldierID( pSoldier, 1, pSoldier->bTeam );
	INT32 sNextGridNo;
	if( ubMedic != NOBODY && MercPtrs[ubMedic]->aiData.bLastAction == AI_ACTION_DOCTOR && 
		(MercPtrs[ubMedic]->usAnimState == GIVING_AID) || (MercPtrs[ubMedic]->usAnimState == GIVING_AID_PRN ) )
	{
		sNextGridNo = NewGridNo( MercPtrs[ubMedic]->sGridNo, DirectionInc( MercPtrs[ubMedic]->ubDirection ) );
		if( pSoldier->sGridNo == sNextGridNo )
		{
			return TRUE;
		}		
	}*/

	return FALSE;
}

BOOLEAN AICheckHasWeaponOfType( SOLDIERTYPE *pSoldier, UINT8 ubWeaponType )
{
	CHECKF(pSoldier);

	INT8 invsize = (INT8)pSoldier->inv.size( );
	for ( INT8 bLoop = 0; bLoop < invsize; ++bLoop )
	{
		if( pSoldier->inv[bLoop].exists( ) && 
			Item[pSoldier->inv[bLoop].usItem].usItemClass == IC_GUN && 
			Weapon[Item[pSoldier->inv[bLoop].usItem].ubClassIndex].ubWeaponType == ubWeaponType)
		{
			return TRUE;
		}
	}

	return FALSE;

}

BOOLEAN AICheckHasFlamethrowerInHands(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	//INT8 invsize = (INT8)pSoldier->inv.size();
	for (INT8 bLoop = HANDPOS; bLoop <= SECONDHANDPOS; bLoop++)
	{
		if (pSoldier->inv[bLoop].exists() &&
			Item[pSoldier->inv[bLoop].usItem].usItemClass == IC_GUN &&
			(pSoldier->inv[bLoop].usItem == FLAMETHROWER || AmmoTypes[pSoldier->inv[bLoop][0]->data.gun.ubGunAmmoType].ammoflag & AMMOFLAG_FLAME))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN AICheckSuccessfulAttack( SOLDIERTYPE *pSoldier, BOOLEAN fGroup )
{
	CHECKF(pSoldier);

	if (pSoldier->LastAttackHit() && pSoldier->sLastTarget != NOWHERE || pSoldier->usSoldierFlagMask2 & SOLDIER_SUCCESSFUL_ATTACK)
	{
		return TRUE;
	}
	if (pSoldier->LastTargetCollapsed() || pSoldier->LastTargetSuppressed())
	{
		return TRUE;
	}
	if( fGroup && CountFriendsLastAttackHit(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4) )
	{
		return TRUE;
	}

	INT32 sClosestOpponent = ClosestKnownOpponent( pSoldier, NULL, NULL );
	if( fGroup && 
		!TileIsOutOfBounds(sClosestOpponent) && 
		CountFriendsLastAttackHit(pSoldier, sClosestOpponent, TACTICAL_RANGE) )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckHasGun( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if( FindAIUsableObjClass( pSoldier, IC_GUN ) != NO_SLOT )
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckShortWeaponRange( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if (!AICheckHasGun(pSoldier))
	{
		return TRUE;
	}

	// less aggressive squadleader/radio operator etc
	/*if (AISpecialRole(pSoldier))
	{
		return FALSE;
	}*/

	if (AIGunRange(pSoldier) < TACTICAL_RANGE / 2)
	{
		return TRUE;
	}

	return FALSE;
}

INT8 AIEstimateInterruptLevel( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	if (!pSoldier->CanInterrupt())
		return 0;

	INT8 bLevel = CalcInterruptDuelPts(pSoldier, NOBODY, FALSE, FALSE);

	if (bLevel == NO_INTERRUPT)
		bLevel = 0;

	return bLevel;
}

INT8 FindMaxEnemyInterruptLevel( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT8 blevel, UINT8 ubDistance )
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT8		bMaxInterruptLevel = 0;
	INT8		bInterruptLevel;

	INT32		sThreatLoc;
	INT8		iThreatLevel;

	UINT8 ubNum = 0;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if(pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// check personal/public knowledge
		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		iThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// if for some reason known location is bad - skip
		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		// check distance
		if( PythSpacesAway(sThreatLoc, sGridNo ) > ubDistance )
		{
			continue;
		}

		// check level
		if( iThreatLevel != blevel )
		{
			continue;
		}

		bInterruptLevel = AIEstimateInterruptLevel(pOpponent);
		if( bInterruptLevel > bMaxInterruptLevel )
		{
			bMaxInterruptLevel = bInterruptLevel;
		}
	}

	return bMaxInterruptLevel;
}

// check if soldier is standing at any public gridno
/*BOOLEAN SoldierIsStandingAtPublicKnownSpot( SOLDIERTYPE *pSoldier )
{
	UINT32		uiLoop;
	SOLDIERTYPE *pEnemy;
	INT8		*pbPublOL;
	BOOLEAN		fFound = FALSE;
	INT8		bKnowledge;
	INT32		sSpot;
	INT8		bLevel;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pEnemy = MercSlots[uiLoop];

		// for every active, living soldier of ANOTHER team
		if( pEnemy && 
			pEnemy->stats.bLife && 
			pEnemy->bTeam != pSoldier->bTeam &&
			ValidOpponent(pSoldier, pEnemy) )
		{
			bKnowledge = PublicKnowledge(pSoldier->bTeam, pEnemy->ubID);

			// check for oldest knowledge
			if( bKnowledge != HEARD_3_TURNS_AGO )
			{
				continue;
			}

			// check gridno and level
			sSpot = KnownPublicLocation(pSoldier->bTeam, pEnemy->ubID);
			bLevel = KnownPublicLevel(pSoldier->bTeam, pEnemy->ubID);

			if( pSoldier->sGridNo != sSpot || 
				pSoldier->pathing.bLevel != bLevel )
			{
				continue;
			}
			
			DebugAI( AI_MSG_INFO, pSoldier, String("standing at obsolete HEARD_3_TURNS_AGO public spot: [%d] knowledge %d spot %d level %d", pEnemy->ubID, bKnowledge, sSpot, bLevel));

			// soldier is standing at public gridno and knowledge is HEARD_3_TURNS_AGO
			// remove public information
			DebugAI( AI_MSG_INFO, pSoldier, String("remove public information for [%d]", pEnemy->ubID));
			UpdatePublic(pSoldier->bTeam, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);

			fFound = TRUE;
		}
	}

	return fFound;
}*/

UINT8	CountNearbyFences(INT32 sSpot, INT8 bLevel)
{
	INT32	sTempGridNo;
	UINT8	ubDirection;
	UINT8	ubMovementCost;
	UINT8	ubFences = 0;

	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo(sSpot, DirectionInc(ubDirection));

		if (sTempGridNo != sSpot)
		{
			ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

			if (ubMovementCost == TRAVELCOST_FENCE)
			{
				ubFences++;
			}
		}
	}

	return ubFences;
}

BOOLEAN	FindNearbyExplosiveStructure(INT32 sSpot, INT8 bLevel)
{
	INT32	sTempGridNo;
	UINT8	ubDirection;

	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo(sSpot, DirectionInc(ubDirection));

		if (sTempGridNo != sSpot)
		{
			if (FindStructFlag(sTempGridNo, bLevel, STRUCTURE_EXPLOSIVE))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

UINT8 TerrainDensity( INT32 sSpot, INT8 bLevel, UINT8 ubDistance, BOOLEAN fGrass )
{
	if( TileIsOutOfBounds(sSpot) )
		return 0;

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sCheckSpot = NOWHERE;
	INT32	sCountSpots = 0;
	INT32	sCountObstacles = 0;
	UINT16	usRoom1, usRoom2;

	STRUCTURE	*pCurrent;
	INT16	sDesiredLevel;

	// determine maximum horizontal limits
	sMaxLeft  = min( ubDistance, (sSpot % MAXCOL));
	sMaxRight = min( ubDistance, MAXCOL - ((sSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp   = min( ubDistance, (sSpot / MAXROW));
	sMaxDown = min( ubDistance, MAXROW - ((sSpot / MAXROW) + 1));

	// count obstacles
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sCheckSpot = sSpot + sXOffset + (MAXCOL * sYOffset);

			if ( TileIsOutOfBounds(sCheckSpot) )
			{
				continue;
			}

			if (InARoom(sSpot, &usRoom1) != InARoom(sCheckSpot, &usRoom2) || usRoom1 != usRoom2)
			{
				continue;
			}

			/*if( sSpot != sCheckSpot && 
				!LOS(sSpot, bLevel, sCheckSpot, bLevel, NO_DISTANCE_LIMIT) )
			{
				//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"LOS skip %d", sCheckSpot);
				continue;
			}*/

			sCountSpots++;

			if( !IsLocationSittableExcludingPeople( sCheckSpot, bLevel ) )
			{
				//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"add obstacle %d", sCheckSpot);
				sCountObstacles++;
			}

			if( fGrass && IsLocationSittableExcludingPeople( sCheckSpot, bLevel ) )
			{
				pCurrent = gpWorldLevelData[ sCheckSpot ].pStructureHead;

				if( bLevel > 0 )
					sDesiredLevel = STRUCTURE_ON_ROOF;
				else
					sDesiredLevel = STRUCTURE_ON_GROUND;

				if( pCurrent != NULL &&
					pCurrent->sCubeOffset == sDesiredLevel &&
					pCurrent->pDBStructureRef->pDBStructure->ubArmour == 4 )	// light vegetation
				{
					sCountObstacles++;
				}				
			}
		}
	}

	//ScreenMsg(FONT_LTGREEN, MSG_INTERFACE, L"total obstacles/spots %d/%d", sCountObstacles, sCountSpots);

	if( sCountSpots > 0 )
		return 100 * sCountObstacles / sCountSpots;
	else
		return 0;
}

UINT8	TerrainJungleDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance)
{
	if (TileIsOutOfBounds(sSpot))
		return 0;

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sCheckSpot = NOWHERE;
	INT32	sCountSpots = 0;
	INT32	sCamoSpots = 0;
	UINT16	usRoom1, usRoom2;

	// determine maximum horizontal limits
	sMaxLeft = min(ubDistance, (sSpot % MAXCOL));
	sMaxRight = min(ubDistance, MAXCOL - ((sSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp = min(ubDistance, (sSpot / MAXROW));
	sMaxDown = min(ubDistance, MAXROW - ((sSpot / MAXROW) + 1));

	// count camo spots
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sCheckSpot = sSpot + sXOffset + (MAXCOL * sYOffset);

			if (TileIsOutOfBounds(sCheckSpot))
			{
				continue;
			}

			if (InARoom(sSpot, &usRoom1) != InARoom(sCheckSpot, &usRoom2) || usRoom1 != usRoom2)
			{
				continue;
			}

			/*if (sSpot != sCheckSpot &&
				!LOS(sSpot, bLevel, sCheckSpot, bLevel, NO_DISTANCE_LIMIT))
			{
				continue;
			}*/

			if (!IsLocationSittableExcludingPeople(sCheckSpot, bLevel))
			{
				continue;
			}

			sCountSpots++;

			if (TerrainJungle(sCheckSpot, bLevel))
			{
				sCamoSpots++;
			}
		}
	}

	if (sCountSpots > 0)
		return 100 * sCamoSpots / sCountSpots;
	else
		return 0;
}

UINT8	TerrainDesertDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance)
{
	if (TileIsOutOfBounds(sSpot))
		return 0;

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sCheckSpot = NOWHERE;
	INT32	sCountSpots = 0;
	INT32	sCamoSpots = 0;
	UINT16	usRoom1, usRoom2;

	// determine maximum horizontal limits
	sMaxLeft = min(ubDistance, (sSpot % MAXCOL));
	sMaxRight = min(ubDistance, MAXCOL - ((sSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp = min(ubDistance, (sSpot / MAXROW));
	sMaxDown = min(ubDistance, MAXROW - ((sSpot / MAXROW) + 1));

	// count camo spots
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sCheckSpot = sSpot + sXOffset + (MAXCOL * sYOffset);

			if (TileIsOutOfBounds(sCheckSpot))
			{
				continue;
			}

			if (InARoom(sSpot, &usRoom1) != InARoom(sCheckSpot, &usRoom2) || usRoom1 != usRoom2)
			{
				continue;
			}

			/*if (sSpot != sCheckSpot &&
				!LOS(sSpot, bLevel, sCheckSpot, bLevel, NO_DISTANCE_LIMIT))
			{
				continue;
			}*/

			if (!IsLocationSittableExcludingPeople(sCheckSpot, bLevel))
			{
				continue;
			}

			sCountSpots++;

			if (TerrainDesert(sCheckSpot, bLevel))
			{
				sCamoSpots++;
			}
		}
	}

	if (sCountSpots > 0)
		return 100 * sCamoSpots / sCountSpots;
	else
		return 0;
}

UINT8	TerrainUrbanDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance)
{
	if (TileIsOutOfBounds(sSpot))
		return 0;

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sCheckSpot = NOWHERE;
	INT32	sCountSpots = 0;
	INT32	sCamoSpots = 0;
	UINT16	usRoom1, usRoom2;

	// determine maximum horizontal limits
	sMaxLeft = min(ubDistance, (sSpot % MAXCOL));
	sMaxRight = min(ubDistance, MAXCOL - ((sSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp = min(ubDistance, (sSpot / MAXROW));
	sMaxDown = min(ubDistance, MAXROW - ((sSpot / MAXROW) + 1));

	// count camo spots
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sCheckSpot = sSpot + sXOffset + (MAXCOL * sYOffset);

			if (TileIsOutOfBounds(sCheckSpot))
			{
				continue;
			}

			if (InARoom(sSpot, &usRoom1) != InARoom(sCheckSpot, &usRoom2) || usRoom1 != usRoom2)
			{
				continue;
			}

			/*if (sSpot != sCheckSpot &&
				!LOS(sSpot, bLevel, sCheckSpot, bLevel, NO_DISTANCE_LIMIT))
			{
				continue;
			}*/

			if (!IsLocationSittableExcludingPeople(sCheckSpot, bLevel))
			{
				continue;
			}

			sCountSpots++;

			if (TerrainUrban(sCheckSpot, bLevel))
			{
				sCamoSpots++;
			}
		}
	}

	if (sCountSpots > 0)
		return 100 * sCamoSpots / sCountSpots;
	else
		return 0;
}

UINT8	TerrainSnowDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance)
{
	if (TileIsOutOfBounds(sSpot))
		return 0;

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sCheckSpot = NOWHERE;
	INT32	sCountSpots = 0;
	INT32	sCamoSpots = 0;
	UINT16	usRoom1, usRoom2;

	// determine maximum horizontal limits
	sMaxLeft = min(ubDistance, (sSpot % MAXCOL));
	sMaxRight = min(ubDistance, MAXCOL - ((sSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp = min(ubDistance, (sSpot / MAXROW));
	sMaxDown = min(ubDistance, MAXROW - ((sSpot / MAXROW) + 1));

	// count camo spots
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sCheckSpot = sSpot + sXOffset + (MAXCOL * sYOffset);

			if (TileIsOutOfBounds(sCheckSpot))
			{
				continue;
			}

			if (InARoom(sSpot, &usRoom1) != InARoom(sCheckSpot, &usRoom2) || usRoom1 != usRoom2)
			{
				continue;
			}

			/*if (sSpot != sCheckSpot &&
				!LOS(sSpot, bLevel, sCheckSpot, bLevel, NO_DISTANCE_LIMIT))
			{
				continue;
			}*/

			if (!IsLocationSittableExcludingPeople(sCheckSpot, bLevel))
			{
				continue;
			}

			sCountSpots++;

			// no snow terrain type
			if (TerrainSnow(sCheckSpot, bLevel))
			{
				sCamoSpots++;
			}
		}
	}

	if (sCountSpots > 0)
		return 100 * sCamoSpots / sCountSpots;
	else
		return 0;
}

UINT8	TerrainDarknessDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance)
{
	if (TileIsOutOfBounds(sSpot))
		return 0;

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sCheckSpot = NOWHERE;
	INT32	sCountSpots = 0;
	INT32	sDarkSpots = 0;
	UINT16	usRoom1, usRoom2;

	// determine maximum horizontal limits
	sMaxLeft = min(ubDistance, (sSpot % MAXCOL));
	sMaxRight = min(ubDistance, MAXCOL - ((sSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp = min(ubDistance, (sSpot / MAXROW));
	sMaxDown = min(ubDistance, MAXROW - ((sSpot / MAXROW) + 1));

	// count dark spots
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sCheckSpot = sSpot + sXOffset + (MAXCOL * sYOffset);

			if (TileIsOutOfBounds(sCheckSpot))
			{
				continue;
			}

			if (InARoom(sSpot, &usRoom1) != InARoom(sCheckSpot, &usRoom2) || usRoom1 != usRoom2)
			{
				continue;
			}

			/*if (sSpot != sCheckSpot &&
				!LOS(sSpot, bLevel, sCheckSpot, bLevel, NO_DISTANCE_LIMIT))
			{
				continue;
			}*/

			if (!IsLocationSittableExcludingPeople(sCheckSpot, bLevel))
			{
				continue;
			}

			sCountSpots++;

			// check night light level
			if(TerrainDark(sCheckSpot, bLevel))
			{
				sDarkSpots++;
			}
		}
	}

	if (sCountSpots > 0)
		return 100 * sDarkSpots / sCountSpots;
	else
		return 0;
}

BOOLEAN	TerrainJungle(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	UINT8 ubTerrainType = GetTerrainTypeForGrid(sSpot, bLevel);
	if (ubTerrainType == LOW_GRASS || ubTerrainType == HIGH_GRASS || ubTerrainType == FLAT_GROUND)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN	TerrainDesert(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	UINT8 ubTerrainType = GetTerrainTypeForGrid(sSpot, bLevel);
	if (ubTerrainType == DIRT_ROAD || ubTerrainType == TRAIN_TRACKS || ubTerrainType == FLAT_GROUND)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN TerrainUrban(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	UINT8 ubTerrainType = GetTerrainTypeForGrid(sSpot, bLevel);
	if (ubTerrainType == FLAT_FLOOR || ubTerrainType == PAVED_ROAD)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN TerrainSnow(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	UINT8 ubTerrainType = GetTerrainTypeForGrid(sSpot, bLevel);
	return FALSE;
}

BOOLEAN TerrainDark(INT32 sSpot, INT8 bLevel)
{
	if (TileIsOutOfBounds(sSpot))
		return FALSE;

	UINT8 ubLightLevel = LightTrueLevel(sSpot, bLevel);

	if (ubLightLevel >= NORMAL_LIGHTLEVEL_NIGHT - 3)
	{
		return TRUE;
	}

	return FALSE;
}

void AddNoiseAtRandomSightCoverSpot(SOLDIERTYPE *pSoldier, INT32 sCheckSpot, INT8 bCheckLevel, BOOLEAN fTeamSight, BOOLEAN fTeamNoise)
{
	if( !pSoldier )
		return;

	INT32	iSearchRange;
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sGridNo;
	UINT16	usNum = 0;
	INT32	CheckSpotList[MAX_SPOT_LIST_SIZE];
	INT32	sNoiseSpot;

	INT32	sRealSpot;
	INT8	bRealLevel;

	// check if soldier already has noise to check
	if (!TileIsOutOfBounds(pSoldier->aiData.sNoiseGridno))
	{
		return;
	}

	// store real location
	sRealSpot = pSoldier->sGridNo;
	bRealLevel = pSoldier->pathing.bLevel;	

	// set max search range
	iSearchRange = DAY_VISION_RANGE;

	if(!AllowComplexAI(pSoldier))
	{
		iSearchRange = iSearchRange / 2;
	}

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (sCheckSpot % MAXCOL));
	sMaxRight = min( iSearchRange, MAXCOL - ((sCheckSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (sCheckSpot / MAXROW));
	sMaxDown = min( iSearchRange, MAXROW - ((sCheckSpot / MAXROW) + 1));	

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = sCheckSpot + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	// fake soldier's location to make path check
	pSoldier->sGridNo = sCheckSpot;
	pSoldier->pathing.bLevel = bCheckLevel;

	gubNPCAPBudget = APBPConstants[AP_MAXIMUM];
	gubNPCDistLimit = (UINT8)iSearchRange;

	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, DetermineMovementMode( pSoldier, AI_ACTION_SEEK_OPPONENT ), COPYREACHABLE, 0 );

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

	// restore soldier's location
	pSoldier->sGridNo = sRealSpot;
	pSoldier->pathing.bLevel = bRealLevel;

	// make a list of reachable spots that nobody sees
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown && usNum < MAX_SPOT_LIST_SIZE; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight && usNum < MAX_SPOT_LIST_SIZE; sXOffset++)
		{
			sGridNo = sCheckSpot + sXOffset + (MAXCOL * sYOffset);

			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if( sGridNo == pSoldier->sGridNo )
			{
				continue;
			}

			if ( Water(sGridNo, bCheckLevel) )
			{
				continue;
			}			

			// exclude locations with tear/mustard gas
			if ( InGas( pSoldier, sGridNo ) )
			{
				continue;
			}

			// check that we can see spot
			if( LOS_Virtual(pSoldier, sGridNo, bCheckLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
			{
				continue;
			}

			// check that our team sees spot
			if (fTeamSight && TeamSeesGridNo(pSoldier->bTeam, sGridNo, bCheckLevel, ANIM_PRONE))
			{
				continue;
			}

			// add spot to list
			CheckSpotList[usNum++] = sGridNo;
		}
	}

	if( usNum == 0 )
	{
		return;
	}

	sNoiseSpot = CheckSpotList[ Random(usNum) ];
	DebugAI( AI_MSG_INFO, pSoldier, String("[%d] set personal noise spot %d level %d volume %d", pSoldier->ubID, sNoiseSpot, bCheckLevel, MAX_MISC_NOISE_DURATION));

	pSoldier->aiData.sNoiseGridno = sNoiseSpot;
	pSoldier->aiData.ubNoiseVolume = MAX_MISC_NOISE_DURATION;
	pSoldier->bNoiseLevel = bCheckLevel;

	//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"[%d] set noise at %d", pSoldier->ubID, sNoiseSpot);
	//BeginMultiPurposeLocator(sNoiseSpot, bCheckLevel, FALSE);

	if (fTeamNoise)
	{
		// tell friends random spots
		SOLDIERTYPE * pFriend;
		UINT8 ubNumFriends = 0;

		// Run through each friendly.
		for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter++ )
		{
			pFriend = MercPtrs[ iCounter ];

			if( pFriend &&
				pFriend->bActive && 
				pFriend->stats.bLife >= OKLIFE &&
				PythSpacesAway( pSoldier->sGridNo, pFriend->sGridNo ) <= DAY_VISION_RANGE &&
				pFriend->aiData.bAlertStatus <= STATUS_RED &&
				!pFriend->aiData.bUnderFire &&
				TileIsOutOfBounds(pFriend->aiData.sNoiseGridno) )
			{
				sNoiseSpot = CheckSpotList[ Random(usNum) ];
				DebugAI( AI_MSG_INFO, pSoldier, String("set personal noise spot for friend [%d] %d level %d volume %d", pFriend->ubID, sNoiseSpot, bCheckLevel, MAX_MISC_NOISE_DURATION));

				pFriend->aiData.sNoiseGridno = sNoiseSpot;
				pFriend->aiData.ubNoiseVolume = MAX_MISC_NOISE_DURATION;
				pFriend->bNoiseLevel = pSoldier->pathing.bLevel;

				ubNumFriends++;
				
				//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"[%d] set noise at %d", pFriend->ubID, sNoiseSpot);
				//BeginMultiPurposeLocator(sNoiseSpot, bCheckLevel, FALSE);

				if( ubNumFriends >= usNum )
				{
					break;
				}
			}
		}
	}	
}

BOOLEAN PossiblyUpdateKnowledge( SOLDIERTYPE *pSoldier )
{
	UINT32		uiLoop;
	SOLDIERTYPE *pEnemy;
	BOOLEAN		fFound = FALSE;

	INT32		sSpot;
	INT8		bLevel;
	INT32		sNewSpot;

	INT32		iSearchRange;
	INT8		bKnowledge;
	INT16		sEnemyAP;

	BOOLEAN fAllowComplexAI = AllowComplexAI(pSoldier);

	iSearchRange = DAY_VISION_RANGE / 4;

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pEnemy = MercSlots[uiLoop];

		// for every active, living soldier of ANOTHER team
		if( pEnemy && 
			pEnemy->stats.bLife && 
			pEnemy->bTeam != pSoldier->bTeam &&
			ValidOpponent(pSoldier, pEnemy) )
		{
			//sEnemyAP = max(APBPConstants[AP_MINIMUM] / 2, AICalcActionPoints(pEnemy));
			sEnemyAP = APBPConstants[AP_MAXIMUM] / 2;

			// check personal knowledge
			bKnowledge = PersonalKnowledge(pSoldier, pEnemy->ubID);
			sSpot = KnownPersonalLocation(pSoldier, pEnemy->ubID);
			bLevel = KnownPersonalLevel(pSoldier, pEnemy->ubID);
				
			// update personal knowledge
			//if( bKnowledge != NOT_HEARD_OR_SEEN && UsePersonalKnowledge(pSoldier, pEnemy->ubID) )
			if( bKnowledge != NOT_HEARD_OR_SEEN )
			{
				if( !TileIsOutOfBounds(sSpot) )
				{
					if (pSoldier->sGridNo == sSpot && pSoldier->pathing.bLevel == bLevel)
					{
						// soldier is standing on known enemy location
						DebugAI(AI_MSG_INFO, pSoldier, String("soldier is standing on known enemy spot for [%d], remove personal knowledge", pEnemy->ubID));
						UpdatePersonal(pSoldier, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);

						// add noise at random unseen spot for this soldier and his friends
						DebugAI(AI_MSG_INFO, pSoldier, String("standing at obsolete personal spot: [%d] knowledge %d spot %d level %d", pEnemy->ubID, bKnowledge, sSpot, bLevel));
						AddNoiseAtRandomSightCoverSpot(pSoldier, sSpot, bLevel, fAllowComplexAI, FALSE);
						//ScreenMsg(FONT_MCOLOR_LTGRAY2, MSG_INTERFACE, L"[%d] standing at old personal spot %d for [%d] %s, remove", pSoldier->ubID, sSpot, pEnemy->ubID, pEnemy->GetName());
					}
					else if (WhoIsThere2(sSpot, bLevel) == NOBODY && 
							(LOS_Virtual(pSoldier, sSpot, bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) ||
							fAllowComplexAI && TeamSeesGridNo(pSoldier->bTeam, sSpot, bLevel, ANIM_PRONE)))
					{
						// our info is old, try to update
						DebugAI( AI_MSG_INFO, pSoldier, String("found obsolete personal spot: [%d] knowledge %d spot %d level %d", pEnemy->ubID, bKnowledge, sSpot, bLevel));

						if( PublicKnowledge(pSoldier->bTeam, pEnemy->ubID) == SEEN_CURRENTLY && 
							!TileIsOutOfBounds(KnownPublicLocation(pSoldier->bTeam, pEnemy->ubID)) )
						{
							// update personal knowledge to public
							DebugAI(AI_MSG_INFO, pSoldier, String("update personal knowledge to public %d", KnownPublicLocation(pSoldier->bTeam, pEnemy->ubID)));
							UpdatePersonalKnowledge(pSoldier, pEnemy->ubID, KnownPublicLocation(pSoldier->bTeam, pEnemy->ubID), KnownPublicLevel(pSoldier->bTeam, pEnemy->ubID));
							fFound = TRUE;

							//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"[%d] update personal knowledge to public %d for [%d] %s", pSoldier->ubID,  KnownPublicLocation(pSoldier->bTeam, pEnemy->ubID), pEnemy->ubID, pEnemy->GetName());
						}
						else
						{
							sNewSpot = FindClosestUnseenSpot(pSoldier, sSpot, bLevel, iSearchRange, 0, sEnemyAP, fAllowComplexAI);

							if( TileIsOutOfBounds(sNewSpot) && gfTurnBasedAI)
							{
								sNewSpot = FindClosestUnseenSpot(pSoldier, sSpot, bLevel, iSearchRange * 2, iSearchRange, sEnemyAP * 2, fAllowComplexAI);
							}

							if (TileIsOutOfBounds(sNewSpot) && gfTurnBasedAI && fAllowComplexAI)
							{
								sNewSpot = FindClosestUnseenSpot(pSoldier, sSpot, bLevel, iSearchRange * 4, iSearchRange * 2, sEnemyAP * 4, fAllowComplexAI);
							}

							if( !TileIsOutOfBounds(sNewSpot) )
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("update personal knowledge to found spot %d", sNewSpot));
								UpdatePersonalKnowledge(pSoldier, pEnemy->ubID, sNewSpot, bLevel);
								fFound = TRUE;

								//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] found new personal spot %d for [%d] %s", pSoldier->ubID, sNewSpot, pEnemy->ubID, pEnemy->GetName());
								//BeginMultiPurposeLocator(sNewSpot, bLevel, FALSE);
							}
							else if (bKnowledge == HEARD_3_TURNS_AGO || bKnowledge == SEEN_3_TURNS_AGO)
							{
								// could not find new spot for enemy, forget it
								DebugAI(AI_MSG_INFO, pSoldier, String("could not find new spot for [%d], remove personal knowledge", pEnemy->ubID));
								UpdatePersonal(pSoldier, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);
								
								// if we have nothing else to do, add noise to remember this location
								if (!GuySawEnemy(pSoldier, SEEN_2_TURNS_AGO) && !GuyHeardEnemy(pSoldier, HEARD_2_TURNS_AGO))
								{
									pSoldier->aiData.sNoiseGridno = sSpot;
									pSoldier->bNoiseLevel = bLevel;
									pSoldier->aiData.ubNoiseVolume = MAX_MISC_NOISE_DURATION;
									DebugAI(AI_MSG_INFO, pSoldier, String("add personal noise at %d level %d for %d turns", pSoldier->aiData.sNoiseGridno, pSoldier->bNoiseLevel, pSoldier->aiData.ubNoiseVolume));
								}								
							}
						}						
					}
				}
				else
				{
					// bad spot, remove knowledge
					DebugAI( AI_MSG_INFO, pSoldier, String("bad personal spot for [%d], remove personal knowledge", pEnemy->ubID));
					UpdatePersonal(pSoldier, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);

					//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] remove bad personal knowledge for [%d] %s", pSoldier->ubID, pEnemy->ubID, pEnemy->GetName());
				}				
			}

			// check public knowledge
			bKnowledge = PublicKnowledge(pSoldier->bTeam, pEnemy->ubID);
			sSpot = KnownPublicLocation(pSoldier->bTeam, pEnemy->ubID);
			bLevel = KnownPublicLevel(pSoldier->bTeam, pEnemy->ubID);			

			if (bKnowledge != NOT_HEARD_OR_SEEN)
			{
				if (!TileIsOutOfBounds(sSpot))
				{
					if (pSoldier->sGridNo == sSpot && pSoldier->pathing.bLevel == bLevel)
					{
						// soldier is standing on known enemy location
						DebugAI(AI_MSG_INFO, pSoldier, String("soldier is standing on known enemy spot for [%d], remove public knowledge", pEnemy->ubID));
						UpdatePublic(pSoldier->bTeam, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);

						// add noise at random unseen spot for this soldier and his friends
						DebugAI(AI_MSG_INFO, pSoldier, String("standing at obsolete HEARD_3_TURNS_AGO public spot: [%d] knowledge %d spot %d level %d", pEnemy->ubID, bKnowledge, sSpot, bLevel));
						AddNoiseAtRandomSightCoverSpot(pSoldier, sSpot, bLevel, TRUE, TRUE);
						//ScreenMsg(FONT_MCOLOR_LTGRAY2, MSG_INTERFACE, L"[%d] standing at old public spot %d for [%d] %s, remove", pSoldier->ubID, sSpot, pEnemy->ubID, pEnemy->GetName());
					}
					else if (WhoIsThere2(sSpot, bLevel) == NOBODY &&
						(LOS_Virtual(pSoldier, sSpot, bLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) ||
						TeamSeesGridNo(pSoldier->bTeam, sSpot, bLevel, ANIM_PRONE)))
					{
						// our info is old, try to update
						DebugAI(AI_MSG_INFO, pSoldier, String("found obsolete public spot: [%d] knowledge %d spot %d level %d", pEnemy->ubID, bKnowledge, sSpot, bLevel));
						sNewSpot = FindClosestUnseenSpot(pSoldier, sSpot, bLevel, iSearchRange, 0, sEnemyAP, TRUE);

						if (TileIsOutOfBounds(sNewSpot) && gfTurnBasedAI)
						{
							sNewSpot = FindClosestUnseenSpot(pSoldier, sSpot, bLevel, iSearchRange * 2, iSearchRange, sEnemyAP * 2, TRUE);
						}

						if (TileIsOutOfBounds(sNewSpot) && gfTurnBasedAI && fAllowComplexAI)
						{
							sNewSpot = FindClosestUnseenSpot(pSoldier, sSpot, bLevel, iSearchRange * 4, iSearchRange * 2, sEnemyAP * 4, TRUE);
						}

						if (!TileIsOutOfBounds(sNewSpot))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("update public knowledge, new spot %d", sNewSpot));
							UpdatePublicKnowledge(pSoldier, pEnemy->ubID, sNewSpot, bLevel);
							fFound = TRUE;

							//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] found new public spot %d for [%d] %s", pSoldier->ubID, sNewSpot, pEnemy->ubID, pEnemy->GetName());
							//BeginMultiPurposeLocator(sNewSpot, bLevel, FALSE);
						}
						else if (bKnowledge == HEARD_3_TURNS_AGO || bKnowledge == SEEN_3_TURNS_AGO || pSoldier->sGridNo == sSpot && pSoldier->pathing.bLevel == bLevel)
						{
							// could not find new spot for enemy, forget it
							DebugAI(AI_MSG_INFO, pSoldier, String("could not find new spot for [%d], remove public knowledge", pEnemy->ubID));
							UpdatePublic(pSoldier->bTeam, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);

							// if we have nothing else to do, add noise to remember this location
							if (!(pSoldier->sGridNo == sSpot && pSoldier->pathing.bLevel == bLevel) &&
								!GuySawEnemy(pSoldier, SEEN_2_TURNS_AGO) && !GuyHeardEnemy(pSoldier, HEARD_2_TURNS_AGO))
							{
								pSoldier->aiData.sNoiseGridno = sSpot;
								pSoldier->bNoiseLevel = bLevel;
								pSoldier->aiData.ubNoiseVolume = MAX_MISC_NOISE_DURATION;
								DebugAI(AI_MSG_INFO, pSoldier, String("add personal noise at %d level %d for %d turns", pSoldier->aiData.sNoiseGridno, pSoldier->bNoiseLevel, pSoldier->aiData.ubNoiseVolume));
							}
						}
					}
				}
				else
				{
					// bad spot, remove knowledge
					DebugAI( AI_MSG_INFO, pSoldier, String("bad public spot for [%d], remove knowledge", pEnemy->ubID));
					UpdatePublic(pSoldier->bTeam, pEnemy->ubID, NOT_HEARD_OR_SEEN, NOWHERE, 0);

					//ScreenMsg(FONT_MCOLOR_LTRED, MSG_INTERFACE, L"[%d] remove bad public knowledge for [%d] %s", pSoldier->ubID, pEnemy->ubID, pEnemy->GetName());
				}				
			}
		}
	}

	return fFound;
}

INT32 FindClosestUnseenSpot(SOLDIERTYPE *pSoldier, INT32 sCheckSpot, INT8 bCheckLevel, INT32 iSearchRange, INT32 iIgnoreRange, INT16 sAPLimit, BOOLEAN fTeamSight)
{
	if( !pSoldier )
		return NOWHERE;

	if( TileIsOutOfBounds(sCheckSpot) )
		return NOWHERE;

	//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"[%d] search for spot %d range %d AP %d", pSoldier->ubID, sCheckSpot, iSearchRange, sAPLimit);

	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sGridNo;

	INT32	sRealSpot;
	INT8	bRealLevel;

	INT32	sFoundSpot = NOWHERE;
	INT16	sMinDistance;

	// store real location
	sRealSpot = pSoldier->sGridNo;
	bRealLevel = pSoldier->pathing.bLevel;

	// set max search range
	gubNPCAPBudget = sAPLimit;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (sCheckSpot % MAXCOL));
	sMaxRight = min( iSearchRange, MAXCOL - ((sCheckSpot % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (sCheckSpot / MAXROW));
	sMaxDown = min( iSearchRange, MAXROW - ((sCheckSpot / MAXROW) + 1));

	gubNPCDistLimit = (UINT8) iSearchRange;

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = sCheckSpot + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	// fake soldier's location to make path check
	pSoldier->sGridNo = sCheckSpot;
	pSoldier->pathing.bLevel = bCheckLevel;

	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, DetermineMovementMode( pSoldier, AI_ACTION_SEEK_OPPONENT ), COPYREACHABLE, 0 );

	// restore soldier's location
	pSoldier->sGridNo = sRealSpot;
	pSoldier->pathing.bLevel = bRealLevel;

	// make a list of reachable spots that nobody sees
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = sCheckSpot + sXOffset + (MAXCOL * sYOffset);

			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if( sGridNo == pSoldier->sGridNo )
			{
				continue;
			}

			if (sXOffset <= iIgnoreRange && sXOffset >= -iIgnoreRange && sYOffset <= iIgnoreRange && sYOffset >= -iIgnoreRange)
			{
				continue;
			}

			// assume opponent will not move into water
			if ( Water(sGridNo, bCheckLevel) )
			{
				continue;
			}			

			// exclude locations with tear/mustard gas
			if ( InGas( pSoldier, sGridNo ) )
			{
				continue;
			}

			// check that we can see spot
			if( LOS_Virtual(pSoldier, sGridNo, bCheckLevel, ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
			{
				continue;
			}

			// check that our team sees spot
			if (fTeamSight && TeamSeesGridNo(pSoldier->bTeam, sGridNo, bCheckLevel, ANIM_PRONE))
			{
				continue;
			}

			// find closest spot
			if( sFoundSpot == NOWHERE || PythSpacesAway(sCheckSpot, sGridNo) < sMinDistance )
			{
				sFoundSpot = sGridNo;
				sMinDistance = PythSpacesAway(sCheckSpot, sGridNo);
			}
		}
	}

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

	DebugAI( AI_MSG_INFO, pSoldier, String("found random unseen spot %d", sFoundSpot));

	//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] found new spot %d", pSoldier->ubID, sFoundSpot);
	//BeginMultiPurposeLocator(sFoundSpot, bCheckLevel, FALSE);

	return sFoundSpot;
}

// limit seek range for soldiers when defending
INT16 SeekRange(SOLDIERTYPE *pSoldier, INT32 *pusFromGridNo)
{
	BOOLEAN fOppPosKnown = FALSE;
	BOOLEAN fInCombat = FALSE;
	BOOLEAN fRedAlert = FALSE;

	if (pSoldier->bTeam != ENEMY_TEAM)
	{
		*pusFromGridNo = pSoldier->sGridNo;
		return MAX_ROAMING_RANGE;
	}

	// sevenfm: for zombies, allow max range
	if (pSoldier->IsZombie())
	{
		*pusFromGridNo = pSoldier->sGridNo;
		return MAX_ROAMING_RANGE;
	}

	if (CREATURE_OR_BLOODCAT(pSoldier))
	{
		//if ( pSoldier->aiData.bAlertStatus == STATUS_BLACK )
		if (pSoldier->aiData.bAlertStatus >= STATUS_RED)
		{
			*pusFromGridNo = pSoldier->sGridNo; // from current position!
			return(MAX_ROAMING_RANGE);
		}
	}

	if (pSoldier->aiData.bOrders == POINTPATROL || pSoldier->aiData.bOrders == RNDPTPATROL)
	{
		// roam near NEXT PATROL POINT, not from where merc starts out
		*pusFromGridNo = pSoldier->aiData.sPatrolGrid[pSoldier->aiData.bNextPatrolPnt];
	}
	else
	{
		// roam around where mercs started
		//*pusFromGridNo = pSoldier->sInitialGridNo;
		*pusFromGridNo = pSoldier->aiData.sPatrolGrid[0];
	}

	// if soldier is under attack or in contact with enemy - use current spot
	if (pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier))
	{
		*pusFromGridNo = pSoldier->sGridNo;
	}

	switch (pSoldier->aiData.bOrders)
	{
	case STATIONARY:	return TACTICAL_RANGE / 2;
	case ONGUARD:		return TACTICAL_RANGE;
	case CLOSEPATROL:
	case POINTPATROL:
	case RNDPTPATROL:	return NightLight() ? TACTICAL_RANGE : TACTICAL_RANGE * 2;
	case FARPATROL:
	case ONCALL:		return NightLight() ? TACTICAL_RANGE * 2 : TACTICAL_RANGE * 4;
	case SEEKENEMY:		return MAX_ROAMING_RANGE;
	default:			return MAX_ROAMING_RANGE;
	}
}

// check that current loaded sector is town
BOOLEAN AICheckTown( void )
{
	// determine sector name
	UINT8 ubTownID = GetTownIdForSector( gWorldSectorX, gWorldSectorY );

	// not underground
	if(gbWorldSectorZ > 0)
	{
		return FALSE;
	}
	// check town sector
	if( ubTownID == BLANK_SECTOR )
	{
		return FALSE;
	}

	return TRUE;
}

UINT8 AISectorType(void)
{
	UINT8	ubSectorID = SECTOR(gWorldSectorX, gWorldSectorY);
	SECTORINFO *pSector = &SectorInfo[ubSectorID];
	UINT8	ubSectorType = PLAINS;
	if (pSector)
	{
		ubSectorType = pSector->ubTraversability[THROUGH_STRATEGIC_MOVE];
	}
	return ubSectorType;
}

// check that current loaded sector is underground
BOOLEAN AICheckUnderground( void )
{
	// not underground
	if(gbWorldSectorZ > 0)
	{
		return TRUE;
	}

	return FALSE;
}

// try to estimate attack percent depending on distance
// use number of enemies this soldier has seen recently
UINT8 AIEstimateAttackWeight(SOLDIERTYPE *pAttacker, INT32 sAttackerSpot, SOLDIERTYPE *pMe, INT32 sMySpot)
{
	CHECKF(pAttacker);
	CHECKF(!TileIsOutOfBounds(sAttackerSpot));

	UINT8	ubTeamLoop;
	UINT8	ubIDLoop;
	INT16	sDistance;
	UINT8	ubWeight;
	UINT8	ubMyWeight = 100;
	UINT8	ubTotalWeight = 0;
	UINT8	ubAttackWeight = 100;
	INT32	sTargetSpot;
	INT8	bTargetLevel;
	SOLDIERTYPE *pTarget;
	INT32	sCloseDistance = TACTICAL_RANGE / 4;

	// find number of seen enemies
	// for each seen enemy, calculate his weight depending on distance
	for( ubTeamLoop = 0; ubTeamLoop < MAXTEAMS; ubTeamLoop++ )
	{
		if( !gTacticalStatus.Team[ubTeamLoop].bTeamActive )
			continue;

		if( gTacticalStatus.Team[ ubTeamLoop ].bSide != pAttacker->bSide )
		{
			// consider guys in this team, which isn't on our side
			for( ubIDLoop = gTacticalStatus.Team[ ubTeamLoop ].bFirstID; ubIDLoop <= gTacticalStatus.Team[ ubTeamLoop ].bLastID; ubIDLoop++ )
			{
				pTarget = MercPtrs[ubIDLoop];
				// if this enemy is seen recently or it is checked enemy
				if (pTarget &&
					pTarget->bActive &&
					ValidOpponent(pAttacker, pTarget) &&
					Knowledge(pAttacker, pTarget->ubID) != NOT_HEARD_OR_SEEN)
				{
					// find enemy spot and level
					sTargetSpot = KnownLocation(pAttacker, pTarget->ubID);
					if (TileIsOutOfBounds(sTargetSpot))
					{
						sTargetSpot = pTarget->sGridNo;
					}
					bTargetLevel = KnownLevel(pAttacker, pTarget->ubID);

					// find distance to enemy, all distances less than sCloseDistance are considered equal
					sDistance = max(PythSpacesAway(sAttackerSpot, sTargetSpot), sCloseDistance);
					
					if (AICheckHasGun(pAttacker))
					{
						// calculate weight based on estimated chance to hit
						ubWeight = AIEstimateCTH(pAttacker, pTarget, sAttackerSpot, sTargetSpot, FALSE);						

						if (PersonalKnowledge(pAttacker, pTarget->ubID) != SEEN_CURRENTLY &&
							PersonalKnowledge(pAttacker, pTarget->ubID) != SEEN_THIS_TURN &&
							PersonalKnowledge(pAttacker, pTarget->ubID) != SEEN_LAST_TURN)
							//!LOS_Soldier(pAttacker, pTarget, LOS_READY, CALC_FROM_ALL_DIRS))
						{
							ubWeight = ubWeight / 2;
						}
						/*if (AnyCoverFromSpot(sAttackerSpot, pAttacker->pathing.bLevel, sTargetSpot, bTargetLevel))
						{
							ubWeight -= ubWeight  * 25 * min(DAY_VISION_RANGE, PythSpacesAway(sAttackerSpot, sTargetSpot)) / (DAY_VISION_RANGE * 100);
						}*/
						if (AnyCoverFromSpot(sTargetSpot, bTargetLevel, sAttackerSpot, pAttacker->pathing.bLevel))
						{
							ubWeight -= ubWeight * 25 * min(TACTICAL_RANGE, PythSpacesAway(sAttackerSpot, sTargetSpot)) / (TACTICAL_RANGE * 100);
						}
						if(pTarget->IsCowering() || pTarget->IsUnconscious())
						{
							ubWeight = ubWeight / 2;
						}
					}
					else
					{
						ubWeight = sCloseDistance / sDistance;
					}					

					// if attacking me, remember my weight
					if (pTarget->ubID == pMe->ubID)
					{
						ubMyWeight = ubWeight;
					}

					ubTotalWeight += ubWeight;
				}
			}
		}
	}

	// divide my weight on total weight to find chance that soldier will attack me
	if( ubTotalWeight > 0 )
	{
		ubAttackWeight = max(1, 100 * ubMyWeight / ubTotalWeight);
	}

	return ubAttackWeight;
}

BOOLEAN AICheckSpotTooFar(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	CHECKF(pSoldier);

	// sevenfm: disable
	return FALSE;

	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	// only for enemy team
	if (pSoldier->bTeam != ENEMY_TEAM)
	{
		return FALSE;
	}

	if (pSoldier->aiData.bOrders == SEEKENEMY)
	{
		return FALSE;
	}

	// always allow max range if soldier is under fire or seen enemy recently
	if ((pSoldier->aiData.bUnderFire || GuySawEnemy(pSoldier) || GuyHeardEnemy(pSoldier, HEARD_2_TURNS_AGO)) &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->aiData.bOrders != ONGUARD)
	{
		return FALSE;
	}

	if (TeamHighPercentKilled(pSoldier->bTeam))
	{
		return FALSE;
	}

	if (pSoldier->aiData.bOrders == SEEKENEMY)
	{
		return FALSE;
	}

	if (!AICheckTown())
	{
		return FALSE;
	}

	if (!InARoom(sSpot))
	{
		return FALSE;
	}

	INT32 sMaxDist = DAY_VISION_RANGE * 4;

	if (NightLight())
	{
		sMaxDist = sMaxDist / 2;
	}

	if (PythSpacesAway(pSoldier->sGridNo, sSpot) > sMaxDist)
	{
		return TRUE;
	}

	INT32 sFromSpot;
	INT16 sSeekRange;
	sSeekRange = SeekRange(pSoldier, &sFromSpot);

	if (SpacesAway(sSpot, sFromSpot) > sSeekRange)
	{
		return TRUE;
	}

	return FALSE;
}

// returns TRUE if target is within allowed roaming range
BOOLEAN AICheckRoamingRange(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	CHECKF(pSoldier);
	CHECKF(!TileIsOutOfBounds(sSpot));

	INT32 sFromSpot;
	INT16 sSeekRange;

	sSeekRange = RoamingRange(pSoldier, &sFromSpot);

	if (SpacesAway(sSpot, sFromSpot) > sSeekRange)
	{
		return FALSE;
	}

	return TRUE;
}

BOOLEAN AICheckDefense( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	// only for enemy team
	if( pSoldier->bTeam != ENEMY_TEAM )
	{
		return FALSE;
	}

	// SEEKENEMY should always try to attack
	if( pSoldier->bTeam == SEEKENEMY )
	{
		return FALSE;
	}

	// only try to defend in towns and underground
	if( !AICheckTown() && !AICheckUnderground() )
	{
		return FALSE;
	}

	return TRUE;
}

BOOLEAN AICheckCanCommunicate( SOLDIERTYPE *pSoldier1, SOLDIERTYPE *pSoldier2 )
{
	CHECKF(pSoldier1);
	CHECKF(pSoldier2);

	if (pSoldier1->stats.bLife < OKLIFE ||
		pSoldier1->IsUnconscious() ||
		pSoldier2->stats.bLife < OKLIFE ||
		pSoldier2->IsUnconscious() ||
		pSoldier1->bDeafenedCounter > 0 ||
		pSoldier2->bDeafenedCounter > 0)
		return FALSE;

	if( HasExtendedEarOn(pSoldier1) &&
		HasExtendedEarOn(pSoldier2) &&
		!SectorJammed())
		//PythSpacesAway(pSoldier1->sGridNo, pSoldier2->sGridNo) < DAY_VISION_RANGE )
	{
		return TRUE;
	}

	if( PythSpacesAway(pSoldier1->sGridNo, pSoldier2->sGridNo) < TACTICAL_RANGE &&
		LOS_Soldier(pSoldier1, pSoldier2, LOS_NORMAL, CALC_FROM_ALL_DIRS) &&
		LOS_Soldier(pSoldier2, pSoldier1, LOS_NORMAL, CALC_FROM_ALL_DIRS))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckWeOutnumberSector(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	UINT32	uiLoop;
	SOLDIERTYPE *pOpponent;

	UINT8	ubNumFriends = 0;
	UINT8	ubNumOpponents = 0;

	// loop through all soldiers in sector
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (ValidOpponent(pSoldier, pOpponent))
		{
			ubNumOpponents++;
		}
		else if (pOpponent->bTeam == pSoldier->bTeam || pOpponent->bSide == pSoldier->bSide)
		{
			ubNumFriends++;
		}
	}

	if (ubNumFriends > ubNumOpponents * 2)
		return TRUE;

	return FALSE;
}

BOOLEAN AICheckWeOutnumberPublic(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	CHECKF(pSoldier);
	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	UINT8 ubFriends = CountNearbyFriends(pSoldier, sSpot, TACTICAL_RANGE);
	UINT8 ubEnemies = CountPublicKnownEnemies(pSoldier, sSpot, VISION_RANGE + TACTICAL_RANGE / 2);

	if (ubEnemies > 0 && ubFriends > 2 * ubEnemies)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckWeOutnumberLocal(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	CHECKF(pSoldier);
	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	UINT8 ubFriends = CountNearbyFriends(pSoldier, pSoldier->sGridNo, TACTICAL_RANGE / 4);
	UINT8 ubEnemies = CountPublicKnownEnemies(pSoldier, sSpot, TACTICAL_RANGE / 2);
	UINT8 ubEnemies2 = CountPublicKnownEnemies(pSoldier, sSpot, TACTICAL_RANGE);

	if (ubEnemies > 0 && 
		ubFriends > 2 * ubEnemies &&
		ubFriends > ubEnemies2)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN AICheckWeOutnumber(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	CHECKF(pSoldier);
	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}
	
	if (AICheckWeOutnumberPublic(pSoldier, sSpot) || AICheckWeOutnumberLocal(pSoldier, sSpot))
	{
		return TRUE;
	}

	return FALSE;
}

// first call PrepareThreatlist to make threat list
UINT8 ClosestKnownThreatID(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);
	
	UINT32	uiLoop;
	INT32	sClosestOpponent = NOWHERE;
	INT32	iRange, iClosestRange;
	UINT8	ubClosestOpponentID = NOBODY;
	
	// use global defined threat list
	for (uiLoop = 0; uiLoop < guiThreatCnt; uiLoop++)
	{
		// if for some reason we have incorrect location
		if( TileIsOutOfBounds(Threat[uiLoop].sGridNo) )
			continue;

		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, Threat[uiLoop].sGridNo );

		if ( ubClosestOpponentID ==  NOBODY || iRange < iClosestRange )
		{
			ubClosestOpponentID = Threat[uiLoop].pOpponent->ubID;			
			iClosestRange = iRange;
			sClosestOpponent = Threat[uiLoop].sGridNo;
		}
	}

	return( ubClosestOpponentID );
}

// first call PrepareThreatlist to make threat list
UINT8 ClosestSeenThreatID(SOLDIERTYPE *pSoldier, UINT8 ubMax)
{
	CHECKF(pSoldier);
	
	UINT32	uiLoop;
	INT32	sClosestOpponent = NOWHERE;
	INT32	iRange, iClosestRange;
	UINT8	ubClosestOpponentID = NOBODY;

	// use global defined threat list
	for (uiLoop = 0; uiLoop < guiThreatCnt; uiLoop++)
	{
		// if for some reason we have incorrect location
		if( TileIsOutOfBounds(Threat[uiLoop].sGridNo) )
			continue;

		// check knowledge
		if (Threat[uiLoop].bPersonalKnowledge < SEEN_CURRENTLY || Threat[uiLoop].bPersonalKnowledge > ubMax)
			continue;

		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, Threat[uiLoop].sGridNo );

		if ( ubClosestOpponentID ==  NOBODY || iRange < iClosestRange )
		{
			ubClosestOpponentID = Threat[uiLoop].pOpponent->ubID;			
			iClosestRange = iRange;
			sClosestOpponent = Threat[uiLoop].sGridNo;
		}
	}

	return( ubClosestOpponentID );
}

void PrepareThreatlist( SOLDIERTYPE *pSoldier )
{
	SOLDIERTYPE *pOpponent;
	INT32	iThreatRange, iClosestThreatRange = 1500;
	UINT32	uiLoop;
	INT8	bPersonalKnowledge;
	INT8	bPublicKnowledge;
	INT8	bKnowledge;
	INT32	sThreatLoc;
	INT8	bThreatLevel;
	INT32	iThreatCertainty;
	INT32	iMaxThreatRange = MAX_THREAT_RANGE + AI_PATHCOST_RADIUS;

	guiThreatCnt = 0;

	if(!pSoldier)
		return;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if(pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);
		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);

		// sevenfm: use attacker
		/*if( pSoldier->ubPreviousAttackerID != NOBODY && pSoldier->ubPreviousAttackerID == pOpponent->ubID )
		{
			sThreatLoc = pOpponent->sGridNo;
			iThreatCertainty = ThreatPercent[6];
			bThreatLevel = pOpponent->pathing.bLevel;

			bKnowledge = HEARD_3_TURNS_AGO;
			bPersonalKnowledge = HEARD_3_TURNS_AGO;
			bPublicKnowledge = NOT_HEARD_OR_SEEN;
		}*/

		// if this opponent is unknown personally and publicly
		if( bKnowledge == NOT_HEARD_OR_SEEN )
		{
			continue;			// next merc
		}			

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);
		iThreatCertainty = ThreatPercent[bKnowledge - OLDEST_HEARD_VALUE];

		// safety check
		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;
		}

		// calculate how far away this threat is (in adjusted pixels)
		iThreatRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sThreatLoc );

		// if this opponent is believed to be too far away to really be a threat
		if (iThreatRange > iMaxThreatRange)
		{
			continue;			// check next opponent
		}

		// remember this opponent as a current threat, but DON'T REDUCE FOR COVER!
		Threat[guiThreatCnt].iValue = CalcManThreatValue(pOpponent,pSoldier->sGridNo,FALSE,pSoldier);

		// if the opponent is no threat at all for some reason
		if (Threat[guiThreatCnt].iValue == -999)
		{
			continue;			// check next opponent
		}

		Threat[guiThreatCnt].pOpponent	= pOpponent;
		Threat[guiThreatCnt].sGridNo	= sThreatLoc;
		Threat[guiThreatCnt].iCertainty	= iThreatCertainty;
		Threat[guiThreatCnt].iOrigRange	= iThreatRange;

		// calculate how many APs he will have at the start of the next turn
		//Threat[guiThreatCnt].iAPs = pOpponent->CalcActionPoints();
		Threat[guiThreatCnt].iAPs = AICalcActionPoints(pOpponent);

		// sevenfm: more information
		Threat[guiThreatCnt].bLevel = bThreatLevel;
		Threat[guiThreatCnt].bKnowledge = bKnowledge;
		Threat[guiThreatCnt].bPersonalKnowledge = bPersonalKnowledge;
		Threat[guiThreatCnt].bPublicKnowledge = bPublicKnowledge;

		if (iThreatRange < iClosestThreatRange)
		{
			iClosestThreatRange = iThreatRange;
		}

		//DebugCover(pSoldier, String("found opponent [%d] GridNo=%d (real=%d) certainty=%d threat range=%d threat AP=%d", pOpponent->ubID, sThreatLoc, pOpponent->sGridNo, iThreatCertainty, iThreatRange));

		guiThreatCnt++;
	}
}

UINT8 CountSeenCovertOpponents( SOLDIERTYPE *pSoldier, UINT8 ubMax )
{
	CHECKF(pSoldier);

	UINT8	ubTeamLoop;
	UINT8	ubIDLoop;
	UINT8	cnt = 0;

	for( ubTeamLoop = 0; ubTeamLoop < MAXTEAMS; ubTeamLoop++ )
	{
		if( !gTacticalStatus.Team[ubTeamLoop].bTeamActive )
			continue;

		if( gTacticalStatus.Team[ ubTeamLoop ].bSide != pSoldier->bSide )
		{
			// consider guys in this team, which isn't on our side
			for( ubIDLoop = gTacticalStatus.Team[ ubTeamLoop ].bFirstID; ubIDLoop <= gTacticalStatus.Team[ ubTeamLoop ].bLastID; ubIDLoop++ )
			{
				// check that opponent is covert and we see him currently
				if( MercPtrs[ubIDLoop] &&
					PersonalKnowledge(pSoldier, ubIDLoop) >= SEEN_CURRENTLY &&
					PersonalKnowledge(pSoldier, ubIDLoop) <= ubMax &&
					MercPtrs[ubIDLoop]->IsCovert() )
					//MercPtrs[ubIDLoop]->usSoldierFlagMask & (SOLDIER_COVERT_CIV|SOLDIER_COVERT_SOLDIER) )
				{
					cnt++;
				}
			}
		}
	}

	return cnt;
}

UINT8 CountNearbyKnownCovertOpponents( SOLDIERTYPE *pSoldier )
{
	CHECKF(pSoldier);

	UINT8	ubTeamLoop;
	UINT8	ubIDLoop;
	UINT8	cnt = 0;
	INT8	bKnowledge;

	for( ubTeamLoop = 0; ubTeamLoop < MAXTEAMS; ubTeamLoop++ )
	{
		if( !gTacticalStatus.Team[ubTeamLoop].bTeamActive )
			continue;

		if( gTacticalStatus.Team[ ubTeamLoop ].bSide != pSoldier->bSide )
		{
			// consider guys in this team, which isn't on our side
			for( ubIDLoop = gTacticalStatus.Team[ ubTeamLoop ].bFirstID; ubIDLoop <= gTacticalStatus.Team[ ubTeamLoop ].bLastID; ubIDLoop++ )
			{
				if( !MercPtrs[ubIDLoop] )
				{
					continue;
				}

				bKnowledge = PersonalKnowledge(pSoldier, ubIDLoop);

				// check that opponent is covert and we see him currently
				if( (bKnowledge == SEEN_CURRENTLY ||
					bKnowledge == SEEN_THIS_TURN ||
					bKnowledge == SEEN_LAST_TURN ||
					bKnowledge == HEARD_THIS_TURN ||
					bKnowledge == HEARD_LAST_TURN ) &&
					PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubIDLoop]->sGridNo) < DAY_VISION_RANGE / 2 &&
					MercPtrs[ubIDLoop]->IsCovert() )
				{
					cnt++;
				}
			}
		}
	}

	return cnt;
}

UINT8 AIDirection(INT32 sSpot1, INT32 sSpot2)
{
	if(TileIsOutOfBounds(sSpot1) || TileIsOutOfBounds(sSpot2))
	{
		return DIRECTION_IRRELEVANT;
	}

	return atan8(CenterX(sSpot1),CenterY(sSpot1),CenterX(sSpot2),CenterY(sSpot2));
}

UINT8 ArmyPercentKilled( void )
{
	if( gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector + gTacticalStatus.ubArmyGuysKilled == 0 )
	{
		return 0;
	}

	return 100 * gTacticalStatus.ubArmyGuysKilled / ( gTacticalStatus.Team[ ENEMY_TEAM ].bMenInSector + gTacticalStatus.ubArmyGuysKilled );
}

UINT8 TeamPercentKilled(INT8 bTeam)
{
	if (bTeam == ENEMY_TEAM)
	{
		return ArmyPercentKilled();
	}
	return 0;
}

BOOLEAN TeamHighPercentKilled(INT8 bTeam)
{
	if (bTeam == ENEMY_TEAM && ArmyPercentKilled() > ArmyPercentKilledTolerance())
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN LocateEnemyJammer(SOLDIERTYPE* pSoldier, INT32 &sSpot, INT8 &bLevel)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if( !pOpponent->IsJamming() )
		{
			continue;
		}

		sSpot = pOpponent->sGridNo;
		bLevel = pOpponent->pathing.bLevel;

		return TRUE;
	}

	return FALSE;
}

BOOLEAN FindEnemyJammer(SOLDIERTYPE* pSoldier)
{
	CHECKF(pSoldier);

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if( pOpponent->IsJamming() )
		{
			return TRUE;
		}
	}

	return FALSE;
}

UINT8 AIChooseFlashlightDirection(SOLDIERTYPE *pSoldier)
{
	if( !pSoldier )
	{
		return DIRECTION_IRRELEVANT;
	}

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT32		sSpot;
	INT8		bLevel;
	UINT8		ubDirection;
	INT8		bKnowledge;

	// loop through all the enemies
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// skip opponents that are seen currently by team
		if( PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) == SEEN_CURRENTLY )
		{
			continue;
		}

		// best personal/public knowledge
		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);

		// check that opponent was seen\heard recently
		if( bKnowledge != SEEN_THIS_TURN &&
			bKnowledge != SEEN_LAST_TURN &&
			bKnowledge != SEEN_2_TURNS_AGO &&
			bKnowledge != HEARD_THIS_TURN &&
			bKnowledge != HEARD_LAST_TURN &&
			bKnowledge != HEARD_2_TURNS_AGO )
		{
			continue;
		}

		sSpot = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if(TileIsOutOfBounds(sSpot))
		{
			continue;
		}

		// skip if too far
		if( PythSpacesAway(pSoldier->sGridNo, sSpot) > pSoldier->AIFlashlightRange() )
		{
			continue;
		}

		ubDirection = AIDirection(pSoldier->sGridNo, pOpponent->sGridNo);		

		// check that we can use light to reveal him
		if( bLevel > 0 ||
			InLightAtNight(sSpot, bLevel) ||
			InGasOrSmoke(pOpponent, sSpot) )
		{
			continue;
		}

		// skip if opponent is hiding so we cannot see him
		if( !LOS_Virtual(pSoldier, sSpot, bLevel, ANIM_CROUCH, LOS_NORMAL, NO_DISTANCE_LIMIT) )
		{
			continue;
		}

		// skip if there are friends in this direction so we don't reveal them
		if( CountFriendsInDirection(pSoldier, ubDirection, pSoldier->AIFlashlightRange(), TRUE) )
		{
			continue;
		}

		// skip if nobody will see him
		if (CountFriendsPossiblySeeFlashlightOpponent(pSoldier, sSpot, bLevel, pOpponent->ubID) < 1)
		{
			continue;
		}

		return ubDirection;
	}

	return DIRECTION_IRRELEVANT;
}

// decide how many soldiers can be killed before alarm will be raised
UINT8 ArmyPercentKilledTolerance( void )
{
	// 50% at day, 25% at night, 25-33% for restricted sectors
	return 100 / (2 + SectorCurfew( TRUE ));
}

UINT8 SectorCurfew( BOOLEAN fNight )
{
	UINT8	ubSectorId = SECTOR(gWorldSectorX, gWorldSectorY);
	UINT8	ubSectorData = 0;

	ubSectorData = SectorExternalData[ubSectorId][gbWorldSectorZ].usCurfewValue;

	if ( fNight && NightLight() )			// suspicious at night
		ubSectorData = max( ubSectorData, 1 );

	if ( gbWorldSectorZ > 0 )	// underground we are always suspicious				
		ubSectorData = max( ubSectorData, 2 );

	return ubSectorData;
}

BOOLEAN AIUseFlashlightCondition(SOLDIERTYPE *pSoldier, UINT8 ubDirection)
{
	CHECKF(pSoldier);

	if( NightTime() &&
		gbWorldSectorZ == 0 &&
		SectorCurfew(FALSE) < 2 &&
		pSoldier->aiData.bOrders != STATIONARY &&
		pSoldier->pathing.bLevel == 0 &&
		!pSoldier->aiData.bUnderFire &&
		pSoldier->aiData.bAIMorale >= MORALE_CONFIDENT &&
		pSoldier->stats.bLife > pSoldier->stats.bLifeMax * 3 / 4 &&
		!AICheckNVG(pSoldier) &&
		!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!InGasOrSmoke(pSoldier, pSoldier->sGridNo) &&
		CountNearbyFriends(pSoldier, pSoldier->sGridNo, 1) == 0 &&
		(ubDirection == DIRECTION_IRRELEVANT || CountFriendsInDirection(pSoldier, ubDirection, pSoldier->AIFlashlightRange(), TRUE) == 0))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN UsePersonalKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	INT8		bPersonalKnowledge;
	INT8		bPublicKnowledge;

	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return FALSE;
	}

	bPersonalKnowledge = PersonalKnowledge(pSoldier, ubOpponentID);
	bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, ubOpponentID);

	if( gubKnowledgeValue[bPublicKnowledge - OLDEST_HEARD_VALUE][bPersonalKnowledge - OLDEST_HEARD_VALUE] > 0 ||
		bPersonalKnowledge != NOT_HEARD_OR_SEEN &&
		TileIsOutOfBounds(KnownPublicLocation(pSoldier->bTeam, ubOpponentID)) && 
		!TileIsOutOfBounds(KnownPersonalLocation(pSoldier, ubOpponentID)) )
	{
		return TRUE;
	}

	return FALSE;
}

INT8 Knowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return NOT_HEARD_OR_SEEN;
	}
	
	if( UsePersonalKnowledge(pSoldier, ubOpponentID) )
	{
		return PersonalKnowledge(pSoldier, ubOpponentID);
	}

	return PublicKnowledge(pSoldier->bTeam, ubOpponentID);
}

INT32 KnownLocation( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return NOWHERE;
	}

	if( UsePersonalKnowledge(pSoldier, ubOpponentID) )
	{
		return KnownPersonalLocation(pSoldier, ubOpponentID);
	}

	return KnownPublicLocation(pSoldier->bTeam, ubOpponentID);
}

INT8 KnownLevel( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return 0;
	}

	if( UsePersonalKnowledge(pSoldier, ubOpponentID) )
	{
		return KnownPersonalLevel(pSoldier, ubOpponentID);
	}

	return KnownPublicLevel(pSoldier->bTeam, ubOpponentID);
}

INT8 PersonalKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return NOT_HEARD_OR_SEEN;
	}

	return pSoldier->aiData.bOppList[ubOpponentID];
}

INT32 KnownPersonalLocation( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return NOWHERE;
	}
	/*if(PersonalKnowledge(pSoldier, ubOpponentID) == NOT_HEARD_OR_SEEN)
	{
		return NOWHERE;
	}*/

	return gsLastKnownOppLoc[pSoldier->ubID][ubOpponentID];
}

INT8 KnownPersonalLevel( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return 0;
	}

	return gbLastKnownOppLevel[pSoldier->ubID][ubOpponentID];
}

INT8 PublicKnowledge( UINT8 bTeam, UINT8 ubOpponentID )
{
	if( bTeam >= MAXTEAMS || ubOpponentID == NOBODY )
	{
		return NOT_HEARD_OR_SEEN;
	}

	return gbPublicOpplist[bTeam][ubOpponentID];
}

INT32 KnownPublicLocation( UINT8 bTeam, UINT8 ubOpponentID )
{
	if( bTeam >= MAXTEAMS || ubOpponentID == NOBODY )
	{
		return NOWHERE;
	}
	/*if (PublicKnowledge(bTeam, ubOpponentID) == NOT_HEARD_OR_SEEN)
	{
		return NOWHERE;
	}*/

	return gsPublicLastKnownOppLoc[bTeam][ubOpponentID];
}

INT8 KnownPublicLevel( UINT8 bTeam, UINT8 ubOpponentID )
{
	if( bTeam >= MAXTEAMS || ubOpponentID == NOBODY )
	{
		return 0;
	}

	return gbPublicLastKnownOppLevel[bTeam][ubOpponentID];
}

void UpdateKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID, INT32 sSpot, INT8 bLevel )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return;
	}

	if( UsePersonalKnowledge( pSoldier, ubOpponentID ) )
	{
		UpdatePersonalKnowledge( pSoldier, ubOpponentID, sSpot, bLevel );
	}
	else
	{
		UpdatePublicKnowledge( pSoldier, ubOpponentID, sSpot, bLevel );
	}
}

void UpdatePersonalKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID, INT32 sSpot, INT8 bLevel )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return;
	}
	if( PersonalKnowledge(pSoldier, ubOpponentID) == NOT_HEARD_OR_SEEN )
	{
		return;
	}

	// allow taking cover since something have changed
	/*if (gsLastKnownOppLoc[pSoldier->ubID][ubOpponentID] != sSpot || gbLastKnownOppLevel[pSoldier->ubID][ubOpponentID] != bLevel)
	{
		pSoldier->usSoldierFlagMask2 &= ~SOLDIER_SKIP_COVER_CHECK;
		DebugAI(AI_MSG_INFO, pSoldier, String("UpdatePersonalKnowledge opponent [%d] spot %d level %d: <disable SkipCoverCheck>", ubOpponentID, sSpot, bLevel));
	}*/

	gsLastKnownOppLoc[pSoldier->ubID][ubOpponentID] = sSpot;
	gbLastKnownOppLevel[pSoldier->ubID][ubOpponentID] = bLevel;
}

void UpdatePublicKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID, INT32 sSpot, INT8 bLevel )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("UpdatePublicKnowledge: !pSoldier || ubOpponentID == NOBODY"));
		return;
	}
	if( PublicKnowledge(pSoldier->bTeam, ubOpponentID) == NOT_HEARD_OR_SEEN )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("UpdatePublicKnowledge: NOT_HEARD_OR_SEEN"));
		return;
	}

	// allow taking cover since something have changed
	/*if (gsPublicLastKnownOppLoc[pSoldier->bTeam][ubOpponentID] != sSpot || gbPublicLastKnownOppLevel[pSoldier->bTeam][ubOpponentID] != bLevel)
	{
		pSoldier->usSoldierFlagMask2 &= ~SOLDIER_SKIP_COVER_CHECK;
		DebugAI(AI_MSG_INFO, pSoldier, String("UpdatePublicKnowledge opponent [%d] spot %d level %d: <disable SkipCoverCheck>", ubOpponentID, sSpot, bLevel));
	}*/

	gsPublicLastKnownOppLoc[pSoldier->bTeam][ubOpponentID] = sSpot;
	gbPublicLastKnownOppLevel[pSoldier->bTeam][ubOpponentID] = bLevel;
}

UINT8 PersonalNoiseVolume( SOLDIERTYPE *pSoldier )
{
	if( !pSoldier ) 
	{
		return 0;
	}
	return pSoldier->aiData.ubNoiseVolume;
}

INT8 PersonalNoiseLevel( SOLDIERTYPE *pSoldier )
{
	if( !pSoldier ) 
	{
		return 0;
	}

	return pSoldier->bNoiseLevel;
}

INT32 PersonalNoiseLocation( SOLDIERTYPE *pSoldier )
{
	if( !pSoldier ) 
	{
		return NOWHERE;
	}

	return pSoldier->aiData.sNoiseGridno;
}

UINT8 PublicNoiseVolume( UINT8 bTeam )
{
	if( bTeam >= MAXTEAMS )
	{
		return 0;
	}

	return gubPublicNoiseVolume[bTeam];
}

INT8 PublicNoiseLevel( UINT8 bTeam )
{
	if( bTeam >= MAXTEAMS )
	{
		return 0;
	}

	if( PublicNoiseVolume(bTeam) == 0 )
	{
		return 0;
	}

	return gbPublicNoiseLevel[bTeam];
}

INT32 PublicNoiseLocation( UINT8 bTeam )
{
	if( bTeam >= MAXTEAMS )
	{
		return NOWHERE;
	}

	if( PublicNoiseVolume(bTeam) == 0 )
	{
		return NOWHERE;
	}

	return gsPublicNoiseGridNo[bTeam];
}

void SetPersonalNoise( SOLDIERTYPE* pSoldier, UINT8 ubVolume, INT32 sLocation, INT8 bLevel )
{
	if( !pSoldier )
	{
		return;
	}

	/*if (pSoldier->aiData.sNoiseGridno != sLocation || pSoldier->bNoiseLevel != bLevel)
	{
		pSoldier->usSoldierFlagMask2 &= ~SOLDIER_SKIP_COVER_CHECK;
		DebugAI(AI_MSG_INFO, pSoldier, String("SetPersonalNoise: <disable SkipCoverCheck>"));
	}*/

	pSoldier->aiData.ubNoiseVolume = ubVolume;
	pSoldier->aiData.sNoiseGridno = sLocation;
	pSoldier->bNoiseLevel = bLevel;
}

void SetPublicNoise( UINT8 bTeam, UINT8 ubVolume, INT32 sLocation, INT8 bLevel )
{
	if( bTeam >= MAXTEAMS )
	{
		return;
	}

	gubPublicNoiseVolume[bTeam] = ubVolume;
	gsPublicNoiseGridNo[bTeam] = sLocation;
	gbPublicNoiseLevel[bTeam] = bLevel;
}

BOOLEAN ValidOpponent( SOLDIERTYPE* pSoldier, SOLDIERTYPE* pOpponent )
{
	if( !pSoldier || !pOpponent )
	{
		return FALSE;
	}

	if( !pOpponent->bActive ||
		!pOpponent->bInSector ||
		pOpponent->stats.bLife <= 0 ||
		CONSIDERED_NEUTRAL(pSoldier, pOpponent) ||
		pSoldier->bSide == pOpponent->bSide ||
		pSoldier->aiData.bAttitude == ATTACKSLAYONLY && pOpponent->ubProfile != SLAY ||
		(pSoldier->IsZombie() || CREATURE_OR_BLOODCAT(pSoldier) || !AICheckHasGun(pSoldier) || pSoldier->aiData.bNeutral) && pOpponent->IsEmptyVehicle() ||
		gTacticalStatus.bBoxingState == BOXING && pSoldier->IsBoxer() && !pOpponent->IsBoxer() ||
		pOpponent->ubBodyType == CROW )
	{
		return FALSE;
	}

	return TRUE;
}

BOOLEAN ValidTeamOpponent( INT8 bTeam, SOLDIERTYPE* pOpponent )
{
	if( bTeam >= MAXTEAMS || !pOpponent )
	{
		return FALSE;
	}

	if( !pOpponent->bActive ||
		!pOpponent->bInSector ||
		pOpponent->stats.bLife <= 0 ||
		CONSIDERED_NEUTRAL_TEAM( bTeam, pOpponent ) || 
		gTacticalStatus.Team[bTeam].bSide == pOpponent->bSide ||
		pOpponent->IsEmptyVehicle() )
	{
		return FALSE;
	}

	return TRUE;
}

BOOLEAN PersonalSeenKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID )
{
	if( !pSoldier || ubOpponentID == NOBODY )
	{
		return NEVER_SEEN;
	}

	return gbSeenOpponents[pSoldier->ubID][ubOpponentID];
}

BOOLEAN CheckFocusSpot(SOLDIERTYPE *pSoldier, INT32 sTargetSpot, INT8 bTargetLevel)
{
	CHECKF(pSoldier);
	CHECKF(!TileIsOutOfBounds(sTargetSpot));

	INT16 sDistance;
	INT16 sDistanceScope;
	INT16 sDistanceNoScope;
	INT32 iSightScope;

	sDistance = PythSpacesAway(pSoldier->sGridNo, sTargetSpot);

	gbForceWeaponNotReady = true;
	sDistanceNoScope = pSoldier->GetMaxDistanceVisible(sTargetSpot, bTargetLevel, CALC_FROM_ALL_DIRS);
	gbForceWeaponNotReady = false;

	gbForceWeaponReady = true;
	sDistanceScope = pSoldier->GetMaxDistanceVisible(sTargetSpot, bTargetLevel, CALC_FROM_ALL_DIRS);
	gbForceWeaponReady = false;

	if (sDistanceNoScope < sDistanceScope &&
		sDistanceScope >= sDistance &&
		pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, sTargetSpot))
	{
		gbForceWeaponReady = true;
		iSightScope = SoldierToVirtualSoldierLineOfSightTest(pSoldier, sTargetSpot, bTargetLevel, ANIM_STAND, TRUE, CALC_FROM_ALL_DIRS);
		gbForceWeaponReady = false;

		if (iSightScope)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN FindRandomFocusSpot(SOLDIERTYPE *pSoldier, INT32 &sFocusSpot, INT8 &bFocusLevel, BOOLEAN fCover)
{
	CHECKF(pSoldier);

	sFocusSpot = NOWHERE;
	bFocusLevel = 0;

	if (!WeaponReady(pSoldier) ||
		!AIGunInHandScoped(pSoldier))
	{
		return FALSE;
	}

	std::vector<std::pair<INT32, INT8>> focus_vector;

	// check all tiles we can focus
	for (INT32 cnt = 0; cnt < WORLD_MAX; cnt++)
	{
		if (!TileIsOutOfBounds(cnt) &&
			GridNoOnVisibleWorldTile(cnt) &&
			PythSpacesAway(pSoldier->sGridNo, cnt) >= (INT16)(TACTICAL_RANGE / 4) &&
			pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, cnt) &&
			pSoldier->bActionPoints >= GetAPsToFocus(pSoldier, cnt))
		{
			for (INT8 bLevel = 0; bLevel < 2; bLevel++)
			{
				if ((!fCover || bLevel > 0 && RoofEdge(cnt) || AnyCoverFromSpot(cnt, bLevel, pSoldier->sGridNo, pSoldier->pathing.bLevel)) &&
					CheckFocusSpot(pSoldier, cnt, bLevel) &&
					pSoldier->CanFocus(cnt, bLevel))
				{
					focus_vector.push_back(std::make_pair(cnt, bLevel));
				}
			}
		}
	}

	UINT32 vector_size = focus_vector.size();
	if (vector_size > 0)
	{
		UINT32 random_spot = Random(vector_size);
		sFocusSpot = focus_vector[random_spot].first;
		bFocusLevel = focus_vector[random_spot].second;

		return TRUE;
	}

	return FALSE;
}

BOOLEAN FindCombatFocusSpot(SOLDIERTYPE *pSoldier, INT32 &sFocusSpot, INT8 &bFocusLevel)
{
	CHECKF(pSoldier);

	sFocusSpot = NOWHERE;
	bFocusLevel = 0;

	if (!WeaponReady(pSoldier) ||
		!AIGunInHandScoped(pSoldier))
	{
		return FALSE;
	}

	std::vector<std::pair<INT32, INT8>> focus_vector;

	// check all known opponents
	// look through this man's personal & public opplists for opponents known
	SOLDIERTYPE *pOpponent;
	INT32 sGridNo;
	INT8 bLevel;

	for (UINT32 uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// if this opponent is unknown personally and publicly
		if (Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sGridNo = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);

		if (!TileIsOutOfBounds(sGridNo) &&
			GridNoOnVisibleWorldTile(sGridNo) &&
			PythSpacesAway(pSoldier->sGridNo, sGridNo) >= (INT16)(TACTICAL_RANGE / 4) &&
			pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, sGridNo) &&
			Knowledge(pSoldier, pOpponent->ubID) != SEEN_CURRENTLY &&
			pSoldier->bActionPoints >= GetAPsToFocus(pSoldier, sGridNo))
		{
			if (CheckFocusSpot(pSoldier, sGridNo, bLevel) &&
				pSoldier->CanFocus(sGridNo, bLevel))
			{
				focus_vector.push_back(std::make_pair(sGridNo, bLevel));
			}
		}
	}

	// check all watched locations
	for (UINT8 ubWatchLoc = 0; ubWatchLoc < NUM_WATCHED_LOCS; ubWatchLoc++)
	{
		sGridNo = gsWatchedLoc[pSoldier->ubID][ubWatchLoc];
		bLevel = gbWatchedLocLevel[pSoldier->ubID][ubWatchLoc];

		if (!TileIsOutOfBounds(sGridNo) &&
			GridNoOnVisibleWorldTile(sGridNo) &&
			PythSpacesAway(pSoldier->sGridNo, sGridNo) >= (INT16)(TACTICAL_RANGE / 4) &&
			pSoldier->ubDirection == AIDirection(pSoldier->sGridNo, sGridNo) &&
			Knowledge(pSoldier, pOpponent->ubID) != SEEN_CURRENTLY &&
			pSoldier->bActionPoints >= GetAPsToFocus(pSoldier, sGridNo))
		{
			if (CheckFocusSpot(pSoldier, sGridNo, bLevel) &&
				pSoldier->CanFocus(sGridNo, bLevel))
			{
				focus_vector.push_back(std::make_pair(sGridNo, bLevel));
			}
		}
	}

	UINT32 vector_size = focus_vector.size();
	if (vector_size > 0)
	{
		UINT32 random_spot = Random(vector_size);
		sFocusSpot = focus_vector[random_spot].first;
		bFocusLevel = focus_vector[random_spot].second;

		return TRUE;
	}

	return FALSE;
}

BOOLEAN AllowDeepWaterFlanking(SOLDIERTYPE *pSoldier)
{
	if( SoldierAI(pSoldier) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		pSoldier->aiData.bOrders == SEEKENEMY &&
		(pSoldier->aiData.bAttitude == CUNNINGSOLO || gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, ATHLETICS_NT)) &&
		pSoldier->aiData.bAlertStatus >= STATUS_RED &&
		!pSoldier->aiData.bUnderFire &&
		!GuySawEnemy(pSoldier) &&		
		AllowDeepWater(pSoldier))
	{
		return TRUE;
	}

	return FALSE;
}

UINT8 FlankingDirection(SOLDIERTYPE *pSoldier)
{
	if(!pSoldier)
	{
		return DIRECTION_IRRELEVANT;
	}

	if (!pSoldier->IsFlanking() || TileIsOutOfBounds(pSoldier->sTacticalSpot))
	{
		return DIRECTION_IRRELEVANT;
	}

	UINT8 ubDir = AIDirection(pSoldier->sGridNo, pSoldier->sTacticalSpot);

	// determine desired direction
	if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT)
	{
		return gTwoCCDirection[ubDir];
	}
	else
	{
		return gTwoCDirection[ubDir];
	}
}

BOOLEAN DangerousSpot(SOLDIERTYPE *pSoldier)
{
	if(!pSoldier)
	{
		return FALSE;
	}

	BOOLEAN fDangerousSpot = FALSE;
	BOOLEAN fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
	BOOLEAN fProneSightCoverUnlimited = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE);
	BOOLEAN fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);

	if (!fProneSightCover && !fAnyCover ||
		NightLight() && !AICheckNVG(pSoldier) && !fProneSightCoverUnlimited && !fAnyCover ||
		InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) && !fProneSightCoverUnlimited && pSoldier->aiData.bOrders != STATIONARY ||
		AICheckUnderAttack(pSoldier, pSoldier->sGridNo, TRUE))
	{
		fDangerousSpot = TRUE;
	}

	return fDangerousSpot;
}

BOOLEAN SafeSpot(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	if (!pSoldier)
	{
		return FALSE;
	}

	if(sSpot == NOWHERE)
	{
		sSpot = pSoldier->sGridNo;
	}

	INT8 bLevel = pSoldier->pathing.bLevel;
	BOOLEAN fSightCover = SightCoverAtSpot(pSoldier, sSpot, ANIM_STAND, FALSE);
	//BOOLEAN fUnlimitedSightCover = SightCoverAtSpot(pSoldier, sSpot, ANIM_STAND, TRUE);
	BOOLEAN fProneSightCover = SightCoverAtSpot(pSoldier, sSpot, ANIM_PRONE, FALSE);
	BOOLEAN fAnyCover = AnyCoverAtSpot(pSoldier, sSpot);

	if ((fSightCover || fProneSightCover && (fAnyCover || InARoom(sSpot, NULL) && bLevel == 0)) &&
		!AICheckSpotUnderAttack(pSoldier, sSpot, TRUE, TRUE) &&
		!InLightAtNight(sSpot, pSoldier->pathing.bLevel) &&
		!AICheckUnderAttack(pSoldier, sSpot, FALSE) &&
		!CorpseWarning(pSoldier, sSpot, pSoldier->pathing.bLevel))
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN SafeMedicSpot(SOLDIERTYPE *pSoldier, INT32 sSpot)
{
	if (!pSoldier)
	{
		return FALSE;
	}

	INT8 bLevel = pSoldier->pathing.bLevel;

	/*if (CorpseWarning(pSoldier, sSpot, bLevel))
	{
		return FALSE;
	}*/
	
	// check sight cover at spot
	if (!SightCoverAtSpot(pSoldier, sSpot, ANIM_STAND, FALSE) ||
		AICheckSpotUnderAttack(pSoldier, sSpot, FALSE, TRUE) ||
		InLightAtNight(sSpot, pSoldier->pathing.bLevel))
	{
		return FALSE;
	}

	UINT8 ubDirection;
	INT32 sTempGridNo;
	UINT8 ubMovementCost;

	// check sight cover around spot
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempGridNo = NewGridNo(sSpot, DirectionInc(ubDirection));

		if (sTempGridNo != sSpot)
		{
			ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

			if (ubMovementCost < TRAVELCOST_BLOCKED &&
				NewOKDestination(pSoldier, sTempGridNo, FALSE, bLevel) &&
				!SightCoverAtSpot(pSoldier, sTempGridNo, ANIM_STAND, FALSE))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOLEAN AbortFinalSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bAction, INT32 sClosestDisturbance, INT8 bDisturbanceLevel, INT32& sDangerousSpot)
{
	sDangerousSpot = NOWHERE;

	if(!pSoldier || TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	INT8 bLevel = pSoldier->pathing.bLevel;

	INT32	sOpponentGridNo;
	INT8	bOpponentLevel;
	INT32	sClosestOpponent = ClosestKnownOpponent(pSoldier, &sOpponentGridNo, &bOpponentLevel);

	if (TileIsOutOfBounds(sClosestDisturbance))
	{
		sClosestDisturbance = sClosestOpponent;
		bDisturbanceLevel = bOpponentLevel;
	}

	if (TileIsOutOfBounds(sClosestDisturbance))
	{
		return FALSE;
	}

	BOOLEAN fSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE);
	BOOLEAN fSightCoverUnlimited = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE);
	BOOLEAN fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
	BOOLEAN fProneSightCoverUnlimited = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE);
	BOOLEAN fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);
	BOOLEAN fSightCoverSpot = SightCoverAtSpot(pSoldier, sSpot, ANIM_STAND, FALSE);
	BOOLEAN fProneSightCoverSpot = SightCoverAtSpot(pSoldier, sSpot, ANIM_PRONE, FALSE);
	BOOLEAN fDangerousSpot = DangerousSpot(pSoldier);
	BOOLEAN fSafeSpot = SafeSpot(pSoldier);
	BOOLEAN fTerrainCover = TerrainCoverAtSpot(pSoldier, sSpot, pSoldier->pathing.bLevel);
	BOOLEAN fUnderAttack = AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, FALSE);
	BOOLEAN fUnderSuccessfulAttack = AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, TRUE);
	BOOLEAN fWeOutnumber = AICheckWeOutnumber(pSoldier, sClosestDisturbance);
	BOOLEAN fWeOutnumberSector = AICheckWeOutnumberSector(pSoldier);
	BOOLEAN fSectorAttack = SectorAttack(pSoldier->bTeam);
	BOOLEAN fSpecialRole = AISpecialRole(pSoldier);
	BOOLEAN fRushAttack = (pSoldier->RushAttackAdvance() || MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE));
	BOOLEAN fRetreat = pSoldier->RetreatActive();
	BOOLEAN fFlankingFriends = (CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) > 0);
	BOOLEAN fFriendsBlack = (CountFriendsBlack(pSoldier, sClosestDisturbance) > 0);
	BOOLEAN fSuccessfulAttack = AICheckSuccessfulAttack(pSoldier, TRUE);
	BOOLEAN fTakenLargeHit = pSoldier->TakenLargeHit();
	BOOLEAN fSeekEnemy = (pSoldier->aiData.bOrders == SEEKENEMY);
	BOOLEAN fCanAttackEnemy = CanAttackEnemy(pSoldier, ANIM_STAND, FALSE);
	BOOLEAN fInARoom = (InARoom(pSoldier->sGridNo, NULL) && bLevel == 0);
	BOOLEAN fInARoomSpot = (InARoom(sSpot, NULL) && bLevel == 0);
	BOOLEAN fWeAttack = WeAttack(pSoldier->bTeam);
	BOOLEAN fAggressive = AggressiveAI(pSoldier);
	INT8	bMorale = pSoldier->aiData.bAIMorale;	

	// abort seek if we see bomb
	if (FindBombNearby(pSoldier, sSpot, BOMB_DETECTION_RANGE))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found bomb! abort!"));
		return TRUE;
	}

	// abort if moving into red smoke
	if (RedSmokeDanger(sSpot, bLevel) &&
		!RedSmokeDanger(pSoldier->sGridNo, bLevel))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("moving into red smoke! abort!"));
		return TRUE;
	}

	// don't go into light at night
	if (!fRushAttack &&
		InLightAtNight(sSpot, bLevel) &&
		!InLightAtNight(pSoldier->sGridNo, bLevel) &&
		!InSmoke(sSpot, bLevel, FALSE) &&
		(fFlankingFriends || !fSeekEnemy) &&
		(!fFriendsBlack || !fSeekEnemy) &&
		(CorpseWarning(pSoldier, sSpot, bLevel) > 0 || !fSightCoverSpot))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("in light at night! abort!"));
		sDangerousSpot = sSpot;
		return TRUE;
	}

	// abort seeking when soldier sees fresh corpse
	if (!fRushAttack &&
		CorpseWarning(pSoldier, sSpot, bLevel) &&
		!InSmoke(sSpot, bLevel, FALSE) &&
		!fFriendsBlack &&
		(!fSeekEnemy || !fWeAttack || !fWeOutnumberSector || fSpecialRole || fInARoomSpot || bMorale < MORALE_FEARLESS) &&
		(fFlankingFriends || !fSeekEnemy || fInARoomSpot || EnemyCanAttackSpot(pSoldier, sSpot, bLevel)))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("fresh corpse! abort!"));

		if (!fSightCoverSpot || fInARoomSpot)
		{
			sDangerousSpot = sSpot;
		}
		return TRUE;
	}	

	// check if enemy can attack spot
	if (!fRushAttack &&
		//!fAggressive &&
		!InSmoke(sSpot, bLevel, FALSE) &&
		pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
		(!fSeekEnemy || fFlankingFriends) &&
		!TileIsOutOfBounds(sClosestDisturbance) &&
		PythSpacesAway(sSpot, sClosestDisturbance) <= (INT16)MAX_VISION_RANGE &&
		fSafeSpot &&
		!fFriendsBlack &&
		(!fSeekEnemy || !fWeAttack || !fWeOutnumberSector || fSpecialRole || fInARoomSpot) &&
		!fSightCoverSpot &&
		EnemyCanAttackSpot(pSoldier, sSpot, bLevel))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("enemy can attack spot! abort!"));
		return TRUE;
	}

	// abort if moving out of sight cover and can attack enemy
	if (!fRushAttack &&
		bAction == AI_ACTION_SEEK_OPPONENT &&
		!TileIsOutOfBounds(sClosestDisturbance) &&
		PythSpacesAway(sSpot, sClosestDisturbance) <= (INT16)MAX_VISION_RANGE &&
		//fSightCover &&
		fSafeSpot &&
		(AIRoleSniper(pSoldier) || AIRoleMachinegunner(pSoldier) || AIRoleMarksman(pSoldier) || AIRoleGLOperator(pSoldier) || AIRoleMortarOperator(pSoldier)) &&
		fCanAttackEnemy &&
		!fSightCoverSpot &&
		!(fProneSightCoverSpot && AnyCoverAtSpot(pSoldier, sSpot)))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("moving out of sight cover, can attack enemy! abort!"));
		return TRUE;
	}

	return FALSE;
}

// needs prepared path before calling this function
BOOLEAN AbortPath(SOLDIERTYPE *pSoldier, INT8 bAction, INT32 sClosestDisturbance, INT8 bDisturbanceLevel, INT32& sDangerousSpot, INT32 &sLastSafeSpot)
{
	sDangerousSpot = NOWHERE;
	sLastSafeSpot = NOWHERE;

	if (!pSoldier)
	{
		return FALSE;
	}

	INT32	sOpponentGridNo;
	INT8	bOpponentLevel;
	INT32	sClosestOpponent = ClosestKnownOpponent(pSoldier, &sOpponentGridNo, &bOpponentLevel);

	if (TileIsOutOfBounds(sClosestDisturbance))
	{
		sClosestDisturbance = sClosestOpponent;
		bDisturbanceLevel = bOpponentLevel;
	}

	if(TileIsOutOfBounds(sClosestDisturbance))
	{
		return FALSE;
	}

	INT8 bLevel = pSoldier->pathing.bLevel;
	BOOLEAN fSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE);
	BOOLEAN fSightCoverUnlimited = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE);
	BOOLEAN fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
	BOOLEAN fProneSightCoverUnlimited = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, TRUE);

	BOOLEAN fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);
	BOOLEAN fDangerousSpot = DangerousSpot(pSoldier);
	BOOLEAN fSafeSpot = SafeSpot(pSoldier);

	BOOLEAN fFlankingFriends = (CountFriendsFlankSameSpot(pSoldier, sClosestDisturbance) > 0);
	BOOLEAN fFriendsBlack = (CountFriendsBlack(pSoldier, sClosestDisturbance) > 0);
	BOOLEAN fSuccessfulAttack = AICheckSuccessfulAttack(pSoldier, TRUE);
	BOOLEAN fSeekEnemy = (pSoldier->aiData.bOrders == SEEKENEMY);
	BOOLEAN fInARoom = (InARoom(pSoldier->sGridNo, NULL) && bLevel == 0);
	BOOLEAN fWeOutnumberSector = AICheckWeOutnumberSector(pSoldier);
	BOOLEAN fSectorAttack = SectorAttack(pSoldier->bTeam);
	BOOLEAN fSpecialRole = AISpecialRole(pSoldier);
	BOOLEAN fWeAttack = WeAttack(pSoldier->bTeam);
	INT8	bMorale = pSoldier->aiData.bAIMorale;
	BOOLEAN fRushAttack = (pSoldier->RushAttackAdvance() || MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE));
	BOOLEAN fAggressive = AggressiveAI(pSoldier);

	INT16 sLoop;
	INT32 sCheckGridNo = pSoldier->sGridNo;	

	for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
	{
		sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

		// sevenfm: don't check fences
		if (IsJumpableFencePresentAtGridNo(sCheckGridNo))
		{
			continue;
		}

		// don't go into light at night (includes smoke check)
		if (!fRushAttack &&
			InLightAtNight(sCheckGridNo, bLevel) &&
			!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
			!InSmoke(sCheckGridNo, bLevel, FALSE) &&
			(fFlankingFriends || !fSeekEnemy) &&
			(!fFriendsBlack || !fSeekEnemy) &&
			(CorpseWarning(pSoldier, sCheckGridNo, bLevel) > 0 || !SightCoverAtSpot(pSoldier, sCheckGridNo, ANIM_STAND, FALSE)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("in light at night! abort!"));
			sDangerousSpot = sCheckGridNo;
			return TRUE;
		}

		// check for fresh corpses
		if (!fRushAttack &&
			CorpseWarning(pSoldier, sCheckGridNo, bLevel) &&
			!InSmoke(sCheckGridNo, pSoldier->pathing.bLevel, FALSE) &&
			!fFriendsBlack &&
			(!fSeekEnemy || !fWeAttack || !fWeOutnumberSector || fSpecialRole || InARoom(sCheckGridNo, NULL) && bLevel == 0 || bMorale < MORALE_FEARLESS) &&
			(fFlankingFriends || !fSeekEnemy || InARoom(sCheckGridNo, NULL) && bLevel == 0 || EnemyCanAttackSpot(pSoldier, sCheckGridNo, bLevel)))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("fresh corpse! abort!"));

			if (!SightCoverAtSpot(pSoldier, sCheckGridNo, ANIM_STAND, FALSE) || InARoom(sCheckGridNo, NULL) && bLevel == 0)
			{
				sDangerousSpot = sCheckGridNo;
			}
			return TRUE;
		}

		// check if enemy can attack spot
		if (!fRushAttack &&
			//!fAggressive &&
			!InSmoke(sCheckGridNo, pSoldier->pathing.bLevel, FALSE) &&
			pSoldier->aiData.bAIMorale < MORALE_FEARLESS &&
			(!fSeekEnemy || fFlankingFriends) &&
			!TileIsOutOfBounds(sClosestDisturbance) &&
			PythSpacesAway(sCheckGridNo, sClosestDisturbance) <= (INT16)MAX_VISION_RANGE &&
			fSafeSpot &&
			!fFriendsBlack &&
			(!fSeekEnemy || !fWeAttack || !fWeOutnumberSector || fSpecialRole || bMorale < MORALE_FEARLESS || InARoom(sCheckGridNo, NULL) && bLevel == 0) &&
			!SightCoverAtSpot(pSoldier, sCheckGridNo, ANIM_STAND, FALSE) &&
			EnemyCanAttackSpot(pSoldier, sCheckGridNo, bLevel))
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("enemy can attack spot! abort!"));
			return TRUE;
		}

		if (SafeSpot(pSoldier, sCheckGridNo))
		{
			sLastSafeSpot = sCheckGridNo;
		}		
	}

	return FALSE;
}

UINT8 CountPathCorpses(SOLDIERTYPE *pSoldier)
{
	UINT8 ubCorpses = 0;

	if (!pSoldier)
	{
		return 0;
	}

	INT16 sLoop;
	INT32 sCheckGridNo = pSoldier->sGridNo;

	for (sLoop = pSoldier->pathing.usPathIndex; sLoop < pSoldier->pathing.usPathDataSize; sLoop++)
	{
		sCheckGridNo = NewGridNo(sCheckGridNo, DirectionInc((UINT8)(pSoldier->pathing.usPathingData[sLoop])));

		// check for fresh corpses
		if (CorpseWarning(pSoldier, sCheckGridNo, pSoldier->pathing.bLevel))
		{
			ubCorpses++;
		}
	}

	return ubCorpses;
}

BOOLEAN CanRushAttack(SOLDIERTYPE *pSoldier)
{
	if (!pSoldier)
	{
		return FALSE;
	}

	if (pSoldier->bTeam != ENEMY_TEAM ||
		!pSoldier->IsMercBodyType() ||
		pSoldier->IsCowering() ||
		pSoldier->bCollapsed ||
		pSoldier->bBreathCollapsed ||
		pSoldier->stats.bLife < pSoldier->stats.bLifeMax / 2 ||
		pSoldier->bBreath < 25 ||
		pSoldier->ShockLevelPercent() > 25 ||
		pSoldier->bTacticalOrder != TACTICAL_ORDER_NONE ||
		pSoldier->aiData.bOrders == STATIONARY ||
		pSoldier->aiData.bOrders == ONGUARD ||
		pSoldier->aiData.bAttitude == DEFENSIVE ||
		InARoom(pSoldier->sGridNo, NULL) && pSoldier->pathing.bLevel == 0 && pSoldier->aiData.bOrders != SEEKENEMY ||
		AISpecialRole(pSoldier) ||
		pSoldier->IsFlanking())
	{
		return FALSE;
	}

	return TRUE;
}

UINT8 CountCanRushAttackFriends(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, INT16 sMaxPathAP, BOOLEAN fCheckStopSeek, BOOLEAN fCheckBlack)
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;
	INT16 iPathCost;
	BOOLEAN fClimbingNecessary;
	INT32 sClimbGridNo;

	// safety check
	if (!pSoldier || TileIsOutOfBounds(sSpot))
		return 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->pathing.bLevel == pSoldier->pathing.bLevel &&
			CanRushAttack(pFriend) &&
			(!fCheckStopSeek || pFriend->StopSeekValue() > 0) &&
			(!fCheckBlack || pFriend->aiData.bOppCnt > 0 || GuySawEnemy(pFriend, SEEN_LAST_TURN)) &&
			PythSpacesAway(sSpot, pFriend->sGridNo) <= sDistance)
		{
			// possibly check path
			iPathCost = 0;
			if (sMaxPathAP > 0)
			{
				iPathCost = EstimatePathCostToLocation(pSoldier, pFriend->sGridNo, pFriend->pathing.bLevel, TRUE, &fClimbingNecessary, &sClimbGridNo);
			}
			
			if(iPathCost <= sMaxPathAP)
			{
				ubFriendCount++;
			}			
		}
	}

	return ubFriendCount;
}

UINT8 CountCanRushAttackTeam(INT8 bTeam)
{
	SOLDIERTYPE * pFriend;
	UINT8 ubFriendCount = 0;
	//	INT16 iPathCost;
	//	BOOLEAN fClimbingNecessary;
	//	INT32 sClimbGridNo;

	// safety check
	if (bTeam < 0 || bTeam >= MAXTEAMS)
		return 0;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[bTeam].bFirstID; iCounter <= gTacticalStatus.Team[bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->pathing.bLevel == 0 &&
			CanRushAttack(pFriend))
		{
			ubFriendCount++;
		}
	}

	return ubFriendCount;
}

void StartRushAttackFriends(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, INT16 sMaxPathAP)
{
	SOLDIERTYPE * pFriend;
	INT16 iPathCost;
	BOOLEAN fClimbingNecessary;
	INT32 sClimbGridNo;

	// safety check
	if (!pSoldier || TileIsOutOfBounds(sSpot))
		return;

	// Run through each friendly.
	for (UINT8 iCounter = gTacticalStatus.Team[pSoldier->bTeam].bFirstID; iCounter <= gTacticalStatus.Team[pSoldier->bTeam].bLastID; iCounter++)
	{
		pFriend = MercPtrs[iCounter];

		if (pFriend &&
			pFriend != pSoldier &&
			pFriend->bActive &&
			pFriend->stats.bLife >= OKLIFE &&
			pFriend->pathing.bLevel == pSoldier->pathing.bLevel &&
			CanRushAttack(pFriend) &&
			PythSpacesAway(sSpot, pFriend->sGridNo) <= sDistance)
		{
			// possibly check path
			iPathCost = 0;
			if (sMaxPathAP > 0)
			{
				iPathCost = EstimatePathCostToLocation(pSoldier, pFriend->sGridNo, pFriend->pathing.bLevel, TRUE, &fClimbingNecessary, &sClimbGridNo);
			}

			if (iPathCost <= sMaxPathAP)
			{
				pFriend->RushAttackStart();
			}
		}
	}
}

BOOLEAN AICheckInterrupt(void)
{
	if (gTacticalStatus.ubTopMessageType == COMPUTER_INTERRUPT_MESSAGE ||
		gTacticalStatus.ubTopMessageType == PLAYER_INTERRUPT_MESSAGE ||
		gTacticalStatus.ubTopMessageType == MILITIA_INTERRUPT_MESSAGE)
	{
		return TRUE;
	}

	return FALSE;
}

UINT8 SectorBuildingPercent(void)
{
	INT32 iRoomTiles = 0;
	UINT8 ubPercent;

	//Loop though all the map elements
	for (INT32 cnt = 0; cnt < WORLD_MAX; cnt++)
	{
		if(!TileIsOutOfBounds(cnt) && GridNoOnVisibleWorldTile(cnt) && InARoom(cnt, NULL))
		{
			iRoomTiles++;
		}
	}

	ubPercent = 100 * iRoomTiles / WORLD_MAX;

	return ubPercent;
}

INT32 RoomSize(INT32 sSpot)
{
	UINT16 usSpotRoomNo, usRoomNo;
	INT32 iRoomSize = 0;
	
	if(TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	if (!InARoom(sSpot, &usSpotRoomNo))
	{
		return 0;
	}

	//Loop though all the map elements
	for (INT32 cnt = 0; cnt < WORLD_MAX; cnt++)
	{
		if (!TileIsOutOfBounds(cnt) && GridNoOnVisibleWorldTile(cnt) && InARoom(cnt, &usRoomNo) && usRoomNo == usSpotRoomNo)
		{
			iRoomSize++;
		}
	}

	return iRoomSize;
}


INT32 BuildingSize(INT32 sSpot)
{
	INT32 iBuildingSize = 0;

	if (TileIsOutOfBounds(sSpot))
	{
		return 0;
	}

	if (gubBuildingInfo[sSpot] == NO_BUILDING)
	{
		return 0;
	}

	//Loop though all the map elements
	for (INT32 cnt = 0; cnt < WORLD_MAX; cnt++)
	{
		if (!TileIsOutOfBounds(cnt) && 
			GridNoOnVisibleWorldTile(cnt) && 
			gubBuildingInfo[cnt] != NO_BUILDING &&
			gubBuildingInfo[cnt] == gubBuildingInfo[sSpot])
		{
			iBuildingSize++;
		}
	}

	return iBuildingSize;
}

BOOLEAN SameRoom(INT32 sSpot1, INT32 sSpot2)
{
	if (RoomNo(sSpot1) == RoomNo(sSpot2))
		return TRUE;

	return FALSE;
}

UINT16 RoomNo(INT32 sSpot)
{
	if (TileIsOutOfBounds(sSpot))
		return NO_ROOM;

	return gusWorldRoomInfo[sSpot];
}


BOOLEAN CorpseEnemyTeam(ROTTING_CORPSE *pCorpse)
{
	CHECKF(pCorpse);

	// check whether corpse has soldier's uniform
	for (UINT8 i = UNIFORM_ENEMY_ADMIN; i <= UNIFORM_ENEMY_ELITE; ++i)
	{
		if (COMPARE_PALETTEREP_ID(pCorpse->def.VestPal, gUniformColors[i].vest) && COMPARE_PALETTEREP_ID(pCorpse->def.PantsPal, gUniformColors[i].pants))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN CorpseMilitiaTeam(ROTTING_CORPSE *pCorpse)
{
	CHECKF(pCorpse);

	// check whether corpse has soldier's uniform
	for (UINT8 i = UNIFORM_MILITIA_ROOKIE; i <= UNIFORM_MILITIA_ELITE; ++i)
	{
		if (COMPARE_PALETTEREP_ID(pCorpse->def.VestPal, gUniformColors[i].vest) && COMPARE_PALETTEREP_ID(pCorpse->def.PantsPal, gUniformColors[i].pants))
		{
			return TRUE;
		}
	}

	return FALSE;
}

UINT8 SpotDangerLevel(SOLDIERTYPE *pSoldier, INT32 sGridNo)
{
	if (!pSoldier)
		return 0;

	if (TileIsOutOfBounds(sGridNo))
		return 0;

	BOOLEAN fAlerted = FALSE;
	BOOLEAN fGreen = FALSE;
	BOOLEAN fProfile = FALSE;
	BOOLEAN fNeutral = FALSE;
	UINT8 ubLevel = 0;	

	if (pSoldier->aiData.bAlertStatus < STATUS_YELLOW)
		fGreen = TRUE;

	if (pSoldier->ubProfile != NO_PROFILE)
		fProfile = TRUE;

	if (pSoldier->aiData.bNeutral)
		fNeutral = TRUE;

	if ((pSoldier->aiData.bAlertStatus >= STATUS_RED || pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE))
		fAlerted = TRUE;	

	if (!fProfile && !fNeutral && NorthSpot(sGridNo, pSoldier->pathing.bLevel) ||
		!fProfile && fGreen && CheckDoorNearGridno(sGridNo) ||
		Water(sGridNo, pSoldier->pathing.bLevel) && !pSoldier->IsFlanking() ||
		CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel) > 1)		// sevenfm: count only fresh corpses
		ubLevel = 1;

	if (fAlerted && !fNeutral && 
		(InLightAtNight(sGridNo, pSoldier->pathing.bLevel) || FindNearbyExplosiveStructure(sGridNo, pSoldier->pathing.bLevel)))
		ubLevel = 2;

	// sevenfm: zombies avoid light, bloodcats avoid light when alerted
	if ((pSoldier->IsZombie() || pSoldier->IsBloodcat() && pSoldier->aiData.bAlertStatus >= STATUS_RED) && InLightAtNight(sGridNo, pSoldier->pathing.bLevel))
		ubLevel = 2;

	if (DeepWater(sGridNo, pSoldier->pathing.bLevel) && !pSoldier->IsFlanking() ||
		RedSmokeDanger(sGridNo, pSoldier->pathing.bLevel))
		ubLevel = 3;

	if (InGas(pSoldier, sGridNo) ||
		FindBombNearby(pSoldier, sGridNo, BOMB_DETECTION_RANGE))
		ubLevel = 4;

	return ubLevel;
}

BOOLEAN CheckDangerousDirection(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel)
{
	CHECKF(pSoldier);
	CHECKF(!TileIsOutOfBounds(sSpot));

	UINT32		uiLoop;
	SOLDIERTYPE *pOpponent;
	INT32		sThreatLoc;
	INT8		bThreatLevel;
	UINT16		usSightLimit;
	UINT16		usAdjustedSight;
	INT8		bKnowledge;
	INT16		sSightAdjustment;
	UINT8		ubDirection;

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[uiLoop];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || pOpponent->stats.bLife < OKLIFE)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		if (pOpponent->IsUnconscious() || pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);

		// if this opponent is unknown personally and publicly
		if (bKnowledge == NOT_HEARD_OR_SEEN)
		{
			continue;
		}

		// obtain opponent's location and level
		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);
		bThreatLevel = KnownLevel(pSoldier, pOpponent->ubID);

		// check that our knowledge is correct
		if (TileIsOutOfBounds(sThreatLoc))
		{
			continue;
		}

		//sSightAdjustment = GetSightAdjustment(pOpponent, pSoldier, sSpot, pSoldier->pathing.bLevel, ANIM_STAND);
		sSightAdjustment = GetSightAdjustment(pOpponent, sSpot, pSoldier->pathing.bLevel);

		gbForceWeaponNotReady = true;
		usSightLimit = pOpponent->GetMaxDistanceVisible(sSpot, pSoldier->pathing.bLevel, CALC_FROM_ALL_DIRS);
		gbForceWeaponNotReady = false;

		usAdjustedSight = max(min(1, usSightLimit), usSightLimit + usSightLimit * sSightAdjustment / 100);
		ubDirection = AIDirection(sThreatLoc, sSpot);

		if (PythSpacesAway(sSpot, sThreatLoc) <= usAdjustedSight &&
			(CountCorpsesInDirection(pSoldier, sThreatLoc, ubDirection, max(usAdjustedSight, DAY_VISION_RANGE), FALSE, TRUE) ||
			CountCorpsesInDirection(pSoldier, sThreatLoc, gOneCDirection[ubDirection], max(usAdjustedSight, DAY_VISION_RANGE), FALSE, TRUE) ||
			CountCorpsesInDirection(pSoldier, sThreatLoc, gOneCCDirection[ubDirection], max(usAdjustedSight, DAY_VISION_RANGE), FALSE, TRUE)) &&
			!AnyCoverFromSpot(sSpot, bLevel, sThreatLoc, bThreatLevel))
			//LocationToLocationLineOfSightTest(sThreatLoc, bThreatLevel, sSpot, bLevel, TRUE, usAdjustedSight, STANDING_LOS_POS, STANDING_LOS_POS))
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN FindFenceAroundSpot(INT32 sSpot)
{
	if (TileIsOutOfBounds(sSpot))
	{
		return FALSE;
	}

	UINT8 ubDirection;
	INT32 sTempSpot;

	// check adjacent locations
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

		if (sTempSpot != sSpot && (IsCuttableWireFenceAtGridNo(sTempSpot) || FindConcertina(sTempSpot)))
		{
			return TRUE;
		}
	}

	return FALSE;
}
