#ifdef PRECOMPILEDHEADERS
#include "AI All.h"
#else
#include "ai.h"
#include "Weapons.h"
#include "opplist.h"
#include "AIInternals.h"
#include "LOS.h"
#include "Physics.h"
#include "Items.h"
#include "Weapons.h"
#include "Spread Burst.h"
#include "overhead.h"
#include "SkillCheck.h"
#include "Soldier Profile.h"
#include "Isometric Utils.h"
#include "Soldier macros.h"
#include "PATHAI.H"
#include "GameSettings.h"
#include "strategicmap.h"
#include "environment.h"
#include "lighting.h"
#include "Sound Control.h"
#include "message.h"
#include "Vehicles.h"
#include "Soldier Functions.h"	//dnl ch69 140913
#include "Reinforcement.h"		// added by Flugente
#include "Town Militia.h"		// added by Flugente
#include "Queen Command.h"		// added by Flugente
#include "Explosion Control.h"	// added by Flugente for GASMASK_MIN_STATUS
// sevenfm
#include "worldman.h"			// WhoIsThere2()
#include "Game Clock.h"			// NightTime()
#include "Render Fun.h"			// InARoom()
#include "SmokeEffects.h"
#include "Structure Wrap.h"		// IsRoofPresentAtGridNo
#include "Drugs And Alcohol.h"
#include "WCheck.h"
#endif

// anv: for enemy taunts
#include "Civ Quotes.h"

extern INT16 DirIncrementer[8];

//
// CJC DG->JA2 conversion notes
//
// Still commented out:
//
// EstimateShotDamage - stuff related to legs?
// EstimateStabDamage - stuff related to armour
// EstimateThrowDamage - waiting for grenade, armour definitions
// CheckIfTossPossible - waiting for grenade definitions

// this define should go in soldier control.h

// sevenfm
extern INT32 gsLastBulletSpot;

void LoadWeaponIfNeeded(SOLDIERTYPE *pSoldier)
{
	UINT16 usInHand;
	INT8 bPayloadPocket;

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("LoadWeaponIfNeeded"));

	usInHand = pSoldier->inv[HANDPOS].usItem;

	if ( IsGrenadeLauncherAttached(&pSoldier->inv[HANDPOS]) )
		usInHand = GetAttachedGrenadeLauncher(&pSoldier->inv[HANDPOS]);

	// if he's got a MORTAR in his hand, make sure he has a MORTARSHELL avail.
	if (Item[usInHand].mortar )
	{
		//		bPayloadPocket = FindObj( pSoldier, MORTAR_SHELL );
		bPayloadPocket = FindNonSmokeLaunchable(pSoldier, usInHand);
		if (bPayloadPocket == NO_SLOT)
		{
			return;	// no shells, can't fire the MORTAR
		}
	}
	// if he's got a GL in his hand, make sure he has some type of GRENADE avail.
	else if (Item[usInHand].grenadelauncher )
	{
		bPayloadPocket = FindGLGrenade( pSoldier );
		if (bPayloadPocket == NO_SLOT || FindNonSmokeLaunchableAttachment( &pSoldier->inv[HANDPOS],usInHand ) != 0 )
		{
			return;	// no grenades, can't fire the GLAUNCHER... or the launcher has a magsize > 1
		}
	}
	// if he's got a RPG7 in his hand, make sure he has some type of RPG avail.
	else if (Item[usInHand].rocketlauncher && !Item[usInHand].singleshotrocketlauncher )
	{
		bPayloadPocket = FindNonSmokeLaunchable(pSoldier, usInHand );
		if (bPayloadPocket == NO_SLOT)
		{
			return;	// no grenades, can't fire
		}
	}
	else if (Item[usInHand].cannon )
	{
		bPayloadPocket = FindNonSmokeLaunchable( pSoldier, usInHand );
		if (bPayloadPocket == NO_SLOT)
		{
			return;
		}
	}
	else
	{
		// regular hand-thrown grenade in hand, nothing to load!
		return;
	}

	DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("LoadWeaponIfNeeded: remove payload from its pocket, and add it as the hand weapon's first attachment"));
	// remove payload from its pocket, and add it as the hand weapon's first attachment

	if ( TANK( pSoldier ) )
	{
		// don't remove ammo
		gTempObject = pSoldier->inv[bPayloadPocket];
		if (gTempObject.ubNumberOfObjects > 1) {
			gTempObject.RemoveObjectsFromStack(gTempObject.ubNumberOfObjects - 1);
		}
		pSoldier->inv[HANDPOS].AttachObject(pSoldier,&gTempObject,FALSE);
	}
	else if (pSoldier->inv[bPayloadPocket].MoveThisObjectTo(gTempObject, 1) == 0) 
	{
		if(pSoldier->inv[HANDPOS].AttachObject(pSoldier, &gTempObject, FALSE))//dnl ch63 250813 return back rest of object or drop it if not proper attachment
		{
			if(gTempObject.ubNumberOfObjects == 1 && gTempObject[0]->data.objectStatus > 0)
				gTempObject.MoveThisObjectTo(pSoldier->inv[bPayloadPocket], 1);
		}
		else
			AddItemToPool(pSoldier->sGridNo, &gTempObject, 0, pSoldier->pathing.bLevel, WORLD_ITEM_DROPPED_FROM_ENEMY, -1);
	}
}

void CalcBestShot(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestShot)
{
	UINT32 uiLoop;
	INT32 iAttackValue, iThreatValue, iHitRate, iBestHitRate, iPercentBetter, iEstDamage, iTrueLastTarget;
	UINT16 usTrueState, usTurningCost, usRaiseGunCost;
	INT16 sAimTime;
	INT16 sMinAimTime;
	INT16 ubMinAPcost;
	INT16 ubRawAPCost;
	INT16 sBestAPcost;
	INT16 sAimAPCost;
	INT16 sChanceToHit;
	INT16 sBestAimTime;
	INT16 sChanceToGetThrough;
	INT16 sBestChanceToGetThrough;
	UINT8 ubFriendlyFireChance;
	UINT8 ubBestFriendlyFireChance;
	INT16 sCurrentChanceToGetThrough;
	INT16 sBestChanceToHit;
	INT16 sStanceAPcost;
	BOOLEAN fAddingTurningCost, fAddingRaiseGunCost;
	UINT8 ubMaxPossibleAimTime, ubStance, ubBestStance, ubChanceToReallyHit;
	INT8 bScopeMode;
	SOLDIERTYPE *pOpponent;
	BOOLEAN fMounted;

	// sevenfm:
	BOOLEAN fSuppression = FALSE;
	BOOLEAN fReturnFire = FALSE;
	INT32 sTarget = NOWHERE;
	INT8 bLevel;
	INT32 iGunHoldingBP;
	//CHAR8 str8[1024];

	BOOLEAN fSniper = (gGameOptions.fNewTraitSystem && NUM_SKILL_TRAITS(pSoldier, SNIPER_NT) == 2);
	BOOLEAN fMarksman = (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, SNIPER_NT));
	BOOLEAN fAllowComplexAI = AllowComplexAI(pSoldier);
	BOOLEAN fUnderFire = pSoldier->aiData.bUnderFire;
	BOOLEAN fCloseCombat = (DistanceToClosestActiveOpponent(pSoldier, pSoldier->sGridNo) < TACTICAL_RANGE / 2);
	BOOLEAN fActiveCombat = (CountEngagedEnemiesLastTurn(pSoldier) > 1);
	UINT8 ubOpponents = CountSeenEnemiesLastTurn(pSoldier);

	INT8 bKnowledge;
	INT8 bPersonalKnowledge;
	INT8 bPublicKnowledge;	

	//DebugShot(pSoldier, String("\nCalcBestShot [%d] AP=%d", pSoldier->ubID, pSoldier->bActionPoints));

	// sevenfm: initialize
	pBestShot->ubPossible = FALSE;
	pBestShot->ubChanceToReallyHit = 0;
	pBestShot->iAttackValue = 0;
	pBestShot->ubOpponent = NOBODY;
	pBestShot->ubFriendlyFireChance = 0;
	pSoldier->bAimShotLocation = AIM_SHOT_RANDOM;

	sBestChanceToHit = sBestAimTime = sChanceToHit = sBestChanceToGetThrough = ubBestFriendlyFireChance = ubChanceToReallyHit = 0;

	// sevenfm: set attacking hand and target
	pSoldier->ubAttackingHand = HANDPOS;
	pSoldier->ubTargetID = NOBODY;

	pSoldier->usAttackingWeapon = pSoldier->inv[HANDPOS].usItem;
	pSoldier->bWeaponMode = WM_NORMAL;

	std::map<INT8, OBJECTTYPE*> ObjList;
	GetScopeLists(pSoldier, &pSoldier->inv[HANDPOS], ObjList);
	pSoldier->bScopeMode = USE_BEST_SCOPE;
	pSoldier->bDoBurst = 0;
	pSoldier->bDoAutofire = 0;

	// determine which attack against which target has the greatest attack value
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];
		fSuppression = FALSE;
		fReturnFire = FALSE;

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->stats.bLife)
		{			
			continue;			// next merc
		}		

		//wcstombs(str8, MercPtrs[pOpponent->ubID]->GetName(), wcslen(MercPtrs[pOpponent->ubID]->GetName()) + 1);
		//wcstombs(str8, MercPtrs[pOpponent->ubID]->GetName(), 1024 - 1);
		//DebugShot(pSoldier, String("\ncheck opponent [%d] %s", pOpponent->ubID, str8));

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			//DebugShot(pSoldier, String("\nbActive %d bInSector %d blife %d neutral %d bSide %d, %d", pOpponent->bActive, pOpponent->bInSector, pOpponent->stats.bLife, CONSIDERED_NEUTRAL(pSoldier, pOpponent), pSoldier->bSide, pOpponent->bSide));

			continue;
		}

		bKnowledge = Knowledge(pSoldier, pOpponent->ubID);
		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);
		//DebugShot(pSoldier, String("public knowledge %d, personal knowledge %d", bPublicKnowledge, bPersonalKnowledge));

		// sevenfm: skip unknown
		if(bKnowledge == NOT_HEARD_OR_SEEN)
		{
			continue;	// next opponent
		}

		// sevenfm: blind soldier can only attack seen/heard personally
		if (pSoldier->bBlindedCounter > 0 &&
			bPersonalKnowledge != SEEN_THIS_TURN &&
			bPersonalKnowledge != HEARD_THIS_TURN)
		{
			//DebugShot(pSoldier, String("blind soldier can only attack seen/heard personally, skip opponent"));
			continue;	// next opponent
		}
		
		sTarget = KnownLocation(pSoldier, pOpponent->ubID);
		bLevel = KnownLevel(pSoldier, pOpponent->ubID);
		//DebugShot(pSoldier, String("knowledge: sTarget %d, bLevel %d", sTarget, bLevel));

		// safety check
		if (TileIsOutOfBounds(sTarget) || sTarget == 0)
		{
			//DebugShot(pSoldier, String("enemy location unknown, skip soldier"));
			continue;
		}

		// return fire
		if (pSoldier->aiData.bUnderFire && 
			(pSoldier->ubPreviousAttackerID == pOpponent->ubID || pSoldier->ubNextToPreviousAttackerID == pOpponent->ubID))
		{
			fReturnFire = TRUE;
		}

		// check knowledge
		if (bKnowledge != SEEN_CURRENTLY &&
			bKnowledge != SEEN_THIS_TURN &&
			bKnowledge != SEEN_LAST_TURN &&
			bKnowledge != HEARD_THIS_TURN &&
			bKnowledge != HEARD_LAST_TURN &&
			!((bKnowledge == SEEN_2_TURNS_AGO || bKnowledge == SEEN_3_TURNS_AGO || bKnowledge == HEARD_2_TURNS_AGO) && Weapon[pSoldier->usAttackingWeapon].ubWeaponType == GUN_LMG))
		{
			//DebugShot(pSoldier, String("skip guys nobody seen or heard recently"));
			continue;	// next opponent
		}

		// sevenfm: determine if we shoot at unseen target for suppression		
		if( bPersonalKnowledge != SEEN_CURRENTLY &&
			bPublicKnowledge != SEEN_CURRENTLY &&
			!LOS_Soldier(pSoldier, pOpponent, LOS_UNREADY, CALC_FROM_ALL_DIRS))
		{
			//DebugShot(pSoldier, String("nobody sees target, use suppression"));
			fSuppression = TRUE;

			// try to randomize location
			if (Weapon[pSoldier->usAttackingWeapon].ubWeaponType == GUN_LMG)
			{
				sTarget = RandomizeLocation(sTarget, bLevel, 1, pSoldier);
			}
			//DebugShot(pSoldier, String("knowledge: sTarget %d, bLevel %d", sTarget, bLevel));
		}		

		// skip if we can see location and location is empty
		/*if (!fSuppression &&
			!LOS_Soldier(pSoldier, pOpponent, LOS_UNREADY, CALC_FROM_ALL_DIRS) &&
			LOS_Virtual(pSoldier, sTarget, bLevel, ANIM_PRONE, SIGHT_READY, CALC_FROM_ALL_DIRS) &&
			WhoIsThere2(sTarget, bLevel) == NOBODY)
		{
			//DebugShot(pSoldier, String("enemy location is empty and we have a LOS to location, skip"));
			continue;
		}*/

		// no fire on unseen opponents with throwing knives
		if ((Item[pSoldier->usAttackingWeapon].usItemClass & IC_THROWING_KNIFE) &&
			bPersonalKnowledge != SEEN_CURRENTLY &&
			!LOS_Soldier(pSoldier, pOpponent, LOS_UNREADY, CALC_FROM_ALL_DIRS))
		{
			//DebugShot(pSoldier, String("cannot attack personally unseen opponent with knife"));
			continue;
		}

		// sevenfm: don't allow blind suppression fire for militia
		if (fSuppression && 
			pSoldier->bTeam == MILITIA_TEAM &&
			!fReturnFire)
		{
			continue;
		}

		// sevenfm: only try to suppress alive and conscious human targets
		if (fSuppression &&
			Weapon[pSoldier->usAttackingWeapon].ubWeaponType != GUN_LMG &&
			(pOpponent->stats.bLife < OKLIFE ||
			pOpponent->bCollapsed && pOpponent->bBreath == 0 ||
			pOpponent->IsCowering() ||
			pOpponent->IsZombie() ||
			!IS_MERC_BODY_TYPE(pOpponent)))
		{
			continue;
		}

		// sevenfm: tanks cannot shoot at very close range
		if (pSoldier->IsTank() &&
			pOpponent->IsHumanBodyType() &&
			(PythSpacesAway(pSoldier->sGridNo, sTarget) < 3 && pOpponent->AnimEndHeight() < ANIM_CROUCH ||
			PythSpacesAway(pSoldier->sGridNo, sTarget) < 2 && pOpponent->AnimEndHeight() < ANIM_STAND))
		{
			continue;
		}

#ifdef DEBUGATTACKS
		DebugAI( String( "%s sees %s at gridno %d\n",pSoldier->GetName(),ExtMen[pOpponent->ubID].GetName(),pOpponent->sGridNo ) );
#endif
		
		// shoot through wall check
		if( bPersonalKnowledge != SEEN_CURRENTLY && 
			bPublicKnowledge != SEEN_CURRENTLY &&
			!LOS_Soldier(pSoldier, pOpponent, LOS_UNREADY, CALC_FROM_ALL_DIRS) &&
			!LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sTarget, bLevel, NO_DISTANCE_LIMIT) &&
			!fReturnFire &&
			!(InARoom(pSoldier->sGridNo, NULL) && InARoom(sTarget, NULL) && pSoldier->pathing.bLevel != bLevel && PythSpacesAway(pSoldier->sGridNo, sTarget) < 2 * SoldierDifficultyLevel(pSoldier)) &&
			!(InARoom(sTarget, NULL) && bLevel == 0 && (Weapon[pSoldier->usAttackingWeapon].ubWeaponType == GUN_LMG || TeamPercentKilled(pSoldier->bTeam) > 100 - 20 * gGameOptions.ubDifficultyLevel)) &&
			!TeamHighPercentKilled(pSoldier->bTeam))
		{
			//DebugShot(pSoldier, String("shooting through wall not allowed, skip opponent"));
			continue;
		}		

		// later will decide if shot is possible this here is just best guess so ignore turnover
		ubMinAPcost = MinAPsToAttack(pSoldier, sTarget, DONTADDTURNCOST, 0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we don't have enough APs left to shoot even a snap-shot at this guy
		if (ubMinAPcost > pSoldier->bActionPoints)
		{
			//DebugShot(pSoldier, String("not enough APs"));
			continue;			// next opponent
		}

		// calculate chance to get through the opponent's cover (if any)
		// sevenfm: find current CTGT, before stance change
		//gUnderFire.Clear();
		//gUnderFire.Enable();

		if( fSuppression )
			sCurrentChanceToGetThrough = AISoldierToLocationChanceToGetThrough( pSoldier, sTarget, bLevel, 3 );
		else
			sCurrentChanceToGetThrough = AISoldierToSoldierChanceToGetThrough( pSoldier, pOpponent );		

		//gUnderFire.Disable();
		//ubFriendlyFireChance = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);*/

		// if we can't hit from standing stance
		/*if (ubChanceToGetThrough == 0)
		{
			//DebugShot(pSoldier, String("CTGT = 0, skip"));
			continue;			// next opponent
		}*/

		if ( (pSoldier->flags.uiStatusFlags & SOLDIER_MONSTER) && (pSoldier->ubBodyType != QUEENMONSTER ) )
		{
			STRUCTURE_FILE_REF *	pStructureFileRef;
			UINT16								usAnimSurface;

			//DebugShot(pSoldier, String("monster soldier"));

			usAnimSurface = DetermineSoldierAnimationSurface( pSoldier, pSoldier->usMovementMode );
			pStructureFileRef = GetAnimationStructureRef( pSoldier->ubID, usAnimSurface,pSoldier->usMovementMode );

			if ( pStructureFileRef )
			{
				UINT16		usStructureID;
				INT8			bDir;

				// must make sure that structure data can be added in the direction of the target
				bDir = (INT8) GetDirection( pSoldier->sGridNo, sTarget );

				// ATE: Only if we have a levelnode...
				if ( pSoldier->pLevelNode != NULL && pSoldier->pLevelNode->pStructureData != NULL )
				{
					usStructureID = pSoldier->pLevelNode->pStructureData->usStructureID;
				}
				else
				{
					usStructureID = INVALID_STRUCTURE_ID;
				}

				if ( ! OkayToAddStructureToWorld( pSoldier->sGridNo, pSoldier->pathing.bLevel, &(pStructureFileRef->pDBStructureRef[ gOneCDirection[ bDir ] ]), usStructureID ) )
				{
					// can't turn in that dir.... next opponent
					continue;
				}
			}
		}

		iBestHitRate = 0;					 // reset best hit rate to minimum

		//DebugShot(pSoldier, String("find best hit rate"));

		//dnl ch69 130913 Hoping to optimize
		// consider alternate holding mode and different scopes
		// sevenfm: alt weapon holding scope mode is used only when ubAllowAlternativeWeaponHolding == 3
		for(pSoldier->bScopeMode = (gGameExternalOptions.ubAllowAlternativeWeaponHolding == 3 ? USE_ALT_WEAPON_HOLD : USE_BEST_SCOPE); 
			pSoldier->bScopeMode <= (gGameExternalOptions.fScopeModes ? NUM_SCOPE_MODES - 1 : USE_BEST_SCOPE );
			pSoldier->bScopeMode++)
		{
			//dnl ch71 180913 throwing knives cannot be used in fire from hip
			if (pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD)
			{
				if (Item[pSoldier->usAttackingWeapon].usItemClass & IC_THROWING_KNIFE)
				{
					//DebugShot(pSoldier, String("no hip firing with throwing knives"));
					continue;
				}
				// sevenfm: no hip firing for tanks, vehicles or monsters
				if(!pSoldier->IsMercBodyType())
				{
					//DebugShot(pSoldier, String("no hip firing for tanks, monsters or vehicles"));
					continue;
				}
			}			

			if( pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD || (pSoldier->bScopeMode >= USE_BEST_SCOPE && ObjList[pSoldier->bScopeMode] != NULL) )
			{
				usTrueState = pSoldier->usAnimState;		// because is used in CalculateRaiseGunCost, CalcAimingLevelsAvailableWithAP, CalculateTurningCost
				iTrueLastTarget = pSoldier->sLastTarget;	// because is used in MinAPsToShootOrStab

				// --------- Standing ---------
				ubStance = ANIM_STAND;
				//DebugShot(pSoldier, String("check standing stance"));
				// sevenfm: shoot heavy guns in standing stance only when using hip fire
				if (pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, sTarget), ubStance) &&
					(pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD || !Weapon[pSoldier->usAttackingWeapon].HeavyGun || !Item[pSoldier->usAttackingWeapon].twohanded || !gGameExternalOptions.ubAllowAlternativeWeaponHolding))
				{
					//DebugShot(pSoldier, String("stance is valid"));

					sStanceAPcost = GetAPsToChangeStance(pSoldier, ubStance);
					//DebugShot(pSoldier, String("sStanceAPcost %d", sStanceAPcost));
					if(sStanceAPcost)
					{
						// Going up so first is stance change then turnover, do animation change before APs calculation
						pSoldier->usAnimState = STANDING;
						pSoldier->sLastTarget = NOWHERE;
					}
					GetAPChargeForShootOrStabWRTGunRaises(pSoldier, sTarget, TRUE, &fAddingTurningCost, &fAddingRaiseGunCost, 0);
					usTurningCost = CalculateTurningCost(pSoldier, pSoldier->usAttackingWeapon, fAddingTurningCost);
					usRaiseGunCost = CalculateRaiseGunCost(pSoldier, fAddingRaiseGunCost, sTarget, 0);
					if(fAddingTurningCost && fAddingRaiseGunCost)//dnl ch71 180913
					{
						if(usRaiseGunCost > usTurningCost)
							usTurningCost = 0;
						else
							usRaiseGunCost = 0;
					}
					ubRawAPCost = MinAPsToShootOrStab(pSoldier, sTarget, 0, FALSE, 2);
					ubMinAPcost = ubRawAPCost + usTurningCost + sStanceAPcost + usRaiseGunCost;

					// sevenfm: determine if weapon would be mounted in current stance
					fMounted = FALSE;
					/*if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && gGameExternalOptions.fWeaponResting && pSoldier->IsWeaponMounted(sTarget))
					{
						fMounted = TRUE;
					}*/

					//DebugShot(pSoldier, String("min AP cost %d, raise cost %d, turning cost %d, raw cost %d, stance cost %d", ubMinAPcost, usRaiseGunCost, usTurningCost, ubRawAPCost, sStanceAPcost));

					if(pSoldier->bActionPoints - ubMinAPcost >= 0)
					{
						// calc next attack's minimum shooting cost (excludes readying & turning & raise gun)
						ubMaxPossibleAimTime = CalcAimingLevelsAvailableWithAP(pSoldier, sTarget, pSoldier->bActionPoints - ubMinAPcost);
						//DebugShot(pSoldier, String("max aim %d", ubMaxPossibleAimTime));

						// sevenfm: check CTGT and friendly fire chance for every stance
						gUnderFire.Clear();
						gUnderFire.Enable();
						if (fSuppression)
							sChanceToGetThrough = SoldierToLocationChanceToGetThrough(pSoldier, sTarget, bLevel, 3, NOBODY);
						else
							sChanceToGetThrough = SoldierToSoldierChanceToGetThrough(pSoldier, pOpponent);
						ubFriendlyFireChance = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);
						gUnderFire.Disable();

						// sevenfm: only use this stance if we can hit target and cannot hit friends
						if (sChanceToGetThrough > 0 && ubFriendlyFireChance <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE)
						{
							sMinAimTime = 0;
							// sevenfm: in some cases, only check max possible aim
							if (!fAllowComplexAI || (fSniper || fMarksman || !fActiveCombat) && !fUnderFire && !fCloseCombat)
							{
								sMinAimTime = ubMaxPossibleAimTime;
							}
							for (sAimTime = sMinAimTime; sAimTime <= ubMaxPossibleAimTime; sAimTime++)
							{
								//DebugShot(pSoldier, String("check aim %d", sAimTime));
								sChanceToHit = AICalcChanceToHitGun(pSoldier, sTarget, sAimTime, AIM_SHOT_TORSO, bLevel, STANDING);
								//DebugShot(pSoldier, String("CTH %d", sChanceToHit));

								sAimAPCost = CalcAPCostForAiming(pSoldier, sTarget, (INT8)sAimTime);
								iHitRate = sChanceToHit * (pSoldier->bActionPoints - (ubMinAPcost - ubRawAPCost)) / (ubRawAPCost + sAimAPCost);
								//DebugShot(pSoldier, String("hit rate %d", iHitRate));

								// sevenfm: penalize standing stance if we are crouched/prone (skip unless it gives really big CTH bonus)
								// only if crouched/prone CTGT is same or better than new
								if (gAnimControl[usTrueState].ubHeight < ubStance && sCurrentChanceToGetThrough >= sChanceToGetThrough && iHitRate > 0)
								{
									iHitRate = max(1, 4 * iHitRate / (4 + SoldierDifficultyLevel(pSoldier)));
								}
								//DebugShot(pSoldier, String("hit rate after stance adjustment %d", iHitRate));

								// sevenfm: penalize standing stance for heavy weapons
								iGunHoldingBP = GetBPCostPer10APsForGunHolding(pSoldier, TRUE);
								if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && iGunHoldingBP > 30 && iHitRate > 0 && !fMounted)
								{
									iHitRate = max(1, iHitRate * 100 / (100 + iGunHoldingBP));
								}

								if (iHitRate * sChanceToGetThrough > iBestHitRate * sBestChanceToGetThrough ||
									(fSniper || fMarksman || !fActiveCombat) && !fUnderFire && !fCloseCombat && sChanceToHit > sBestChanceToHit ||
									(Item[pSoldier->usAttackingWeapon].usItemClass & IC_THROWING_KNIFE) && sChanceToHit > sBestChanceToHit)// rather take best chance for throwing knives
								{
									iBestHitRate = iHitRate;
									sBestAimTime = sAimTime;
									sBestChanceToHit = sChanceToHit;
									sBestChanceToGetThrough = sChanceToGetThrough;
									ubBestFriendlyFireChance = ubFriendlyFireChance;
									bScopeMode = pSoldier->bScopeMode;
									sBestAPcost = ubMinAPcost;
									ubBestStance = ubStance;

									//DebugShot(pSoldier, String("select standing stance"));
								}
							}
						}
					}
					pSoldier->usAnimState = usTrueState;
					pSoldier->sLastTarget = iTrueLastTarget;
				}

				// no crouched/prone if we are tank or using throwing knife or hip firing
				if( pSoldier->bScopeMode == USE_ALT_WEAPON_HOLD ||
					TANK(pSoldier) ||
					(Item[pSoldier->usAttackingWeapon].usItemClass & IC_THROWING_KNIFE) )
				{
					//DebugShot(pSoldier, String("skip crouched/prone for hip firing, tanks and throwing knives"));
					continue;
				}

				// --------- Crouched ---------
				ubStance = ANIM_CROUCH;
				//DebugShot(pSoldier, String("check crouched stance"));
				if (pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, sTarget), ubStance) )
				{
					// change stance then turn
					sStanceAPcost = GetAPsToChangeStance(pSoldier, ubStance);
					//DebugShot(pSoldier, String("sStanceAPcost %d", sStanceAPcost));
					if (sStanceAPcost)
					{
						pSoldier->usAnimState = CROUCHING;
						pSoldier->sLastTarget = NOWHERE;
					}
					GetAPChargeForShootOrStabWRTGunRaises(pSoldier, sTarget, TRUE, &fAddingTurningCost, &fAddingRaiseGunCost, 0);
					usTurningCost = CalculateTurningCost(pSoldier, pSoldier->usAttackingWeapon, fAddingTurningCost);
					usRaiseGunCost = CalculateRaiseGunCost(pSoldier, fAddingRaiseGunCost, sTarget, 0);
					if (fAddingTurningCost && fAddingRaiseGunCost)//dnl ch71 180913
					{
						if (usRaiseGunCost > usTurningCost)
							usTurningCost = 0;
						else
							usRaiseGunCost = 0;
					}
					ubRawAPCost = MinAPsToShootOrStab(pSoldier, sTarget, 0, FALSE, 2);
					ubMinAPcost = ubRawAPCost + usTurningCost + sStanceAPcost + usRaiseGunCost;

					// sevenfm: determine if weapon would be mounted in current stance
					fMounted = FALSE;
					/*if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && gGameExternalOptions.fWeaponResting && pSoldier->IsWeaponMounted(sTarget))
					{
						fMounted = TRUE;
					}*/

					if(pSoldier->bActionPoints - ubMinAPcost >= 0)
					{
						ubMaxPossibleAimTime = CalcAimingLevelsAvailableWithAP(pSoldier, sTarget, pSoldier->bActionPoints - ubMinAPcost);

						// sevenfm: check CTGT and friendly fire chance for every stance
						gUnderFire.Clear();
						gUnderFire.Enable();
						if (fSuppression)
							sChanceToGetThrough = SoldierToLocationChanceToGetThrough(pSoldier, sTarget, bLevel, 3, NOBODY);
						else
							sChanceToGetThrough = SoldierToSoldierChanceToGetThrough(pSoldier, pOpponent);
						ubFriendlyFireChance = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);
						gUnderFire.Disable();

						// sevenfm: only use this stance if we can hit target and cannot hit friends
						if (sChanceToGetThrough > 0 && ubFriendlyFireChance <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE)
						{
							sMinAimTime = 0;
							// sevenfm: in some cases, only check max possible aim
							if (!fAllowComplexAI || (fSniper || fMarksman || !fActiveCombat) && !fUnderFire && !fCloseCombat)
							{
								sMinAimTime = ubMaxPossibleAimTime;
							}
							for (sAimTime = sMinAimTime; sAimTime <= ubMaxPossibleAimTime; sAimTime++)
							{
								//DebugShot(pSoldier, String("check aim %d", sAimTime));
								sChanceToHit = AICalcChanceToHitGun(pSoldier, sTarget, sAimTime, AIM_SHOT_TORSO, bLevel, CROUCHING);
								//DebugShot(pSoldier, String("CTH %d", sChanceToHit));

								sAimAPCost = CalcAPCostForAiming(pSoldier, sTarget, (INT8)sAimTime);
								iHitRate = sChanceToHit * (pSoldier->bActionPoints - (ubMinAPcost - ubRawAPCost)) / (ubRawAPCost + sAimAPCost);
								//DebugShot(pSoldier, String("hit rate %d", iHitRate));

								// sevenfm: penalize crouched stance if we are prone (skip unless it gives really big CTH bonus)
								// only if prone CTGT is same or better than new
								if (gAnimControl[usTrueState].ubHeight < ubStance && sCurrentChanceToGetThrough >= sChanceToGetThrough && iHitRate > 0 && !fMounted)
								{
									iHitRate = max(1, 4 * iHitRate / (4 + SoldierDifficultyLevel(pSoldier)));
								}
								//DebugShot(pSoldier, String("hit rate after stance adjustment %d", iHitRate));

								// sevenfm: penalize crouched stance for heavy weapons
								iGunHoldingBP = GetBPCostPer10APsForGunHolding(pSoldier, TRUE);
								if (iGunHoldingBP > 50 && iHitRate > 0)
								{
									iHitRate = max(1, iHitRate * 100 / (100 + iGunHoldingBP));
								}

								if (iHitRate * sChanceToGetThrough > iBestHitRate * sBestChanceToGetThrough ||
									(fSniper || fMarksman || !fActiveCombat) && !fUnderFire && !fCloseCombat && sChanceToHit > sBestChanceToHit)
								{
									iBestHitRate = iHitRate;
									sBestAimTime = sAimTime;
									sBestChanceToHit = sChanceToHit;
									sBestChanceToGetThrough = sChanceToGetThrough;
									ubBestFriendlyFireChance = ubFriendlyFireChance;
									bScopeMode = pSoldier->bScopeMode;
									sBestAPcost = ubMinAPcost;
									ubBestStance = ubStance;

									//DebugShot(pSoldier, String("select crouched stance"));
								}
							}
						}						
					}
					pSoldier->usAnimState = usTrueState;
					pSoldier->sLastTarget = iTrueLastTarget;
				}

				// no prone stance if we have to change direction and change stance at the same time
				if (pSoldier->ubDirection != AIDirection(pSoldier->sGridNo, sTarget) &&
					gAnimControl[pSoldier->usAnimState].ubEndHeight > ANIM_PRONE)
				{
					//DebugShot(pSoldier, String("skip prone if we have to change direction and stance at the same time"));
					continue;
				}

				// --------- Prone ---------
				ubStance = ANIM_PRONE;
				//DebugShot(pSoldier, String("check prone stance"));
				if (pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, sTarget), ubStance))
				{
					sStanceAPcost = GetAPsToChangeStance(pSoldier, ubStance);
					if (sStanceAPcost)
					{
						pSoldier->usAnimState = PRONE;
						pSoldier->sLastTarget = NOWHERE;
					}
					GetAPChargeForShootOrStabWRTGunRaises(pSoldier, sTarget, TRUE, &fAddingTurningCost, &fAddingRaiseGunCost, 0);
					usTurningCost = CalculateTurningCost(pSoldier, pSoldier->usAttackingWeapon, fAddingTurningCost);
					usRaiseGunCost = CalculateRaiseGunCost(pSoldier, fAddingRaiseGunCost, sTarget, 0);
					ubRawAPCost = MinAPsToShootOrStab(pSoldier, sTarget, 0, FALSE, 2);
					ubMinAPcost = ubRawAPCost + usTurningCost + sStanceAPcost + usRaiseGunCost;

					// sevenfm: determine if weapon would be mounted in current stance
					fMounted = FALSE;
					/*if (pSoldier->bScopeMode != USE_ALT_WEAPON_HOLD && gGameExternalOptions.fWeaponResting && pSoldier->IsWeaponMounted(sTarget))
					{
						fMounted = TRUE;
					}*/

					if (pSoldier->bActionPoints - ubMinAPcost >= 0)
					{
						ubMaxPossibleAimTime = CalcAimingLevelsAvailableWithAP(pSoldier, sTarget, pSoldier->bActionPoints - ubMinAPcost);

						// sevenfm: check CTGT and friendly fire chance for every stance
						gUnderFire.Clear();
						gUnderFire.Enable();
						if (fSuppression)
							sChanceToGetThrough = SoldierToLocationChanceToGetThrough(pSoldier, sTarget, bLevel, 3, NOBODY);
						else
							sChanceToGetThrough = SoldierToSoldierChanceToGetThrough(pSoldier, pOpponent);
						ubFriendlyFireChance = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);
						gUnderFire.Disable();

						// sevenfm: only use this stance if we can hit target and cannot hit friends
						if (sChanceToGetThrough > 0 && ubFriendlyFireChance <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE)
						{
							sMinAimTime = 0;
							// sevenfm: in some cases, only check max possible aim
							if (!fAllowComplexAI || !fUnderFire && !fCloseCombat)
							{
								sMinAimTime = ubMaxPossibleAimTime;
							}
							for (sAimTime = sMinAimTime; sAimTime <= ubMaxPossibleAimTime; sAimTime++)
							{
								//DebugShot(pSoldier, String("check aim %d", sAimTime));
								sChanceToHit = AICalcChanceToHitGun(pSoldier, sTarget, sAimTime, AIM_SHOT_TORSO, bLevel, PRONE);
								//DebugShot(pSoldier, String("CTH %d", sChanceToHit));

								sAimAPCost = CalcAPCostForAiming(pSoldier, sTarget, (INT8)sAimTime);
								iHitRate = sChanceToHit * (pSoldier->bActionPoints - (ubMinAPcost - ubRawAPCost)) / (ubRawAPCost + sAimAPCost);
								//DebugShot(pSoldier, String("hit rate %d", iHitRate));

								if (iHitRate * sChanceToGetThrough > iBestHitRate * sBestChanceToGetThrough ||
									!fUnderFire && !fCloseCombat && sChanceToHit > sBestChanceToHit)
								{
									iBestHitRate = iHitRate;
									sBestAimTime = sAimTime;
									sBestChanceToHit = sChanceToHit;
									sBestChanceToGetThrough = sChanceToGetThrough;
									ubBestFriendlyFireChance = ubFriendlyFireChance;
									bScopeMode = pSoldier->bScopeMode;
									sBestAPcost = ubMinAPcost;
									ubBestStance = ubStance;

									//DebugShot(pSoldier, String("select prone stance"));
								}
							}
						}						
					}
					pSoldier->usAnimState = usTrueState;
					pSoldier->sLastTarget = iTrueLastTarget;
				}
			}
		}

		//DebugShot(pSoldier, String("final iBestHitRate %d", iBestHitRate));

		// if we can't get any kind of hit rate at all
		if (iBestHitRate == 0)
		{
			//DebugShot(pSoldier, String("hit rate = 0, skip"));
			continue;			// next opponent
		}

		// calculate chance to REALLY hit: shoot accurately AND get past cover
		ubChanceToReallyHit = (UINT8)ceil((sBestChanceToHit * sBestChanceToGetThrough) / 100.0f);

		// if we can't REALLY hit at all
		if (ubChanceToReallyHit == 0)
		{
			//DebugShot(pSoldier, String("CTH = 0, skip"));
			continue;			// next opponent
		}

		// really limit knife throwing so it doesn't look wrong
		if ( Item[ pSoldier->usAttackingWeapon ].usItemClass == IC_THROWING_KNIFE &&
			(ubChanceToReallyHit < 25 || (PythSpacesAway(pSoldier->sGridNo, sTarget) > CalcMaxTossRange(pSoldier, pSoldier->usAttackingWeapon, FALSE))))// Madd / 2 ) ) ) //dnl ch69 160913 was ubChanceToReallyHit < 30
		{
			//DebugShot(pSoldier, String("knife throwing too far, skip"));
			continue; // don't bother... next opponent
		}

		// calculate this opponent's threat value (factor in my cover from him)
		iThreatValue = CalcManThreatValue(pOpponent, pSoldier->sGridNo, TRUE, pSoldier);
		//DebugShot(pSoldier, String("iThreatValue %d", iThreatValue));

		// estimate the damage this shot would do to this opponent
		iEstDamage = EstimateShotDamage(pSoldier, pOpponent, sBestChanceToHit);		
		//DebugShot(pSoldier, String("estimated shot damage %d", iEstDamage));

		// calculate the combined "attack value" for this opponent
		// highest possible value before division should be about 1.8 billion...
		// normal value before division should be about 5 million...

		iAttackValue = (iEstDamage * iBestHitRate * ubChanceToReallyHit * iThreatValue) / 1000;

		//DebugShot(pSoldier, String("iAttackValue %d, iThreatValue %d, iEstDamage %d", iAttackValue, iThreatValue, iEstDamage));

		// sevenfm: take into account friendly fire chance		
		if (ubBestFriendlyFireChance > 0)
		{
			//iAttackValue = iAttackValue * (100 - ubFriendlyFireChance) / 100;
			iAttackValue = iAttackValue / 2;
		}

		// if this soldier attacked me, attack him!
		/*if( pSoldier->aiData.bUnderFire && pOpponent->sLastTarget == pSoldier->sGridNo )
		{
			iAttackValue *= 2;
			//DebugShot(pSoldier, String("bonus to attack opponent that attacked me!, iAttackValue = %d", iAttackValue));
		}*/

		// if not under fire, attack important targets
		if ((!pSoldier->aiData.bUnderFire || AICheckHasFlamethrowerInHands(pOpponent)) &&
			(AIRoleSniper(pOpponent) || 
			AIRoleMachinegunner(pOpponent) || 
			AIRoleMortarOperator(pOpponent) || 
			AIRoleCommander(pOpponent) ||
			AICheckHasFlamethrowerInHands(pOpponent)))
		{
			if (AICheckHasFlamethrowerInHands(pOpponent))
				iAttackValue *= 8;
			else if (AIRoleSniper(pSoldier) || AIRoleMachinegunner(pOpponent))
				iAttackValue *= 4;
			else
				iAttackValue *= 2;

			//DebugShot(pSoldier, String("prefer to attack important targets, iAttackValue = %d", iAttackValue));
		}

		// sevenfm: penalize suppression fire
		if( fSuppression )
		{
			// 25% penalty for shooting at invisible target
			iAttackValue = iAttackValue / 2;
			//DebugShot(pSoldier, String("penalize suppression fire, iAttackValue = %d", iAttackValue));
		}

		// penalize up to 75% of value from opponent's shock
		//iAttackValue = iAttackValue * (100 - pOpponent->ShockLevelPercent() / 2) / 100;
		//DebugShot(pSoldier, String("penalty from opponent's shock, iAttackValue = %d", iAttackValue));

		// special stuff for assassins to ignore militia more
		if ( pSoldier->IsAssassin() && pOpponent->bTeam == MILITIA_TEAM )
		{			
			iAttackValue /= 2;
			//DebugShot(pSoldier, String("penalty for assassin to attack militia, iAttackValue = %d", iAttackValue));
		}

		// sevenfm: empty vehicles have very low priority
		if( pOpponent->IsEmptyVehicle() )
		{
			iAttackValue /= 4;
			//DebugShot(pSoldier, String("penalty to attack empty vehicle, iAttackValue = %d", iAttackValue));
		}

		// sevenfm: dying, cowering or unconscious soldiers have very low priority
		if (pOpponent->stats.bLife < OKLIFE || pOpponent->IsUnconscious())
		{			
			iAttackValue /= 4;
			//DebugShot(pSoldier, String("penalty to attack dying/collapsed soldier, iAttackValue = %d", iAttackValue));
		}

		// sevenfm: penalty for attacking attractive opponent
		if (pSoldier->IsMaleBodyType() && 
			pOpponent->HasStunningLooks() &&
			PersonalSeenKnowledge(pSoldier, pOpponent->ubID) &&
			pSoldier->ubPreviousAttackerID != pOpponent->ubID &&
			pSoldier->ubNextToPreviousAttackerID != pOpponent->ubID)
		{
			iAttackValue /= 4;
		}
		else if (pSoldier->IsFemaleBodyType() &&
			pOpponent->HasStunningLooks() &&
			PersonalSeenKnowledge(pSoldier, pOpponent->ubID))
		{
			iAttackValue *= 2;

			if (pSoldier->ubPreviousAttackerID != pOpponent->ubID &&
				pSoldier->ubNextToPreviousAttackerID != pOpponent->ubID)
			{
				iAttackValue *= 2;
			}
		}

