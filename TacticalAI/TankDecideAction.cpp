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
//extern void IncrementWatchedLoc( UINT8 ubID, INT32 sGridNo, INT8 bLevel, BOOLEAN fCommunicate );
extern void BeginMultiPurposeLocator( INT32 sGridNo, INT8 bLevel, BOOLEAN fSlideTo );

extern STR8 gStr8AlertStatus[];
extern STR8 gStr8Attitude[];
extern STR8 gStr8Orders[];
extern STR8 gStr8Team[];
extern STR8 gStr8Class[];

extern STR szAction[];

INT8 TankDecideActionGreen(SOLDIERTYPE *pSoldier)
{
	DebugMsg(TOPIC_JA2, DBG_LEVEL_3,String("TankDecideActionGreen, orders = %d",pSoldier->aiData.bOrders));
	
	DebugAI( AI_MSG_START, pSoldier, String("[Green]"));
	LogDecideInfo(pSoldier);

	////////////////////////////////////////////////////////////////////////////
	// Init Data
	////////////////////////////////////////////////////////////////////////////
	pSoldier->bStealthMode = FALSE;
	pSoldier->bReverse = FALSE;
	pSoldier->bWeaponMode = WM_NORMAL;
	pSoldier->StopSeekReset();
	////////////////////////////////////////////////////////////////////////////

	DebugAI(AI_MSG_INFO, pSoldier, String("tank: nothing to do in green mode"));
	return AI_ACTION_NONE;
}