#ifdef DEBUGATTACKS
		DebugAI( String( "CalcBestShot: best AttackValue vs %d = %d\n",uiLoop,iAttackValue ) );
#endif
		//DebugShot(pSoldier, String("iAttackValue %d, CTH %d, best CTH %d", iAttackValue, ubChanceToReallyHit, pBestShot->ubChanceToReallyHit));

		// if we can hurt the guy, OR probably not, but at least it's our best
		// chance to actually hit him and maybe scare him, knock him down, etc.
		if ((iAttackValue > 0) || (ubChanceToReallyHit > pBestShot->ubChanceToReallyHit))
		{
			//DebugShot(pSoldier, String("check attack against %d, iAttackValue %d, CTH %d", pOpponent->ubID, iAttackValue, ubChanceToReallyHit));

			// if there already was another viable target
			if (pBestShot->ubChanceToReallyHit > 0)
			{
				// OK, how does our chance to hit him compare to the previous best one?
				iPercentBetter = ((ubChanceToReallyHit * 100) / pBestShot->ubChanceToReallyHit) - 100;
				//DebugShot(pSoldier, String("iPercentBetter %d", iPercentBetter));

				//dnl ch62 180813 ignore firing into breathless targets if there are targets in better condition
				// sevenfm: check that best opponent exists
				if( pBestShot->ubOpponent != NOBODY &&
					Menptr[pBestShot->ubOpponent].IsUnconscious() &&
					!pOpponent->IsUnconscious() )
				{
					//DebugShot(pSoldier, String("best opponent is breathless, chose new opponent - set PERCENT_TO_IGNORE_THREAT"));
					iPercentBetter = PERCENT_TO_IGNORE_THREAT;
				}

				// sevenfm: if best opponent is dying and new opponent is ok, use new opponent
				if( pBestShot->ubOpponent != NOBODY && 
					Menptr[pBestShot->ubOpponent].stats.bLife < OKLIFE &&
					pOpponent->stats.bLife >= OKLIFE )
				{
					//DebugShot(pSoldier, String("best opponent is dying, choose new opponent - set PERCENT_TO_IGNORE_THREAT"));
					iPercentBetter = PERCENT_TO_IGNORE_THREAT;
				}

				// sevenfm: if best opponent is empty vehicle, switch to new opponent
				if (pBestShot->ubOpponent != NOBODY &&
					MercPtrs[pBestShot->ubOpponent]->IsEmptyVehicle() &&
					!pOpponent->IsEmptyVehicle())
				{
					//DebugShot(pSoldier, String("best opponent is empty vehicle, choose new opponent - set PERCENT_TO_IGNORE_THREAT"));
					iPercentBetter = PERCENT_TO_IGNORE_THREAT;
				}

				// if this chance to really hit is more than 50% worse, and the other
				// guy is conscious at all
				// sevenfm: check that best opponent exists
				if( iPercentBetter < -PERCENT_TO_IGNORE_THREAT &&
					pBestShot->ubOpponent != NOBODY &&
					Menptr[pBestShot->ubOpponent].stats.bLife >= OKLIFE )
				{
					// then stick with the older guy as the better target
					//DebugShot(pSoldier, String("this chance to really hit is more than 50% worse, and the other guy is conscious - skip"));
					continue;
				}				

				// if this chance to really hit between 50% worse to 50% better
				if (iPercentBetter < PERCENT_TO_IGNORE_THREAT)
				{
					// then the one with the higher ATTACK VALUE is the better target
					if (iAttackValue < pBestShot->iAttackValue)
					{
						// the previous guy is more important since he's more dangerous
						//DebugShot(pSoldier, String("the previous guy is more important since he's more dangerous - skip"));
						continue;			// next opponent
					}
				}
			}

			// sevenfm: if new opponent is dying and best opponent is ok, ignore new opponent
			if( pBestShot->ubOpponent != NOBODY && 
				Menptr[pBestShot->ubOpponent].stats.bLife >= OKLIFE &&
				pOpponent->stats.bLife < OKLIFE )
			{
				//DebugShot(pSoldier, String("new opponent is dying, best opponent is ok - skip"));
				continue;
			}

			// sevenfm: if new opponent is empty vehicle and best opponent is regular opponent, ignore new opponent
			if (pBestShot->ubOpponent != NOBODY &&
				MercPtrs[pBestShot->ubOpponent]->IsEmptyVehicle() &&
				!pOpponent->IsEmptyVehicle())
			{
				//DebugShot(pSoldier, String("new opponent is empty vehicle, best opponent is ok - skip"));
				continue;
			}

			// OOOF!	That was a lot of work!	But we've got a new best target!
			pBestShot->ubPossible			= TRUE;
			pBestShot->ubOpponent			= pOpponent->ubID;
			pBestShot->ubAimTime			= sBestAimTime;
			pBestShot->ubChanceToReallyHit	= ubChanceToReallyHit;
			pBestShot->sTarget				= sTarget;
			pBestShot->bTargetLevel			= bLevel;
			pBestShot->iAttackValue			= iAttackValue;
			pBestShot->ubAPCost				= sBestAPcost;
			pBestShot->ubStance				= ubBestStance;
			pBestShot->bScopeMode			= bScopeMode;
			pBestShot->ubFriendlyFireChance = ubBestFriendlyFireChance;

			//DebugShot(pSoldier, String("Attack opponent %d, iAttackValue %d, sTarget %d, bLevel %d, aim time %d, CTH %d, AP cost %d, scope mode %d, friendly fire %d", pOpponent->ubID, iAttackValue, sTarget, bLevel, sBestAimTime, ubChanceToReallyHit, sBestAPcost, bScopeMode, pBestShot->ubFriendlyFireChance));
			//DebugShot(pSoldier, String("best attack: opponent %d, ubAimTime %d, ubAPCost %d, iAttackValue %d, ubChanceToReallyHit", pBestShot->ubOpponent, pBestShot->ubAimTime, pBestShot->ubAPCost, pBestShot->iAttackValue, pBestShot->ubChanceToReallyHit));
		}
	}
	pSoldier->bScopeMode = USE_BEST_SCOPE; // better reset this back
}

// JA2Gold: added
BOOLEAN CloseEnoughForGrenadeToss( INT32 sGridNo, INT32 sGridNo2 )
{
	INT32	sTempGridNo;
	UINT8	ubDirection;
	INT16	sXPos, sYPos, sXPos2, sYPos2;
	UINT8	ubMovementCost;

	if (sGridNo == sGridNo2 )
	{
		// checking the same space; if there is a closed door next to location in ANY direction then forget it
		// (could be the player closed a door on us)
		for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
		{
			sTempGridNo = NewGridNo( sGridNo, DirectionInc( ubDirection ) );
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ 0 ];
			if ( IS_TRAVELCOST_DOOR( ubMovementCost ) )
			{
				ubMovementCost = DoorTravelCost( NULL, sTempGridNo, ubMovementCost, FALSE, NULL );
			}
			if ( ubMovementCost >= TRAVELCOST_BLOCKED)
			{
				return( FALSE );
			}
		}
	}
	else
	{
		if ( CardinalSpacesAway( sGridNo, sGridNo2 ) > 2 )
		{
			return( FALSE );
		}

		// we are within 1 space diagonally or at most 2 horizontally or vertically,
		// so we can now do a loop safely

		sTempGridNo = sGridNo;

		sXPos = CenterX( sGridNo );
		sYPos = CenterY( sGridNo );
		sXPos2 = CenterX( sGridNo2 );
		sYPos2 = CenterY( sGridNo2 );
		ubDirection = atan8( sXPos, sYPos, sXPos2, sYPos2 );

		// For each step of the loop, we are checking for door or obstacle movement costs.	If we
		// find we're blocked, then this is no good for grenade tossing!
		do
		{
			sTempGridNo = NewGridNo( sTempGridNo, DirectionInc( ubDirection ) );
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ 0 ];
			if ( IS_TRAVELCOST_DOOR( ubMovementCost ) )
			{
				ubMovementCost = DoorTravelCost( NULL, sTempGridNo, ubMovementCost, FALSE, NULL );
			}
			if ( ubMovementCost >= TRAVELCOST_BLOCKED)
			{
				return( FALSE );
			}
		} while( sTempGridNo != sGridNo2 );
	}

	return( TRUE );
}

#define EXCLUDE_TILE_SIZE 100

void CalcBestThrow(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//DebugShot(pSoldier, String("\nCalcBestThrow [%d] AP=%d", pSoldier->ubID, pSoldier->bActionPoints));

	// September 9, 1998: added code for LAWs (CJC)
	UINT8	ubLoop, ubLoop2;
	INT32	iAttackValue;
	INT32	iHitRate, iThreatValue, iTotalThreatValue,iOppThreatValue[MAXMERCS];
	INT32	sGridNo, sEndGridNo, sFriendTile[MAXMERCS], sOpponentTile[MAXMERCS];
	INT8	bFriendLevel[MAXMERCS], bOpponentLevel[MAXMERCS];
	INT32	iEstDamage;
	UINT8	ubFriendCnt = 0,ubOpponentCnt = 0, ubOpponentID[MAXMERCS];
	UINT8	ubMaxPossibleAimTime;
	INT16	sRawAPCost, sMinAPcost, sAimAPCost;
	UINT8	ubChanceToHit, ubChanceToGetThrough, ubChanceToReallyHit, ubFriendlyFireChance;
	UINT32	uiPenalty;
	UINT8	ubSearchRange;
	UINT16	usOppDist;
	BOOLEAN fFriendsNearby;
	UINT16	usInHand, usGrenade;
	UINT8	ubOppsInRange, ubOppsAdjacent;
	BOOLEAN fSkipLocation;
	INT8	bPayloadPocket;
	INT8	bMaxLeft,bMaxRight,bMaxUp,bMaxDown,bXOffset,bYOffset;
	INT8	bPersonalKnowledge, bPublicKnowledge, bKnowledge;
	SOLDIERTYPE *pOpponent, *pFriend;
	static INT16	sExcludeTile[EXCLUDE_TILE_SIZE]; // This array is for storing tiles that we have already considered, to prevent duplication of effort
	UINT8	ubNumExcludedTiles = 0;
	INT32	iTossRange;
	UINT8	ubSafetyMargin = 0;
	UINT8	ubDiff;
	INT8	bEndLevel;
	OBJECTTYPE *pObjGL = NULL;//dnl ch63 240813
	BOOLEAN	fJammerFound;	// one of the opponents is jamming
	BOOLEAN fHandGrenade = FALSE;
	BOOLEAN fRedSmoke = FALSE;
	BOOLEAN fMortar = FALSE;
	BOOLEAN fCannon = FALSE;
	BOOLEAN fGrenadeLauncher = FALSE;
	BOOLEAN fRocketLauncher = FALSE;
	BOOLEAN fSectorAttack = FALSE;
	BOOLEAN fSectorCurFew = FALSE;
	BOOLEAN fArtilleryStrikeOrdered = FALSE;
	UINT16	usRoom1, usRoom2;

	// sevenfm
	UINT16	usTrueState = pSoldier->usAnimState;
	BOOLEAN fStandUp = FALSE;
	BOOLEAN fAllowComplexAI = AllowComplexAI(pSoldier);
	BOOLEAN fFoundTank = FALSE;

	usInHand = pSoldier->inv[HANDPOS].usItem;
	usGrenade = NOTHING;

	// sevenfm: initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;

	// use throw attacks more often when attacking town sector
	if (SectorAttack(pSoldier->bTeam) && AICheckTown())
	{
		fSectorAttack = TRUE;
	}

	if(pSoldier->bTeam == ENEMY_TEAM && SectorCurfew(FALSE) > 1)
	{
		fSectorCurFew = TRUE;
	}

	fArtilleryStrikeOrdered = CheckArtilleryStrike();

	if ( IsGrenadeLauncherAttached(&pSoldier->inv[HANDPOS]) )
	{
		//DebugShot( pSoldier, String("use attached GL"));
		usInHand = GetAttachedGrenadeLauncher(&pSoldier->inv[HANDPOS]);
	}

	// if he's got a MORTAR in his hand, make sure he has a MORTARSHELL avail.
	if (Item[usInHand].mortar)
	{
		//DebugShot( pSoldier, String("using mortar"));
		bPayloadPocket = FindNonSmokeLaunchable(pSoldier, usInHand);
		if (bPayloadPocket == NO_SLOT)
		{
			bPayloadPocket = FindLaunchable(pSoldier, usInHand);
		}
		if (bPayloadPocket == NO_SLOT)
		{
			//DebugShot( pSoldier, String("no shells, exit"));
			return;	// no shells, can't fire the MORTAR
		}
		// sevenfm: don't use mortar in building or underground
		if( IsRoofPresentAtGridNo( pSoldier->sGridNo ) && pSoldier->pathing.bLevel == 0 ||
			gfCaves || 
			gfBasement )
		{
			//DebugShot( pSoldier, String("mortar in a room or underground, exit"));
			return;	
		}
		if(pSoldier->bBlindedCounter > 0)
		{
			//DebugShot(pSoldier, String("cannot use mortar if blinded, exit"));
			return;
		}
		usGrenade = pSoldier->inv[bPayloadPocket].usItem;
		ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 2, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));
		if (usGrenade && Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SIGNAL_SMOKE)
		{
			ubSafetyMargin = TACTICAL_RANGE;
			fRedSmoke = TRUE;
		}
		fMortar = TRUE;
	}
	// if he's got a GL in his hand, make sure he has some type of GRENADE avail.
	else if (Item[usInHand].grenadelauncher)
	{
		// use up pocket 2 first, they get left as drop items
		//DebugShot( pSoldier, String("using GL"));

		// sevenfm: don't use grenade launcher underground
		// allow using grenade launcher underground as it uses lower trajectory now
		/*if( gfCaves || gfBasement )
		{
			//DebugShot( pSoldier, String("grenade launcher underground, exit"));
			return;	
		}*/

		if (pSoldier->bBlindedCounter > 0)
		{
			//DebugShot(pSoldier, String("cannot use grenade launcher if blinded, exit"));
			return;
		}

		//dnl ch63 240813 Check if grenade is already attach or find one in pockets
		bPayloadPocket = HANDPOS;
		pObjGL = FindAttachment_GrenadeLauncher(&pSoldier->inv[bPayloadPocket]);
		OBJECTTYPE *pAttachment = FindLaunchableAttachment(&pSoldier->inv[bPayloadPocket], usInHand);
		if(pAttachment->exists())
		{
			usGrenade = pAttachment->usItem;
			ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 2, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));
		}
		else if((bPayloadPocket=FindAmmoToReload(pSoldier, bPayloadPocket, NO_SLOT)) != NO_SLOT)
		{
			usGrenade = pSoldier->inv[bPayloadPocket].usItem;
			ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 2, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));
		}
		else
		{
			//DebugShot( pSoldier, String("no ammo to reload - return"));
			return;
		}
		if (usGrenade && Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SIGNAL_SMOKE)
		{
			ubSafetyMargin = TACTICAL_RANGE;
			fRedSmoke = TRUE;
		}
		fGrenadeLauncher = TRUE;
	}
	else if ( Item[usInHand].rocketlauncher )
	{
		//DebugShot( pSoldier, String("using rocket launcher"));

		if (pSoldier->bBlindedCounter > 0)
		{
			//DebugShot(pSoldier, String("cannot use rocket launcher if blinded, exit"));
			return;
		}

		// put in hand
		bPayloadPocket = HANDPOS;//dnl ch63 240813
		if ( Item[usInHand].singleshotrocketlauncher )
		{
			//DebugShot( pSoldier, String("using single shot launcher"));

			// sevenfm: for single shot rocket launchers, use buddy item instead
			if( Item[usInHand].usBuddyItem && Item[Item[usInHand].usBuddyItem].usItemClass & IC_EXPLOSV )
			{
				usGrenade = Item[usInHand].usBuddyItem;
				ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 2, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));
			}
			else
			{
				// as C1
				usGrenade = C1;
				ubSafetyMargin = (UINT8)(1 + Explosive[Item[usGrenade].ubClassIndex].ubRadius);
			}
		}
		else
		{
			//DebugShot( pSoldier, String("check launchable for rocket launcher"));

			bPayloadPocket = FindNonSmokeLaunchable ( pSoldier, usInHand );
			if (bPayloadPocket == NO_SLOT)
			{
				bPayloadPocket = FindLaunchable(pSoldier, usInHand);
			}
			if (bPayloadPocket == NO_SLOT)
			{
				//DebugShot( pSoldier, String("no ammo, can't fire"));
				return;	// no ammo, can't fire
			}
			usGrenade = pSoldier->inv[bPayloadPocket].usItem;
			ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 2, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));
		}
		fRocketLauncher = TRUE;
	}
	else if ( Item[usInHand].cannon )
	{
		//DebugShot( pSoldier, String("TANK - using cannon"));

		bPayloadPocket = FindNonSmokeLaunchable ( pSoldier, usInHand );
		//DebugShot( pSoldier, String("found ammo in pocket %d", bPayloadPocket));
		if (bPayloadPocket == NO_SLOT)
		{
			bPayloadPocket = FindLaunchable(pSoldier, usInHand);
		}
		if (bPayloadPocket == NO_SLOT)
		{
			//DebugShot( pSoldier, String("no ammo, can't fire - return"));
			return;	// no ammo, can't fire
		}		
		usGrenade = pSoldier->inv[bPayloadPocket].usItem;
		ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[pSoldier->inv[bPayloadPocket].usItem].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 2, Explosive[Item[pSoldier->inv[bPayloadPocket].usItem].ubClassIndex].ubFragRange / CELL_X_SIZE)));		
		fCannon = TRUE;
	}
	else
	{
		// else it's a plain old grenade, now in his hand
		//DebugShot( pSoldier, String("using grenade"));
		bPayloadPocket = HANDPOS;
		usGrenade = pSoldier->inv[bPayloadPocket].usItem;
		ubSafetyMargin = (UINT8)(1 + max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 4, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE)));

		if ( Item[usGrenade].flare )
		{
			ubSafetyMargin /= 2;
			//ubSafetyMargin = min(TACTICAL_RANGE / 4, ubSafetyMargin);
		}
		if (usGrenade && Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SIGNAL_SMOKE)
		{
			ubSafetyMargin = TACTICAL_RANGE / 2;
			fRedSmoke = TRUE;
		}
		fHandGrenade = TRUE;
	}
	// sevenfm: limit ubSafetyMargin in case it is set too high in XML
	ubSafetyMargin = min(ubSafetyMargin, TACTICAL_RANGE / 2);

	//DebugShot( pSoldier, String("safety margin %d", ubSafetyMargin));
	//DebugShot( pSoldier, String("grenade %d", usGrenade));
	//DebugShot( pSoldier, String("ammo in pocket %d", bPayloadPocket));

	if (EXPLOSIVE_GUN(usInHand))
	{
		//DebugShot( pSoldier, String("explosive gun"));
		iTossRange = GetModifiedGunRange(usInHand) / CELL_X_SIZE;
	}
	else
	{
		//DebugShot( pSoldier, String("calc max toss range"));
		// sevenfm: for hand grenades, use standing stance for max range
		if (fHandGrenade &&
			pSoldier->IsMercBodyType() &&
			IsValidStance(pSoldier, ANIM_STAND) &&
			gAnimControl[pSoldier->usAnimState].ubEndHeight < ANIM_STAND)
		{
			pSoldier->usAnimState = STANDING;
			iTossRange = CalcMaxTossRange(pSoldier, usInHand, TRUE);
			pSoldier->usAnimState = usTrueState;
		}
		else
		{
			iTossRange = CalcMaxTossRange(pSoldier, usInHand, TRUE);
		}
	}
	//DebugShot( pSoldier, String("max toss range %d", iTossRange));

	// use flares only at night
	if( usGrenade != NOTHING &&
		Item[usGrenade].flare &&
		!NightLight() )
	{
		//DebugShot( pSoldier, String("flare in day time, exit"));
		return;
	}

	// use red smoke only above ground
	if (fRedSmoke && gbWorldSectorZ)
	{
		//DebugShot( pSoldier, String("red smoke underground, exit"));
		return;
	}

	// sevenfm: don't use red smoke if artillery strikes disabled
	if (fRedSmoke && !gSkillTraitValues.fROAllowArtillery)
	{
		//DebugShot( pSoldier, String("red smoke with artillery strikes disabled, exit"));
		return;
	}
	
	ubDiff = SoldierDifficultyLevel( pSoldier );

	// make a list of tiles one's friends are positioned in
	for (ubLoop = 0; ubLoop < guiNumMercSlots; ubLoop++)
	{
		pFriend = MercSlots[ubLoop];

		if ( !pFriend )
		{
			continue; // next soldier
		}

		if (pFriend->stats.bLife == 0)
		{
			continue;
		}

		/*
		// if this soldier is inactive, at base, on assignment, or dead
		if (!Menptr[ubLoop].bActive || !Menptr[ubLoop].bInSector || !Menptr[ubLoop].bLife)
		continue;			// next soldier
		*/

		// if this man is neutral / NOT on the same side, he's not a friend
		if (pFriend->aiData.bNeutral || (pSoldier->bSide != pFriend->bSide))
		{
			continue;			// next soldier
		}

		// active friend, remember where he is so that we DON'T blow him up!
		// this includes US, since we don't want to blow OURSELVES up either
		sFriendTile[ubFriendCnt] = pFriend->sGridNo;
		bFriendLevel[ubFriendCnt] = pFriend->pathing.bLevel;
		ubFriendCnt++;
	}
	//DebugShot( pSoldier, String("finished making friends list, total friends %d", ubFriendCnt));

	// make a list of tiles one's CURRENTLY SEEN opponents are positioned in
	for (ubLoop = 0; ubLoop < guiNumMercSlots; ubLoop++)
	{
		pOpponent = MercSlots[ubLoop];

		if (!pOpponent)
		{
			// inactive or not in sector
			continue;			// next soldier
		}

		if (!pOpponent->stats.bLife)
		{
			continue;			// next soldier
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// sevenfm: skip attractive opponents
		if (pSoldier->IsMaleBodyType() &&
			pOpponent->HasStunningLooks() &&
			PersonalSeenKnowledge(pSoldier, pOpponent->ubID) &&
			pSoldier->ubPreviousAttackerID != pOpponent->ubID &&
			pSoldier->ubNextToPreviousAttackerID != pOpponent->ubID)
		{
			continue;
		}

		//DebugShot( pSoldier, String("checking opponent [%d]", pOpponent->ubID));

		bPersonalKnowledge = PersonalKnowledge(pSoldier, pOpponent->ubID);
		bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pOpponent->ubID);
		bKnowledge = Knowledge( pSoldier, pOpponent->ubID );

		// ignore unknown enemy if it's not jammer
		if (bKnowledge == NOT_HEARD_OR_SEEN && !(pOpponent->IsJamming() && TeamIsScanning(pSoldier->bTeam)))
		{
			//DebugShot( pSoldier, String("unknown enemy, ignore"));
			continue;
		}

		if (pSoldier->bBlindedCounter > 0 &&
			bPersonalKnowledge != SEEN_CURRENTLY &&
			bPersonalKnowledge != SEEN_THIS_TURN &&
			bPersonalKnowledge != HEARD_THIS_TURN)
		{
			//DebugShot(pSoldier, String("blind soldier cannot attack if not seen/heard this turn, ignore"));
			continue;
		}

		// sevenfm: blind attacks only for enemy team
		if (bKnowledge != SEEN_CURRENTLY &&
			pSoldier->bTeam != ENEMY_TEAM &&
			!(pSoldier->aiData.bUnderFire && fHandGrenade))
		{
			continue;
		}

		/*if(TileIsOutOfBounds(KnownLocation(pSoldier, pOpponent->ubID)))
		{
			//DebugShot(pSoldier, String("unknown location, ignore"));
			continue;
		}

		if(KnownLocation(pSoldier, pOpponent->ubID) == 0)
		{
			//DebugShot(pSoldier, String("known location == 0, ignore"));
			continue;
		}*/

		// sevenfm: don't use gas/flashbang against zombies
		if( usGrenade != NOTHING &&
			!Item[usGrenade].flare &&
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_NORMAL && 
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_STUN && 
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_CREATUREGAS &&
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_BURNABLEGAS &&
			pOpponent->IsZombie() )
		{
			//DebugShot( pSoldier, String("don't use gas/flashbang against zombies, ignore"));
			continue;
		}

		// limit smoke grenade use
		if (usGrenade != NOTHING &&
			Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SMOKE && 
			!pSoldier->RushAttackPrepare() &&
			!fSectorAttack &&
			(pOpponent->IsZombie() || 
			!AICheckHasGun(pOpponent) || 
			pOpponent->IsCowering() || 
			pOpponent->InShock() ||
			!AIRoleMarksman(pOpponent) && !AIRoleSniper(pOpponent) && !pOpponent->LastAttackHit() && !AIRoleMachinegunner(pOpponent) && !pOpponent->IsSpotting()))
		{
			//DebugShot( pSoldier, String("don't use smoke against zombies, melee soldiers and opponents with low marksmanship, ignore"));
			continue;
		}

		// sevenfm: only normal type explosives against robots, vehicles and tanks
		if( usGrenade != NOTHING &&
			!Item[usGrenade].flare &&
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_NORMAL &&
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_CREATUREGAS &&
			Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_BURNABLEGAS &&
			(pOpponent->IsTank() || pOpponent->IsVehicle() || pOpponent->IsRobot()) ) 
		{
			//DebugShot( pSoldier, String("only normal/creature/burnable type explosives against robots, vehicles and tanks, ignore"));
			continue;
		}

		// sevenfm: don't use grenades against dying enemies
		if( pOpponent->stats.bLife < OKLIFE && !pOpponent->IsZombie())
		{
			//DebugShot( pSoldier, String("don't use grenades against dying enemies, ignore"));
			continue;
		}

		// don't use stun/gas grenades against collapsed enemies
		if( usGrenade != NOTHING &&
			!Item[usGrenade].flare &&
			!pOpponent->IsZombie() &&
			//Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_NORMAL &&
			//Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_CREATUREGAS &&
			//Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_BURNABLEGAS &&
			//Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_MUSTGAS &&
			(pOpponent->bCollapsed || pOpponent->bBreathCollapsed) ) 
		{
			//DebugShot( pSoldier, String("don't use stun/gas grenades against collapsed enemies, ignore"));
			continue;
		}

		// don't use flare if soldier is in light
		if( usGrenade != NOTHING &&
			Item[usGrenade].flare &&
			InLightAtNight(pOpponent->sGridNo, pOpponent->pathing.bLevel) ) 
		{
			//DebugShot( pSoldier, String("don't use flare if soldier is in light, ignore"));
			continue;
		}		

		// don't use flares against opponents on roof
		if( usGrenade != NOTHING &&
			Item[usGrenade].flare &&
			KnownLevel(pSoldier, pOpponent->ubID) > 0 )
		{
			//DebugShot( pSoldier, String("flares don't work on roof, ignore"));
			continue;
		}

		// only use red smoke against player team or if artillery strike already ordered
		if (fRedSmoke && pOpponent->bTeam != OUR_TEAM && !fArtilleryStrikeOrdered)
		{
			continue;
		}

		// sevenfm: tanks cannot shoot at very close range
		if (pSoldier->IsTank() &&
			!pOpponent->IsVehicle() &&
			PythSpacesAway(pSoldier->sGridNo, KnownLocation(pSoldier, pOpponent->ubID)) < 6)
		{
			continue;
		}

		if( Item[usInHand].mortar )
		{
			//DebugShot( pSoldier, String("checking mortar"));

			// active KNOWN opponent, remember where he is so that we DO blow him up!
			if (bPersonalKnowledge == SEEN_CURRENTLY || 
				bPublicKnowledge == SEEN_CURRENTLY)
			{
				//DebugShot( pSoldier, String("opponent is seen currently, use exact location"));
				sOpponentTile[ubOpponentCnt] = pOpponent->sGridNo;
				bOpponentLevel[ubOpponentCnt] = pOpponent->pathing.bLevel;				
			}
			else if( (bKnowledge == SEEN_THIS_TURN || bKnowledge == SEEN_LAST_TURN || bKnowledge == HEARD_THIS_TURN || bKnowledge == HEARD_LAST_TURN) &&
				(AIRoleSniper(pOpponent) || AIRoleMortarOperator(pOpponent) || AIRoleRadioOperator(pOpponent) || TeamHighPercentKilled(pSoldier->bTeam) || fSectorCurFew || fSectorAttack) &&
					!TileIsOutOfBounds(KnownLocation( pSoldier, pOpponent->ubID )) )
			{
				//DebugShot( pSoldier, String("recently seen/heard, use known location"));				
				sOpponentTile[ubOpponentCnt] = KnownLocation( pSoldier, pOpponent->ubID );
				bOpponentLevel[ubOpponentCnt] = KnownLevel( pSoldier, pOpponent->ubID );
			}
			else if (pOpponent->IsJamming() && TeamIsScanning(pSoldier->bTeam))
			{
				//DebugShot( pSoldier, String("opponent is jamming, use location with randomization"));				
				sOpponentTile[ubOpponentCnt] = RandomizeOpponentLocation(pOpponent->sGridNo, pOpponent, DAY_VISION_RANGE / 2);
				bOpponentLevel[ubOpponentCnt] = pOpponent->pathing.bLevel;
				//DebugShot( pSoldier, String("new randomized spot %d", sOpponentTile[ubOpponentCnt]));
			}
			else
			{
				//DebugShot( pSoldier, String("target is not seen personally or public, ignore"));
				continue;			// next soldier
			}
		}
		else if (Item[usInHand].rocketlauncher)
		{
			//DebugShot( pSoldier, String("checking rocket launcher"));

			if (bPersonalKnowledge == SEEN_CURRENTLY || bPublicKnowledge == SEEN_CURRENTLY)
			{
				//DebugShot( pSoldier, String("opponent is seen currently (personal/public)"));

				// active KNOWN opponent, remember where he is so that we DO blow him up!
				sOpponentTile[ubOpponentCnt] = pOpponent->sGridNo;
				bOpponentLevel[ubOpponentCnt] = pOpponent->pathing.bLevel;
			}
			// check if opponent is in a building and was seen/heard recently
			else if(pSoldier->bTeam == ENEMY_TEAM && 
					(bKnowledge == SEEN_THIS_TURN || bKnowledge == SEEN_LAST_TURN || bKnowledge == HEARD_THIS_TURN || bKnowledge == HEARD_LAST_TURN) &&
					!TileIsOutOfBounds(KnownLocation( pSoldier, pOpponent->ubID )) &&
					(InARoom(KnownLocation( pSoldier, pOpponent->ubID ), NULL) || fSectorCurFew || fSectorAttack) )
			{
				//DebugShot( pSoldier, String("recently seen/heard, use known location"));

				sOpponentTile[ubOpponentCnt] = KnownLocation( pSoldier, pOpponent->ubID );
				bOpponentLevel[ubOpponentCnt] = KnownLevel( pSoldier, pOpponent->ubID );
			}
			else
			{
				continue;
			}			
		}
		// tank
		else if (Item[usInHand].cannon)
		{
			if (bPersonalKnowledge == SEEN_CURRENTLY || bPublicKnowledge == SEEN_CURRENTLY)
			{
				//DebugShot( pSoldier, String("opponent is seen currently (personal/public)"));

				// active KNOWN opponent, remember where he is so that we DO blow him up!
				sOpponentTile[ubOpponentCnt] = pOpponent->sGridNo;
				bOpponentLevel[ubOpponentCnt] = pOpponent->pathing.bLevel;
			}
			else if ((bKnowledge != NOT_HEARD_OR_SEEN && bKnowledge != HEARD_3_TURNS_AGO) &&
				(bKnowledge == SEEN_THIS_TURN || bKnowledge == SEEN_LAST_TURN || bKnowledge == HEARD_THIS_TURN || bKnowledge == HEARD_LAST_TURN || pSoldier->aiData.bUnderFire || TeamHighPercentKilled(pSoldier->bTeam)) &&
				!TileIsOutOfBounds(KnownLocation(pSoldier, pOpponent->ubID)))
			{
				sOpponentTile[ubOpponentCnt] = KnownLocation(pSoldier, pOpponent->ubID);
				bOpponentLevel[ubOpponentCnt] = KnownLevel(pSoldier, pOpponent->ubID);
			}
			else
			{
				continue;
			}

		}
		//else if( Item[usInHand].grenadelauncher )
		else	// hand grenade or GL
		{
			//DebugShot( pSoldier, String("checking grenade"));
			if (bPersonalKnowledge == SEEN_CURRENTLY || bPublicKnowledge == SEEN_CURRENTLY)
			{
				//DebugShot( pSoldier, String("opponent is seen currently (personal/public)"));

				// active KNOWN opponent, remember where he is so that we DO blow him up!
				sOpponentTile[ubOpponentCnt] = pOpponent->sGridNo;
				bOpponentLevel[ubOpponentCnt] = pOpponent->pathing.bLevel;
			}
			else if ( (bKnowledge == SEEN_THIS_TURN || bKnowledge == SEEN_LAST_TURN) &&
						!TileIsOutOfBounds(KnownLocation( pSoldier, pOpponent->ubID )) )
			{
				//DebugShot( pSoldier, String("opponent was seen this/last turn  (personal)"));

				/*if ( !CloseEnoughForGrenadeToss( pOpponent->sGridNo, KnownLocation( pSoldier, pOpponent->ubID ) ) )
				{
					//DebugShot( pSoldier, String("too far for grenade toss"));
					continue;
				}*/

				sOpponentTile[ubOpponentCnt] = KnownLocation( pSoldier, pOpponent->ubID );
				bOpponentLevel[ubOpponentCnt] = KnownLevel( pSoldier, pOpponent->ubID );
			}
			else if ( (bKnowledge == HEARD_THIS_TURN || bKnowledge == HEARD_LAST_TURN || bKnowledge == HEARD_2_TURNS_AGO) &&
						!TileIsOutOfBounds(KnownLocation( pSoldier, pOpponent->ubID )) )
			{
				//DebugShot( pSoldier, String("opponent was heard this/last turn (personal)"));

				/*if ( !CloseEnoughForGrenadeToss( pOpponent->sGridNo, KnownLocation( pSoldier, pOpponent->ubID ) ) )
				{
					//DebugShot( pSoldier, String("too far for grenade toss"));
					continue;
				}*/

				BOOLEAN fSkipGrenade = TRUE;

				// can always use flares
				if( usGrenade != NOTHING && Item[usGrenade].flare )
				{
					fSkipGrenade = FALSE;
				}

				// allow using non-lethal grenades on heard opponents
				/*if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					usGrenade != NOTHING &&
					//PersonalSeenKnowledge(pSoldier, pOpponent->ubID) &&
					//(pSoldier->stats.bLife < pSoldier->stats.bLifeMax || Chance(TeamPercentKilled(pSoldier->bTeam))) &&
					( Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_STUN ||
						Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_TEARGAS ||
						Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SMOKE ||
						Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_FLASHBANG && NightTime()))
				{
					fSkipGrenade = FALSE;
				}*/

				// allow using in dangerous situation
				if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					(	pSoldier->aiData.bUnderFire && pSoldier->stats.bLife < pSoldier->stats.bLifeMax ||
						//CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2) > 1 ||
						Chance(TeamPercentKilled(pSoldier->bTeam)) ||
						Chance(ubDiff * 10) ||
						pSoldier->RushAttackActive() ||
						//PersonalSeenKnowledge(pSoldier, pOpponent->ubID) ||
						fSectorCurFew && pSoldier->aiData.bOrders == STATIONARY ||
						fSectorCurFew && pSoldier->pathing.bLevel > KnownLevel(pSoldier, pOpponent->ubID) && pSoldier->aiData.bAlertStatus == STATUS_RED ||
						fSectorAttack && InARoom(KnownLocation(pSoldier, pOpponent->ubID), NULL)))
				{
					fSkipGrenade = FALSE;
				}

				// always allow in restricted sectors or if many soldiers are killed
				/*if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
					(fSectorCurFew ||
					TeamHighPercentKilled(pSoldier->bTeam) ||
					fSectorAttack))
				{
					fSkipGrenade = FALSE;
				}*/

				if( fSkipGrenade )
				{
					//DebugShot( pSoldier, String("skip using grenade for this opponent"));
					continue;
				}

				sOpponentTile[ubOpponentCnt] = KnownLocation( pSoldier, pOpponent->ubID );
				bOpponentLevel[ubOpponentCnt] = KnownLevel( pSoldier, pOpponent->ubID );
			}
			else
			{
				continue;
			}
		}

		// remember if we found tank as opponent
		if (pOpponent->IsTank() || pOpponent->IsVehicle() || pOpponent->IsRobot())
		{
			fFoundTank = TRUE;
		}

		// also remember who he is (which soldier #)
		ubOpponentID[ubOpponentCnt] = pOpponent->ubID;

		// remember how relatively dangerous this opponent is (ignore my cover)
		iOppThreatValue[ubOpponentCnt] = CalcManThreatValue(pOpponent,pSoldier->sGridNo,FALSE,pSoldier);

		// finally, test opponent tile
		if (TileIsOutOfBounds(sOpponentTile[ubOpponentCnt]))
		{
			//DebugShot(pSoldier, String("incorrect opponent tile %d, skip!", sOpponentTile[ubOpponentCnt]));
			continue;
		}

		// sevenfm: don't even check opponents too far for a throw
		if (PythSpacesAway(pSoldier->sGridNo, sOpponentTile[ubOpponentCnt]) > iTossRange + max(3, ubSafetyMargin / 2))
		{
			//DebugShot(pSoldier, String("too far - skip opponent"));
			// can't throw there!
			continue;
		}

		// hack fix - zero tile
		if (sOpponentTile[ubOpponentCnt] == 0)
		{
			//DebugShot(pSoldier, String("zero opponent tile, skip!"));
			continue;
		}

		//DebugShot(pSoldier, String("added opponent [%d] spot %d level %d threat %d", pOpponent->ubID, sOpponentTile[ubOpponentCnt], bOpponentLevel[ubOpponentCnt], iOppThreatValue[ubOpponentCnt]));

		ubOpponentCnt++;
	}

	//DebugShot( pSoldier, String("finished making opponents list, total opponents %d", ubOpponentCnt));

	// this is try to minimize enemies wasting their (limited) toss attacks, with the exception of break lights
	BOOLEAN fSpare = TRUE;

	// tanks don't spare
	if (fCannon)
	{
		fSpare = FALSE;
	}

	// don't spare non lethal grenades
	if (usGrenade != NOTHING &&
		(Item[usGrenade].flare ||
		Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_STUN ||
		//Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SMOKE ||
		Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_TEARGAS ||
		Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_FLASHBANG && NightTime()))
	{
		fSpare = FALSE;
	}

	// chance if some friends are killed
	if (Chance(TeamPercentKilled(pSoldier->bTeam)) && Chance(ubDiff * 10))
	{
		fSpare = FALSE;
	}

	// don't spare when soldier is under attack, rushing or too many killed
	if (pSoldier->aiData.bUnderFire ||
		pSoldier->RushAttackActive() ||
		MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO) ||
		TeamHighPercentKilled(pSoldier->bTeam) ||		
		fSectorCurFew ||
		fSectorAttack ||
		CountTeamUnderAttack(pSoldier->bTeam, pSoldier->sGridNo, DAY_VISION_RANGE / 2) > 0 ||
		pSoldier->aiData.bOrders == STATIONARY ||
		pSoldier->pathing.bLevel > 0 && pSoldier->aiData.bAlertStatus == STATUS_RED && fHandGrenade)
	{
		fSpare = FALSE;
	}	

	// don't spare RPGs and normal/burnable grenades against tanks/vehicles/robots
	if (fFoundTank &&
		//(pOpponent->IsTank() || pOpponent->IsVehicle() || pOpponent->IsRobot()) &&
		(fRocketLauncher || 
		usGrenade != NOTHING &&	(Item[usGrenade].flare || Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_NORMAL || Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_BURNABLEGAS)))
	{
		fSpare = FALSE;
	}

	// militia always try to spare grenades unless under attack or using flares or fighting tanks
	if (pSoldier->bTeam == MILITIA_TEAM &&
		!pSoldier->aiData.bUnderFire &&
		!Item[usGrenade].flare &&
		!fFoundTank)
		//!pOpponent->IsTank())
	{
		fSpare = TRUE;
	}

	// need 3 opponents for 0 difficulty, 1 opponent for max difficulty
	if ( fSpare && ubOpponentCnt < 3 - ubDiff / 2 )
	{
		return;
	}

	// look at the squares near each known opponent and try to find the one
	// place where a tossed projectile would do the most harm to the opponents
	// while avoiding one's friends
	for (ubLoop = 0; ubLoop < ubOpponentCnt; ubLoop++)
	{
		//DebugShot( pSoldier, String("check opponent %d", ubOpponentID[ubLoop]));

		fJammerFound = FALSE;
		
		if (MercPtrs[ubOpponentID[ubLoop]]->IsJamming() && TeamIsScanning(pSoldier->bTeam))
		{
			fJammerFound = TRUE;
		}

		// search all tiles within certain distance from opponent
		ubSearchRange = MAX_TOSS_SEARCH_DIST;

		// search 2 tiles if complex AI allowed
		if (fAllowComplexAI)
		{
			ubSearchRange++;
		}

		// sevenfm: increase possible distance from opponent when opponent in a building
		if (fAllowComplexAI &&
			gpWorldLevelData[sOpponentTile[ubLoop]].ubTerrainID == FLAT_FLOOR)
		{
			ubSearchRange++;
		}

		// sevenfm: increase possible distance when throwing gas grenades
		/*if (fAllowComplexAI &&
			usGrenade != NOTHING &&
			(Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_TEARGAS ||
			Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_MUSTGAS ||
			Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_BURNABLEGAS ||
			Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SIGNAL_SMOKE))
		{
			//DebugShot( pSoldier, String("increase range for gas grenade"));
			ubSearchRange++;
		}*/

		// increase possible distance when throwing flares		
		if (fAllowComplexAI && 
			Item[usGrenade].flare)
		{
			//DebugShot( pSoldier, String("increase range for flare"));
			ubSearchRange++;
		}

		// determine maximum horizontal limits
		bMaxLeft = ubSearchRange;
		bMaxRight = ubSearchRange;

		// determine maximum vertical limits
		bMaxUp	= ubSearchRange;
		bMaxDown = ubSearchRange;

		//DebugShot( pSoldier, String("evaluate every tile for its opponent-damaging potential"));
		// evaluate every tile for its opponent-damaging potential
		for (bYOffset = -bMaxUp; bYOffset <= bMaxDown; bYOffset++)
		{
			for (bXOffset = -bMaxLeft; bXOffset <= bMaxRight; bXOffset++)
			{
				// calculate the next potential gridno near this opponent
				sGridNo = sOpponentTile[ubLoop] + bXOffset + (MAXCOL * bYOffset);
				//DebugShot(pSoldier, String("check gridno %d", sGridNo));

				fStandUp = FALSE;

				// this shouldn't ever happen
				if ((sGridNo < 0) || (sGridNo >= GRIDSIZE))
				{
					//DebugShot(pSoldier, String("skip invalid gridno"));
					continue;
				}

				// sevenfm: exclude offscreen tiles as they may cause bugs
				if (!GridNoOnVisibleWorldTile(sGridNo))
					continue;

				if (PythSpacesAway(pSoldier->sGridNo, sGridNo) > iTossRange)
				{
					//DebugShot(pSoldier, String("too far - skip gridno"));
					// can't throw there!
					continue;
				}

				// if considering a gas/smoke grenade, check to see if there is such stuff already there!
				// sevenfm: check that only gas grenades are limited
				if (usGrenade &&
					(Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_TEARGAS && gpWorldLevelData[sGridNo].usExtFlags[bOpponentLevel[ubLoop]] & MAPELEMENT_EXT_TEARGAS ||
					Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_MUSTGAS && gpWorldLevelData[sGridNo].usExtFlags[bOpponentLevel[ubLoop]] & MAPELEMENT_EXT_MUSTARDGAS ||
					Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_BURNABLEGAS && gpWorldLevelData[sGridNo].usExtFlags[bOpponentLevel[ubLoop]] & MAPELEMENT_EXT_BURNABLEGAS ||
					Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SMOKE && gpWorldLevelData[sGridNo].usExtFlags[bOpponentLevel[ubLoop]] & MAPELEMENT_EXT_SMOKE ||
					Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SIGNAL_SMOKE && gpWorldLevelData[sGridNo].usExtFlags[bOpponentLevel[ubLoop]] & MAPELEMENT_EXT_SIGNAL_SMOKE))
				{
					//DebugShot(pSoldier, String("there's gas already - skip"));
					continue;
				}

				// don't use smoke grenade if there's already smoke nearby
				if (usGrenade && 
					Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SMOKE && 
					InSmokeNearby(sGridNo, bOpponentLevel[ubLoop]))
				{
					//DebugShot(pSoldier, String("smoke grenade, found smoke nearby - skip"));
					continue;
				}
			
				// sevenfm: skip tile if there's obstacle between opponent and spot
				if (sOpponentTile[ubLoop] != sGridNo &&
					!(usGrenade != NOTHING && Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_NORMAL && Explosive[Item[usGrenade].ubClassIndex].ubDamage >= 40) &&
					(InARoom(sOpponentTile[ubLoop], &usRoom1) != InARoom(sGridNo, &usRoom2) ||
					!LOS_Location(sOpponentTile[ubLoop], bOpponentLevel[ubLoop], sGridNo, bOpponentLevel[ubLoop], NO_DISTANCE_LIMIT)) )
				{
					//DebugShot( pSoldier, String("obstacle between opponent and spot - skip"));
					continue;
				}

				// don't throw signal smoke into room
				if (fRedSmoke && bOpponentLevel[ubLoop] == 0 && InARoom(sOpponentTile[ubLoop], NULL))
				{
					//DebugShot( pSoldier, String("throwing signal smoke into room - skip"));
					continue;
				}

				// skip if there's low obstacle between opponent and spot
				if (sOpponentTile[ubLoop] != sGridNo &&
					usGrenade &&
					Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_SMOKE &&
					Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_FLARE &&
					Explosive[Item[usGrenade].ubClassIndex].ubType != EXPLOSV_BURNABLEGAS &&
					!(Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_NORMAL && Explosive[Item[usGrenade].ubClassIndex].ubDamage >= 40) &&					
					!LOS_LocationStance(sOpponentTile[ubLoop], bOpponentLevel[ubLoop], ANIM_PRONE, sGridNo, bOpponentLevel[ubLoop], ANIM_PRONE, NO_DISTANCE_LIMIT))
				{
					//DebugShot(pSoldier, String("low obstacle between opponent and spot - skip"));
					continue;
				}
								
				fSkipLocation = FALSE;
				// Check to see if we have considered this tile before:
				for (ubLoop2 = 0; ubLoop2 < ubNumExcludedTiles; ubLoop2++)
				{
					if (sExcludeTile[ubLoop2] == sGridNo)
					{
						// already checked!
						fSkipLocation = TRUE;
						break;
					}
				}
				if (fSkipLocation)
				{
					//DebugShot( pSoldier, String("already checked - skip"));
					continue;
				}

				// calculate minimum action points required to throw at this gridno
				sMinAPcost = MinAPsToAttack(pSoldier, sGridNo, ADDTURNCOST, 0);
				//DebugShot(pSoldier, String("min APs to attack %d", sMinAPcost));

				// if we don't have enough APs left to throw even without aiming
				//DebugShot( pSoldier, String("soldier APs %d", pSoldier->bActionPoints));
				if (sMinAPcost > pSoldier->bActionPoints)
				{
					//DebugShot( pSoldier, String("not enough AP - skip"));
					continue;				// next gridno
				}

				// check whether there are any friends standing near this gridno
				fFriendsNearby = FALSE;

				for (ubLoop2 = 0; ubLoop2 < ubFriendCnt; ubLoop2++)
				{
					if ( (bFriendLevel[ubLoop2] == bOpponentLevel[ubLoop]) && ( PythSpacesAway(sFriendTile[ubLoop2],sGridNo) <= ubSafetyMargin ) )
					{
						//DebugShot( pSoldier, String("found friends nearby"));
						fFriendsNearby = TRUE;
						break;		// don't bother checking any other friends
					}
				}

				if (fFriendsNearby)
				{
					//DebugShot( pSoldier, String("found friends nearby - skip"));
					continue;		// this location is no good, move along now
				}

				// Well this place shows some promise, evaluate its "damage potential"
				iTotalThreatValue = 0;
				ubOppsInRange = 0;
				ubOppsAdjacent = 0;

				//DebugShot( pSoldier, String("check opponents"));
				for (ubLoop2 = 0; ubLoop2 < ubOpponentCnt; ubLoop2++)
				{
					usOppDist = PythSpacesAway( sOpponentTile[ubLoop2], sGridNo );

					// if this opponent is close enough to the target gridno
					if (usOppDist <= max(3, ubSafetyMargin / 2))
					{
						// start with this opponents base threat value
						iThreatValue = iOppThreatValue[ubLoop2];

						// estimate how much damage this tossed item would do to him
						iEstDamage = EstimateThrowDamage(pSoldier,bPayloadPocket,MercPtrs[ubOpponentID[ubLoop2]],sGridNo);
						//DebugShot( pSoldier, String("estimate damage %d", iEstDamage));

						if (usOppDist)
						{
							// reduce the estimated damage for his distance from gridno
							// use 100% at range 0, 80% at range 1, and 60% at range 2, etc.
							iEstDamage = (iEstDamage * (100 - (20 * usOppDist))) / 100;
						}

						// add the product of his threat value & damage caused to total
						iTotalThreatValue += (iThreatValue * iEstDamage);

						// only count opponents still standing worth shooting at (in range)
						if (Menptr[ ubOpponentID[ubLoop2] ].stats.bLife >= OKLIFE)
						{
							ubOppsInRange++;
							if (usOppDist < 2)
							{
								ubOppsAdjacent++;
							}
						}
					}
				}

				if( ubOppsInRange == 0 )
				{
					//DebugShot( pSoldier, String("no opponents - skip"));
					continue;
				}

				if (ubOppsAdjacent >= 1 && ubNumExcludedTiles < EXCLUDE_TILE_SIZE)
				{
					// add to exclusion list so we don't calculate for this location twice
					sExcludeTile[ubNumExcludedTiles] = sGridNo;
					ubNumExcludedTiles++;
				}

				// calculate chance to get through any cover to this gridno
				//ubChanceToGetThrough = ChanceToGetThrough(pSoldier,sGridNo,NOTFAKE,ACTUAL,TESTWALLS,9999,M9PISTOL,NOT_FOR_LOS);

				if (EXPLOSIVE_GUN(usInHand))
				{
					//DebugShot( pSoldier, String("using explosive gun"));
					gUnderFire.Clear();
					gUnderFire.Enable();
					ubChanceToGetThrough = AISoldierToLocationChanceToGetThrough( pSoldier, sGridNo, bOpponentLevel[ubLoop], 0 );
					ubFriendlyFireChance = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);
					gUnderFire.Disable();
					//DebugShot( pSoldier, String("CTGT = %d", ubChanceToGetThrough));

					// if opponent is in a room, use last spot instead
					if( pSoldier->bTeam == ENEMY_TEAM &&
						ubChanceToGetThrough == 0 &&
						bOpponentLevel[ubLoop] == 0 &&
						InARoom(sOpponentTile[ubLoop], NULL) &&
						!TileIsOutOfBounds(gsLastBulletSpot) &&
						usGrenade != NOTHING &&
						Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_NORMAL &&
						Explosive[Item[usGrenade].ubClassIndex].ubDamage >= 40 &&
						PythSpacesAway(pSoldier->sGridNo, gsLastBulletSpot) > ubSafetyMargin * 2 &&
						CountNearbyFriends(pSoldier, gsLastBulletSpot, ubSafetyMargin) == 0 &&
						CountNearbyNeutrals(pSoldier, gsLastBulletSpot, ubSafetyMargin == 0) )
					{
						//DebugShot( pSoldier, String("found last bullet spot %d", gsLastBulletSpot));
						
						UINT8	ubChanceToGetThrough2;
						UINT8	ubFriendlyFireChance2;
						INT32	sLastSpot;

						INT32	sTempGridNo;
						UINT8	ubDirection;						

						sLastSpot = gsLastBulletSpot;

						gUnderFire.Clear();
						gUnderFire.Enable();
						ubChanceToGetThrough2 = AISoldierToLocationChanceToGetThrough( pSoldier, sLastSpot, bOpponentLevel[ubLoop], 0 );
						ubFriendlyFireChance2 = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);
						gUnderFire.Disable();

						if( ubChanceToGetThrough2 > 0 &&
							ubFriendlyFireChance2 <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE &&
							LOS_Virtual(pSoldier, sLastSpot, bOpponentLevel[ubLoop], ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
						{
							sGridNo = sLastSpot;
							ubChanceToGetThrough = ubChanceToGetThrough2;
							ubFriendlyFireChance = ubFriendlyFireChance2;
							//DebugShot( pSoldier, String("use new sGridNo %d new CTGT %d", sGridNo, ubChanceToGetThrough));
						}

						// check adjacent tiles
						for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
						{
							sTempGridNo = NewGridNo( sLastSpot, DirectionInc( ubDirection ) );

							if( sTempGridNo != sLastSpot )
							{
								gUnderFire.Clear();
								gUnderFire.Enable();
								ubChanceToGetThrough2 = AISoldierToLocationChanceToGetThrough( pSoldier, sTempGridNo, bOpponentLevel[ubLoop], 0 );
								ubFriendlyFireChance2 = gUnderFire.Chance(pSoldier->bTeam, pSoldier->bSide, TRUE);
								gUnderFire.Disable();

								if( ubChanceToGetThrough2 > ubChanceToGetThrough &&
									ubFriendlyFireChance2 <= MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE &&
									LOS_Virtual(pSoldier, sTempGridNo, bOpponentLevel[ubLoop], ANIM_PRONE, LOS_NORMAL, CALC_FROM_ALL_DIRS) )
								{
									sGridNo = sTempGridNo;
									ubChanceToGetThrough = ubChanceToGetThrough2;
									ubFriendlyFireChance = ubFriendlyFireChance2;
									//DebugShot( pSoldier, String("use new adjacent sGridNo %d new CTGT %d", sGridNo, ubChanceToGetThrough));
								}
							}							
						}
					}

					// sevenfm: avoid using RPG if can hit friends
					if (ubChanceToGetThrough == 0 ||
						!pSoldier->IsTank() && ubFriendlyFireChance > MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE ||
						PythSpacesAway(pSoldier->sGridNo, gsLastBulletSpot) < ubSafetyMargin ||
						CountNearbyFriends(pSoldier, gsLastBulletSpot, ubSafetyMargin) > 0 ||
						!pSoldier->IsTank() && CountNearbyNeutrals(pSoldier, gsLastBulletSpot, ubSafetyMargin) > 0 && !pSoldier->aiData.bUnderFire ||
						!pSoldier->IsTank() && !MercPtrs[ubOpponentID[ubLoop]]->IsTank() && CountFriendsInDirection(pSoldier, AIDirection(pSoldier->sGridNo, sGridNo), PythSpacesAway(pSoldier->sGridNo, sGridNo), TRUE))
					{
						//DebugShot( pSoldier, String("CTGT = 0 - skip"));
						continue; // next gridno
					}
				}
				else
				{
					//DebugShot( pSoldier, String("checking chance for launcher to beat cover"));

					ubChanceToGetThrough = 100 * CalculateLaunchItemChanceToGetThrough( pSoldier, (pObjGL ? pObjGL : &pSoldier->inv[bPayloadPocket]), sGridNo, bOpponentLevel[ubLoop], 0, &sEndGridNo, TRUE, &bEndLevel, FALSE );//dnl ch63 240813
					ubFriendlyFireChance = 0;

					//DebugShot( pSoldier, String("CTGT = %d", ubChanceToGetThrough));
					// if we can't possibly get through all the cover
					if (ubChanceToGetThrough == 0 )
					{
						//DebugShot( pSoldier, String("CTGT = 0"));
						if ( bEndLevel == bOpponentLevel[ubLoop] && ubSafetyMargin > 1 )
						{
							// rate "chance of hitting" according to how far away this is from the target
							// but keeping in mind that we don't want to hit far, subtract 1 from the radius here
							// to penalize being far from the target
							uiPenalty = 100 * PythSpacesAway( sGridNo, sEndGridNo ) / (ubSafetyMargin - 1);
							// sevenfm: check that spots are in the same room or there is LOS between
							if (uiPenalty < 100 && 
								(InARoom(sGridNo, &usRoom1) == InARoom(sEndGridNo, &usRoom2) && usRoom1 == usRoom2 && usRoom1 != NO_ROOM || 
								LOS_Location(sGridNo, bOpponentLevel[ubLoop], sEndGridNo, bEndLevel, NO_DISTANCE_LIMIT)))
							{								
								ubChanceToGetThrough = 100 - (UINT8) uiPenalty;
								//DebugShot( pSoldier, String("calculated CTGT %d", ubChanceToGetThrough));
							}
							/*else
							{
								//DebugShot( pSoldier, String("skip"));
								continue;
							}*/
						}
						else if ( Item[usInHand].mortar && 
								usGrenade != NOTHING && 
								Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_NORMAL &&
								Explosive[Item[usGrenade].ubClassIndex].ubDamage >= 40 &&
								bOpponentLevel[ubLoop] == 0 && 
								InARoom(sOpponentTile[ubLoop], NULL) &&
								bEndLevel == 1 &&
								PythSpacesAway(sGridNo, sEndGridNo) < DAY_VISION_RANGE / 4)
						{
							// calc new chance to hit roof above spot
							ubChanceToGetThrough = 100 * CalculateLaunchItemChanceToGetThrough( pSoldier, (pObjGL ? pObjGL : &pSoldier->inv[bPayloadPocket]), sGridNo, 1, 0, &sEndGridNo, TRUE, &bEndLevel, FALSE );
						}
						/*else
						{
							//DebugShot( pSoldier, String("skip"));
							continue;
						}*/

						// sevenfm: for hand grenade, try throwing from standing stance
						if (ubChanceToGetThrough == 0 &&
							fHandGrenade &&
							pSoldier->IsMercBodyType() &&
							pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, sGridNo), ANIM_STAND) &&
							gAnimControl[pSoldier->usAnimState].ubEndHeight < ANIM_STAND &&
							MinAPsToThrow(pSoldier, sGridNo, TRUE) + GetAPsToChangeStance(pSoldier, ANIM_STAND) <= pSoldier->bActionPoints)
						{
							// fake soldier standing
							pSoldier->usAnimState = STANDING;
							// calc APs to throw, including turning cost when standing
							sMinAPcost = MinAPsToAttack(pSoldier, sGridNo, ADDTURNCOST, 0);
							// if can afford AP cost, check toss chance from standing stance
							if (sMinAPcost + GetAPsToChangeStance(pSoldier, ANIM_STAND) <= pSoldier->bActionPoints)
							{
								ubChanceToGetThrough = 100 * CalculateLaunchItemChanceToGetThrough(pSoldier, (pObjGL ? pObjGL : &pSoldier->inv[bPayloadPocket]), sGridNo, bOpponentLevel[ubLoop], 0, &sEndGridNo, TRUE, &bEndLevel, FALSE);
								if (ubChanceToGetThrough == 0 && bEndLevel == bOpponentLevel[ubLoop] && ubSafetyMargin > 1)
								{
									// rate "chance of hitting" according to how far away this is from the target
									// but keeping in mind that we don't want to hit far, subtract 1 from the radius here
									// to penalize being far from the target
									uiPenalty = 100 * PythSpacesAway(sGridNo, sEndGridNo) / (ubSafetyMargin - 1);
									// sevenfm: check that spots are in the same room or there is LOS between
									if (uiPenalty < 100 &&
										(InARoom(sGridNo, &usRoom1) == InARoom(sEndGridNo, &usRoom2) && usRoom1 == usRoom2 && usRoom1 != NO_ROOM ||
										LOS_Location(sGridNo, bOpponentLevel[ubLoop], sEndGridNo, bEndLevel, NO_DISTANCE_LIMIT)))
									{
										ubChanceToGetThrough = 100 - (UINT8)uiPenalty;
										//DebugShot( pSoldier, String("calculated CTGT %d", ubChanceToGetThrough));
									}
								}
							}
							// restore soldier's state
							pSoldier->usAnimState = usTrueState;

							fStandUp = TRUE;
						}

						// if still cannot hit, skip location
						if (ubChanceToGetThrough == 0)
						{
							//DebugShot(pSoldier, String("skip"));
							continue;
						}
					}
				}

				// this is try to minimize enemies wasting their (few) mortar shells or LAWs
				// they won't use them on less than 2 targets as long as half life left
				// for militia team, only attack if can hit several opponents
				if( Item[usInHand].mortar && 
					ubOppsInRange < 2 && 
					!MercPtrs[ubOpponentID[ubLoop]]->IsTank() &&
					!fJammerFound &&
					(pSoldier->bTeam == MILITIA_TEAM ||
					!pSoldier->aiData.bUnderFire &&
					!fSectorCurFew &&
					!fSectorAttack &&
					!AIRoleSniper(MercPtrs[ ubOpponentID[ubLoop] ]) &&
					!AIRoleRadioOperator(MercPtrs[ ubOpponentID[ubLoop] ]) &&
					!AIRoleMortarOperator(MercPtrs[ ubOpponentID[ubLoop] ]) &&
					!TeamHighPercentKilled(pSoldier->bTeam)))
				{
					//DebugShot( pSoldier, String("using mortar against single opponent - skip"));
					continue;				// next gridno
				}

				// for militia team, only attack if can hit several opponents
				if( Item[usInHand].rocketlauncher && 
					ubOppsInRange < 2 && 
					!MercPtrs[ubOpponentID[ubLoop]]->IsTank() &&
					!MercPtrs[ubOpponentID[ubLoop]]->IsVehicle() &&
					(pSoldier->bTeam == MILITIA_TEAM ||
					!pSoldier->aiData.bUnderFire &&
					!InARoom(sOpponentTile[ubLoop], NULL) &&
					!pSoldier->RushAttackPrepare() &&
					!TeamHighPercentKilled(pSoldier->bTeam) &&
					!fSectorCurFew &&
					!fSectorAttack &&
					!AnyCoverFromSpot(sOpponentTile[ubLoop], bOpponentLevel[ubLoop], pSoldier->sGridNo, pSoldier->pathing.bLevel)) )
				{
					//DebugShot( pSoldier, String("using rocket launcher against single opponent - skip"));
					continue;				// next gridno
				}
				
				// calc next attack's minimum AP cost (excludes readying & turning)

				// since grenades & shells are far too valuable to waste, ALWAYS
				// aim for the longest time possible!

				//DebugShot( pSoldier, String("check CTH"));
				if (EXPLOSIVE_GUN(usInHand))
				{
					//DebugShot( pSoldier, String("using explosive gun"));
					// calculate the maximum possible aiming time
					// HEADROCK HAM 4: Required for new Aiming Level Limits function
					ubMaxPossibleAimTime = CalcAimingLevelsAvailableWithAP(pSoldier, sGridNo, pSoldier->bActionPoints - sMinAPcost);//dnl ch63 250813
					//DebugShot( pSoldier, String("max aim time %d", ubMaxPossibleAimTime));

					sRawAPCost = MinAPsToShootOrStab(pSoldier, sGridNo, ubMaxPossibleAimTime, FALSE);
					//DebugShot(pSoldier, String("raw AP cost %d", sRawAPCost));

					ubChanceToHit = (UINT8)AICalcChanceToHitGun(pSoldier, sGridNo, ubMaxPossibleAimTime, AIM_SHOT_TORSO, bOpponentLevel[ubLoop], STANDING);//dnl ch59 130813
					//DebugShot( pSoldier, String("CTH %d", ubChanceToHit));
				}
				else
				{
					ubMaxPossibleAimTime = 0;
					// NB grenade launcher is NOT a direct fire weapon!
					sRawAPCost = (UINT8) MinAPsToThrow(pSoldier, sGridNo, FALSE);
					//DebugShot(pSoldier, String("raw AP cost %d", sRawAPCost));

					if (fStandUp)
					{
						pSoldier->usAnimState = STANDING;
						// calculate throw AP cost when standing
						sRawAPCost = (UINT8)MinAPsToThrow(pSoldier, sGridNo, FALSE);
						ubChanceToHit = (UINT8)CalcThrownChanceToHit(pSoldier, sGridNo, ubMaxPossibleAimTime, AIM_SHOT_TORSO);
						pSoldier->usAnimState = usTrueState;
						// don't add standing AP cost for raw AP cost as it's only applied once
						//sRawAPCost += GetAPsToChangeStance(pSoldier, ANIM_STAND);
					}
					else
					{
						ubChanceToHit = (UINT8)CalcThrownChanceToHit(pSoldier, sGridNo, ubMaxPossibleAimTime, AIM_SHOT_TORSO);
					}					
					//DebugShot( pSoldier, String("CTH %d", ubChanceToHit));
				}

				// sevenfm: need higher chance to hit when attacking opponent in a room with hand grenade or grenade launcher
				if ((fHandGrenade || fGrenadeLauncher) && 
					(InARoom(pSoldier->sGridNo, NULL) && pSoldier->pathing.bLevel == 0 || InARoom(sGridNo, NULL) && bOpponentLevel[ubLoop] == 0 || gfCaves || gfBasement) &&
					(ubChanceToHit < 80 || ubChanceToGetThrough < 80))
				{
					continue;
				}

				// mortars are inherently quite inaccurate, don't get proximity bonus
				// rockets can go right past people too so...
				if (!EXPLOSIVE_GUN(usInHand) || MercPtrs[ubOpponentID[ubLoop]]->IsTank() || MercPtrs[ubOpponentID[ubLoop]]->IsVehicle())
				{
					//DebugShot( pSoldier, String("bonus 50% to CTH"));
					// special 50% to Hit bonus: this reflects that even if a tossed item
					// misses by a bit, it's still likely to affect the intended target(s)
					ubChanceToHit += (ubChanceToHit / 2);

					// still can't let it go over 100% chance, though
					ubChanceToHit = min(ubChanceToHit, 100);
				}

				if (sRawAPCost < 1)
					sRawAPCost = sMinAPcost;

				sAimAPCost = CalcAPCostForAiming(pSoldier, sGridNo, (INT8)ubMaxPossibleAimTime);
				iHitRate = (pSoldier->bActionPoints * ubChanceToHit) / (sRawAPCost + sAimAPCost);
				//DebugShot( pSoldier, String("hit rate %d", iHitRate));

				// calculate chance to REALLY hit: throw accurately AND get past cover
				ubChanceToReallyHit = (ubChanceToHit * ubChanceToGetThrough) / 100;
				//DebugShot( pSoldier, String("chance to really hit %d", ubChanceToReallyHit));

				// if we can't REALLY hit at all
				if (ubChanceToReallyHit == 0)
				{
					//DebugShot( pSoldier, String("chance to really hit = 0 - skip"));
					continue;				// next gridno
				}

				// calculate the combined "attack value" for this opponent
				// maximum possible attack value here should be about 140 million
				// typical attack value here should be about 500 thousand
				
				iAttackValue = (iHitRate * ubChanceToReallyHit * iTotalThreatValue) / 1000;
				//DebugShot( pSoldier, String("attack value %d", iAttackValue));

				// modify smoke attack value depending on range
				if (usGrenade && Explosive[Item[usGrenade].ubClassIndex].ubType == EXPLOSV_SMOKE && PythSpacesAway(pSoldier->sGridNo, sGridNo) < DAY_VISION_RANGE)
				{
					iAttackValue = iAttackValue * PythSpacesAway(pSoldier->sGridNo, sGridNo) / DAY_VISION_RANGE;
					//DebugShot(pSoldier, String("lower smoke attack value %d", iAttackValue));
				}
				
				// unlike SHOOTing and STABbing, find strictly the highest attackValue
				// sevenfm: empty vehicle has low priority
				if ((iAttackValue > pBestThrow->iAttackValue || 
					iAttackValue > 0 && pBestThrow->ubPossible == TRUE && pBestThrow->ubOpponent != NOBODY && MercPtrs[pBestThrow->ubOpponent]->IsEmptyVehicle()) &&
					(pBestThrow->ubPossible == FALSE || 
					pBestThrow->ubOpponent != NOBODY && MercPtrs[pBestThrow->ubOpponent]->IsEmptyVehicle() || 
					ubOpponentID[ubLoop] != NOBODY && !MercPtrs[ubOpponentID[ubLoop]]->IsEmptyVehicle()))
				{
#ifdef DEBUGATTACKS
					DebugAI( String( "CalcBestThrow: new best attackValue vs %d = %d\n",ubOpponentID[ubLoop],iAttackValue ) );
#endif

					// OOOF!	That was a lot of work!	But we've got a new best target!
					pBestThrow->ubPossible			= TRUE;
					pBestThrow->ubOpponent			= ubOpponentID[ubLoop];
					pBestThrow->ubAimTime			= ubMaxPossibleAimTime;
					pBestThrow->ubChanceToReallyHit = ubChanceToReallyHit;
					pBestThrow->sTarget				= sGridNo;
					pBestThrow->iAttackValue		= iAttackValue;
					pBestThrow->bTargetLevel		= bOpponentLevel[ubLoop];
					pBestThrow->ubFriendlyFireChance = ubFriendlyFireChance;

					//pBestThrow->ubAPCost			= sMinAPcost + CalcAPCostForAiming(pSoldier, sGridNo, ubMaxPossibleAimTime);//dnl ch64 310813
					//pBestThrow->ubStance			= fStandUp ? ANIM_STAND : gAnimControl[pSoldier->usAnimState].ubEndHeight;

					if (fStandUp)
					{
						// if we change stance, calculate throw cost when standing, add AP cost to stand in current stance
						// fake standing stance
						pSoldier->usAnimState = STANDING;
						// calculate throw AP cost when standing
						pBestThrow->ubAPCost = MinAPsToAttack(pSoldier, sGridNo, ADDTURNCOST, ubMaxPossibleAimTime);
						// restore current stance
						pSoldier->usAnimState = usTrueState;

						// add AP cost to stand from current stance
						pBestThrow->ubAPCost += GetAPsToChangeStance(pSoldier, ANIM_STAND);
						// set standing stance
						pBestThrow->ubStance = ANIM_STAND;
					}
					else
					{
						// if not changing stance, use standard calculation with max aim and turning cost
						pBestThrow->ubAPCost = MinAPsToAttack(pSoldier, sGridNo, ADDTURNCOST, ubMaxPossibleAimTime);
						// set current stance
						pBestThrow->ubStance = gAnimControl[pSoldier->usAnimState].ubEndHeight;
					}

					//DebugShot( pSoldier, String("found attack; opponent %d, aim time %d, CTH %d, Target %d, Level %d, AP cost %d", pBestThrow->ubOpponent, pBestThrow->ubAimTime, pBestThrow->ubChanceToReallyHit, pBestThrow->sTarget, pBestThrow->bTargetLevel, pBestThrow->ubAPCost));
					//sprintf(tempstr,"new best THROW AttackValue = %d at grid #%d",iAttackValue/100000,gridno);
					//PopMessage(tempstr);
				}
			}
		}
	}

	// this is try to minimize enemies wasting their (limited) toss attacks:
	// sevenfm 80-40% depending on soldier difficulty
	UINT8 ubMinChanceToReallyHit = 40 + 10 * ubDiff;
	if( Item[usGrenade].flare )
	{
		ubMinChanceToReallyHit = 30;
	}

	// sevenfm: allow throwing more when soldier is under attack or has no gun or many team soldiers killed
	if (!fRocketLauncher &&
		!fCannon &&
		(pSoldier->aiData.bUnderFire || !AICheckHasGun(pSoldier) || pSoldier->RushAttackPrepare() || TeamHighPercentKilled(pSoldier->bTeam) || fSectorAttack))
	{
		ubMinChanceToReallyHit = ubMinChanceToReallyHit / 2;
	}

	// sevenfm: need higher chance when using rocket launcher
	if (fRocketLauncher && 
		!MercPtrs[pBestThrow->ubOpponent]->IsTank() && 
		!MercPtrs[pBestThrow->ubOpponent]->IsVehicle() &&
		!InARoom(pBestThrow->sTarget, NULL) &&
		//!pSoldier->aiData.bUnderFire &&
		!TeamHighPercentKilled(pSoldier->bTeam))
	{
		ubMinChanceToReallyHit += 20;
	}

	// sevenfm: need much lower cth when using tank cannon
	if (fCannon)
	{
		ubMinChanceToReallyHit /= 2;

		if (pSoldier->aiData.bUnderFire || TeamHighPercentKilled(pSoldier->bTeam))
			ubMinChanceToReallyHit /= 2;
	}

	if( pBestThrow->ubChanceToReallyHit < ubMinChanceToReallyHit )
	{
		pBestThrow->ubPossible = FALSE;
		//DebugShot( pSoldier, String("too low chance to really hit - disable attack"));
	}

	DebugMsg (TOPIC_JA2,DBG_LEVEL_3,"calcbestthrow done");
}