INT8 TankDecideActionYellow(SOLDIERTYPE *pSoldier)
{
	INT32 iChance;
	INT8  bInGas;
	INT32 sNoiseGridNo;
	UINT8 ubNoiseDir;

	DebugAI( AI_MSG_START, pSoldier, String("[Yellow]"));
	LogDecideInfo(pSoldier);

	////////////////////////////////////////////////////////////////////////////
	// Init Data
	////////////////////////////////////////////////////////////////////////////
	pSoldier->bStealthMode = FALSE;
	pSoldier->bReverse = FALSE;
	pSoldier->bWeaponMode = WM_NORMAL;
	pSoldier->StopSeekReset();
	////////////////////////////////////////////////////////////////////////////

	// prepare data
	bInGas = InGas( pSoldier, pSoldier->sGridNo );

	// determine the most important noise heard, and its relative value
	sNoiseGridNo = MostImportantNoiseHeard(pSoldier, NULL, NULL, NULL);

	if (TileIsOutOfBounds(sNoiseGridNo))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("tank: no noise heard, nothing to do"));
		return AI_ACTION_NONE;
	}

	////////////////////////////////////////////////////////////////////////////
	// LOOK AROUND TOWARD NOISE: determine %chance for man to turn towards noise
	////////////////////////////////////////////////////////////////////////////

	// determine direction from this soldier in which the noise lies
	ubNoiseDir = AIDirection(pSoldier->sGridNo, sNoiseGridNo);

	// if soldier is not already facing in that direction,
	// and the noise source is close enough that it could possibly be seen
	if (GetAPsToLook(pSoldier) <= pSoldier->bActionPoints)
	{
		if (pSoldier->ubDirection != ubNoiseDir && 
			PythSpacesAway(pSoldier->sGridNo,sNoiseGridNo) <= pSoldier->GetMaxDistanceVisible(sNoiseGridNo) )
		{
			// set base chance according to orders (STATIONARY)
			iChance = 50;

			if ((INT16)PreRandom(100) < iChance && 
				pSoldier->InternalIsValidStance( ubNoiseDir, gAnimControl[ pSoldier->usAnimState ].ubEndHeight ) )
			{
				pSoldier->aiData.usActionData = ubNoiseDir;
				return AI_ACTION_CHANGE_FACING;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// RADIO YELLOW ALERT: determine %chance to call others and report noise
	////////////////////////////////////////////////////////////////////////////

	// if we have the action points remaining to RADIO
	// (we never want NPCs to choose to radio if they would have to wait a turn)
	if (pSoldier->aiData.bAlertStatus < STATUS_RED &&
		pSoldier->RadioCallValue() == 0 &&
		IsActionAffordable(pSoldier, AI_ACTION_YELLOW_ALERT) &&
		gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1)
	{
		// base chance depends on how much new info we have to radio to the others
		iChance = 5 * WhatIKnowThatPublicDont(pSoldier,FALSE);   // use 5 * for YELLOW alert

		// CJC: this addition allows for varying difficulty levels for soldier types
		iChance += gbDiff[DIFF_RADIO_RED_ALERT][SoldierDifficultyLevel(pSoldier)] / 2;

		// STATIONARY
		iChance += 20;

		if ((INT16)PreRandom(100) < iChance)
		{
			return AI_ACTION_YELLOW_ALERT;
		}
	}

	DebugAI(AI_MSG_INFO, pSoldier, String("tank: nothing to do in yellow mode"));
	return AI_ACTION_NONE;
}


INT8 TankDecideActionRed(SOLDIERTYPE *pSoldier)
{
	INT32	iChance;
	INT32	sClosestOpponent = NOWHERE, sClosestFriend = NOWHERE;
	INT32	sClosestDisturbance = NOWHERE;
	INT8	bClosestDisturbanceLevel = 0;
	INT32	sClosestSeenThisTurnOpponent = NOWHERE, sClosestSeenThisTurnOpponentGridNo;
	INT32	sClosestPossiblySeenOpponent;
	INT8	bClosestSeenThisTurnOpponentLevel;
	UINT8	ubOpponentDir;
	INT8	bInGas, bInGasSpot;
	ATTACKTYPE	BestThrow, BestShot;

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

	INT32	sClosestThreat = NOWHERE;
	INT8	bClosestThreatLevel = 0;

	INT32	sSmokeSpot = NOWHERE;
	INT8	bSmokeLevel = 0;

	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,String("TankDecideActionRed: soldier orders = %d",pSoldier->aiData.bOrders));

	DebugAI( AI_MSG_START, pSoldier, String("[Red]"));
	LogDecideInfo(pSoldier);

	////////////////////////////////////////////////////////////////////////////
	// Init Data
	////////////////////////////////////////////////////////////////////////////
	pSoldier->bStealthMode = FALSE;
	pSoldier->bReverse = FALSE;
	pSoldier->bWeaponMode = WM_NORMAL;
	////////////////////////////////////////////////////////////////////////////

	// if we have absolutely no action points, we can't do a thing under RED!
	if ( pSoldier->bActionPoints <= 0 ) //Action points can be negative
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("No action points - nothing to do!"));
		pSoldier->aiData.usActionData = NOWHERE;
		return AI_ACTION_NONE;
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

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Find opponents]"));
	// sevenfm: find closest opponent
	sClosestOpponent = ClosestKnownOpponent(pSoldier, &sOpponentGridNo, &bOpponentLevel);
	sClosestSeenThisTurnOpponent = ClosestSeenThisTurnOpponent(pSoldier, &sClosestSeenThisTurnOpponentGridNo, &bClosestSeenThisTurnOpponentLevel);
	sClosestPossiblySeenOpponent = ClosestPossiblySeenOpponent(pSoldier, NULL, NULL);

	DebugAI(AI_MSG_INFO, pSoldier, String("sClosestOpponent %d, sClosestNoise %d, sClosestSeenThisTurnOpponent %d, sClosestPossiblySeenOpponent %d", sClosestOpponent, sClosestNoise, sClosestSeenThisTurnOpponent, sClosestPossiblySeenOpponent));

	// first, set closest threat to closest recently seen opponent
	if (!TileIsOutOfBounds(sClosestSeenThisTurnOpponent) &&
		LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestSeenThisTurnOpponent, bClosestSeenThisTurnOpponentLevel, MAX_VISION_RANGE))
	{
		sClosestThreat = sClosestSeenThisTurnOpponent;
		bClosestThreatLevel = bClosestSeenThisTurnOpponentLevel;
	}
	// if not found, use closest known opponent
	if (!TileIsOutOfBounds(sClosestOpponent) && TileIsOutOfBounds(sClosestThreat))
	{
		sClosestThreat = sClosestOpponent;
		bClosestThreatLevel = bOpponentLevel;
	}
	// if not found, use closest noise
	if (!TileIsOutOfBounds(sClosestNoise) && TileIsOutOfBounds(sClosestThreat))
	{
		sClosestThreat = sClosestNoise;
		bClosestThreatLevel = 0;
	}

	DebugAI(AI_MSG_INFO, pSoldier, String("sClosestThreat %d, bClosestThreatLevel %d", sClosestThreat, bClosestThreatLevel));

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

	DebugAI(AI_MSG_INFO, pSoldier, String("fProneSightCover %d, fAnyCover %d fCanBeSeen %d, fDangerousSpot %d", fProneSightCover, fAnyCover, fCanBeSeen, fDangerousSpot));

	bInGas = InGas( pSoldier, pSoldier->sGridNo );
	bInGasSpot = InGasSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel);

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[raise alert for others]"));
	if (pSoldier->bTeam == ENEMY_TEAM &&
		!pSoldier->RaisedAlertThisTurn() &&
		!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
		Chance(SoldierDifficultyLevel(pSoldier) * 20) &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT))
	{
		ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT]);
		return(AI_ACTION_RED_ALERT);
	}

	////////////////////////////////////////////////////////////////////////
	// IF POSSIBLE, FIRE LONG RANGE WEAPONS AT TARGETS REPORTED BY RADIO
	////////////////////////////////////////////////////////////////////////
	DebugAI( AI_MSG_TOPIC, pSoldier, String("[sniper fire/use mortar]"));

	// can't do this in realtime, because the player could be shooting a gun or whatever at the same time!
	if (gfTurnBasedAI)
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
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("throw possible"));

				if (Chance(BestThrow.ubChanceToReallyHit + TeamPercentKilled(pSoldier->bTeam) + pSoldier->aiData.bUnderFire * 25))
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

					// no changing stance for tank
					pSoldier->aiData.usActionData = BestThrow.sTarget;
					pSoldier->bTargetLevel = BestThrow.bTargetLevel;
					pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

					return AI_ACTION_TOSS_PROJECTILE;
				}
			}
			else		// toss/throw/launch not possible
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("throw not possible"));

				// WDS - Fix problem when there is no "best thrown" weapon (i.e., BestThrow.bWeaponIn == NO_SLOT)
				// if this dude has a long-range weapon on him (longer than normal
				// sight range), and there's at least one other team-mate around, and
				// spotters haven't already been called for, then DO SO!

				if ((BestThrow.bWeaponIn != NO_SLOT) &&
					(CalcMaxTossRange(pSoldier, pSoldier->inv[BestThrow.bWeaponIn].usItem, TRUE) > MaxNormalDistanceVisible()) &&
					(gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1) &&
					(gTacticalStatus.ubSpottersCalledForBy == NOBODY))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("throw not possible, call for spotters!"));

					// then call for spotters!  Uses up the rest of his turn (whatever
					// that may be), but from now on, BLACK AI NPC may radio sightings!
					gTacticalStatus.ubSpottersCalledForBy = pSoldier->ubID;

					pSoldier->aiData.usActionData = NOWHERE;
					return AI_ACTION_NONE;
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

					// if this dude has a long-range weapon on him (longer than normal
					// sight range), and there's at least one other team-mate around, and
					// spotters haven't already been called for, then DO SO!

					if ((gTacticalStatus.Team[pSoldier->bTeam].bMenInSector > 1) &&
						gTacticalStatus.ubSpottersCalledForBy == NOBODY &&
						IsActionAffordable(pSoldier, AI_ACTION_FAKE_ALERT))
					{
						// then call for spotters!  Uses up the rest of his turn (whatever that may be), but from now on, BLACK AI NPC may radio sightings!
						DebugAI(AI_MSG_INFO, pSoldier, String("calling for spotters"));
						gTacticalStatus.ubSpottersCalledForBy = pSoldier->ubID;
						pSoldier->aiData.usActionData = NOWHERE;
						return AI_ACTION_NONE;
					}
				}

				//SUPPRESSION FIRE
				DebugAI(AI_MSG_TOPIC, pSoldier, String("[suppression fire]"));

				// suppression fire
				if (BestShot.ubPossible &&
					BestShot.bWeaponIn != NO_SLOT &&
					!TileIsOutOfBounds(BestShot.sTarget) &&
					// check valid opponent for suppression
					BestShot.ubOpponent != NOBODY &&
					MercPtrs[BestShot.ubOpponent] &&
					// check soldier and weapon
					(Chance(BestShot.ubChanceToReallyHit) || Chance(AISuppressionEffectiveness())) &&
					(Chance(BestShot.ubChanceToReallyHit) || Chance(gGameOptions.ubDifficultyLevel * 20) || Chance((100 + TeamPercentKilled(pSoldier->bTeam)) / 2)) &&
					IsGunAutofireCapable(&pSoldier->inv[BestShot.bWeaponIn]) &&
					// avoid friendly fire
					BestShot.ubFriendlyFireChance <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE &&
					// shoot through wall check
					(BestShot.ubChanceToReallyHit >= 1 ||
					LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS) ||									// can see opponent after turning
					//LOS_Virtual(pSoldier, BestShot.sTarget, BestShot.bTargetLevel, ANIM_STAND, SIGHT_NORMAL, NO_DISTANCE_LIMIT) ||			// can suppress opponent
					LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, BestShot.sTarget, BestShot.bTargetLevel, NO_DISTANCE_LIMIT))		// have standing LOS
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
					pSoldier->aiData.bAimTime = BestShot.ubAimTime;
					pSoldier->bDoAutofire = 0;
					pSoldier->bDoBurst = 1;
					pSoldier->bScopeMode = BestShot.bScopeMode;
					pSoldier->bAimShotLocation = AIM_SHOT_RANDOM;
					// new code by Asdow
					OBJECTTYPE &weapon = pSoldier->inv[BestShot.bWeaponIn];
					UINT16 ubRemainingAmmo = weapon[0]->data.gun.ubGunShotsLeft;

					INT16 ubBurstAPs = 0;
					FLOAT dTotalRecoil = 0;
					INT32 sActualAimAP;
					UINT8 ubAutoPenalty;
					UINT8 ubMinAuto = 5;
					INT16 sReserveAP = 0;

					INT8 bAmmoSlot = FindAmmoToReload(pSoldier, HANDPOS, NO_SLOT);

					if (BestShot.ubAimTime > 0 &&
						!UsingNewCTHSystem() &&
						Chance((100 - BestShot.ubChanceToReallyHit) * (100 - BestShot.ubChanceToReallyHit) / 100))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("set ubAimTime = 0 for OCTH suppression"));
						BestShot.ubAimTime = 0;
					}

					if (PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) > DAY_VISION_RANGE || pSoldier->aiData.bUnderFire)
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
							pSoldier->bDoAutofire <= 30);
							//(dTotalRecoil <= 20.0f ||
							//pSoldier->bDoAutofire < ubMinAuto);
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
							pSoldier->bDoAutofire <= 30);
							//(ubAutoPenalty * pSoldier->bDoAutofire <= 80 || 
							//pSoldier->bDoAutofire < ubMinAuto || 
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
								SoldierToSoldierBodyPartChanceToGetThrough(pSoldier, pTargetSoldier, AIM_SHOT_HEAD) > OK_CHANCE_TO_GET_THROUGH)
								pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
						}

						pSoldier->aiData.usActionData = BestShot.sTarget;
						// show "suppression fire" message only if opponent cannot be seen after turning
						if (!LOS_Soldier(pSoldier, MercPtrs[BestShot.ubOpponent], LOS_READY, CALC_FROM_ALL_DIRS))
							ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[MSG113_SUPPRESSIONFIRE]);

						return(AI_ACTION_FIRE_GUN);
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
	}

	// sevenfm: always MORALE_FEARLESS for tanks
	pSoldier->aiData.bAIMorale = MORALE_FEARLESS;
	DebugAI( AI_MSG_INFO, pSoldier, String("AI morale %d", pSoldier->aiData.bAIMorale));


	DebugAI( AI_MSG_TOPIC, pSoldier, String("[radio RED alert]"));
	////////////////////////////////////////////////////////////////////////////
	// RADIO RED ALERT: determine %chance to call others and report contact
	////////////////////////////////////////////////////////////////////////////

	// if we're a computer merc, and we have the action points remaining to RADIO
	// (we never want NPCs to choose to radio if they would have to wait a turn)
	if (!pSoldier->RaisedAlertThisTurn() &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT) &&
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

		// if I actually know something they don't
		if (iChance)
		{
			// sevenfm: lower chance to radio if some nearby friend radioed this turn
			iChance = iChance / (1 + CountFriendsUsedRadio(pSoldier, DAY_VISION_RANGE));

			if ((INT16) PreRandom(100) < iChance)
			{
				DebugAI( AI_MSG_INFO, pSoldier, String("decided to radio red alert, iChance = %d", iChance));
				return AI_ACTION_RED_ALERT;
			}
		}
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Tank - check smoke/gas activation]"));
	////////////////////////////////////////////////////////////////////////////
	// SMOKE GRENADE ACTIVATION FOR TANK
	////////////////////////////////////////////////////////////////////////////

	UINT16 usSmokeItem = GetHandGrenadeOfType(SMOKE_GRENADE, EXPLOSV_SMOKE);

	// use smoke grenade if Tank is under attack but cannot see target
	if (pSoldier->aiData.bUnderFire &&
		pSoldier->stats.bLife < pSoldier->stats.bLifeMax - 20 &&
		!GuySawEnemy(pSoldier) &&
		//pSoldier->TakenLargeHit() &&
		usSmokeItem > 0 &&
		pSoldier->HasItem(usSmokeItem) &&
		!InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, TRUE) &&
		//!CheckSmokeEffect( pSoldier->sGridNo, pSoldier->pathing.bLevel, NORMAL_SMOKE_EFFECT ) &&
		Chance(gGameOptions.ubDifficultyLevel * 5) &&
		IsActionAffordable(pSoldier, AI_ACTION_SELFDETONATE))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("activate smoke grenade"));
		pSoldier->aiData.usActionData = usSmokeItem;
		return AI_ACTION_SELFDETONATE;
	}

	usSmokeItem = GetHandGrenadeOfType(MUSTARD_GRENADE, EXPLOSV_MUSTGAS);

	// use mustard gas if enemy soldier is very close
	if (!TileIsOutOfBounds(sClosestThreat) &&
		PythSpacesAway(pSoldier->sGridNo, sClosestThreat) <= 4 &&
		usSmokeItem > 0 &&
		pSoldier->HasItem(usSmokeItem) &&
		!CheckSmokeEffect(pSoldier->sGridNo, pSoldier->pathing.bLevel, MUSTARDGAS_SMOKE_EFFECT) &&
		Chance(gGameOptions.ubDifficultyLevel * 10) &&
		IsActionAffordable(pSoldier, AI_ACTION_SELFDETONATE))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("activate mustard grenade"));
		pSoldier->aiData.usActionData = usSmokeItem;
		return AI_ACTION_SELFDETONATE;
	}

	////////////////////////////////////////////////////////////////////////////
	// TANK AI
	////////////////////////////////////////////////////////////////////////////
	DebugAI(AI_MSG_INFO, pSoldier, String("[Tank AI]"));

	// try turning in a random direction as we still can't see anyone.
	if (GetAPsToLook(pSoldier) <= pSoldier->bActionPoints)
	{
		if (!TileIsOutOfBounds(sClosestNoise))
		{
			ubOpponentDir = AIDirection(pSoldier->sGridNo, sClosestNoise);

			if (pSoldier->ubDirection == ubOpponentDir)
			{
				ubOpponentDir = (UINT8)PreRandom(NUM_WORLD_DIRECTIONS);
			}
		}
		else
		{
			ubOpponentDir = (UINT8)PreRandom(NUM_WORLD_DIRECTIONS);
		}

		if ((pSoldier->ubDirection != ubOpponentDir))
		{
			if ((pSoldier->CheckInitialAP() || (INT16)PreRandom(100) < 60) &&
				pSoldier->InternalIsValidStance(ubOpponentDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
			{
				pSoldier->aiData.usActionData = ubOpponentDir;

				// limit turning a bit... if the last thing we did was also a turn, add a 60% chance of this being our last turn
				if (pSoldier->aiData.bLastAction == AI_ACTION_CHANGE_FACING && PreRandom(100) < 60)
				{
					if (gfTurnBasedAI)
					{
						pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
						pSoldier->aiData.usNextActionData = 0;
					}
					else
					{
						pSoldier->aiData.bNextAction = AI_ACTION_WAIT;
						pSoldier->aiData.usNextActionData = (UINT16)REALTIME_AI_DELAY;
					}
				}

				return(AI_ACTION_CHANGE_FACING);
			}
		}
	}

	// that's it for tanks
	DebugAI(AI_MSG_TOPIC, pSoldier, String("End of Tank AI"));
	return AI_ACTION_NONE;
}

INT8 TankDecideActionBlack(SOLDIERTYPE *pSoldier)
{
	INT32	iChance;
	INT32	sClosestOpponent = NOWHERE,sBestCover = NOWHERE;//dnl ch58 160813	
	INT32	sClosestDisturbance = NOWHERE;
	INT8	bClosestDisturbanceLevel = 0;
	UINT8	ubBestAttackAction = AI_ACTION_NONE;
	INT8	bCanAttack;

	DebugAI( AI_MSG_START, pSoldier, String("[Black]"));
	LogDecideInfo(pSoldier);

	ATTACKTYPE BestShot, BestThrow, BestStab ,BestAttack;//dnl ch69 150913
	INT16	ubBurstAPs;
	UINT8	ubOpponentDir;

	// sevenfm
	INT32	sClosestSeenOpponent = NOWHERE;
	INT32	sClosestPossiblySeenOpponent = NOWHERE;
	BOOLEAN fAllowCoverCheck = FALSE;
	BOOLEAN fExtraClip = FALSE;
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

	////////////////////////////////////////////////////////////////////////////
	// Init Data
	////////////////////////////////////////////////////////////////////////////
	pSoldier->StopSeekReset();
	pSoldier->bWeaponMode = WM_NORMAL;
	pSoldier->bStealthMode = FALSE;
	pSoldier->bReverse = FALSE;
	////////////////////////////////////////////////////////////////////////////

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Check if can do anything]"));
	// if we have absolutely no action points, we can't do a thing under BLACK!
	if (pSoldier->bActionPoints <= 0)
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("No action points - nothing to do"));
		pSoldier->aiData.usActionData = NOWHERE;
		return AI_ACTION_NONE;
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Possibly update knowledge]"));
	if (pSoldier->bTeam == ENEMY_TEAM && (gfTurnBasedAI || pSoldier->CheckInitialAP()))
	{
		PossiblyUpdateKnowledge(pSoldier);
	}

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Prepare data]"));
	BOOLEAN fSectorAttack = SectorAttack(pSoldier->bTeam);
	sClosestOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL);
	sClosestSeenOpponent = ClosestSeenOpponent(pSoldier, NULL, NULL);
	sClosestPossiblySeenOpponent = ClosestPossiblySeenOpponent(pSoldier, NULL, NULL);

	DebugAI(AI_MSG_TOPIC, pSoldier, String("[Check sight]"));
	fProneSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
	fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);
	fDangerousSpot = DangerousSpot(pSoldier);
	fSafeSpot = SafeSpot(pSoldier);	

	// raise alert for others
	if (pSoldier->bTeam == ENEMY_TEAM &&
		!pSoldier->RaisedAlertThisTurn() &&
		!gTacticalStatus.Team[pSoldier->bTeam].bAwareOfOpposition &&
		IsActionAffordable(pSoldier, AI_ACTION_RED_ALERT))
	{
		ScreenMsg(FONT_ORANGE, MSG_INTERFACE, New113Message[MSG113_ENEMY_ALERT]);
		return(AI_ACTION_RED_ALERT);
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[Can attack]"));
	bCanAttack = CanNPCAttack(pSoldier);

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

			if (BestShot.ubFriendlyFireChance > MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("Skip shooting!"));
				BestShot.ubPossible = FALSE;
			}
		}

		if (BestShot.ubPossible)
		{
			// now we KNOW FOR SURE that we will do something (shoot, at least)
			DebugAI(AI_MSG_INFO, pSoldier, String("Shot is possible"));
			NPCDoesAct(pSoldier);
		}

		//////////////////////////////////////////////////////////////////////////
		// THROW A TOSSABLE ITEM AT OPPONENT(S)
		//////////////////////////////////////////////////////////////////////////

		DebugAI( AI_MSG_TOPIC, pSoldier, String("[Throw]"));

		// this looks for throwables, and sets BestThrow.ubPossible if it can be done
		CheckIfTossPossible(pSoldier,&BestThrow);

		if (BestThrow.ubPossible)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("Throw is possible"));

			if ( BestThrow.ubPossible )
			{
				// now we KNOW FOR SURE that we will do something (throw, at least)
				NPCDoesAct(pSoldier);
			}
		}

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
			BestThrow.ubPossible)
		{
			BestShot.ubPossible = FALSE;
		}

		if (BestShot.ubPossible)
		{
			BestAttack.iAttackValue = BestShot.iAttackValue;
			ubBestAttackAction = AI_ACTION_FIRE_GUN;
			DebugAI( AI_MSG_INFO, pSoldier, String("best action = fire gun, iAttackValue = %d", BestAttack.iAttackValue));	

			// reduce chance for throw when shot is possible
			if (BestThrow.ubPossible)
			{
				BestThrow.iAttackValue /= 2;
				DebugAI(AI_MSG_INFO, pSoldier, String("reduce chance to throw when shot is possible"));
			}
		}

		if (BestThrow.ubPossible && 
			(BestThrow.iAttackValue > BestAttack.iAttackValue || ubBestAttackAction == AI_ACTION_NONE) && 
			(ubBestAttackAction == AI_ACTION_NONE || BestShot.ubChanceToReallyHit < 20 || Random(2) == 0))
		{
			ubBestAttackAction = AI_ACTION_TOSS_PROJECTILE;
			DebugAI( AI_MSG_INFO, pSoldier, String("best action = throw something, iAttackValue = %d", BestThrow.iAttackValue));
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

		default:
			// set to empty
			DebugAI( AI_MSG_INFO, pSoldier, String("Best attack - no good attack"));
			memset( &BestAttack, 0, sizeof( BestAttack ) );
			break;
		}
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
					// Max chance for tanks
					iChance = 100;

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
				
				FLOAT dTotalRecoil = 0.0f;
				pSoldier->bDoAutofire = 0;

				UINT8 ubMinAuto = 4;

				if (PythSpacesAway(pSoldier->sGridNo, BestShot.sTarget) > DAY_VISION_RANGE || pSoldier->aiData.bUnderFire)
				{
					ubMinAuto++;
				}

				if (UsingNewCTHSystem())
				{
					do
					{
						pSoldier->bDoAutofire++;
						dTotalRecoil += AICalcRecoilForShot(pSoldier, &(pSoldier->inv[BestShot.bWeaponIn]), pSoldier->bDoAutofire);
						ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &(pSoldier->inv[BestShot.bWeaponIn]), pSoldier->bDoAutofire, pSoldier);
					} while (pSoldier->bActionPoints >= BestShot.ubAPCost + ubBurstAPs + sActualAimAP + sReserveAP &&
						pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= pSoldier->bDoAutofire &&
						(dTotalRecoil <= 20.0f ||
						pSoldier->bDoAutofire < ubMinAuto));
				}
				else
				{
					do
					{
						pSoldier->bDoAutofire++;
						ubBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &(pSoldier->inv[BestAttack.bWeaponIn]), pSoldier->bDoAutofire, pSoldier);
					} while (pSoldier->bActionPoints >= BestAttack.ubAPCost + ubBurstAPs + sActualAimAP + sReserveAP &&
						pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft >= pSoldier->bDoAutofire &&
						// sevenfm limit max auto penalty if target has shock (suppressed)
						(GetAutoPenalty(pSoldier, &pSoldier->inv[BestAttack.bWeaponIn], gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE) * pSoldier->bDoAutofire <= max(BestAttack.ubChanceToReallyHit, 40 + 80 / (2 + MercPtrs[BestAttack.ubOpponent]->aiData.bShock)) ||
						pSoldier->bDoAutofire < ubMinAuto));
				}

				pSoldier->bDoAutofire--;

				sAimTimeNoBurst = pSoldier->aiData.bAimTime;
				DebugAI(AI_MSG_INFO, pSoldier, String("bDoAutofire %d bAimTime %d sActualAimAP %d", pSoldier->bDoAutofire, pSoldier->aiData.bAimTime, sActualAimAP));

				if (pSoldier->bDoAutofire > 0)
				{
					ubBurstAPs = CalcAPsToAutofire( pSoldier->CalcActionPoints(), &(pSoldier->inv[BestAttack.bWeaponIn]), pSoldier->bDoAutofire, pSoldier );

					if (pSoldier->bActionPoints >= BestAttack.ubAPCost + sActualAimAP + ubBurstAPs + sReserveAP )
					{
						// always do autofire for tanks
						// decided to use autofire, determine if use half burst
						pSoldier->bDoBurst = 1;
						INT16 ubHalfBurstAPs = 0;

						if (pSoldier->inv[BestAttack.bWeaponIn][0]->data.gun.ubGunShotsLeft < 4)
						{
							iChance = 0;
							DebugAI(AI_MSG_INFO, pSoldier, String("not enough ammo for 2 short burst"));
						}
						else
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("have enough ammo for 2 short bursts"));
							iChance = BestAttack.ubChanceToReallyHit / 2;
							ubHalfBurstAPs = CalcAPsToAutofire(pSoldier->CalcActionPoints(), &pSoldier->inv[BestAttack.bWeaponIn], 2, pSoldier);

							if (pSoldier->aiData.bOppCnt > 1 || !CheckSuppressionDirection(pSoldier, BestAttack.sTarget, BestAttack.bTargetLevel))
							{
								iChance += (100 - iChance) / 2;
							}
							DebugAI(AI_MSG_INFO, pSoldier, String("short autofire chance = %d", iChance));
						}

						// sevenfm: check to use half burst
						if (pSoldier->bActionPoints >= 2 * (BestAttack.ubAPCost + ubHalfBurstAPs + sActualAimAP) + sReserveAP &&
							Chance(iChance))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("decided to use short autofire"));
							pSoldier->bDoAutofire = 2;
							BestAttack.ubAPCost += ubHalfBurstAPs + sActualAimAP;
						}
						else
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("decided to use full autofire"));
							BestAttack.ubAPCost += ubBurstAPs + sActualAimAP;
						}
					}
				}
			}

			if (!pSoldier->bDoBurst)
			{
				pSoldier->aiData.bAimTime = sAimTimeNoBurst;
				pSoldier->bDoBurst			= 0;
				pSoldier->bDoAutofire		= 0;
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
			DebugAI(AI_MSG_INFO, pSoldier, String("fire gun"));
			pSoldier->aiData.usActionData = BestAttack.sTarget;
			pSoldier->bTargetLevel = BestAttack.bTargetLevel;
			return(AI_ACTION_FIRE_GUN);
		}
		else if (ubBestAttackAction == AI_ACTION_TOSS_PROJECTILE)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("toss attack"));
			pSoldier->bDoBurst = 0;
			pSoldier->bDoAutofire = 0;

			DebugAI(AI_MSG_INFO, pSoldier, String("prepare throw at spot %d level %d aimtime %d", BestAttack.sTarget, BestAttack.bTargetLevel, BestAttack.ubAimTime));

			// sevenfm: correctly set weapon mode for attached GL
			if (IsGrenadeLauncherAttached(&pSoldier->inv[HANDPOS]))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("set attached GL mode"));
				pSoldier->bWeaponMode = WM_ATTACHED_GL;
			}

			pSoldier->aiData.usActionData = BestAttack.sTarget;
			pSoldier->bTargetLevel = BestAttack.bTargetLevel;
			pSoldier->aiData.bAimTime = BestAttack.ubAimTime;

			return(AI_ACTION_TOSS_PROJECTILE);
		}
		// other attacks
		/*else
		{
			pSoldier->aiData.usActionData = BestAttack.sTarget;
			pSoldier->bTargetLevel = BestAttack.bTargetLevel;
			return(ubBestAttackAction);
		}*/
	}

	DebugAI( AI_MSG_TOPIC, pSoldier, String("[End of Tank AI]"));
	// end of tank AI
	return AI_ACTION_NONE;
}