void CalcBestStab(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestStab, BOOLEAN fBladeAttack )
{
	UINT32 uiLoop;
	INT32 iAttackValue;
	INT32 iThreatValue,iHitRate,iBestHitRate,iPercentBetter, iEstDamage;
	BOOLEAN fSurpriseStab;
	INT16 ubRawAPCost,ubMinAPCost,ubMaxPossibleAimTime = 0;
	INT16 ubChanceToReallyHit = 0;
	INT16 ubAimTime,ubChanceToHit,ubBestAimTime;
	SOLDIERTYPE *pOpponent;
	UINT16 usTrueMovementMode;
	INT16 ubBestChanceToHit;	

	//DebugShot(pSoldier, String("\nCalcBestStab [%d] AP=%d", pSoldier->ubID, pSoldier->bActionPoints));

	pSoldier->usAttackingWeapon = pSoldier->inv[HANDPOS].usItem;

	// sevenfm: initialize
	pBestStab->ubPossible = FALSE;
	pBestStab->iAttackValue = 0;	
	pBestStab->ubChanceToReallyHit = 0;
	pBestStab->ubOpponent = NOBODY;

	// temporarily make this guy run so we get a proper AP cost value
	// from CalcTotalAPsToAttack
	usTrueMovementMode = pSoldier->usMovementMode;

	if(pSoldier->bBlindedCounter > 0)
		pSoldier->usMovementMode = SWATTING;
	else
		pSoldier->usMovementMode = RUNNING;

	// determine which attack against which target has the greatest attack value

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->stats.bLife)
		{
			continue;			// next merc
		}

		if (!ValidOpponent(pSoldier, pOpponent))
		{
			continue;
		}

		// sevenfm: don't try to stab vehicle
		if (pOpponent->IsEmptyVehicle())
		{
			continue;
		}

		// if this opponent is not currently in sight (ignore known but unseen!)
		if (PersonalKnowledge(pSoldier, pOpponent->ubID) != SEEN_CURRENTLY &&
			PublicKnowledge(pSoldier->bTeam, pOpponent->ubID) != SEEN_CURRENTLY)
		{
			continue;			// next merc
		}

		// blind soldier can only attack personally seen/heard opponents
		if (pSoldier->bBlindedCounter > 0 &&
			PersonalKnowledge(pSoldier, pOpponent->ubID) != SEEN_CURRENTLY &&
			PersonalKnowledge(pSoldier, pOpponent->ubID) != SEEN_THIS_TURN &&
			PersonalKnowledge(pSoldier, pOpponent->ubID) != HEARD_THIS_TURN)
		{
			continue;			// next merc
		}

		// blind soldier can attack only opponents next to him
		if (pSoldier->bBlindedCounter > 0 &&
			SpacesAway(pSoldier->sGridNo, pOpponent->sGridNo) > 1)
		{
			continue;			// next merc
		}

		// if this opponent is not on the same level
		if (pSoldier->pathing.bLevel != pOpponent->pathing.bLevel)
		{
			continue;			// next merc
		}

#ifdef DEBUGATTACKS
		DebugAI( String( "%s can see %s\n",pSoldier->name,ExtMen[pOpponent->ubID].name ) );
#endif

		//DebugShot(pSoldier, String("check opponent %d", pOpponent->ubID));

		// calculate minimum action points required to stab at this opponent
		ubMinAPCost = CalcTotalAPsToAttack( pSoldier,pOpponent->sGridNo,ADDTURNCOST, 0 );

		//DebugShot(pSoldier, String("ubMinAPCost %d", ubMinAPCost));

		//ubMinAPCost = MinAPsToAttack(pSoldier,pOpponent->sGridNo,ADDTURNCOST);

		// Human: if I don't have enough APs left to get there & stab at this guy, skip 'im.
		// Monster:	I'll do an extra check later on to see if I can reach the guy this turn.

		// if 0 is returned then no path!
		if ( ubMinAPCost > pSoldier->bActionPoints || ubMinAPCost == 0 )
		{
			//DebugShot(pSoldier, String("not enough AP or zero ubMinAPCost, skip"));
			continue;
		}

		//KeepInterfaceGoing();

		// calc next attack's minimum stabbing cost (excludes movement & turning)
		//ubRawAPCost = MinAPsToShootOrStab(pSoldier,pOpponent->sGridNo, FALSE) - APBPConstants[AP_CHANGE_TARGET];
		ubRawAPCost = MinAPsToAttack(pSoldier, pOpponent->sGridNo, FALSE,0) - APBPConstants[AP_CHANGE_TARGET];
		//DebugShot(pSoldier, String("ubRawAPCost %d", ubRawAPCost));

		// sevenfm: minimum possible value for ubRawAPCost
		if (ubRawAPCost < 1)
		{
			//ubRawAPCost = ubMinAPCost;
			ubRawAPCost = 1;
		}

		// determine if this is a surprise stab (must be next to opponent & unseen)
		fSurpriseStab = FALSE;		// assume it is not a surprise stab

		// if opponent doesn't see the attacker
		if( PersonalKnowledge(pOpponent, pSoldier->ubID) != SEEN_CURRENTLY)
		{
			// and he's only one space away from attacker
			if (SpacesAway(pSoldier->sGridNo,pOpponent->sGridNo) == 1)
			{
				fSurpriseStab = TRUE;	// we got 'im lined up where we want 'im!
				//DebugShot(pSoldier, String("surprise stab!"));
			}
		}

		iBestHitRate = 0;					 // reset best hit rate to minimum

		// calculate the maximum possible aiming time
		// HEADROCK HAM 4: Required for new Aiming Level Limits function
		//ubMaxPossibleAimTime = min(AllowedAimingLevels(pSoldier, pOpponent->sGridNo),pSoldier->bActionPoints - ubMinAPCost);
		// sevenfm: 100 AP system
		ubMaxPossibleAimTime = min(AllowedAimingLevels(pSoldier, pOpponent->sGridNo), (pSoldier->bActionPoints - ubMinAPCost) / APBPConstants[AP_CLICK_AIM]);
		//DebugShot(pSoldier, String("ubMaxPossibleAimTime %d", ubMaxPossibleAimTime));

		// consider the various aiming times
		for (ubAimTime = 0; ubAimTime <= ubMaxPossibleAimTime; ubAimTime++)
		{
			//HandleMyMouseCursor(KEYBOARDALSO);
			//DebugShot(pSoldier, String("check ubAimTime %d", ubAimTime));

			if (!fSurpriseStab)
			{
				if (fBladeAttack)
				{
					ubChanceToHit = (INT16) CalcChanceToStab(pSoldier,pOpponent,ubAimTime);
				}
				else
				{
					ubChanceToHit = (INT16) CalcChanceToPunch(pSoldier,pOpponent,ubAimTime);
				}
			}
			else
			{
				// HEADROCK (HAM): Externalized maximum to JA2_OPTIONS.INI
				// ubChanceToHit = MAXCHANCETOHIT;
				ubChanceToHit = gGameExternalOptions.ubMaximumCTH;
			}
			//DebugShot(pSoldier, String("ubChanceToHit %d", ubChanceToHit));

			//sprintf(tempstr,"Vs. %s, at AimTime %d, ubChanceToHit = %d",ExtMen[pOpponent->ubID].name,ubAimTime,ubChanceToHit);
			//PopMessage(tempstr);

			// sevenfm: 100 AP system
			// sevenfm: take into account pathing and turning
			//iHitRate = (pSoldier->bActionPoints * ubChanceToHit) / (ubRawAPCost + ubAimTime * APBPConstants[AP_CLICK_AIM]);
			iHitRate = (pSoldier->bActionPoints - (ubMinAPCost - ubRawAPCost)) * ubChanceToHit / (ubRawAPCost + ubAimTime * APBPConstants[AP_CLICK_AIM]);
			//DebugShot(pSoldier, String("iHitRate %d", iHitRate));

			// if aiming for this amount of time produces a better hit rate
			if (iHitRate > iBestHitRate)
			{
				iBestHitRate = iHitRate;
				ubBestAimTime = ubAimTime;
				ubBestChanceToHit = ubChanceToHit;

				//DebugShot(pSoldier, String("use best: ubBestAimTime %d, ubBestChanceToHit %d", ubBestAimTime, ubBestChanceToHit));
			}
		}

		//DebugShot(pSoldier, String("iBestHitRate %d", iBestHitRate));

		// if we can't get any kind of hit rate at all
		if (iBestHitRate == 0)
			continue;			// next opponent

		// stabs are not affected by cover, so the chance to REALLY hit is the same
		ubChanceToReallyHit = ubBestChanceToHit;

		// calculate this opponent's threat value
		// NOTE: ignore my cover!	By the time I run beside him I won't have any!
		//iThreatValue = CalcManThreatValue(pOpponent,pSoldier->sGridNo,FALSE,pSoldier);
		iThreatValue = CalcManThreatValue(pOpponent,pSoldier->sGridNo,TRUE,pSoldier);
		//DebugShot(pSoldier, String("iThreatValue %d", iThreatValue));

		// estimate the damage this stab would do to this opponent
		iEstDamage = EstimateStabDamage(pSoldier,pOpponent,ubBestChanceToHit, fBladeAttack );
		//DebugShot(pSoldier, String("iEstDamage %d", iEstDamage));

		// calculate the combined "attack value" for this opponent
		// highest possible value before division should be about 1 billion...
		// normal value before division should be about 5 million...
		iAttackValue = ( iEstDamage * iBestHitRate * ubChanceToReallyHit * iThreatValue) / 1000;		
		//DebugShot(pSoldier, String("iAttackValue %d", iAttackValue));

		// sevenfm: lower value for attractive opponent
		if (pSoldier->IsMaleBodyType() &&
			pOpponent->HasStunningLooks() &&
			PersonalSeenKnowledge(pSoldier, pOpponent->ubID) &&
			pSoldier->ubPreviousAttackerID != pOpponent->ubID &&
			pSoldier->ubNextToPreviousAttackerID != pOpponent->ubID)
		{
			iAttackValue = iAttackValue / 4;
		}
		else if (pSoldier->IsFemaleBodyType() &&
			pOpponent->HasStunningLooks() &&
			PersonalSeenKnowledge(pSoldier, pOpponent->ubID))
		{
			iAttackValue = iAttackValue * 2;
		}

#ifdef DEBUGATTACKS
		DebugAI( String( "CalcBestStab: best AttackValue vs %d = %d\n",ubLoop,iAttackValue ) );
#endif

		// if we can hurt the guy, OR probably not, but at least it's our best
		// chance to actually hit him and maybe scare him, knock him down, etc.
		if ((iAttackValue > 0) || (ubChanceToReallyHit > pBestStab->ubChanceToReallyHit))
		{
			// if there already was another viable target
			if (pBestStab->ubChanceToReallyHit > 0)
			{
				// OK, how does our chance to hit him compare to the previous best one?
				iPercentBetter = ((ubChanceToReallyHit * 100) / pBestStab->ubChanceToReallyHit) - 100;

				// if this chance to really hit is more than 50% worse, and the other
				// guy is conscious at all
				// sevenfm: check that best opponent exists
				if( iPercentBetter < -PERCENT_TO_IGNORE_THREAT && 
					pBestStab->ubOpponent != NOBODY &&
					Menptr[pBestStab->ubOpponent].stats.bLife >= OKLIFE )
				{
					// then stick with the older guy as the better target
					continue;
				}

				// if this chance to really hit between 50% worse to 50% better
				if (iPercentBetter < PERCENT_TO_IGNORE_THREAT)
				{
					// then the one with the higher ATTACK VALUE is the better target
					if (iAttackValue < pBestStab->iAttackValue)
					{
						// the previous guy is more important since he's more dangerous
						continue;			// next opponent
					}
				}

				// sevenfm: skip attractive opponent if we have other opponent
				/*if (pSoldier->IsMaleBodyType() &&
					pOpponent->HasStunningLooks() &&
					pSoldier->ubPreviousAttackerID != pOpponent->ubID &&
					pSoldier->ubNextToPreviousAttackerID != pOpponent->ubID &&
					pBestStab->ubOpponent != NOBODY)
				{
					continue;
				}*/
			}

			// OOOF!	That was a lot of work!	But we've got a new best target!
			pBestStab->ubPossible			= TRUE;
			pBestStab->ubOpponent			= pOpponent->ubID;
			pBestStab->ubAimTime			= ubBestAimTime;
			pBestStab->ubChanceToReallyHit = ubChanceToReallyHit;
			pBestStab->sTarget			 = pOpponent->sGridNo;
			pBestStab->bTargetLevel		= pOpponent->pathing.bLevel;
			pBestStab->iAttackValue		= iAttackValue;
			pBestStab->ubAPCost			= ubMinAPCost + ubBestAimTime;

			//DebugShot(pSoldier, String("best attack: opponent %d, ubAimTime %d, ubAPCost %d, iAttackValue %d, ubChanceToReallyHit", pBestStab->ubOpponent, pBestStab->ubAimTime, pBestStab->ubAPCost, pBestStab->iAttackValue, pBestStab->ubChanceToReallyHit));
		}
	}

	pSoldier->usMovementMode = usTrueMovementMode;
}

void CalcTentacleAttack(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestStab )
{
	UINT32 uiLoop;
	INT32 iAttackValue;
	INT32 iThreatValue,iHitRate,iBestHitRate, iEstDamage;
	BOOLEAN fSurpriseStab;
	UINT8 ubMaxPossibleAimTime = 0;
	INT16 ubBestChanceToHit,ubAimTime,ubMinAPCost,ubChanceToHit,ubBestAimTime,ubRawAPCost;
	INT16 ubChanceToReallyHit = 0;
	SOLDIERTYPE *pOpponent;

	// sevenfm: initialize
	pBestStab->ubPossible = FALSE;
	pBestStab->iAttackValue = 0;	
	pBestStab->ubChanceToReallyHit = 0;
	pBestStab->ubOpponent = NOBODY;

	pSoldier->usAttackingWeapon = pSoldier->inv[HANDPOS].usItem;

	// determine which attack against which target has the greatest attack value

	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, or dead
		if (!pOpponent || !pOpponent->stats.bLife)
			continue;			// next merc

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		// if this opponent is not currently in sight (ignore known but unseen!)
		if( PersonalKnowledge(pSoldier, pOpponent->ubID) != SEEN_CURRENTLY )
			continue;			// next merc

		// if this opponent is not on the same level
		if (pSoldier->pathing.bLevel != pOpponent->pathing.bLevel)
			continue;			// next merc

		UINT16 usRange = GetModifiedGunRange(CREATURE_QUEEN_TENTACLES);
		// if this opponent is outside the range of our tentacles
		if ( GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, pOpponent->sGridNo ) > usRange )
		{
			continue; // next merc
		}

#ifdef DEBUGATTACKS
		DebugAI( String( "%s can see %s\n",pSoldier->GetName(),ExtMen[pOpponent->ubID].GetName() ) );
#endif

		// calculate minimum action points required to stab at this opponent
		ubMinAPCost = CalcTotalAPsToAttack( pSoldier,pOpponent->sGridNo,ADDTURNCOST, 0 );
		//ubMinAPCost = MinAPsToAttack(pSoldier,pOpponent->sGridNo,ADDTURNCOST);

		// calc next attack's minimum stabbing cost (excludes movement & turning)
		//ubRawAPCost = MinAPsToShootOrStab(pSoldier,pOpponent->sGridNo, FALSE) - APBPConstants[AP_CHANGE_TARGET];
		ubRawAPCost = MinAPsToAttack(pSoldier,pOpponent->sGridNo, FALSE,0) - APBPConstants[AP_CHANGE_TARGET];

		// sevenfm: set minimum possible AP cost
		if (ubRawAPCost < 1)
		{
			//ubRawAPCost = ubMinAPCost;
			ubRawAPCost = 1;
		}

		// determine if this is a surprise stab (for tentacles, enemy must not see us, no dist limit)
		fSurpriseStab = FALSE;		// assume it is not a surprise stab

		// if opponent doesn't see the attacker
		if( PersonalKnowledge(pOpponent, pSoldier->ubID) != SEEN_CURRENTLY )
		{
			fSurpriseStab = TRUE;	// we got 'im lined up where we want 'im!
		}

		iBestHitRate = 0;					 // reset best hit rate to minimum

		// calculate the maximum possible aiming time

		//ubMaxPossibleAimTime = min(AllowedAimingLevels(pSoldier),pSoldier->bActionPoints - ubMinAPCost);
		ubMaxPossibleAimTime = 0;

		// consider the various aiming times
		for (ubAimTime = 0; ubAimTime <= ubMaxPossibleAimTime; ubAimTime++)
		{
			//HandleMyMouseCursor(KEYBOARDALSO);

			if (!fSurpriseStab)
			{
				ubChanceToHit = (INT16) CalcChanceToStab(pSoldier,pOpponent,ubAimTime);
			}
			else
				// HEADROCK (HAM): Externalized maximum to JA2_OPTIONS.INI
				// ubChanceToHit = MAXCHANCETOHIT;
				ubChanceToHit = gGameExternalOptions.ubMaximumCTH;

			//sprintf(tempstr,"Vs. %s, at AimTime %d, ubChanceToHit = %d",ExtMen[pOpponent->ubID].name,ubAimTime,ubChanceToHit);
			//PopMessage(tempstr);			

			// sevenfm: 100AP system, take into account moving and turning
			iHitRate = (pSoldier->bActionPoints - (ubMinAPCost - ubRawAPCost)) * ubChanceToHit / (ubRawAPCost + ubAimTime * APBPConstants[AP_CLICK_AIM]);

			// if aiming for this amount of time produces a better hit rate
			if (iHitRate > iBestHitRate)
			{
				iBestHitRate = iHitRate;
				ubBestAimTime = ubAimTime;
				ubBestChanceToHit = ubChanceToHit;
			}
		}

		// if we can't get any kind of hit rate at all
		if (iBestHitRate == 0)
			continue;			// next opponent

		// stabs are not affected by cover, so the chance to REALLY hit is the same
		ubChanceToReallyHit = ubBestChanceToHit;

		// calculate this opponent's threat value
		// NOTE: ignore my cover!	By the time I run beside him I won't have any!
		iThreatValue = CalcManThreatValue(pOpponent,pSoldier->sGridNo,FALSE,pSoldier);

		// estimate the damage this stab would do to this opponent
		iEstDamage = EstimateStabDamage(pSoldier,pOpponent,ubBestChanceToHit, TRUE );

		// calculate the combined "attack value" for this opponent
		// highest possible value before division should be about 1 billion...
		// normal value before division should be about 5 million...
		iAttackValue = ( iEstDamage * iBestHitRate * ubChanceToReallyHit * iThreatValue) / 1000;

#ifdef DEBUGATTACKS
		DebugAI( String( "CalcBestStab: best AttackValue vs %d = %d\n",ubLoop,iAttackValue ) );
#endif

		// if we can hurt the guy, OR probably not, but at least it's our best
		// chance to actually hit him and maybe scare him, knock him down, etc.
		if (iAttackValue > 0)
		{
			// OOOF!	That was a lot of work!	But we've got a new best target!
			pBestStab->ubPossible			= TRUE;
			pBestStab->ubOpponent			= pOpponent->ubID;
			pBestStab->ubAimTime			= ubBestAimTime;
			pBestStab->ubChanceToReallyHit = ubChanceToReallyHit;
			pBestStab->sTarget			 = pOpponent->sGridNo;
			pBestStab->bTargetLevel		= pOpponent->pathing.bLevel;

			// ADD this target's attack value to our TOTAL...
			pBestStab->iAttackValue				+= iAttackValue;

			pBestStab->ubAPCost			= ubMinAPCost + ubBestAimTime;

		}
	}
}

UINT8 NumMercsCloseTo( INT32 sGridNo, UINT8 ubMaxDist )
{
	INT8						bNumber = 0;
	UINT32					uiLoop;
	SOLDIERTYPE *		pSoldier;

	for ( uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pSoldier = MercSlots[ uiLoop ];

		// sevenfm: count all teams except creatures
		if ( pSoldier && pSoldier->bTeam != CREATURE_TEAM && pSoldier->stats.bLife >= OKLIFE )
		//if ( pSoldier && pSoldier->bTeam == gbPlayerNum && pSoldier->stats.bLife >= OKLIFE )
		{
			if (PythSpacesAway( sGridNo, pSoldier->sGridNo ) <= ubMaxDist)
			{
				bNumber++;
			}
		}
	}

	return( bNumber );
}

INT32 EstimateShotDamage(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent, INT16 ubChanceToHit)
{
	INT32 iRange,iMaxRange,iPowerLost;
	INT32 iDamage;
	INT32 iDamageBeforeArmour;
	UINT8 ubBonus;
	INT32 iHeadProt = 0, iTorsoProt = 0, iLegProt = 0;
	INT32 iTotalProt;
	UINT8 ubAmmoType;
	UINT16 usItem;
	OBJECTTYPE *pObj;

	if( pSoldier == NULL || pOpponent == NULL )
		return 0;

	//DebugShot(pSoldier, String("\nEstimateShotDamage: Soldier [%d], Opponent [%d], CTH %d", pSoldier->ubID, pOpponent->ubID, ubChanceToHit));

	pObj = &(pSoldier->inv[pSoldier->ubAttackingHand]);
	usItem = pObj->usItem;
	//DebugShot(pSoldier, String("using item %d", usItem));

	if ( Item[ usItem ].usItemClass & IC_THROWING_KNIFE )
	{
		ubAmmoType = AMMO_KNIFE;
		//DebugShot(pSoldier, String("using throwing knife"));
	}
	else
	{		
		ubAmmoType = (*pObj)[0]->data.gun.ubGunAmmoType;
		//DebugShot(pSoldier, String("ammo type %d", ubAmmoType));
	}

	// calculate distance to target, obtain his gun's maximum range rating

	iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, pOpponent->sGridNo );
	// SANDRO - added specific range calculation
	if ( Item[ usItem ].usItemClass & IC_GUN )
		iMaxRange = GunRange( pObj, pSoldier );
	else
		iMaxRange = GetModifiedGunRange(usItem);

	//DebugShot(pSoldier, String("max range %d", iMaxRange));

	// bullet loses speed and penetrating power, 50% loss per maximum range
	// SANDRO - you know, 50% is a lot, it often leads to negative damage values in the end
	// while there is no true effect of this in game !!! Made it 25%!
	// sevenfm: disabled
	//iPowerLost = ((25 * iRange) / iMaxRange);	
	iPowerLost = 0;
	//DebugShot(pSoldier, String("iPowerLost %d", iPowerLost));

	// up to 50% extra impact for making particularly accurate successful shots
	ubBonus = ubChanceToHit / 4;		// /4 is really /2 and /2 again
	//DebugShot(pSoldier, String("bonus for accurate shot %d", ubBonus));

	iDamage = ((GetDamage(pObj)) * (100 - iPowerLost + ubBonus) / 100);
	//DebugShot(pSoldier, String("iDamage %d", iDamage));

	// if opponent is wearing a helmet
	if (pOpponent->inv[HELMETPOS].usItem)
	{
		// sevenfm: added coverage
		iHeadProt += (INT32) Armour[Item[pOpponent->inv[HELMETPOS].usItem].ubClassIndex].ubProtection *
					Armour[Item[pOpponent->inv[HELMETPOS].usItem].ubClassIndex].ubCoverage *
					(INT32) pOpponent->inv[HELMETPOS][0]->data.objectStatus / (100 * 100);
	}	

	// if opponent is wearing a protective vest
	if ( !AmmoTypes[ubAmmoType].ignoreArmour )
	{
		// monster spit and knives ignore kevlar vests
		if (pOpponent->inv[VESTPOS].usItem)
		{
			// sevenfm: added coverage
			iTorsoProt += (INT32) Armour[Item[pOpponent->inv[VESTPOS].usItem].ubClassIndex].ubProtection *
						(INT32) Armour[Item[pOpponent->inv[VESTPOS].usItem].ubClassIndex].ubCoverage *
						(INT32) pOpponent->inv[VESTPOS][0]->data.objectStatus / (100 * 100);
		}
	}

	// check for ceramic plates; these do affect monster spit
	for (attachmentList::iterator iter = pOpponent->inv[VESTPOS][0]->attachments.begin(); iter != pOpponent->inv[VESTPOS][0]->attachments.end(); ++iter) 
	{
		if (iter->exists() && Item[iter->usItem].usItemClass == IC_ARMOUR && (*iter)[0]->data.objectStatus > 0)
		{
			// sevenfm: added coverage
			iTorsoProt += (INT32) Armour[Item[iter->usItem].ubClassIndex].ubProtection *
						(INT32) Armour[Item[iter->usItem].ubClassIndex].ubCoverage *
						(INT32) (*iter)[0]->data.objectStatus / (100 * 100);
		}
	}

	// if opponent is wearing armoured leggings (LEGPOS)
	if ( !AmmoTypes[ubAmmoType].ignoreArmour )
	{	// monster spit and knives ignore kevlar leggings
		if (pOpponent->inv[LEGPOS].usItem)
		{
			// sevenfm: added coverage
			iLegProt += (INT32) Armour[Item[pOpponent->inv[LEGPOS].usItem].ubClassIndex].ubProtection *
						(INT32) Armour[Item[pOpponent->inv[LEGPOS].usItem].ubClassIndex].ubCoverage *
						(INT32) pOpponent->inv[LEGPOS][0]->data.objectStatus / (100 * 100);
		}
	}

	// 15% of all shots are to the head, 80% are to the torso.	Calc. avg. prot.
	// NB: make AI guys shoot at head 15% of time, 5% of time at legs

	iTotalProt = ((15 * iHeadProt) + (75 * iTorsoProt) + 5 * iLegProt) / 100;
	iTotalProt = (INT32) (iTotalProt * AmmoTypes[ubAmmoType].armourImpactReductionMultiplier / max(1,AmmoTypes[ubAmmoType].armourImpactReductionDivisor) );

	//DebugShot(pSoldier, String("iHeadProt %d, iTorsoProt %d, iLegProt %d", iHeadProt, iTorsoProt, iLegProt));
	//DebugShot(pSoldier, String("iTotalProt %d", iTotalProt));

	iDamageBeforeArmour = iDamage;
	//iDamage -= iTotalProt;
	iDamage = max(0, iDamage - iTotalProt);

	//DebugShot(pSoldier, String("iDamage before protection %d", iDamageBeforeArmour));
	//DebugShot(pSoldier, String("iDamage after protection %d", iDamage));

	iDamage = (INT32)(iDamage * AmmoTypes[ubAmmoType].afterArmourDamageMultiplier / max(1,AmmoTypes[ubAmmoType].afterArmourDamageDivisor) ) ;
	//DebugShot(pSoldier, String("iDamage after ammo multiplier %d", iDamage));

	// sevenfm: take into account BP loss from hit
	if( iDamage < iDamageBeforeArmour )
	{
		iDamage += (iDamageBeforeArmour - iDamage) / 2;
		//DebugShot(pSoldier, String("iDamage after adding possible BP loss %d", iDamage));
	}	

	if (AmmoTypes[ubAmmoType].monsterSpit )
	{
		switch( usItem )
		{
		// explosive damage is 100-200% that of the rated, so multiply by 3/2s here
		case CREATURE_QUEEN_SPIT: //TODO: Madd - remove the hardcoding here
			iDamage += ( 3 * Explosive[ Item[ LARGE_CREATURE_GAS ].ubClassIndex ].ubDamage * NumMercsCloseTo( pOpponent->sGridNo, (UINT8)Explosive[ Item[ LARGE_CREATURE_GAS ].ubClassIndex ].ubRadius ) ) / 2;
			break;
		case CREATURE_OLD_MALE_SPIT:
			iDamage += ( 3 * Explosive[ Item[ SMALL_CREATURE_GAS ].ubClassIndex ].ubDamage * NumMercsCloseTo( pOpponent->sGridNo, (UINT8)Explosive[ Item[ SMALL_CREATURE_GAS ].ubClassIndex	].ubRadius ) ) / 2;
			break;
		default:
			iDamage += ( 3 * Explosive[ Item[ VERY_SMALL_CREATURE_GAS ].ubClassIndex ].ubDamage * NumMercsCloseTo( pOpponent->sGridNo, (UINT8)Explosive[ Item[ VERY_SMALL_CREATURE_GAS ].ubClassIndex	].ubRadius ) ) / 2;
			break;
		}
		//DebugShot(pSoldier, String("iDamage after monster modifier %d", iDamage));
	}

	// assume we can do at LEAST 1 pt minimum damage
	iDamage = max(1, iDamage);

	//DebugShot(pSoldier, String("final damage %d\n", iDamage));

	return( iDamage );
}

INT32 EstimateThrowDamage( SOLDIERTYPE *pSoldier, UINT8 ubItemPos, SOLDIERTYPE *pOpponent, INT32 sGridNo )
{
	UINT16	ubExplosiveIndex;
	INT32	iExplosDamage, iBreathDamage, iArmourAmount, iDamage = 0;
	INT8	bSlot;

	if (pSoldier == NULL || pOpponent == NULL || ubItemPos > pSoldier->inv.size() || sGridNo > giNumberOfTiles)
		return 0;

	if (pSoldier->inv[ubItemPos].exists() == false)
		return 0;

	//DebugShot(pSoldier, String("\nEstimateThrowDamage Soldier [%d], Opponent %d, itempos %d, gridno %d", pSoldier->ubID, pOpponent->ubID, ubItemPos, sGridNo));

	UINT16 usItem = pSoldier->inv[ubItemPos].usItem;

	if (Item[usItem].singleshotrocketlauncher)
	{
		if (Item[usItem].usBuddyItem != 0 && Item[Item[usItem].usBuddyItem].usItemClass & IC_EXPLOSV)
			ubExplosiveIndex = Item[Item[usItem].usBuddyItem].ubClassIndex;
		else
			ubExplosiveIndex = Item[C1].ubClassIndex;
	}
	else if (Item[usItem].rocketlauncher || Item[usItem].grenadelauncher || Item[usItem].mortar)
	{
		OBJECTTYPE* pAttachment = FindLaunchableAttachment(&pSoldier->inv[ubItemPos], usItem);
		if (pAttachment->exists())
			ubExplosiveIndex = Item[pAttachment->usItem].ubClassIndex;
		else
			return 0;
	}
	else if (IsGrenadeLauncherAttached(&pSoldier->inv[ubItemPos]))//dnl ch63 240813 situation when grenade is already in launcher
	{
		OBJECTTYPE *pAttachment = FindLaunchableAttachment(&pSoldier->inv[ubItemPos], GetAttachedGrenadeLauncher(&pSoldier->inv[ubItemPos]));
		if (pAttachment->exists())
			ubExplosiveIndex = Item[pAttachment->usItem].ubClassIndex;
		else
			return(0);
	}
	else if (Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_SMOKE)
	{
		// don't use smoke on melee opponents
		if (pOpponent->IsZombie() || !AICheckHasGun(pOpponent) || AICheckShortWeaponRange(pOpponent))
		{
			return 0;
		}

		// calculate damage depending on opponent's threat
		iDamage = EffectiveMarksmanship(pOpponent) / 10;
		if (AIRoleMarksman(pOpponent) || AIRoleMachinegunner(pOpponent) || AIRoleSniper(pOpponent))
		{
			iDamage = iDamage * 2;
		}

		INT16 sDistance = PythSpacesAway(pSoldier->sGridNo, sGridNo);

		// don't use smoke when too close
		if (sDistance < DAY_VISION_RANGE / 4)
		{
			return 0;
		}

		// lower damage at close range
		if (sDistance < DAY_VISION_RANGE)
		{
			iDamage = iDamage * sDistance / DAY_VISION_RANGE;
		}

		// bonus if enemy sees many opponents, penalty if my team sees enemy
		iDamage = (iDamage + CountSeenEnemiesLastTurn(pOpponent)) / (1 + CountTeamSeeSoldier(pSoldier->bTeam, pOpponent));

		//return 5;
		return iDamage;
	}
	// simulate C1 damage for red smoke as it will cause artillery strike
	else if (Explosive[Item[usItem].ubClassIndex].ubType == EXPLOSV_SIGNAL_SMOKE)
	{
		ubExplosiveIndex = Item[C1].ubClassIndex;
		iExplosDamage = (((INT32)GetModifiedExplosiveDamage(Explosive[ubExplosiveIndex].ubDamage, 0)) * 3) / 2;

		if (CheckArtilleryStrike())
			return iExplosDamage / 2;
		else
			return iExplosDamage / 4;
	}
	else
	{
		ubExplosiveIndex = Item[usItem].ubClassIndex;
	}

	// JA2Gold: added
	if (Item[usItem].flare)
	{
		return max(0, 5 * ((INT16)LightTrueLevel(pOpponent->sGridNo, pOpponent->pathing.bLevel) - NORMAL_LIGHTLEVEL_DAY - (INT16)CountTeamSeeSoldier(pSoldier->bTeam, pOpponent)));
	}

	iExplosDamage = (((INT32)GetModifiedExplosiveDamage(Explosive[ubExplosiveIndex].ubDamage, 0)) * 3) / 2;
	iBreathDamage = (((INT32)GetModifiedExplosiveDamage(Explosive[ubExplosiveIndex].ubStunDamage, 1)) * 5) / 4;

	// sevenfm: IndoorModifier - increase damage inside buildings
	if (gpWorldLevelData[sGridNo].ubTerrainID == FLAT_FLOOR)
	{
		iExplosDamage += (INT32)(iExplosDamage * Explosive[ubExplosiveIndex].bIndoorModifier);
	}

	// sevenfm: add damage from fragments
	if (Explosive[ubExplosiveIndex].ubType == EXPLOSV_NORMAL &&
		Explosive[ubExplosiveIndex].usNumFragments > 0)
	{
		// sevenfm: use NumFragments/10, but no more than 20 fragments
		iExplosDamage += min(20, Explosive[ubExplosiveIndex].usNumFragments / 10) * Explosive[ubExplosiveIndex].ubFragDamage;
	}

	if (Explosive[ubExplosiveIndex].ubType == EXPLOSV_TEARGAS || Explosive[ubExplosiveIndex].ubType == EXPLOSV_MUSTGAS)
	{
		// if target gridno is outdoors (where tear gas lasts only 1-2 turns)
		if (gpWorldLevelData[sGridNo].ubTerrainID != FLAT_FLOOR)
			iBreathDamage /= 2;		// reduce effective breath damage by 1/2

		bSlot = FindGasMask(pOpponent); //FindObj( pOpponent, GASMASK );
		// sevenfm: r8315
		if ((bSlot == HEAD1POS || bSlot == HEAD2POS || bSlot == HELMETPOS) && pSoldier->inv[bSlot][0]->data.objectStatus >= GASMASK_MIN_STATUS)
			//if ((bSlot == HEAD1POS || bSlot == HEAD2POS || bSlot == HELMETPOS) && pSoldier->inv[bSlot][0]->data.objectStatus >= 70)
		{
			// take condition of the gas mask into account - it could be leaking
			iBreathDamage = (iBreathDamage * (100 - pOpponent->inv[bSlot][0]->data.objectStatus)) / 100;
		}

		// sevenfm: tear/mustard gas is much less effective
		iExplosDamage = iExplosDamage / 2;
	}
	if (Explosive[ubExplosiveIndex].ubType == EXPLOSV_BURNABLEGAS)
	{
		// if target gridno is outdoors (where tear gas lasts only 1-2 turns)
		if (gpWorldLevelData[sGridNo].ubTerrainID != FLAT_FLOOR)
			iBreathDamage /= 2;		// reduce effective breath damage by 1/2
	}
	else if (iExplosDamage)
	{
		// EXPLOSION DAMAGE is spread amongst locations
		iArmourAmount = ArmourVersusExplosivesPercent(pSoldier);
		iExplosDamage -= iExplosDamage * iArmourAmount / 100;

		if (iExplosDamage < 1)
			iExplosDamage = 1;
	}

	// if this opponent is standing
	// sevenfm: bonus only for normal type explosion
	if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND &&
		Explosive[ubExplosiveIndex].ubType == EXPLOSV_NORMAL)
	{
		// 15 pt. flat bonus for knocking him down
		iDamage += 15;
	}

	if (pOpponent->IsUnconscious() || AM_A_ROBOT(pOpponent))
	{
		// don't bother to count breath damage against people already down
		iBreathDamage = 0;
	}

	// estimate combined "damage" value considering combined health/breath damage
	iDamage += iExplosDamage + (iBreathDamage / 3);

	// approximate chance of the grenade going off (Ian's formulas are too funky)
	// then use that to reduce the expected damage because thing may not blow!
	iDamage = (iDamage * pSoldier->inv[ubItemPos][0]->data.objectStatus) / 100;

	//DebugShot(pSoldier, String("final damage %d\n", iDamage));

	return(iDamage);
}

INT32 EstimateStabDamage( SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent, INT16 ubChanceToHit, BOOLEAN fBladeAttack )
{
	INT32 iImpact, iFluke, iBonus;
	UINT16	usItem;

	CHECKF(pSoldier);
	CHECKF(pOpponent);

	//DebugShot(pSoldier, String("\nEstimateStabDamage Soldier [%d], Opponent %d, CTH %d, blade %d", pSoldier->ubID, pOpponent->ubID, ubChanceToHit, fBladeAttack));

	usItem = pSoldier->inv[HANDPOS].usItem;
	//DebugShot(pSoldier, String("using item %d", usItem));

	if (fBladeAttack)
	{
		iImpact = GetDamage(&pSoldier->inv[HANDPOS]);
		//DebugShot(pSoldier, String("blade attack: iImpact %d", iImpact));

		iImpact += EffectiveStrength( pSoldier, FALSE ) / 20; // 0 to 5 for strength, adjusted by damage taken
		//DebugShot(pSoldier, String("blade attack: iImpact + strength %d", iImpact));
	}
	else
	{
		iImpact = EffectiveStrength(pSoldier, FALSE) / 10;

		// NB martial artists don't get a bonus for using brass knuckles! - oh, they do in STOMP - SANDRO
		if (pSoldier->usAttackingWeapon)
		{
			if (gGameOptions.fNewTraitSystem)
			{
				iImpact += GetDamage(&pSoldier->inv[HANDPOS]);

				if (AM_A_ROBOT(pOpponent))
				{
					iImpact /= 2;
				}
			}
			else
			{
				if (!HAS_SKILL_TRAIT(pSoldier, MARTIALARTS_OT))
				{
					iImpact += GetDamage(&pSoldier->inv[HANDPOS]);
				}
				if (AM_A_ROBOT(pOpponent))
				{
					iImpact /= 2;
				}
			}
		}
		else
		{
			// base HTH damage
			// Enhanced Close Combat System - Slightly reduced for we can now attack to head for bigger damage
			if (gGameExternalOptions.fEnhancedCloseCombatSystem)
			{
				iImpact += 4;
			}
			else
			{
				iImpact += 5;
			}

			// Add melee damage multiplier to HtH attacks as well - SANDRO
			// actually I make the influence a little lesser, because to the blades and so,
			// only the item impact is multiplied, not the level and strength bonus, but here it does
			iImpact = (INT32)(iImpact * gGameExternalOptions.iMeleeDamageModifier / 100);

			if (AM_A_ROBOT(pOpponent))
			{
				iImpact = 0;
			}
		}
	}

	iImpact += (pSoldier->stats.bExpLevel / 2); // 0 to 4 for level // SANDRO - added effective level calc
	//DebugShot(pSoldier, String("iImpact + level %d", iImpact));

	iFluke = 0;
	iBonus = ubChanceToHit / 4;				// up to 50% extra impact for accurate attacks

	iImpact = iImpact * (100 + iFluke + iBonus) / 100;
	//DebugShot(pSoldier, String("iImpact + bonus from accurate attack %d", iImpact));

	if (!fBladeAttack)
	{
		// add bonuses for hand-to-hand and martial arts
		// Check for new traits - SANDRO
		if (gGameOptions.fNewTraitSystem)
		{
			if (!pSoldier->usAttackingWeapon || Item[pSoldier->inv[HANDPOS].usItem].brassknuckles)
			{
				// add bonuses for martial arts
				if (HAS_SKILL_TRAIT(pSoldier, MARTIAL_ARTS_NT))
				{
					// also add breath damage bonus into consideration
					iImpact = (INT32)(((FLOAT)iImpact * (FLOAT)(100 + (gSkillTraitValues.ubMABonusDamageHandToHand + gSkillTraitValues.ubMABonusBreathDamageHandToHand) * NUM_SKILL_TRAITS(pSoldier, MARTIAL_ARTS_NT)) / 100.0f) + 0.5f);
				}
			}
			else
			{
				// +30% damage of blunt weapons for melee character
				if (HAS_SKILL_TRAIT(pSoldier, MELEE_NT))
				{
					iImpact = (INT32)(((FLOAT)iImpact * (FLOAT)(100 + gSkillTraitValues.ubMEDamageBonusBlunt) / 100.0f) + 0.5f);
				}
			}
		}
		else
		{
			if (HAS_SKILL_TRAIT(pSoldier, MARTIALARTS_OT))
			{
				iImpact = (INT32)(((FLOAT)iImpact * (FLOAT)(100 + gbSkillTraitBonus[MARTIALARTS_OT] * NUM_SKILL_TRAITS(pSoldier, MARTIALARTS_OT)) / 100.0f) + 0.5f);
			}
			if (HAS_SKILL_TRAIT(pSoldier, HANDTOHAND_OT))
			{
				iImpact = (INT32)(((FLOAT)iImpact * (FLOAT)(100 + gbSkillTraitBonus[HANDTOHAND_OT] * NUM_SKILL_TRAITS(pSoldier, HANDTOHAND_OT)) / 100.0f) + 0.5f);
			}
		}
		// SANDRO - Enhanced Close Combat System - aiming at body parts makes difference
		if (gGameExternalOptions.fEnhancedCloseCombatSystem && (gAnimControl[ pOpponent->usAnimState ].ubEndHeight == ANIM_PRONE))
		{
			iImpact = (INT32)(iImpact * 150 / 100);  // 50% increased damage to lying characters
		}
		// Here, if we're doing a bare-fisted attack,
		// we want to pay attention just to wounds inflicted
		// SANDRO - No, we may consider the breath damage as a sort of "real" damage too, so only reduce it by half
		// sevenfm: divide by 4
		iImpact = iImpact / 4;
	}
	// SANDRO - damage bonus to melee trait
	else 
	{
		// sevenfm: always give MELEE bonus when using bayonet
		if (pSoldier->bWeaponMode == WM_ATTACHED_BAYONET || gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSoldier, MELEE_NT))
		{
			iImpact += (iImpact * (100 + gSkillTraitValues.ubMEDamageBonusBlades ) / 100); // +30% damage
		}

		// SANDRO - Enhanced Close Combat System
		if (gGameExternalOptions.fEnhancedCloseCombatSystem)
		{
			if (gAnimControl[ pOpponent->usAnimState ].ubEndHeight == ANIM_PRONE)
			{
				iImpact = (INT32)(iImpact * 140 / 100);  // 40% increased damage to lying characters
			}
		}	
	}
	//DebugShot(pSoldier, String("iImpact %d", iImpact));

	// Flugente: Add personal damage bonus
	iImpact = (iImpact * (100 + pSoldier->GetMeleeDamageBonus() ) / 100);
	//DebugShot(pSoldier, String("iImpact + personal damage bonus %d", iImpact));

	if (iImpact < 1)
	{
		iImpact = 1;
	}

	//DebugShot(pSoldier, String("final iImpact %d\n", iImpact));

	return( iImpact );
}

INT8 TryToReload( SOLDIERTYPE * pSoldier )
{
	INT8		bSlot;
	WEAPONTYPE *pWeapon;
	OBJECTTYPE *pObj, *pObj2;

	DebugAI( AI_MSG_INFO, pSoldier, String("---TryToReload [%d]", pSoldier->ubID));

	// HEADROCK HAM 3.3: Attempt to reload now takes magazine type into account. Preference will be given to magazines of similar type.
	pObj = &(pSoldier->inv[HANDPOS]);
	pWeapon = &(Weapon[pSoldier->inv[HANDPOS].usItem]);
	bSlot = FindAmmo( pSoldier, pWeapon->ubCalibre, pWeapon->ubMagSize, GetAmmoType(pObj), NO_SLOT );

	DebugAI( AI_MSG_INFO, pSoldier, String("---weapon in hand %d ammo slot %d ammo type %d calibre %d mag size %d", pObj->usItem, bSlot, GetAmmoType(pObj), pWeapon->ubCalibre, pWeapon->ubMagSize));

	if ((*pObj)[0]->data.gun.ubGunShotsLeft && !((*pObj)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
	{
		(*pObj)[0]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;

		INT16 sModifiedReloadAP = Weapon[Item[(pObj)->usItem].ubClassIndex].APsToReloadManually;

		// modify by ini values
		if ( Item[ pObj->usItem ].usItemClass == IC_GUN )
			sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierGun[Weapon[pObj->usItem].ubWeaponType]);
		else if ( Item[ pObj->usItem ].usItemClass == IC_LAUNCHER )
			sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierLauncher);

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// STOMP traits - SANDRO
		if ( gGameOptions.fNewTraitSystem )
		{
			// Sniper trait makes chambering a round faster
			if (( Weapon[Item[(pObj)->usItem].ubClassIndex].ubWeaponType == GUN_SN_RIFLE || Weapon[Item[(pObj)->usItem].ubClassIndex].ubWeaponType == GUN_RIFLE ) && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ))
				DeductPoints(pSoldier, (INT16)((((FLOAT)sModifiedReloadAP * (FLOAT)(100 - gSkillTraitValues.ubSNChamberRoundAPsReduction * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT )))/100.0f) + 0.5f), 0);
			// Ranger trait makes pumping shotguns faster
			else if (( Weapon[Item[(pObj)->usItem].ubClassIndex].ubWeaponType == GUN_SHOTGUN ) && HAS_SKILL_TRAIT( pSoldier, RANGER_NT ))
				DeductPoints(pSoldier, (INT16)((((FLOAT)sModifiedReloadAP * (FLOAT)(100 - gSkillTraitValues.ubRAPumpShotgunsAPsReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT )))/100.0f) + 0.5f), 0);
			else
				DeductPoints(pSoldier, sModifiedReloadAP, 0);
		}
		else
		{
			DeductPoints(pSoldier, sModifiedReloadAP, 0);
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////

		//PlayJA2Sample(Weapon[Item[pObj->usItem].ubClassIndex].ManualReloadSound, RATE_11025, SoundVolume(LNLVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
		pSoldier->PlayLNL(pObj->usItem);

		if ( pSoldier->IsValidSecondHandShot( ) )
		{
			pObj2 = &(pSoldier->inv[SECONDHANDPOS]);

			if ((*pObj2)[0]->data.gun.ubGunShotsLeft && !((*pObj2)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
			{
				(*pObj2)[0]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;
				//PlayJA2Sample(Weapon[Item[pObj2->usItem].ubClassIndex].ManualReloadSound, RATE_11025, SoundVolume(MIDVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
				pSoldier->PlayLNL(pObj2->usItem);
			}
		}

		return TRUE;
	}
	else
	{
		if ( pSoldier->IsValidSecondHandShot( ) )
		{
			pObj2 = &(pSoldier->inv[SECONDHANDPOS]);

			if ((*pObj2)[0]->data.gun.ubGunShotsLeft && !((*pObj2)[0]->data.gun.ubGunState & GS_CARTRIDGE_IN_CHAMBER) )
			{
				(*pObj2)[0]->data.gun.ubGunState |= GS_CARTRIDGE_IN_CHAMBER;

				INT16 sModifiedReloadAP = Weapon[Item[(pObj2)->usItem].ubClassIndex].APsToReloadManually;

				// modify by ini values
				if ( Item[ pObj2->usItem ].usItemClass == IC_GUN )
					sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierGun[Weapon[pObj2->usItem].ubWeaponType]);
				else if ( Item[ pObj2->usItem ].usItemClass == IC_LAUNCHER )
					sModifiedReloadAP = (INT16)(sModifiedReloadAP * gItemSettings.fAPtoReloadManuallyModifierLauncher);

				////////////////////////////////////////////////////////////////////////////////////////////////////////
				// STOMP traits - SANDRO (well, I don't know any one-handed sniper rifle, but what the hell...)
				if ( gGameOptions.fNewTraitSystem )
				{
					// Sniper trait makes chambering a round faster
					if (( Weapon[Item[(pObj2)->usItem].ubClassIndex].ubWeaponType == GUN_SN_RIFLE || Weapon[Item[(pObj2)->usItem].ubClassIndex].ubWeaponType == GUN_RIFLE ) && HAS_SKILL_TRAIT( pSoldier, SNIPER_NT ))
						DeductPoints(pSoldier, (INT16)((((FLOAT)sModifiedReloadAP * (FLOAT)(100 - gSkillTraitValues.ubSNChamberRoundAPsReduction * NUM_SKILL_TRAITS( pSoldier, SNIPER_NT )))/100.0f) + 0.5f), 0);
					// Ranger trait makes pumping shotguns faster
					else if (( Weapon[Item[(pObj2)->usItem].ubClassIndex].ubWeaponType == GUN_SHOTGUN ) && HAS_SKILL_TRAIT( pSoldier, RANGER_NT ))
						DeductPoints(pSoldier, (INT16)((((FLOAT)sModifiedReloadAP * (FLOAT)(100 - gSkillTraitValues.ubRAPumpShotgunsAPsReduction * NUM_SKILL_TRAITS( pSoldier, RANGER_NT )))/100.0f) + 0.5f), 0);
					else
						DeductPoints(pSoldier, sModifiedReloadAP, 0);
				}
				else
				{
					DeductPoints(pSoldier, sModifiedReloadAP, 0);
				}
				////////////////////////////////////////////////////////////////////////////////////////////////////////

				//PlayJA2Sample(Weapon[Item[pObj2->usItem].ubClassIndex].ManualReloadSound, RATE_11025, SoundVolume(MIDVOLUME, pSoldier->sGridNo), 1, SoundDir(pSoldier->sGridNo));
				pSoldier->PlayLNL(pObj2->usItem);

				return TRUE;
			}
		}
	}
	//</SB>

	if (bSlot != NO_SLOT && ReloadGun( pSoldier, &(pSoldier->inv[HANDPOS]), &(pSoldier->inv[bSlot]) ))
	{
		return( TRUE );
	}

	return( NOSHOOT_NOAMMO );
}

INT8 CanNPCAttack(SOLDIERTYPE *pSoldier)
{
	INT8		bCanAttack;
	INT8		bWeaponIn;

	DebugAI( AI_MSG_INFO, pSoldier, String("---CanNPCAttack"));

	// NEUTRAL civilians are not allowed to attack, but those that are not
	// neutral (KILLNPC mission guynums, escorted guys) can, if they're armed
	if (PTR_CIVILIAN && pSoldier->aiData.bNeutral)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---NEUTRAL civilians are not allowed to attack"));
		return(FALSE);
	}

	// test if if we are able to attack (in general, not at any specific target)
	bCanAttack = OKToAttack(pSoldier,NOWHERE);
	DebugAI( AI_MSG_INFO, pSoldier, String("---OKToAttack = %d", bCanAttack));

	// if soldier can't attack because he doesn't have a weapon or is out of ammo
	// or his weapon isn't loaded
	if ( bCanAttack == NOSHOOT_NOAMMO ) // || NOLOAD
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_NOAMMO"));

		// try to reload it
		bCanAttack = TryToReload( pSoldier );
		
		if (bCanAttack == TRUE)
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("---successfully reloaded"));
			if (Chance(gGameExternalOptions.iChanceSayAnnoyingPhrase) || GetMagSize(&(pSoldier->inv[HANDPOS])) > 4)
				PossiblyStartEnemyTaunt( pSoldier, TAUNT_RELOAD );
		}
		else
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("---failed to reload"));
			//ScreenMsg(FONT_MCOLOR_LTRED, MSG_INTERFACE, L"[%d] failed to reload", pSoldier->ubID);

			if (Chance(gGameExternalOptions.iChanceSayAnnoyingPhrase) || GetMagSize(&(pSoldier->inv[HANDPOS])) > 4)
				PossiblyStartEnemyTaunt( pSoldier, TAUNT_OUT_OF_AMMO );
		}
	}
	else if (bCanAttack == NOSHOOT_NOWEAPON)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---NOSHOOT_NOWEAPON"));

		// sevenfm: look for IC_GUN first, then for IC_WEAPON if not found
		bWeaponIn = FindAIUsableObjClass( pSoldier, IC_GUN );
		if (bWeaponIn == NO_SLOT)
		{
			bWeaponIn = FindAIUsableObjClass( pSoldier, IC_WEAPON );
		}

		// look for another weapon
		//bWeaponIn = FindAIUsableObjClass( pSoldier, IC_WEAPON );

		if (bWeaponIn != NO_SLOT)
		{
			DebugAI( AI_MSG_INFO, pSoldier, String("---found another weapon, swapping weapon into hand"));

			RearrangePocket( pSoldier, HANDPOS, bWeaponIn, FOREVER );
			// look for another weapon if this one is 1-handed
			//			if ( (Item[ pSoldier->inv[ HANDPOS ].usItem ].usItemClass == IC_GUN) && !(Item[ pSoldier->inv[ HANDPOS ].usItem ].fFlags & ITEM_TWO_HANDED ) )
			if ( (Item[ pSoldier->inv[ HANDPOS ].usItem ].usItemClass == IC_GUN) && !(Item[ pSoldier->inv[ HANDPOS ].usItem ].twohanded ) )
			{
				// look for another pistol/SMG if available
				// CHRISL: Change final parameter to use dynamic pocket definition
				// sevenfm: better search for IC_GUN if we check it later anyway
				bWeaponIn = FindAIUsableObjClassWithin( pSoldier, IC_GUN, BIGPOCKSTART, NUM_INV_SLOTS );
				//bWeaponIn = FindAIUsableObjClassWithin( pSoldier, IC_WEAPON, BIGPOCKSTART, NUM_INV_SLOTS );
				//				if (bWeaponIn != NO_SLOT && (Item[ pSoldier->inv[ bWeaponIn ].usItem ].usItemClass == IC_GUN) && !(Item[ pSoldier->inv[ bWeaponIn ].usItem ].fFlags & ITEM_TWO_HANDED ) )
				if (bWeaponIn != NO_SLOT && (Item[ pSoldier->inv[ bWeaponIn ].usItem ].usItemClass == IC_GUN) && !(Item[ pSoldier->inv[ bWeaponIn ].usItem ].twohanded ) )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("---found weapon for second hand"));
					RearrangePocket( pSoldier, SECONDHANDPOS, bWeaponIn, FOREVER );
				}
			}
			// might need to reload
			DebugAI( AI_MSG_INFO, pSoldier, String("---might need to reload"));
			bCanAttack = CanNPCAttack( pSoldier );
		}
	}

#ifdef DEBUGDECISIONS
	if (bCanAttack != TRUE) // if for any reason we can't attack right now
	{
		//LocateMember(pSoldier->ubID,SETLOCATOR); // locate to this NPC, don't center
		STR16 tempstr;
		sprintf(tempstr,"%s can't attack! (not OKToAttack, Reason code = %d)",pSoldier->GetName(),bCanAttack);
		AIPopMessage(tempstr);
	}
#endif

	return( bCanAttack );
}

void CheckIfTossPossible(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//DebugShot( pSoldier, String("\n[Check if toss possible]"));
	INT16 ubMinAPcost;

	pSoldier->bWeaponMode = WM_NORMAL;

	if ( TANK( pSoldier ) )
	{
		//DebugShot(pSoldier, String("find cannon"));
		pBestThrow->bWeaponIn = FindCannon(pSoldier);
		//DebugShot(pSoldier, String("found cannon in slot %d", pBestThrow->bWeaponIn));
	}
	else
	{
		pBestThrow->bWeaponIn = FindAIUsableObjClass( pSoldier, IC_LAUNCHER );
		//DebugShot(pSoldier, String("found launcher in slot %d", pBestThrow->bWeaponIn));

		if( pBestThrow->bWeaponIn == NO_SLOT || 
			( !EnoughAmmo( pSoldier, FALSE, pBestThrow->bWeaponIn) && FindAmmoToReload( pSoldier, pBestThrow->bWeaponIn, NO_SLOT) == NO_SLOT) )
		{
			// Consider rocket launcher/cannon
			pBestThrow->bWeaponIn = FindRocketLauncherOrCannon( pSoldier );

			if( pBestThrow->bWeaponIn == NO_SLOT ||
				( !EnoughAmmo( pSoldier, FALSE, pBestThrow->bWeaponIn) && FindAmmoToReload( pSoldier, pBestThrow->bWeaponIn, NO_SLOT) == NO_SLOT) )
			{
				//dnl ch63 240813
				// no rocket launcher (or empty) -- let's look for an underslung/attached GL and a launchable grenade!
				INT8 bGunSlot = FindAIUsableObjClass(pSoldier, IC_GUN);
				pSoldier->bWeaponMode = WM_ATTACHED_GL;// So that EnoughAmmo will check for a grenade not a bullet, also need in calculation during CalcBestThrow
				if( bGunSlot != NO_SLOT &&
					IsGrenadeLauncherAttached(&pSoldier->inv[bGunSlot]) &&
					(EnoughAmmo(pSoldier, FALSE, bGunSlot) || FindAmmoToReload(pSoldier, bGunSlot, NO_SLOT) != NO_SLOT) )
				{
					pBestThrow->bWeaponIn = bGunSlot;
				}
				else
				{
					// no rocket launcher or attached GL, consider grenades
					pBestThrow->bWeaponIn = FindThrowableGrenade(pSoldier);
					pSoldier->bWeaponMode = WM_NORMAL;
				}
			}
			else
			{
				// Have rocket launcher... maybe have grenades as well.	which one to use?
				if ( pSoldier->aiData.bAIMorale > MORALE_WORRIED && PreRandom( 2 ) )
				{
					//dnl ch63 240813 use grenade if have one
					INT8 bGrenadeIn = FindThrowableGrenade(pSoldier);
					if(bGrenadeIn != NO_SLOT)
					{
						pBestThrow->bWeaponIn = bGrenadeIn;
					}
				}
			}
		}
		//DebugShot(pSoldier, String("throw weapon in slot %d", pBestThrow->bWeaponIn));
	}

	// if the soldier does have a tossable item somewhere
	if (pBestThrow->bWeaponIn != NO_SLOT)
	{
		// if it's in his holster, swap it into his hand temporarily
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}
		
		// get the minimum cost to attack with this tossable item
		ubMinAPcost = MinAPsToAttack( pSoldier, pSoldier->sLastTarget, DONTADDTURNCOST,0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we can afford the minimum AP cost to throw this tossable item
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			//DebugShot(pSoldier, String("check CalcBestThrow"));
			// then look around for a worthy target (which sets bestThrow.ubPossible)
			CalcBestThrow( pSoldier, pBestThrow );
			if( pBestThrow->ubPossible )
			{
				//DebugShot(pSoldier, String("throw possible"));
			}
			else
			{
				//DebugShot(pSoldier, String("throw not possible"));
			}
		}

		// if it was in his holster, swap it back into his holster for now
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket( pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY );
		}
	}
	pSoldier->bWeaponMode = WM_NORMAL;//dnl ch63 240813
}

INT8 CountAdjacentSpreadTargets( SOLDIERTYPE * pSoldier, INT16 sFirstTarget, INT8 bTargetLevel )
{
	// return the number of people next to this guy for burst-spread purposes

	INT8	bDirLoop, bDir, bCheckDir, bTargetIndex, bTargets;
	INT16	sTarget;
	SOLDIERTYPE * pTarget, * pTargets[5] = {NULL};

	bTargetIndex = -1;
	bCheckDir = -1;

	pTargets[2] = SimpleFindSoldier( sFirstTarget, bTargetLevel );
	if (pTargets[2] == NULL)
	{
		return( 0 );
	}
	bTargets = 1;

	bDir = (INT8) GetDirection( pSoldier->sGridNo, sFirstTarget );

	for (bDirLoop = 0; bDirLoop < 8; bDirLoop++)
	{
		if (bDir % 2)
		{
			// odd direction = diagonal direction
			switch( bDirLoop )
			{
			case 0:
				bCheckDir = (bDir + 6) % 8;
				bTargetIndex = 0;
				break;
			case 1:
				bCheckDir = (bDir + 5) % 8;
				bTargetIndex = 1;
				break;
			case 2:
				bCheckDir = (bDir + 7) % 8;
				bTargetIndex = 1;
				break;
			case 3:
				bCheckDir = (bDir + 3) % 8;
				bTargetIndex = 3;
				break;
			case 4:
				bCheckDir = (bDir + 1) % 8;
				bTargetIndex = 3;
				break;
			case 5:
				bCheckDir = (bDir + 2) % 8;
				bTargetIndex = 4;
				break;
			case 6:
				// check in front
				bCheckDir = (bDir + 4) % 8;
				bTargetIndex = 1;
				break;
			case 7:
				// check behind
				bCheckDir = (bDir) % 8;
				bTargetIndex = 3;
				break;
			}
		}
		else
		{
			// even = straight
			switch( bDirLoop )
			{
			case 0:
				bCheckDir = (bDir + 5) % 8;
				bTargetIndex = 1;
				break;
			case 1:
				bCheckDir = (bDir + 6) % 8;
				bTargetIndex = 1;
				break;
			case 2:
				bCheckDir = (bDir + 7) % 8;
				bTargetIndex = 1;
				break;
			case 3:
				bCheckDir = (bDir + 3) % 8;
				bTargetIndex = 3;
				break;
			case 4:
				bCheckDir = (bDir + 2) % 8;
				bTargetIndex = 3;
				break;
			case 5:
				bCheckDir = (bDir + 1) % 8;
				bTargetIndex = 3;
				break;
			case 6:
				// check in front
				bCheckDir = (bDir + 4) % 8;
				bTargetIndex = 1;
				break;
			case 7:
				// check behind
				bCheckDir = (bDir) % 8;
				bTargetIndex = 3;
				break;

			}
		}
		if (bDirLoop == 6 && bTargets > 1)
		{
			// we're done!	otherwise we continue and try to find people in front/behind
			break;
		}
		else if (pTargets[bTargetIndex] != NULL)
		{
			continue;
		}
		sTarget = sFirstTarget + DirIncrementer[bCheckDir];
		pTarget = SimpleFindSoldier( sTarget, bTargetLevel );
		if (pTarget)
		{
			// check to see if guy is visible
			if( PersonalKnowledge(pSoldier, pTarget->ubID) == SEEN_CURRENTLY )
			{
				pTargets[bTargetIndex] = pTarget;
				bTargets++;
			}
		}
	}
	return( bTargets - 1 );
}

INT16 CalcSpreadBurst( SOLDIERTYPE * pSoldier, INT16 sFirstTarget, INT8 bTargetLevel )
{
	INT8	bDirLoop, bDir, bCheckDir, bTargetIndex = 0, bLoop, bTargets;
	INT16	sTarget;
	SOLDIERTYPE * pTarget, * pTargets[5] = {NULL};
	INT8 bAdjacents, bOtherAdjacents;


	bCheckDir = -1;

	pTargets[2] = SimpleFindSoldier( sFirstTarget, bTargetLevel );
	if (pTargets[2] == NULL)
	{
		return( sFirstTarget );
	}
	bTargets = 1;
	bAdjacents = CountAdjacentSpreadTargets( pSoldier, sFirstTarget, bTargetLevel );

	bDir = (INT8) GetDirection( pSoldier->sGridNo, sFirstTarget );

	for (bDirLoop = 0; bDirLoop < 8; bDirLoop++)
	{
		if (bDir % 2)
		{
			// odd direction = diagonal direction
			switch( bDirLoop )
			{
			case 0:
				bCheckDir = (bDir + 6) % 8;
				bTargetIndex = 0;
				break;
			case 1:
				bCheckDir = (bDir + 5) % 8;
				bTargetIndex = 1;
				break;
			case 2:
				bCheckDir = (bDir + 7) % 8;
				bTargetIndex = 1;
				break;
			case 3:
				bCheckDir = (bDir + 3) % 8;
				bTargetIndex = 3;
				break;
			case 4:
				bCheckDir = (bDir + 1) % 8;
				bTargetIndex = 3;
				break;
			case 5:
				bCheckDir = (bDir + 2) % 8;
				bTargetIndex = 4;
				break;
			case 6:
				// check in front
				bCheckDir = (bDir + 4) % 8;
				bTargetIndex = 1;
				break;
			case 7:
				// check behind
				bCheckDir = (bDir) % 8;
				bTargetIndex = 3;
				break;
			}
		}
		else
		{
			// even = straight
			switch( bDirLoop )
			{
			case 0:
				bCheckDir = (bDir + 5) % 8;
				bTargetIndex = 1;
				break;
			case 1:
				bCheckDir = (bDir + 6) % 8;
				bTargetIndex = 1;
				break;
			case 2:
				bCheckDir = (bDir + 7) % 8;
				bTargetIndex = 1;
				break;
			case 3:
				bCheckDir = (bDir + 3) % 8;
				bTargetIndex = 3;
				break;
			case 4:
				bCheckDir = (bDir + 2) % 8;
				bTargetIndex = 3;
				break;
			case 5:
				bCheckDir = (bDir + 1) % 8;
				bTargetIndex = 3;
				break;
			case 6:
				// check in front
				bCheckDir = (bDir + 4) % 8;
				bTargetIndex = 1;
				break;
			case 7:
				// check behind
				bCheckDir = (bDir) % 8;
				bTargetIndex = 3;
				break;

			}
		}
		if (bDirLoop == 6 && bTargets > 1)
		{
			// we're done!	otherwise we continue and try to find people in front/behind
			break;
		}
		else if (pTargets[bTargetIndex] != NULL)
		{
			continue;
		}
		sTarget = sFirstTarget + DirIncrementer[bCheckDir];
		pTarget = SimpleFindSoldier( sTarget, bTargetLevel );
		if (pTarget && PersonalKnowledge(pSoldier, pTarget->ubID) == SEEN_CURRENTLY)
		{
			bOtherAdjacents = CountAdjacentSpreadTargets( pSoldier, sTarget, bTargetLevel );
			if (bOtherAdjacents > bAdjacents)
			{
				// we should do a spread-burst there instead!
				return( CalcSpreadBurst( pSoldier, sTarget, bTargetLevel ) );
			}
			pTargets[bTargetIndex] = pTarget;
			bTargets++;
		}
	}

	if (bTargets > 1)
	{
		// Move all the locations down in the array if necessary
		// Check the 4th position
		if (pTargets[3] == NULL && pTargets[4] != NULL)
		{
			pTargets[3] = pTargets[4];
			pTargets[4] = NULL;
		}
		// Check the first two positions; we know the 3rd value is set because
		// it's our initial target
		if (pTargets[1] == NULL)
		{
			pTargets[1] = pTargets[2];
			pTargets[2] = pTargets[3];
			pTargets[3] = pTargets[4];
			pTargets[4] = NULL;
		}
		if (pTargets[0] == NULL)
		{
			pTargets[0] = pTargets[1];
			pTargets[1] = pTargets[2];
			pTargets[2] = pTargets[3];
			pTargets[3] = pTargets[4];
			pTargets[4] = NULL;
		}
		// now 50% chance to reorganize to fire in reverse order
		if (Random( 2 ))
		{
			for( bLoop = 0; bLoop < bTargets / 2; bLoop++)
			{
				pTarget = pTargets[bLoop];
				pTargets[bLoop] = pTargets[bTargets - 1 - bLoop];
				pTargets[bTargets - 1 - bLoop] = pTarget;
			}
		}
		AIPickBurstLocations( pSoldier, bTargets, pTargets );
		pSoldier->flags.fDoSpread = TRUE;
	}
	return( sFirstTarget );
}

INT16 AdvanceToFiringRange( SOLDIERTYPE * pSoldier, INT16 sClosestOpponent )
{
	// see how far we can go down a path and still shoot

	INT16		bAttackCost, bTrueActionPoints;
	UINT16	usActionData;

	bAttackCost = MinAPsToAttack(pSoldier, sClosestOpponent, ADDTURNCOST,pSoldier->aiData.bAimTime);

	if (bAttackCost >= pSoldier->bActionPoints)
	{
		// probably want to go as far as possible!
		// return( NOWHERE );
		return( GoAsFarAsPossibleTowards( pSoldier, sClosestOpponent, AI_ACTION_SEEK_OPPONENT ) );
	}

	bTrueActionPoints = pSoldier->bActionPoints;

	pSoldier->bActionPoints -= bAttackCost;

	usActionData = GoAsFarAsPossibleTowards( pSoldier, sClosestOpponent, AI_ACTION_SEEK_OPPONENT );
	//POSSIBLE STRUCTURE PROBLEM HERE.  GOTTHARD 7/15/08
	pSoldier->bActionPoints = bTrueActionPoints;

	return( usActionData );

}

void CheckIfShotPossible(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestShot)
{
	//DebugShot( pSoldier, String("\nCheckIfShotPossible"));
	INT16 ubMinAPcost;
	pBestShot->ubPossible = FALSE;
	pBestShot->bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN);
	//DebugShot( pSoldier, String("found weapon in slot %d", pBestShot->bWeaponIn));

	// if the soldier does have a long range rifle
	if (pBestShot->bWeaponIn != NO_SLOT)
	{
		//OBJECTTYPE * pObj = &pSoldier->inv[pBestShot->bWeaponIn];
		//DebugShot( pSoldier, String("found a gun item %d", pObj->usItem));
		//DebugShot( pSoldier, String("weapon type %d",Weapon [pObj->usItem ].ubWeaponType));

		// if it's in his holster, swap it into his hand temporarily
		if (pBestShot->bWeaponIn != HANDPOS)
		{
			//DebugShot( pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestShot->bWeaponIn, TEMPORARILY);
		}

		// get the minimum cost to attack with this item
		ubMinAPcost = MinAPsToAttack( pSoldier, pSoldier->sLastTarget, ADDTURNCOST, 0);
		//DebugShot( pSoldier, String("min AP cost: %d", ubMinAPcost));

		// if we can afford the minimum AP cost
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			// then look around for a worthy target (which sets bestThrow.ubPossible)
			CalcBestShot( pSoldier, pBestShot);
			//DebugShot( pSoldier, pBestShot->ubPossible ? String("shot possible") : String("shot not possible"));
		}		

		// if it was in his holster, swap it back into his holster for now
		if (pBestShot->bWeaponIn != HANDPOS)
		{
			//DebugShot( pSoldier, String("rearrange pocket"));
			RearrangePocket( pSoldier, HANDPOS, pBestShot->bWeaponIn, TEMPORARILY );
		}

		// try to find sidearm
		if (pSoldier->bActionPoints < ubMinAPcost && pSoldier->IsMercBodyType())
		{
			//DebugShot( pSoldier, String("try to find sidearm"));
			pBestShot->bWeaponIn = FindAIUsableObjClass(pSoldier, IC_GUN, TRUE);

			if (pBestShot->bWeaponIn != NO_SLOT)
			{
				//OBJECTTYPE * pObj = &pSoldier->inv[pBestShot->bWeaponIn];
				//DebugShot( pSoldier, String("found a gun item %d", pObj->usItem));
				//DebugShot( pSoldier, String("weapon type %d",Weapon [pObj->usItem ].ubWeaponType));

				// if it's in his holster, swap it into his hand temporarily
				if (pBestShot->bWeaponIn != HANDPOS)
				{
					//DebugShot( pSoldier, String("rearrange pocket"));
					RearrangePocket(pSoldier, HANDPOS, pBestShot->bWeaponIn, TEMPORARILY);
				}

				// get the minimum cost to attack with this item
				ubMinAPcost = MinAPsToAttack(pSoldier, pSoldier->sLastTarget, ADDTURNCOST, 0);

				if (pSoldier->bActionPoints >= ubMinAPcost)
				{
					// then look around for a worthy target (which sets bestThrow.ubPossible)
					CalcBestShot(pSoldier, pBestShot);
					//DebugShot( pSoldier, pBestShot->ubPossible ? String("shot possible") : String("shot not possible"));
				}

				// if it was in his holster, swap it back into his holster for now
				if (pBestShot->bWeaponIn != HANDPOS)
				{
					//DebugShot( pSoldier, String("rearrange pocket"));
					RearrangePocket(pSoldier, HANDPOS, pBestShot->bWeaponIn, TEMPORARILY);
				}
			}
		}
	}
}

// SANDRO - function to determine if we should try to steal the enemy gun
BOOLEAN AIDetermineStealingWeaponAttempt( SOLDIERTYPE * pSoldier, SOLDIERTYPE * pOpponent )
{
	INT16 sChance = 0;
	UINT32 uiSuccessChance = 0;

	CHECKF(pOpponent);

	if( pOpponent->bTeam != gbPlayerNum )
	{
		return FALSE;
	}

	if( pOpponent->bCollapsed || pOpponent->bBreathCollapsed )
	{
		return FALSE;
	}

	// sevenfm: don't steal weapons from cowering or prone enemy to prevent possible bug
	if( pOpponent->IsCowering() ||
		gAnimControl[ pOpponent->usAnimState ].ubEndHeight == ANIM_PRONE )
	{
		return FALSE;
	}

	if(pSoldier->bBlindedCounter > 0)
		pSoldier->usMovementMode = SWATTING;
	else
		pSoldier->usMovementMode = RUNNING;

	if( pSoldier->bActionPoints < GetAPsToStealItem( pSoldier, NULL, pOpponent->sGridNo ) )
	{
		return FALSE;
	}

	if ((pOpponent->inv[HANDPOS].exists() != true))
	{
		return FALSE;
	}

	if ( !(Item[pOpponent->inv[HANDPOS].usItem].usItemClass & IC_WEAPON) )
	{
		return FALSE;
	}

	// sevenfm: trying to steal weapon with rifle sling may result in bug
	if (HasAttachmentOfClass(&(pOpponent->inv[HANDPOS]), AC_SLING))
	{
		return FALSE;
	}

	uiSuccessChance = CalcChanceToSteal(pSoldier, pOpponent, 0);
	if ( uiSuccessChance >= 100 )
		sChance = 90;
	else if ( uiSuccessChance >= 85 ) 
		sChance = 75;
	else if ( uiSuccessChance >= 70 ) 
		sChance = 60;
	else if ( uiSuccessChance >= 50 ) 
		sChance = 40;
	else if ( uiSuccessChance >= 25 ) 
		sChance = 15;
	else 
		return FALSE;

	if( gGameOptions.fNewTraitSystem )
	{
		if( !HAS_SKILL_TRAIT( pSoldier, MARTIAL_ARTS_NT ) )
			return FALSE;
		else if( NUM_SKILL_TRAITS( pSoldier, MARTIAL_ARTS_NT ) > 1 )
			sChance += 50;
	}
	else 
	{
		if( !HAS_SKILL_TRAIT( pSoldier, MARTIALARTS_OT ) && !HAS_SKILL_TRAIT( pSoldier, HANDTOHAND_OT ) )
			return FALSE;
		else if(( NUM_SKILL_TRAITS( pSoldier, MARTIALARTS_OT ) > 1 ) || ( NUM_SKILL_TRAITS( pSoldier, HANDTOHAND_OT ) > 1 ))
			sChance += 25;
	}

	if( pSoldier->stats.bLife < pSoldier->stats.bLifeMax )
	{
		sChance -= (pSoldier->stats.bLifeMax - pSoldier->stats.bLife);
	}

	if( pSoldier->bBreath < 100 )
	{
		sChance -= ((100 - pSoldier->bBreath) / 2);
	}

	if( pOpponent->stats.bLife < pOpponent->stats.bLifeMax )
	{
		sChance += (pOpponent->stats.bLifeMax - pOpponent->stats.bLife);
	}

	if( pOpponent->bBreath < pOpponent->bBreathMax )
	{
		sChance += ((pOpponent->bBreathMax - pOpponent->bBreath) / 2);
	}

	if( pSoldier->bActionPoints > (GetAPsToStealItem( pSoldier, NULL, pOpponent->sGridNo ) +  (2 * ApsToPunch( pSoldier ))) )
		sChance += 35;
	else if ( pSoldier->bActionPoints > (GetAPsToStealItem( pSoldier, NULL, pOpponent->sGridNo ) +  ApsToPunch( pSoldier )) )
		sChance += 20;
	else 
		sChance -= 10;

	if (Chance(sChance))
		return TRUE;

	return FALSE;
}

// HEADROCK HAM 4: This is required for the AI to be able to assess the length of autofire volleys using the new
// recoil system. 
FLOAT AICalcRecoilForShot( SOLDIERTYPE *pSoldier, OBJECTTYPE *pWeapon, UINT8 ubShotNum)
{
	FLOAT bRecoilX = 0;
	FLOAT bRecoilY = 0;
	GetRecoil( pSoldier, pWeapon, &bRecoilX, &bRecoilY, ubShotNum );
	// Return average shooter's ability to control this gun.
	// HEADROCK HAM 4: TODO: Incorporate items that alter max counter-force.
	FLOAT AverageRecoil = max(0, ( sqrt( (bRecoilX * bRecoilX) + (bRecoilY * bRecoilY) ) - (gGameCTHConstants.RECOIL_MAX_COUNTER_FORCE * 0.7f) ) );
	return AverageRecoil;
}

//dnl ch61 180813
UnderFire gUnderFire;

void UnderFire::Clear(void)
{
	usUnderFireCnt = 0;
	memset(usUnderFireID, 0, sizeof(usUnderFireID));
	memset(ubUnderFireCTH, 0, sizeof(ubUnderFireCTH));
}

void UnderFire::Add(UINT16 usID, UINT8 ubCTH)
{
	if (!fEnable)
		return;

	if (usUnderFireCnt < MAXUNDERFIRE)
	{
		for (int i = 0; i < usUnderFireCnt; i++)
		{
			if (usUnderFireID[i] == usID)
			{
				if (ubUnderFireCTH[i] < ubCTH)		// sevenfm: use max value
					ubUnderFireCTH[i] = ubCTH;
				return;
			}
		}
		ubUnderFireCTH[usUnderFireCnt] = ubCTH;		// sevenfm: store CTH too!
		usUnderFireID[usUnderFireCnt] = usID;
		usUnderFireCnt++;
	}
}

UINT16 UnderFire::Count(INT8 bTeam)
{
	UINT16 cnt = 0;
	for (UINT16 i = 0; i < usUnderFireCnt; i++)
	{
		if (MercPtrs[usUnderFireID[i]]->bTeam == bTeam)
			++cnt;
	}
	return(cnt);
}

UINT8 UnderFire::Chance(INT8 bTeam, INT8 bSide, BOOLEAN fCheckNeutral)
{
	UINT8 cth = 0;
	for (UINT16 i = 0; i < usUnderFireCnt; i++)
	{
		if ((MercPtrs[usUnderFireID[i]]->bTeam == bTeam || MercPtrs[usUnderFireID[i]]->bSide == bSide || fCheckNeutral && MercPtrs[usUnderFireID[i]]->aiData.bNeutral) &&
			ubUnderFireCTH[i] > cth)
		{
			cth = ubUnderFireCTH[i];
		}
	}
	return(cth);
}

// Flugente AI functions
// determine a gridno that would allow us to hit as many enemies as possible given an effect with radius aRadius tiles
// return true if sufficient gridno is found
// pGridNo will be the GridNo
// aRadius is the area effect radius to use
// uCheckFriends: 0 - do not consider friends at all 1 - consider with negative weight else: ignore any location that might also hit friends
// success only if at a rating of at least aMinRating can be achieved
// any enemy soldiers not fulfilling cond will be excluded from this calculation
// if an enemy soldier fulfills taboo, make sure to not hit him at all!
#define MAX_EXCLUDE_TILES 100
BOOLEAN GetBestAoEGridNo(SOLDIERTYPE *pSoldier, INT32* pGridNo, INT16 aRadius, UINT8 uCheckFriends, UINT8 aMinRating, SOLDIER_CONDITION cond, SOLDIER_CONDITION taboo)
{
	UINT8	ubLoop, ubLoop2;
	INT32	sGridNo, sFriendTile[MAXMERCS], sOpponentTile[MAXMERCS], sTabooTile[MAXMERCS];
	UINT8	ubFriendCnt = 0,ubOpponentCnt = 0, ubTabooCnt = 0, ubOpponentID[MAXMERCS];
	INT32	bMaxLeft,bMaxRight,bMaxUp,bMaxDown, i, j;
	INT8	bKnowledge;
	SOLDIERTYPE *pFriend;
	static INT16	sExcludeTile[MAX_EXCLUDE_TILES]; // This array is for storing tiles that we have already considered, to prevent duplication of effort
	UINT8	ubNumExcludedTiles = 0;

	INT32 lowestX  = 999999;
	INT32 highestX = 0;
	INT32 lowestY  = 999999;
	INT32 highestY = 0;

	// make lists of enemies and friends
	for (ubLoop = 0; ubLoop < guiNumMercSlots; ++ubLoop)
	{
		pFriend = MercSlots[ubLoop];

		if ( !pFriend || !pFriend->bActive || !pFriend->bInSector )
			continue;

		if (pFriend->stats.bLife == 0)
			continue;

		// dying or captured friends are 'helpless' anyway, we are willing to sacrifice them :-)
		if ( uCheckFriends && pSoldier->bSide == pFriend->bSide && pFriend->stats.bLife >= OKLIFE && !pFriend->IsPOW() )
		{
			// active friend, remember where he is so that we DON'T blow him up!
			// this includes US, since we don't want to blow OURSELVES up either
			sFriendTile[ubFriendCnt] = pFriend->sGridNo;
			ubFriendCnt++;
		}
		else
		{
			// if an enemy fulfills taboo, we will remember his tile and be careful not to ever hit it!
			if ( taboo(pFriend) )
			{
				sTabooTile[ubTabooCnt] = pFriend->sGridNo;
				++ubTabooCnt;
				continue;
			}

			// sevenfm: checked in ValidOpponent()
			// Special stuff for Carmen the bounty hunter
			//if (pSoldier->aiData.bAttitude == ATTACKSLAYONLY && pFriend->ubProfile != SLAY)
			//	continue;

			// check whether this guy fulfills the target condition
			if (!cond(pFriend))
				continue;

			if (!ValidOpponent(pSoldier, pFriend))
				continue;

			//bPersonalKnowledge = PersonalKnowledge(pSoldier, pFriend->ubID);
			//bPublicKnowledge = PublicKnowledge(pSoldier->bTeam, pFriend->ubID);
			bKnowledge = Knowledge(pSoldier, pFriend->ubID);

			if (bKnowledge == SEEN_CURRENTLY)
			{
				// active KNOWN opponent, remember where he is so that we DO blow him up!
				sOpponentTile[ubOpponentCnt] = pFriend->sGridNo;
			}
			//else if ( bKnowledge == SEEN_LAST_TURN || bKnowledge == HEARD_LAST_TURN || bKnowledge == HEARD_THIS_TURN || bKnowledge == SEEN_THIS_TURN)
			else if (bKnowledge >= HEARD_2_TURNS_AGO && bKnowledge <= SEEN_2_TURNS_AGO)
			{
				sOpponentTile[ubOpponentCnt] = KnownLocation(pSoldier, pFriend->ubID);
				/*sOpponentTile[ubOpponentCnt] = gsLastKnownOppLoc[pSoldier->ubID][pFriend->ubID];				
				// cheat; only allow throw if person is REALLY within 2 tiles of where last seen
				if ( SpacesAway( pFriend->sGridNo, gsLastKnownOppLoc[ pSoldier->ubID ][ pFriend->ubID ] ) < 3 )
				{
					sOpponentTile[ubOpponentCnt] = gsLastKnownOppLoc[ pSoldier->ubID ][ pFriend->ubID ];
				}*/
			}			
			else
			{
				continue;
			}

			// also remember who he is (which soldier #)
			ubOpponentID[ubOpponentCnt] = pFriend->ubID;

			// update lowest and highest x and y values
			lowestX  = min(lowestX,  sOpponentTile[ubOpponentCnt] % MAXCOL );
			highestX = max(highestX, sOpponentTile[ubOpponentCnt] % MAXCOL );
			lowestY  = min(lowestY,  sOpponentTile[ubOpponentCnt] / MAXCOL );
			highestY = max(highestY, sOpponentTile[ubOpponentCnt] / MAXCOL );

			ubOpponentCnt++;
		}
	}

	// no/not enough enemies found -> no area effect location advisable
	if ( !ubOpponentCnt || ubOpponentCnt < aMinRating )
		return FALSE;

	BOOLEAN fGridNoFound = FALSE;
	INT32 bestGridNo = -1;
	INT8 bestGridNoCnt = aMinRating;

	INT32 currentSoldierGridNo = -1;	

	INT8 enemiesnear = 0;
	INT8 friendsnear = 0;

	// look at the squares near each known opponent and try to find the one
	// place where a tossed projectile would do the most harm to the opponents
	// while avoiding one's friends
	for (ubLoop = 0; ubLoop < ubOpponentCnt; ++ubLoop)
	{
		currentSoldierGridNo = sOpponentTile[ubLoop];

		// determine maximum horizontal limits
		bMaxLeft  = max(currentSoldierGridNo % MAXCOL - aRadius, lowestX);
		bMaxRight = min(currentSoldierGridNo % MAXCOL + aRadius, highestX);

		// determine maximum vertical limits
		bMaxDown  = max(currentSoldierGridNo / MAXCOL - aRadius, lowestY);
		bMaxUp	  = min(currentSoldierGridNo / MAXCOL + aRadius, highestY);

		// evaluate every tile for its opponent-damaging potential
		for (i = bMaxLeft; i <= bMaxRight; ++i)
		{
			for (j = bMaxDown; j <= bMaxUp; ++j)
			{
				// calculate the next potential gridno near this opponent
				sGridNo = i + (MAXCOL * j);

				// this shouldn't ever happen
				if ((sGridNo < 0) || (sGridNo >= GRIDSIZE))
					continue;

				// sevenfm: exclude offscreen tiles as they may cause bugs
				if (!GridNoOnVisibleWorldTile(sGridNo))
					continue;

				if (PythSpacesAway(currentSoldierGridNo, sGridNo) > aRadius)
					continue;

				// if this tile is taboo, don't even think about targeting it!
				for (ubLoop2 = 0; ubLoop2 < ubTabooCnt; ++ubLoop2)
				{
					if (sTabooTile[ubLoop2] == sGridNo)
						continue;
				}

				// Check to see if we have considered this tile before:
				for (ubLoop2 = 0; ubLoop2 < ubNumExcludedTiles; ++ubLoop2)
				{
					if (sExcludeTile[ubLoop2] == sGridNo)
						continue;
				}

				// add this tile to the list of already checked tiles
				if (ubNumExcludedTiles < MAX_EXCLUDE_TILES)
				{
					sExcludeTile[ubNumExcludedTiles] = sGridNo;
					++ubNumExcludedTiles;
				}

				// loop over all enemies and friends to determine how many are in range
				enemiesnear = 0;
				friendsnear = 0;

				// check whether there are any friends near this gridno
				for (ubLoop2 = 0; ubLoop2 < ubFriendCnt; ++ubLoop2)
				{
					if ( PythSpacesAway(sFriendTile[ubLoop2], sGridNo) <= aRadius )
						++friendsnear;
				}

				// ignore this location if friends are found and we want to absolutely ignore friendly fire
				if ( friendsnear && uCheckFriends > 1 )
					continue;

				// check whether there are any enemies near this gridno
				for (ubLoop2 = 0; ubLoop2 < ubOpponentCnt; ++ubLoop2)
				{
					if ( PythSpacesAway(sOpponentTile[ubLoop2], sGridNo) <= aRadius )
					{
						++enemiesnear;
					}
				}

				if ( enemiesnear - friendsnear >= bestGridNoCnt )
				{
					bestGridNoCnt = enemiesnear - friendsnear;

					bestGridNo = sGridNo;
					fGridNoFound = TRUE;
				}		
			}
		}
	}

	*pGridNo = bestGridNo;

	return fGridNoFound;
}

// Get the ID of the farthest opponent  we can see, with an optional minimum range
// puID - ID of the farthest opponent pSoldier can see
// sRange - only return an true and give an idea if opponent found is further away than this
BOOLEAN GetFarthestOpponent(SOLDIERTYPE *pSoldier, UINT8* puID, INT16 sRange)
{
	INT32 sGridNo;
	UINT32 uiLoop;
	INT32 iRange = 0;;
	SOLDIERTYPE * pOpp;
	BOOLEAN found = FALSE;

	*puID = NOBODY;

	// look through this man's personal & public opplists for opponents known
	for (uiLoop = 0; uiLoop < guiNumMercSlots; ++uiLoop)
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

		// if this opponent is not seen personally
		if( PersonalKnowledge(pSoldier, pOpp->ubID) != SEEN_CURRENTLY )
		{
			continue;			// next merc
		}

		// since we're dealing with seen people, use exact gridnos
		sGridNo = pOpp->sGridNo;

		// if we are standing at that gridno(!, obviously our info is old...)
		if (sGridNo == pSoldier->sGridNo)
		{
			continue;			// next merc
		}

		// I hope this will be good enough; otherwise we need a fractional/world-units-based 2D distance function
		//sRange = PythSpacesAway( pSoldier->sGridNo, sGridNo);
		iRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sGridNo );

		if (iRange > sRange)
		{
			sRange = iRange;
			*puID = uiLoop;
			found = TRUE;
		}
	}

	return( found );
}

// are there more allies than friends in adjacent sectors?
BOOLEAN MoreFriendsThanEnemiesInNearbysectors(UINT8 ausTeam, INT16 aX, INT16 aY, INT8 aZ)
{
	UINT16 enemyteam = NumEnemiesInFiveSectors(aX, aY) - NumEnemiesInAnySector(aX, aY, aZ);
	UINT16 militiateam = CountAllMilitiaInFiveSectors(aX, aY) - CountAllMilitiaInSector(aX, aY);

	if ( ausTeam == ENEMY_TEAM )
		return (enemyteam > militiateam);

	return (militiateam > enemyteam);
}

void CheckTossGrenadeAt(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow, INT32 sTargetSpot, INT8 bTargetLevel, UINT8 ubGrenadeType)
{
	//DebugShot( pSoldier, String("\nCheckTossGrenadeAt"));

	INT16 ubMinAPcost;
	INT8 bGrenadeIn = NO_SLOT;

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;	

	if ( !IS_MERC_BODY_TYPE( pSoldier ) )
	{
		//DebugShot( pSoldier, String("not merc body type"));
		return;
	}

	if( TileIsOutOfBounds(sTargetSpot) || !GridNoOnVisibleWorldTile(sTargetSpot))
	{
		//DebugShot( pSoldier, String("bad sTargetSpot %d", sTargetSpot));
		return;
	}

	pSoldier->bWeaponMode = WM_NORMAL;

	bGrenadeIn = FindThrowableGrenade(pSoldier, ubGrenadeType);

	if(bGrenadeIn != NO_SLOT)
	{
		pBestThrow->bWeaponIn = bGrenadeIn;
		//DebugShot(pSoldier, String("found grenade in slot %d", pBestThrow->bWeaponIn));

		// if it's in his holster, swap it into his hand temporarily
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}

		// get the minimum cost to attack with this tossable item
		ubMinAPcost = MinAPsToAttack( pSoldier, pSoldier->sGridNo, DONTADDTURNCOST,0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we can afford the minimum AP cost to throw this tossable item
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			//DebugShot(pSoldier, String("check if can throw grenade"));
			CheckTossAt(pSoldier, pBestThrow, sTargetSpot, bTargetLevel, NOBODY);
		}

		// if it was in his holster, swap it back into his holster for now
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket( pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY );
		}
	}
	else
	{
		//DebugShot( pSoldier, String("not found grenade"));
	}

	pSoldier->bWeaponMode = WM_NORMAL;
}

void CheckTossSelfSmoke(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//DebugShot( pSoldier, String("\nCheckTossSelfSmoke"));

	INT16 ubMinAPcost;
	INT8 bGrenadeIn = NO_SLOT;	

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;	

	if ( !IS_MERC_BODY_TYPE( pSoldier ) )
	{
		//DebugShot( pSoldier, String("not merc body type"));
		return;
	}

	pSoldier->bWeaponMode = WM_NORMAL;

	bGrenadeIn = FindThrowableGrenade(pSoldier, EXPLOSV_SMOKE);

	// prepare threat list for ClosestSeenThreatID(), ClosestKnownThreatID()
	PrepareThreatlist(pSoldier);

	if(bGrenadeIn != NO_SLOT)
	{
		pBestThrow->bWeaponIn = bGrenadeIn;
		//DebugShot(pSoldier, String("found smoke grenade in slot %d", pBestThrow->bWeaponIn));

		// if it's in his holster, swap it into his hand temporarily
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}

		// get the minimum cost to attack with this tossable item
		ubMinAPcost = MinAPsToAttack( pSoldier, pSoldier->sGridNo, DONTADDTURNCOST,0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we can afford the minimum AP cost to throw this tossable item
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			//DebugShot(pSoldier, String("check if can throw grenade"));
			
			INT32	sSpot = pSoldier->sGridNo;
			INT8	bLevel = pSoldier->pathing.bLevel;

			INT32	sTargetSpot = NOWHERE;
			INT8	bTargetLevel = bLevel;

			INT32	sClosestThreat;
			UINT8	ubClosestThreatID;

			ubClosestThreatID = pSoldier->ubPreviousAttackerID;

			// try to find good spot for smoke
			if( ubClosestThreatID != NOBODY &&
				MercPtrs[ubClosestThreatID] &&
				!TileIsOutOfBounds( MercPtrs[ubClosestThreatID]->sGridNo ) )
			{
				sClosestThreat = MercPtrs[ubClosestThreatID]->sGridNo;

				//DebugShot( pSoldier, String("closest attacker [%d] at %d %d", ubClosestThreatID, sClosestThreat, MercPtrs[ubClosestThreatID]->pathing.bLevel));

				sTargetSpot = FindTossSpotInDirection(sSpot, bLevel, sClosestThreat, TRUE, TRUE);
			}

			if( TileIsOutOfBounds(sTargetSpot) )
			{
				ubClosestThreatID = ClosestSeenThreatID( pSoldier, SEEN_LAST_TURN );

				if( ubClosestThreatID != NOBODY &&
					MercPtrs[ubClosestThreatID] &&
					!TileIsOutOfBounds(MercPtrs[ubClosestThreatID]->sGridNo) )
				{
					sClosestThreat = MercPtrs[ubClosestThreatID]->sGridNo;

					//DebugShot( pSoldier, String("closest seen threat [%d] at %d %d", ubClosestThreatID, sClosestThreat, MercPtrs[ubClosestThreatID]->pathing.bLevel));

					sTargetSpot = FindTossSpotInDirection(sSpot, bLevel, sClosestThreat, TRUE, TRUE);
				}				
			}

			if( TileIsOutOfBounds(sTargetSpot) )
			{
				ubClosestThreatID = ClosestKnownThreatID( pSoldier );

				if( ubClosestThreatID != NOBODY &&
					MercPtrs[ubClosestThreatID] &&
					!TileIsOutOfBounds(MercPtrs[ubClosestThreatID]->sGridNo) )
				{
					sClosestThreat = MercPtrs[ubClosestThreatID]->sGridNo;

					//DebugShot( pSoldier, String("closest known threat [%d] at %d %d", ubClosestThreatID, sClosestThreat, MercPtrs[ubClosestThreatID]->pathing.bLevel));

					sTargetSpot = FindTossSpotInDirection(sSpot, bLevel, sClosestThreat, TRUE, TRUE);
				}
			}

			if( !TileIsOutOfBounds(sTargetSpot) )
			{
				CheckTossAt(pSoldier, pBestThrow, sTargetSpot, bTargetLevel, pSoldier->ubID);
			}
			else
			{
				//DebugShot(pSoldier, String("could not find good throwing spot"));
			}
		}

		// if it was in his holster, swap it back into his holster for now
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket( pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY );
		}
	}
	else
	{
		//DebugShot( pSoldier, String("not found grenade"));
	}

	pSoldier->bWeaponMode = WM_NORMAL;
}

void CheckTossFlankFence(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//ScreenMsg(FONT_KHAKI, MSG_INTERFACE, L"[%d] Check Toss Flank Fence", pSoldier->ubID);
	//DebugShot(pSoldier, String("\nCheckTossFlankFence"));

	INT16 ubMinAPcost;
	INT8 bGrenadeIn = NO_SLOT;

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;
	pBestThrow->ubOpponent = NOBODY;

	if (!IS_MERC_BODY_TYPE(pSoldier))
	{
		//DebugShot(pSoldier, String("not merc body type"));
		return;
	}

	pSoldier->bWeaponMode = WM_NORMAL;

	bGrenadeIn = FindThrowableGrenade(pSoldier, EXPLOSV_NORMAL, 10);

	if (bGrenadeIn != NO_SLOT)
	{
		pBestThrow->bWeaponIn = bGrenadeIn;
		//DebugShot(pSoldier, String("found grenade in slot %d", pBestThrow->bWeaponIn));

		// if it's in his holster, swap it into his hand temporarily
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}

		// get the minimum cost to attack with this tossable item
		ubMinAPcost = MinAPsToAttack(pSoldier, pSoldier->sGridNo, DONTADDTURNCOST, 0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we can afford the minimum AP cost to throw this tossable item
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			// get direction of position to flank from soldier's position
			UINT8 ubDesiredDir = FlankingDirection(pSoldier);

			INT32 sFenceSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubDesiredDir));

			if (sFenceSpot != pSoldier->sGridNo && 
				(FindConcertina(sFenceSpot) || IsCuttableWireFenceAtGridNo(sFenceSpot) || FindStruct(sFenceSpot, pSoldier->pathing.bLevel, BLUEFLAG_GRAPHIC)))
			{
				CheckTossAt(pSoldier, pBestThrow, sFenceSpot, pSoldier->pathing.bLevel, NOBODY);

				// check adjacent tiles
				if (!pBestThrow->ubPossible)
				{					
					INT32 sTempSpot;
					
					for (UINT8 ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
					{
						sTempSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubDirection));

						if (sTempSpot != pSoldier->sGridNo)
						{
							CheckTossAt(pSoldier, pBestThrow, sTempSpot, pSoldier->pathing.bLevel, NOBODY);

							if (pBestThrow->ubPossible)
							{
								break;
							}
						}
					}
				}
			}

			// if it was in his holster, swap it back into his holster for now
			if (pBestThrow->bWeaponIn != HANDPOS)
			{
				//DebugShot(pSoldier, String("rearrange pocket"));
				RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
			}
		}
		else
		{
			//DebugShot(pSoldier, String("not enough AP to throw grenade"));
		}
	}
	else
	{
		//DebugShot(pSoldier, String("not found grenade"));
	}

	pSoldier->bWeaponMode = WM_NORMAL;
	//ScreenMsg(FONT_KHAKI, MSG_INTERFACE, L"Check Toss Flank Fence end");
}

void CheckTossGrenadeSpecial(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"[%d] Check Toss Grenade Special", pSoldier->ubID);
	//BeginMultiPurposeLocator(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE);
	//DebugShot(pSoldier, String("\nCheckTossGrenadeSpecial"));

	INT16 ubMinAPcost;
	INT8 bGrenadeIn = NO_SLOT;
	UINT16 usGrenade;
	UINT8 ubType;
	UINT8 ubRadius;
	UINT8 ubMinDamage;

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;
	pBestThrow->ubOpponent = NOBODY;

	if (!IS_MERC_BODY_TYPE(pSoldier))
	{
		//DebugShot(pSoldier, String("not merc body type"));
		return;
	}

	pSoldier->bWeaponMode = WM_NORMAL;

	for (ubType = EXPLOSV_NORMAL; ubType < EXPLOSV_ANY_TYPE; ubType++)
	{
		switch(ubType)
		{
		case EXPLOSV_NORMAL:
		case EXPLOSV_STUN:
			ubMinDamage = 10; break;
		case EXPLOSV_MUSTGAS:
		case EXPLOSV_CREATUREGAS:
		case EXPLOSV_BURNABLEGAS:
			ubMinDamage = 5; break;
		case EXPLOSV_FLASHBANG:
			ubMinDamage = 1; break;
		case EXPLOSV_FLARE:
		case EXPLOSV_NOISE:
		case EXPLOSV_SMOKE:
		case EXPLOSV_SIGNAL_SMOKE:
		case EXPLOSV_TEARGAS:
			ubMinDamage = 0; break;
		default:
			ubMinDamage = 10;
		}

		bGrenadeIn = FindThrowableGrenade(pSoldier, ubType, ubMinDamage);

		if (bGrenadeIn != NO_SLOT)
		{
			//if (ubType == EXPLOSV_NORMAL)
			//	ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"soldier has grenade %d", pSoldier->inv[bGrenadeIn].usItem);

			pBestThrow->bWeaponIn = bGrenadeIn;
			usGrenade = pSoldier->inv[bGrenadeIn].usItem;
			//ubType = Explosive[Item[usGrenade].ubClassIndex].ubType;
			ubRadius = max(Explosive[Item[usGrenade].ubClassIndex].ubRadius, min(TACTICAL_RANGE / 4, Explosive[Item[usGrenade].ubClassIndex].ubFragRange / CELL_X_SIZE));
			//DebugShot(pSoldier, String("found grenade in slot %d", pBestThrow->bWeaponIn));

			// if it's in his holster, swap it into his hand temporarily
			if (pBestThrow->bWeaponIn != HANDPOS)
			{
				//DebugShot(pSoldier, String("rearrange pocket"));
				RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
			}

			// get the minimum cost to attack with this tossable item
			ubMinAPcost = MinAPsToAttack(pSoldier, pSoldier->sGridNo, DONTADDTURNCOST, 0);
			//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

			// if we can afford the minimum AP cost to throw this tossable item
			if (pSoldier->bActionPoints >= ubMinAPcost)
			{
				//DebugShot(pSoldier, String("Enough AP, find best spot for grenade"));

				INT16 sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
				INT32 iSearchRange = TACTICAL_RANGE / 2;
				INT32 sSpot;
				INT32 sBestSpot = NOWHERE;
				INT8 bSoldierLevel = pSoldier->pathing.bLevel;
				INT8 bLevel = 0;	// always throw at ground level
				UINT8 ubMinDistance = TACTICAL_RANGE / 8;
				INT16 sMaxEnemyDistance = TACTICAL_RANGE * 2;
				INT32 iValue;
				INT32 iBestValue = 0;

				INT32	sClosestOpponent;
				INT32	sClosestOpponentPathCost = 0;
				INT32	sClosestOpponentStraightPathCost = 0;
				INT32	sEnemySpot = NOWHERE;
				INT8	bEnemyLevel = 0;
				INT16	sClosestOpponentDistance = 0;
				UINT8	ubSpotDir;
				INT16	sObstaclePercent = 0;
				//UINT8	ubStructureExplosionRadius = Explosive[Item[STRUCTURE_IGNITE].ubClassIndex].ubRadius;
				UINT8	ubStructureExplosionRadius = (UINT8)Explosive[Item[C1].ubClassIndex].ubRadius;
				UINT16	usRoom;

				sClosestOpponent = ClosestKnownOpponent(pSoldier, &sEnemySpot, &bEnemyLevel);

				if (!TileIsOutOfBounds(sEnemySpot) && PythSpacesAway(pSoldier->sGridNo, sEnemySpot) <= sMaxEnemyDistance)
				{
					sClosestOpponentPathCost = PlotPath(pSoldier, sEnemySpot, FALSE, FALSE, FALSE, RUNNING, 0, FALSE, 0);
					sClosestOpponentDistance = PythSpacesAway(pSoldier->sGridNo, sEnemySpot);
					sClosestOpponentStraightPathCost = sClosestOpponentDistance * (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_RUN]);
					sObstaclePercent = min(100, 100 * abs(sClosestOpponentPathCost - sClosestOpponentStraightPathCost) / sClosestOpponentStraightPathCost);
				}
				//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"[%d] enemy %d level %d path cost %d distance %d straight cost %d", pSoldier->ubID, sEnemySpot, bEnemyLevel, sClosestOpponentPathCost, sClosestOpponentDistance, sClosestOpponentStraightPathCost);

				// determine maximum horizontal limits
				sMaxLeft = min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
				sMaxRight = min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));

				// determine maximum vertical limits
				sMaxUp = min(iSearchRange, (pSoldier->sGridNo / MAXROW));
				sMaxDown = min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

				//DebugShot(pSoldier, String("search range %d max left %d max right %d max up %d max down %d", iSearchRange, sMaxLeft, sMaxRight, sMaxUp, sMaxDown));

				// check all tiles in range
				for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
				{
					for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
					{
						sSpot = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

						if (TileIsOutOfBounds(sSpot))
						{
							continue;
						}

						// basic checks
						if (Water(sSpot, bLevel))
						{
							continue;
						}

						// too close to soldier or any friend
						if (PythSpacesAway(pSoldier->sGridNo, sSpot) < ubMinDistance || CountNearbyFriends(pSoldier, sSpot, ubMinDistance) > 0)
						{
							//continue;
						}

						// check for neutrals
						if (CountNearbyNeutrals(pSoldier, sSpot, ubMinDistance))
						{
							//continue;
						}

						// if there's already active grenade there
						if (FindBombNearby(pSoldier, sSpot, 1))
						{
							continue;
						}

						iValue = 0;

						////DebugShot(pSoldier, String("check spot %d for zombie corpse", sSpot));
						// check for zombie corpse
						if (gGameSettings.fOptions[TOPTION_ZOMBIES])
						{
							INT32			cnt;
							ROTTING_CORPSE *pCorpse;
							UINT16 recanimstate;

							for (cnt = 0; cnt < giNumRottingCorpse; ++cnt)
							{
								pCorpse = &(gRottingCorpse[cnt]);

								if (pCorpse &&
									pCorpse->fActivated &&
									pCorpse->def.bLevel == bLevel &&
									pCorpse->def.sGridNo == sSpot &&
									!(pCorpse->def.usFlags & (ROTTING_CORPSE_HEAD_TAKEN | ROTTING_CORPSE_NEVER_RISE_AGAIN)) &&
									CorpseOkToSpawnZombie(pCorpse, &recanimstate) &&
									(ubType == EXPLOSV_NORMAL || ubType == EXPLOSV_BURNABLEGAS))
								{
									iValue = max(iValue, 5 * gGameExternalOptions.sZombieDifficultyLevel);
									//DebugShot(pSoldier, String("spot %d, found zombie corpse!", sSpot));
									break;
								}
							}
						}

						////DebugShot(pSoldier, String("check spot %d for blue flag", sSpot));
						// check for obstacles between soldier and closest enemy
						if (sObstaclePercent > 25 &&
							bSoldierLevel == 0 &&
							pSoldier->aiData.bOrders == SEEKENEMY &&
							!TileIsOutOfBounds(sEnemySpot) &&
							ubType == EXPLOSV_NORMAL &&
							(FindStruct(sSpot, bLevel, BLUEFLAG_GRAPHIC) || FindConcertina(sSpot) || IsCuttableWireFenceAtGridNo(sSpot)))
						{
							INT32	sTempSpot;
							UINT8	ubDirection;
							UINT8	ubCount = 0;
							//INT32	sPathCost;

							/*INT32	sClosestSpot = NOWHERE;
							INT32	sClosestPathCost;							

							// check adjacent tiles, find closest reachable spot
							for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
							{
								sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

								if (sTempSpot != sSpot && NewOKDestination(pSoldier, sTempSpot, FALSE, pSoldier->pathing.bLevel))
								{
									sPathCost = PlotPath(pSoldier, sTempSpot, FALSE, FALSE, FALSE, RUNNING, 0, FALSE, 0);

									if (sPathCost > 0 && (sClosestSpot == NOWHERE || sPathCost < sClosestPathCost))
									{
										sClosestSpot = sTempSpot;
										sClosestPathCost = sPathCost;
									}
								}
							}

							// found reachable spot around destructible object, check if there's another spot around object tile which greatly reduces path to closest enemy
							if (!TileIsOutOfBounds(sClosestSpot))
							{
								INT32 sClosestNewSpot = NOWHERE;
								INT32 sClosestNewPathCost;

								// remember original spot
								INT32 sOriginalSpot = pSoldier->sGridNo;

								// check path from adjacent spots
								for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
								{
									sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

									if (sTempSpot != sSpot && sTempSpot != sClosestSpot)
									{
										if (NewOKDestination(pSoldier, sTempSpot, FALSE, pSoldier->pathing.bLevel))
										{
											// move soldier to new spot
											pSoldier->sGridNo = sTempSpot;

											sPathCost = PlotPath(pSoldier, sEnemySpot, FALSE, FALSE, FALSE, RUNNING, 0, FALSE, 0);

											if (sPathCost > 0 && (sClosestNewSpot == NOWHERE || sPathCost < sClosestNewPathCost))
											{
												sClosestNewSpot = sTempSpot;
												sClosestNewPathCost = sPathCost;
											}
										}
										else if (FindStruct(sTempSpot, bLevel, BLUEFLAG_GRAPHIC) || FindConcertina(sTempSpot) || IsCuttableWireFenceAtGridNo(sTempSpot))
										{
											// maybe try one more step
											INT32 sTempSpot2 = NewGridNo(sTempSpot, DirectionInc(ubDirection));

											if (NewOKDestination(pSoldier, sTempSpot2, FALSE, pSoldier->pathing.bLevel))
											{
												// move soldier to new spot
												pSoldier->sGridNo = sTempSpot2;

												sPathCost = PlotPath(pSoldier, sEnemySpot, FALSE, FALSE, FALSE, RUNNING, 0, FALSE, 0);

												if (sPathCost > 0 && (sClosestNewSpot == NOWHERE || sPathCost < sClosestNewPathCost))
												{
													sClosestNewSpot = sTempSpot2;
													sClosestNewPathCost = sPathCost;
												}
											}
										}								
									}									
								}

								// move soldier back to his original spot
								pSoldier->sGridNo = sOriginalSpot;

								// if closest opponent is not reachable currently but reachable from new spot or if new path is shorter than current
								if (!TileIsOutOfBounds(sClosestNewSpot) && 
									(sClosestOpponentPathCost == 0 || sClosestOpponentPathCost - (sClosestPathCost + sClosestNewPathCost) > APBPConstants[AP_MAXIMUM]))
								{
									ubSpotDir = AIDirection(pSoldier->sGridNo, sSpot);
									iValue = max(iValue, sObstaclePercent);
								}
							}*/

							// --------------------------------------------------
							// check adjacent tiles
							for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
							{
								sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

								if (sTempSpot != sSpot && (FindStruct(sTempSpot, bLevel, BLUEFLAG_GRAPHIC) || FindConcertina(sTempSpot) || IsCuttableWireFenceAtGridNo(sTempSpot)))
								{
									ubCount++;
								}
							}

							if(ubCount >= 2)
							{
								ubSpotDir = AIDirection(pSoldier->sGridNo, sSpot);

								iValue = max(iValue, sObstaclePercent * min(100, 10 * CountKnownEnemiesInDirection(pSoldier, ubSpotDir, sMaxEnemyDistance, FALSE) +	
									5 * CountKnownEnemiesInDirection(pSoldier, gOneCDirection[ubSpotDir], sMaxEnemyDistance, FALSE) + 
									5 * CountKnownEnemiesInDirection(pSoldier, gOneCCDirection[ubSpotDir], sMaxEnemyDistance, FALSE)) / 100);

								//ScreenMsg(FONT_MCOLOR_LTBLUE, MSG_INTERFACE, L"spot %d count %d value %d", sSpot, ubCount, iValue);
							}							
							/*ubSpotDir = AIDirection(pSoldier->sGridNo, sSpot);
							iValue = max(iValue, sObstaclePercent * (10 * (CountKnownEnemiesInDirection(pSoldier, ubSpotDir, sMaxEnemyDistance, FALSE) > 0) +
								5 * (CountKnownEnemiesInDirection(pSoldier, gOneCDirection[ubSpotDir], sMaxEnemyDistance, FALSE) > 0) +
								5 * (CountKnownEnemiesInDirection(pSoldier, gOneCCDirection[ubSpotDir], sMaxEnemyDistance, FALSE) > 0)) / 100);*/
							// --------------------------------------------------

							//DebugShot(pSoldier, String("spot %d, found obstacle and %d nearby! value %d opponents in dir %d", sSpot, ubCount, iValue, CountKnownEnemiesInDirection(pSoldier, ubSpotDir, sMaxEnemyDistance, TRUE)));
						}

						////DebugShot(pSoldier, String("check spot %d for explosive", sSpot));
						// check for explosive
						if (FindStructFlag(sSpot, bLevel, STRUCTURE_EXPLOSIVE) &&
							ubType == EXPLOSV_NORMAL &&
							PythSpacesAway(pSoldier->sGridNo, sSpot) > ubStructureExplosionRadius &&
							CountSeenEnemiesLastTurn(pSoldier) == 0 &&
							CountNearbyFriends(pSoldier, sSpot, ubStructureExplosionRadius) == 0 &&
							CountNearbyNeutrals(pSoldier, sSpot, ubStructureExplosionRadius) == 0)
						{
							//DebugShot(pSoldier, String("spot %d, found explosive! safe radius %d", sSpot, ubStructureExplosionRadius));

							if (CountPublicKnownEnemies(pSoldier, sSpot, TACTICAL_RANGE / 8) > 0)
							{
								iValue = max(iValue, 10 * min(5, CountPublicKnownEnemies(pSoldier, sSpot, TACTICAL_RANGE / 8)));
								//DebugShot(pSoldier, String("%d public enemies near explosive, value %d", CountPublicKnownEnemies(pSoldier, sSpot, DAY_VISION_RANGE / 8), iValue));
							}

							INT32	sTempSpot;
							UINT8	ubDirection;
							UINT8	ubCount = 0;

							// check if we can blow up the wall
							if (!InARoom(sSpot, NULL))
							{
								// check adjacent tiles for room
								for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
								{
									sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));

									if (sTempSpot != sSpot && InARoom(sTempSpot, &usRoom))
									{
										//DebugShot(pSoldier, String("found room %d, at spot %d", usRoom, sTempSpot));
										ubCount = CountKnownEnemiesInRoom(pSoldier, usRoom);
										if (ubCount > 0)
										{
											//DebugShot(pSoldier, String("found room nearby and %d enemies in a room!", ubCount));
											iValue = max(iValue, 10 * min(5, ubCount));
										}
									}
								}
							}
						}

						// throw flashbang/gas into room
						if ((ubType == EXPLOSV_FLASHBANG || ubType == EXPLOSV_BURNABLEGAS || ubType == EXPLOSV_MUSTGAS || ubType == EXPLOSV_TEARGAS) &&
							PythSpacesAway(pSoldier->sGridNo, sSpot) > ubRadius &&
							CountSeenEnemiesLastTurn(pSoldier) == 0 &&
							CountNearbyFriends(pSoldier, sSpot, ubRadius) == 0 &&
							InARoom(sSpot, &usRoom) &&
							CountFriendsInRoom(pSoldier, usRoom) == 0)
						{
							UINT8 ubCount = CountKnownEnemiesInRoom(pSoldier, usRoom);
							if (ubCount > 0)
							{
								UINT16 usSumDistance = SumDistanceToKnownEnemiesInRoom(pSoldier, usRoom, sSpot);

								if (usSumDistance > 0 && usSumDistance <= ubCount * ubRadius)
								{
									//DebugShot(pSoldier, String("found room nearby and %d enemies in a room!", ubCount));
									iValue = max(iValue, 100 - 100 * usSumDistance / (ubCount * ubRadius));
								}
							}
						}

						// too close to soldier or any friend
						if (PythSpacesAway(pSoldier->sGridNo, sSpot) < ubMinDistance || CountNearbyFriends(pSoldier, sSpot, ubMinDistance) > 0 || CountNearbyNeutrals(pSoldier, sSpot, ubMinDistance))
						{
							iValue = iValue / 2;
						}

						// for regular explosives, prefer closest spot
						/*if (ubType == EXPLOSV_NORMAL)
						{
							iValue = iValue * iSearchRange / (iSearchRange + PythSpacesAway(pSoldier->sGridNo, sSpot));
						}*/

						// found possible throw spot
						if (iValue > iBestValue)
						{
							CheckTossAt(pSoldier, pBestThrow, sSpot, bLevel, NOBODY);

							/*if (ubType == EXPLOSV_NORMAL || ubType == EXPLOSV_BURNABLEGAS)
							{
								// check adjacent tiles
								if (!pBestThrow->ubPossible)
								{
									INT32 sTempSpot;
									for (UINT8 ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
									{
										sTempSpot = NewGridNo(sSpot, DirectionInc(ubDirection));
										if (sTempSpot != sSpot)
										{
											CheckTossAt(pSoldier, pBestThrow, sTempSpot, bLevel, NOBODY);
											if (pBestThrow->ubPossible)
											{
												break;
											}
										}
									}
								}
							}*/

							// can throw, set best spot
							if (pBestThrow->ubPossible)
							{
								// take into account chance to really hit for all spots
								iValue = iValue * pBestThrow->ubChanceToReallyHit;
								// if still best spot to throw
								if (iValue > iBestValue)
								{
									sBestSpot = sSpot;
									iBestValue = iValue;
								}
								
								//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"throw at %d possible! value %d", sSpot, iValue);
								//DebugShot(pSoldier, String("found new best value %d spot %d", iBestValue, sBestSpot));
							}
							else
							{
								//ScreenMsg(FONT_LTRED, MSG_INTERFACE, L"throw at %d not possible! value %d", sSpot, iValue);
							}
						}
					}
				}

				// finally, prepare data for toss
				pBestThrow->ubPossible = FALSE;
				pBestThrow->ubChanceToReallyHit = 0;
				pBestThrow->iAttackValue = 0;
				pBestThrow->ubOpponent = NOBODY;

				if (!TileIsOutOfBounds(sBestSpot))
				{
					CheckTossAt(pSoldier, pBestThrow, sBestSpot, bLevel, NOBODY);
					pBestThrow->iAttackValue = iBestValue;
					//DebugShot(pSoldier, String("CheckTossAt result: possible %d cth %d attack value %d", pBestThrow->ubPossible, pBestThrow->ubChanceToReallyHit, pBestThrow->iAttackValue));
				}
				else
				{
					//DebugShot(pSoldier, String("could not find good throwing spot"));
				}
			}

			// if it was in his holster, swap it back into his holster for now
			if (pBestThrow->bWeaponIn != HANDPOS)
			{
				//DebugShot(pSoldier, String("rearrange pocket"));
				RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
			}
		}
		else
		{
			//DebugShot(pSoldier, String("not found grenade"));
		}
	}

	pSoldier->bWeaponMode = WM_NORMAL;
	//ScreenMsg(FONT_MCOLOR_LTGREEN, MSG_INTERFACE, L"Check Toss Grenade Special end");
}

void CheckTossCoverSmoke(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"[%d] Check Toss Cover Smoke", pSoldier->ubID);
	//DebugShot(pSoldier, String("\nCheckTossCoverSmoke"));

	INT16 ubMinAPcost;
	INT8 bGrenadeIn = NO_SLOT;	
	UINT16 usGrenade;
	UINT8 ubChance = 50;

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;
	pBestThrow->ubOpponent = NOBODY;

	if (!IS_MERC_BODY_TYPE(pSoldier))
	{
		//DebugShot(pSoldier, String("not merc body type"));
		return;
	}

	pSoldier->bWeaponMode = WM_NORMAL;
	bGrenadeIn = FindThrowableGrenade(pSoldier, EXPLOSV_SMOKE);
	
	if (bGrenadeIn != NO_SLOT)
	{
		pBestThrow->bWeaponIn = bGrenadeIn;
		//DebugShot(pSoldier, String("found smoke grenade in slot %d", pBestThrow->bWeaponIn));

		usGrenade = pSoldier->inv[bGrenadeIn].usItem;
		//ubSmokeTestRadius = Explosive[Item[usGrenade].ubClassIndex].ubRadius;

		// if it's in his holster, swap it into his hand temporarily
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}

		// get the minimum cost to attack with this tossable item
		ubMinAPcost = MinAPsToAttack(pSoldier, pSoldier->sGridNo, DONTADDTURNCOST, 0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we can afford the minimum AP cost to throw this tossable item
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			//DebugShot(pSoldier, String("find best spot for cover smoke"));

			INT16 sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
			INT32 iSearchRange = TACTICAL_RANGE;
			INT32 sMaxRange = CalcMaxTossRange(pSoldier, usGrenade, TRUE);
			INT32 sSpot;
			INT32 sBestSpot = NOWHERE;
			INT8 bLevel = pSoldier->pathing.bLevel;
			UINT8 ubMinDistance = 2;
			INT32 iValue = 0;
			INT32 iBestValue = 0;
			INT32 iSituationBefore, iSituationAfter;
			//INT32 iPercent;

			iSituationBefore = CalcSituationValue(pSoldier, NOWHERE, 0, TACTICAL_RANGE);

			// determine maximum horizontal limits
			sMaxLeft = min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
			sMaxRight = min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));

			// determine maximum vertical limits
			sMaxUp = min(iSearchRange, (pSoldier->sGridNo / MAXROW));
			sMaxDown = min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

			// check all tiles in range
			for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
			{
				for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
				{
					sSpot = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

					if (TileIsOutOfBounds(sSpot))
					{
						continue;
					}

					// basic checks
					if (InSmoke(sSpot, bLevel, TRUE) ||
						Water(sSpot, bLevel) ||
						NightLight() && !InLightAtNight(sSpot, bLevel))
					{
						continue;
					}

					// too close to soldier or any friend
					if (PythSpacesAway(pSoldier->sGridNo, sSpot) < ubMinDistance || CountNearbyFriends(pSoldier, sSpot, ubMinDistance) > 0)
					{
						continue;
					}

					if (PythSpacesAway(pSoldier->sGridNo, sSpot) > sMaxRange)
					{
						continue;
					}

					if (!CheckBetween(pSoldier, sSpot, bLevel, TACTICAL_RANGE, TRUE))
					{
						continue;
					}

					/*if(!Chance(ubChance))
					{
						continue;
					}*/

					iSituationAfter = CalcSituationValue(pSoldier, sSpot, bLevel, TACTICAL_RANGE);
					//iPercent = 100 * (iSituationAfter - iSituationBefore) / max(1, iSituationBefore);
					iValue = iSituationAfter - iSituationBefore;

					// found possible smoke value
					//if (iValue > 100 / (1 + SoldierDifficultyLevel(pSoldier)) && iValue > iBestValue)
					if (iValue > iBestValue)
					{
						pBestThrow->ubPossible = FALSE;
						CheckTossAt(pSoldier, pBestThrow, sSpot, bLevel, NOBODY);

						if (pBestThrow->ubPossible)
						{
							iBestValue = iValue;
							sBestSpot = sSpot;
						}						
					}
				}
			}

			// finally, prepare data for toss
			pBestThrow->ubPossible = FALSE;
			pBestThrow->ubChanceToReallyHit = 0;
			pBestThrow->iAttackValue = 0;
			pBestThrow->ubOpponent = NOBODY;

			if (!TileIsOutOfBounds(sBestSpot))
			{
				CheckTossAt(pSoldier, pBestThrow, sBestSpot, bLevel, NOBODY);
				pBestThrow->iAttackValue = iBestValue;
			}
			else
			{
				//DebugShot(pSoldier, String("could not find good throwing spot"));
			}
		}

		// if it was in his holster, swap it back into his holster for now
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}
	}
	else
	{
		//DebugShot(pSoldier, String("not found grenade"));
	}

	pSoldier->bWeaponMode = WM_NORMAL;
	//ScreenMsg(FONT_MCOLOR_LTGRAY, MSG_INTERFACE, L"Check Toss Cover Smoke end");
}

void CheckTossFriendSmoke(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow)
{
	//DebugShot( pSoldier, String("\nCheckTossFriendSmoke"));

	INT16 ubMinAPcost;
	INT8 bGrenadeIn = NO_SLOT;

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;
	pBestThrow->ubOpponent = NOBODY;

	if ( !IS_MERC_BODY_TYPE( pSoldier ) )
	{
		//DebugShot( pSoldier, String("not merc body type"));
		return;
	}	

	pSoldier->bWeaponMode = WM_NORMAL;

	bGrenadeIn = FindThrowableGrenade(pSoldier, EXPLOSV_SMOKE);

	if(bGrenadeIn != NO_SLOT)
	{
		pBestThrow->bWeaponIn = bGrenadeIn;
		//DebugShot(pSoldier, String("found smoke grenade in slot %d", pBestThrow->bWeaponIn));

		// if it's in his holster, swap it into his hand temporarily
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket(pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY);
		}

		// get the minimum cost to attack with this tossable item
		ubMinAPcost = MinAPsToAttack( pSoldier, pSoldier->sGridNo, DONTADDTURNCOST,0);
		//DebugShot(pSoldier, String("min APs to attack %d", ubMinAPcost));

		// if we can afford the minimum AP cost to throw this tossable item
		if (pSoldier->bActionPoints >= ubMinAPcost)
		{
			//DebugShot(pSoldier, String("find friend who needs cover"));

			INT32	sSpot = pSoldier->sGridNo;
			INT8	bLevel = pSoldier->pathing.bLevel;

			// check all friends
			SOLDIERTYPE * pFriend;
			INT32	sClosestFriendSpot = NOWHERE;
			INT8	bClosestFriendLevel = 0;
			UINT8	ubClosestFriendID = NOBODY;

			INT32	sFriendSpot;
			INT8	bFriendLevel;

			INT32	sClosestOpponent;

			// check adjacent spots
			UINT8	ubMovementCost;
			INT32	sTempGridNo;
			UINT8	ubDirection;

			// Run through each friendly.
			for ( UINT8 iCounter = gTacticalStatus.Team[ pSoldier->bTeam ].bFirstID ; iCounter <= gTacticalStatus.Team[ pSoldier->bTeam ].bLastID ; iCounter ++ )
			{
				pFriend = MercPtrs[ iCounter ];

				// check that friend is alive and needs cover
				if( pFriend &&
					pFriend != pSoldier && 
					pFriend->bActive && 
					pFriend->stats.bLife >= OKLIFE &&
					RangeChangeDesire(pFriend) <= 3 &&
					(pFriend->IsFlanking() && !TileIsOutOfBounds(pFriend->sTacticalSpot) && PythSpacesAway(pFriend->sGridNo, pFriend->sTacticalSpot) < (INT16)(MAX_VISION_RANGE) && LOS_Location(pFriend->sGridNo, pFriend->pathing.bLevel, pFriend->sTacticalSpot, pFriend->pathing.bLevel, NO_DISTANCE_LIMIT) || 
					pFriend->aiData.bUnderFire && (pFriend->IsCowering() || pFriend->TakenLargeHit() || pFriend->TakenHit() && pFriend->ShockLevelPercent() > 50 && pFriend->stats.bLife < pFriend->stats.bLifeMax * 3 / 4))
					)
				{
					sFriendSpot = pFriend->sGridNo;
					bFriendLevel = pFriend->pathing.bLevel;
					sClosestOpponent = ClosestKnownOpponent(pFriend, NULL, NULL);

					//DebugShot(pSoldier, String("check friend [%d] at %d %d closest opponent at %d", pFriend->ubID, sFriendSpot, bFriendLevel, sClosestOpponent));

					if (PythSpacesAway(sSpot, sFriendSpot) <= DAY_VISION_RANGE &&
						PythSpacesAway( sSpot, sFriendSpot ) > DAY_VISION_RANGE / 4 &&
						!InSmoke(sFriendSpot, bFriendLevel, TRUE) &&
						(!NightLight() || InLightAtNight(sFriendSpot, bFriendLevel)) &&
						!Water(sFriendSpot, bFriendLevel) &&
						!TileIsOutOfBounds(sClosestOpponent) &&
						PythSpacesAway(sFriendSpot, sClosestOpponent) > DAY_VISION_RANGE / 4 &&
						!SightCoverAtSpot(pFriend, sFriendSpot, ANIM_PRONE, TRUE) &&
						//!SightCoverAtSpot(pFriend, sFriendSpot, ANIM_STAND, FALSE) &&
						//!AnyCoverAtSpot(pFriend, sFriendSpot) &&
						!SightCoverNearSpot(pFriend, sFriendSpot, ANIM_PRONE) &&
						(TileIsOutOfBounds(sClosestFriendSpot) || PythSpacesAway(sSpot, sFriendSpot) < PythSpacesAway(sSpot, sClosestFriendSpot)) &&
						(pFriend->TakenLargeHit() || pFriend->ShockLevelPercent() > 50 && pFriend->stats.bLife < pFriend->stats.bLifeMax * 3 / 4))
					{
						// check that we can toss grenade
						CheckTossAt(pSoldier, pBestThrow, sFriendSpot, bFriendLevel, pFriend->ubID);

						if( pBestThrow->ubPossible )
						{
							sClosestFriendSpot = sFriendSpot;
							bClosestFriendLevel = bFriendLevel;
							ubClosestFriendID = pFriend->ubID;

							//DebugShot(pSoldier, String("throw possible, spot %d, level %d, friend %d", sClosestFriendSpot, bClosestFriendLevel, pBestThrow->ubOpponent));
						}
						else
						{
							// find adjacent spots
							for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
							{
								sTempGridNo = NewGridNo( sFriendSpot, DirectionInc( ubDirection ) );

								if( sTempGridNo != sFriendSpot )
								{
									ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ bFriendLevel ];

									if( !TileIsOutOfBounds(sTempGridNo) && 
										ubMovementCost < TRAVELCOST_BLOCKED &&
										!Water(sTempGridNo, bFriendLevel) )
									{
										// check this gridno
										CheckTossAt(pSoldier, pBestThrow, sTempGridNo, bFriendLevel, pFriend->ubID);

										if( pBestThrow->ubPossible )
										{
											sClosestFriendSpot = sTempGridNo;
											bClosestFriendLevel = bFriendLevel;
											ubClosestFriendID = pFriend->ubID;											

											//DebugShot(pSoldier, String("throw possible, spot %d, level %d, friend %d", sClosestFriendSpot, bClosestFriendLevel, pBestThrow->ubOpponent));
											break;
										}
									}
								}
							}
						}
					}					
				}
			}

			// finally, prepare data for toss
			pBestThrow->ubPossible = FALSE;
			pBestThrow->ubChanceToReallyHit = 0;
			pBestThrow->iAttackValue = 0;
			pBestThrow->ubOpponent = NOBODY;

			if( !TileIsOutOfBounds(sClosestFriendSpot) )
			{
				CheckTossAt(pSoldier, pBestThrow, sClosestFriendSpot, bClosestFriendLevel, ubClosestFriendID);
			}					
			else
			{
				//DebugShot(pSoldier, String("could not find good throwing spot"));
			}
		}

		// if it was in his holster, swap it back into his holster for now
		if (pBestThrow->bWeaponIn != HANDPOS)
		{
			//DebugShot(pSoldier, String("rearrange pocket"));
			RearrangePocket( pSoldier, HANDPOS, pBestThrow->bWeaponIn, TEMPORARILY );
		}
	}
	else
	{
		//DebugShot( pSoldier, String("not found grenade"));
	}

	pSoldier->bWeaponMode = WM_NORMAL;
}

// check if we can toss grenade at spot, and prepare attack data
// grenade should be in hand
void CheckTossAt(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow, INT32 sTargetSpot, INT8 bTargetLevel, UINT8 ubOpponentID)
{
	UINT16	usInHand, usGrenade;
	INT32	iTossRange;	

	INT32	sEndSpot = NOWHERE;
	INT8	bEndLevel = 0;

	UINT8	ubAPCost;
	UINT8	ubChanceToHit;
	UINT8	ubChanceToReallyHit;
	UINT8	ubChanceToGetThrough;
	INT32	iHitRate;
	INT32	iAttackValue;
	INT32	iTotalThreatValue = 100;
	UINT8	ubMaxPossibleAimTime = 0;
	UINT16	usTrueState = pSoldier->usAnimState;
	UINT8	ubStance = gAnimControl[pSoldier->usAnimState].ubEndHeight;

	usInHand = pSoldier->inv[HANDPOS].usItem;

	// initialize
	pBestThrow->ubPossible = FALSE;
	pBestThrow->ubChanceToReallyHit = 0;
	pBestThrow->iAttackValue = 0;	

	//DebugShot( pSoldier, String("check toss at %d %d", sTargetSpot, bTargetLevel));

	// sevenfm: safety check
	if (TileIsOutOfBounds(sTargetSpot) || !GridNoOnVisibleWorldTile(sTargetSpot))
	{
		return;
	}

	iTossRange = CalcMaxTossRange( pSoldier, usInHand, TRUE );
	//DebugShot( pSoldier, String("toss range %d", iTossRange));

	usGrenade = pSoldier->inv[ HANDPOS ].usItem;
	//DebugShot( pSoldier, String("grenade %d", usGrenade));

	ubChanceToGetThrough = 100 * CalculateLaunchItemChanceToGetThrough( pSoldier, &pSoldier->inv[HANDPOS], sTargetSpot, bTargetLevel, 0, &sEndSpot, TRUE, &bEndLevel, FALSE );
	//DebugShot( pSoldier, String("CTGT %d end spot %d end level %d", ubChanceToGetThrough, sEndSpot, bEndLevel));

	ubAPCost = (UINT8)MinAPsToThrow(pSoldier, sTargetSpot, TRUE) + CalcAPCostForAiming(pSoldier, sTargetSpot, ubMaxPossibleAimTime);
	//DebugShot( pSoldier, String("AP cost %d", ubAPCost));

	ubChanceToHit = (UINT8) CalcThrownChanceToHit( pSoldier, sTargetSpot, 0, AIM_SHOT_TORSO );
	//DebugShot( pSoldier, String("CTH %d", ubChanceToHit));

	ubChanceToReallyHit = (ubChanceToHit * ubChanceToGetThrough) / 100;
	//DebugShot( pSoldier, String("chance to really hit %d", ubChanceToReallyHit));

	iHitRate = (pSoldier->bActionPoints * ubChanceToHit) / ubAPCost;
	//DebugShot( pSoldier, String("hit rate %d", iHitRate));

	iAttackValue = (iHitRate * ubChanceToReallyHit * iTotalThreatValue) / 1000;
	//DebugShot( pSoldier, String("attack value %d", iAttackValue));

	// maybe try to stand up for better range
	if (ubChanceToReallyHit == 0 &&
		gAnimControl[pSoldier->usAnimState].ubEndHeight < ANIM_STAND &&
		pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, sTargetSpot), ANIM_STAND) &&
		pSoldier->bActionPoints >= ubAPCost + GetAPsToChangeStance(pSoldier, ANIM_STAND))
	{
		pSoldier->usAnimState = STANDING;

		iTossRange = CalcMaxTossRange(pSoldier, usInHand, TRUE);
		//DebugShot( pSoldier, String("toss range %d", iTossRange));

		usGrenade = pSoldier->inv[HANDPOS].usItem;
		//DebugShot( pSoldier, String("grenade %d", usGrenade));

		ubChanceToGetThrough = 100 * CalculateLaunchItemChanceToGetThrough(pSoldier, &pSoldier->inv[HANDPOS], sTargetSpot, bTargetLevel, 0, &sEndSpot, TRUE, &bEndLevel, FALSE);
		//DebugShot( pSoldier, String("CTGT %d end spot %d end level %d", ubChanceToGetThrough, sEndSpot, bEndLevel));

		ubAPCost = (UINT8)MinAPsToThrow(pSoldier, sTargetSpot, TRUE) + CalcAPCostForAiming(pSoldier, sTargetSpot, ubMaxPossibleAimTime);
		//DebugShot( pSoldier, String("AP cost %d", ubAPCost));

		ubChanceToHit = (UINT8)CalcThrownChanceToHit(pSoldier, sTargetSpot, 0, AIM_SHOT_TORSO);
		//DebugShot( pSoldier, String("CTH %d", ubChanceToHit));

		ubChanceToReallyHit = (ubChanceToHit * ubChanceToGetThrough) / 100;
		//DebugShot( pSoldier, String("chance to really hit %d", ubChanceToReallyHit));

		iHitRate = (pSoldier->bActionPoints * ubChanceToHit) / ubAPCost;
		//DebugShot( pSoldier, String("hit rate %d", iHitRate));

		iAttackValue = (iHitRate * ubChanceToReallyHit * iTotalThreatValue) / 1000;
		//DebugShot( pSoldier, String("attack value %d", iAttackValue));

		pSoldier->usAnimState = usTrueState;

		ubStance = ANIM_STAND;
		ubAPCost += GetAPsToChangeStance(pSoldier, ANIM_STAND);
	}

	if ( ubChanceToReallyHit > 0 )
	{
		// OOOF!	That was a lot of work!	But we've got a new best target!
		pBestThrow->ubPossible			= TRUE;
		pBestThrow->ubOpponent			= ubOpponentID;
		pBestThrow->ubAimTime			= ubMaxPossibleAimTime;
		pBestThrow->ubChanceToReallyHit = ubChanceToReallyHit;
		pBestThrow->sTarget				= sTargetSpot;
		pBestThrow->iAttackValue		= iAttackValue;
		pBestThrow->ubAPCost			= ubAPCost;
		pBestThrow->bTargetLevel		= bTargetLevel;
		pBestThrow->ubStance			= ubStance;

		//DebugShot( pSoldier, String("throw possible: opponent %d, aim time %d, CTH %d, Target %d, Level %d, AP cost %d", pBestThrow->ubOpponent, pBestThrow->ubAimTime, pBestThrow->ubChanceToReallyHit, pBestThrow->sTarget, pBestThrow->bTargetLevel, pBestThrow->ubAPCost));
	}
	else
	{
		//DebugShot(pSoldier, String("throw not possible"));
	}
}

INT32 FindTossSpotInDirection(INT32 sSpot, INT8 bLevel, INT32 sTargetSpot, BOOLEAN fCheckAdjacentDirections, BOOLEAN fCheckFarther)
{
	// find adjacent spot
	UINT8	ubMovementCost;
	INT32	sTempGridNo, sOldSpot;
	UINT8	ubDirection;

	// safety check
	if( TileIsOutOfBounds(sSpot) || TileIsOutOfBounds(sTargetSpot) )
	{
		return NOWHERE;
	}

	// check direction
	ubDirection = AIDirection(sSpot, sTargetSpot);

	sTempGridNo = NewGridNo( sSpot, DirectionInc( ubDirection ) );

	if( sTempGridNo!= sSpot )
	{
		ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ bLevel ];

		if( ubMovementCost < TRAVELCOST_BLOCKED && 
			!Water(sTempGridNo, bLevel))
		{
			return sTempGridNo;
		}

		if (fCheckFarther)
		{
			sOldSpot = sTempGridNo;
			sTempGridNo = NewGridNo(sOldSpot, DirectionInc(ubDirection));

			if(sTempGridNo != sOldSpot)
			{
				ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

				if (ubMovementCost < TRAVELCOST_BLOCKED &&
					!Water(sTempGridNo, bLevel))
				{
					return sTempGridNo;
				}
			}

			// check C direction
			ubDirection = gOneCDirection[AIDirection(sOldSpot, sTargetSpot)];
			sTempGridNo = NewGridNo(sOldSpot, DirectionInc(ubDirection));

			if (sTempGridNo != sOldSpot)
			{
				ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

				if (ubMovementCost < TRAVELCOST_BLOCKED && 
					!Water(sTempGridNo, bLevel))
				{
					return sTempGridNo;
				}
			}

			// check CC direction
			ubDirection = gOneCCDirection[AIDirection(sOldSpot, sTargetSpot)];
			sTempGridNo = NewGridNo(sOldSpot, DirectionInc(ubDirection));

			if (sTempGridNo != sOldSpot)
			{
				ubMovementCost = gubWorldMovementCosts[sTempGridNo][ubDirection][bLevel];

				if (ubMovementCost < TRAVELCOST_BLOCKED && 
					!Water(sTempGridNo, bLevel))
				{
					return sTempGridNo;
				}
			}
		}
	}	
	
	if( fCheckAdjacentDirections )
	{
		// check C direction
		ubDirection = gOneCDirection[AIDirection(sSpot, sTargetSpot)];
		sTempGridNo = NewGridNo( sSpot, DirectionInc( ubDirection ) );

		if( sTempGridNo != sSpot )
		{
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ bLevel ];

			if (ubMovementCost < TRAVELCOST_BLOCKED &&
				!Water(sTempGridNo, bLevel))
			{
				return sTempGridNo;
			}
		}		

		// check CC direction
		ubDirection = gOneCCDirection[AIDirection(sSpot, sTargetSpot)];
		sTempGridNo = NewGridNo( sSpot, DirectionInc( ubDirection ) );

		if( sTempGridNo != sSpot )
		{
			ubMovementCost = gubWorldMovementCosts[ sTempGridNo ][ ubDirection ][ bLevel ];

			if (ubMovementCost < TRAVELCOST_BLOCKED &&
				!Water(sTempGridNo, bLevel))
			{
				return sTempGridNo;
			}
		}
	}
	
	return NOWHERE;
}