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

// sevenfm
extern UINT16 PickSoldierReadyAnimation( SOLDIERTYPE *pSoldier, BOOLEAN fEndReady, BOOLEAN fHipStance );
extern SECTOR_EXT_DATA	SectorExternalData[256][4];

BOOLEAN DecideCuttingFenceFlank(SOLDIERTYPE *pSoldier)
{
	INT8 bWirecutterSlot = FindWirecutters(pSoldier);

	if (bWirecutterSlot != NO_SLOT &&
		pSoldier->pathing.bLevel == 0 &&
		!TileIsOutOfBounds(pSoldier->sTacticalSpot) &&
		pSoldier->aiData.bOrders == SEEKENEMY &&
		pSoldier->bActionPoints >= GetAPsToCutFence(pSoldier) + GetAPsToLook(pSoldier) &&
		FindFenceAroundSpot(pSoldier->sGridNo))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found wire cutter, check if we can find fence to cut"));

		UINT8 ubDir = AIDirection(pSoldier->sGridNo, pSoldier->sTacticalSpot);
		UINT8 ubDesiredDir;
		UINT8 ubCheckDir;
		INT32 sNewSpot;
		INT32 sNextSpot;

		// determine desired direction
		if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT)
			ubDesiredDir = gTwoCCDirection[ubDir];
		else
			ubDesiredDir = gTwoCDirection[ubDir];

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
				IsLocationSittableExcludingPeople(sNextSpot, pSoldier->pathing.bLevel))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("found cuttable fence at %d", sNewSpot));
				if (pSoldier->ubDirection == ubCheckDir)
				{
					RearrangePocket(pSoldier, HANDPOS, bWirecutterSlot, FOREVER);
					pSoldier->aiData.usActionData = sNewSpot;
					pSoldier->aiData.bAction = AI_ACTION_HANDLE_ITEM;
					return TRUE;
				}
				else if (pSoldier->InternalIsValidStance(ubCheckDir, gAnimControl[pSoldier->usAnimState].ubEndHeight))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("turn before cutting fence"));
					RearrangePocket(pSoldier, HANDPOS, bWirecutterSlot, FOREVER);
					pSoldier->aiData.usActionData = ubCheckDir;
					pSoldier->aiData.bNextAction = AI_ACTION_HANDLE_ITEM;
					pSoldier->aiData.usNextActionData = sNewSpot;
					pSoldier->aiData.bAction = AI_ACTION_CHANGE_FACING;
					return TRUE;
				}
			}
		}		
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideBlowUpObstacle(SOLDIERTYPE *pSoldier, INT32 sClosestOpponent)
{
	CHECKF(pSoldier);
	INT8 bTNTSlot = FindTNT(pSoldier);
	
	if (bTNTSlot != NO_SLOT &&
		SoldierAI(pSoldier) &&
		pSoldier->bTeam == ENEMY_TEAM &&
		IsActionAffordable(pSoldier, AI_ACTION_PLANT_BOMB) &&
		pSoldier->pathing.bLevel == 0 &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		(Chance(SoldierDifficultyLevel(pSoldier) * 10) ||
		InARoom(sClosestOpponent) &&
		CountFriendsInRoom(pSoldier, RoomNo(sClosestOpponent)) == 0 &&
		!SameRoom(sClosestOpponent, pSoldier->sGridNo) &&
		(CountCorpsesInRoom(pSoldier, RoomNo(sClosestOpponent), 0) > 0 || TeamHighPercentKilled(pSoldier->bTeam) || EstimatePathCostToLocation(pSoldier, sClosestOpponent, pSoldier->pathing.bLevel, FALSE, NULL, NULL) == 0)))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found TNT in slot %d, check to plant bomb", bTNTSlot));

		INT8 bLevel = pSoldier->pathing.bLevel;
		UINT8 ubDesiredDir = AIDirection(pSoldier->sGridNo, sClosestOpponent);
		UINT8 ubCheckDir;
		INT32 sNewSpot;
		INT32 sNextSpot;
		INT32 sCheckSpot;
		INT32 sPathCost, sNewPathCost;
		INT32 sOriginalGridNo;
		UINT16 usRoom;

		for (ubCheckDir = 0; ubCheckDir < NUM_WORLD_DIRECTIONS; ubCheckDir++)
		{
			sNewSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubCheckDir));
			sNextSpot = NewGridNo(sNewSpot, DirectionInc(ubCheckDir));

			if (sNewSpot == pSoldier->sGridNo ||
				sNextSpot == sNewSpot ||
				gpWorldLevelData[sNewSpot].ubHeight != gpWorldLevelData[pSoldier->sGridNo].ubHeight ||
				gpWorldLevelData[sNextSpot].ubHeight != gpWorldLevelData[pSoldier->sGridNo].ubHeight ||
				//ubCheckDir != ubDesiredDir && ubCheckDir != gOneCDirection[ubDesiredDir] && ubCheckDir != gOneCCDirection[ubDesiredDir] ||
				gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] < TRAVELCOST_BLOCKED)
			{
				continue;
			}

			sCheckSpot = NOWHERE;

			if ((IsCuttableWireFenceAtGridNo(sNewSpot) && !IsCutWireFenceAtGridNo(sNewSpot) || gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_OBSTACLE) &&
				IsLocationSittable(sNextSpot, bLevel) && NewOKDestination(pSoldier, sNextSpot, TRUE, bLevel) && !Water(sNextSpot, bLevel))
			{
				// found fence, jump 2 tiles
				sCheckSpot = sNextSpot;
			}
			else if ((gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_WALL || gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_DOOR) &&
				IsLocationSittable(sNewSpot, bLevel) && NewOKDestination(pSoldier, sNewSpot, TRUE, bLevel) && !Water(sNewSpot, bLevel))
			{
				// found wall, jump 1 tile
				sCheckSpot = sNewSpot;
			}

			if (!TileIsOutOfBounds(sCheckSpot))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("check if jumping to %d improves path cost", sCheckSpot));

				// check if removing obstacle improves situation
				sPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
				sOriginalGridNo = pSoldier->sGridNo;
				pSoldier->sGridNo = sCheckSpot;
				sNewPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
				pSoldier->sGridNo = sOriginalGridNo;

				if (sNewPathCost > 0 &&
					(sPathCost == 0 ||
					sPathCost > sNewPathCost && sPathCost - sNewPathCost > APBPConstants[AP_MAXIMUM] ||
					InARoom(sCheckSpot, &usRoom) &&
					!SameRoom(sCheckSpot, pSoldier->sGridNo) &&
					CountFriendsInRoom(pSoldier, usRoom) == 0 &&
					CountKnownEnemiesInRoom(pSoldier, usRoom) > 0 &&
					(CountCorpsesInRoom(pSoldier, usRoom, 0) > 0 || TeamHighPercentKilled(pSoldier->bTeam))))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("blowing up fence/wall/obstacle improves path cost, plant bomb"));

					RearrangePocket(pSoldier, HANDPOS, bTNTSlot, FOREVER);
					pSoldier->aiData.usActionData = 0;
					pSoldier->aiData.bAction = AI_ACTION_PLANT_BOMB;
					return TRUE;
				}
			}
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideBlowUpObstacleFlank(SOLDIERTYPE *pSoldier)
{
	CHECKF(pSoldier);

	INT8 bTNTSlot = FindTNT(pSoldier);	

	if (bTNTSlot != NO_SLOT &&
		pSoldier->pathing.bLevel == 0 &&
		!TileIsOutOfBounds(pSoldier->sTacticalSpot) &&
		pSoldier->aiData.bOrders == SEEKENEMY &&
		IsActionAffordable(pSoldier, AI_ACTION_PLANT_BOMB))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("found TNT, check if we can blow up wall/fence/obstacle"));

		UINT8 ubDir = AIDirection(pSoldier->sGridNo, pSoldier->sTacticalSpot);
		UINT8 ubDesiredDir;
		UINT8 ubCheckDir;
		INT32 sNewSpot;
		INT32 sNextSpot;
		INT8 bLevel = pSoldier->pathing.bLevel;
		INT32 sPathCost;
		INT32 sJumpSpot;

		// determine desired direction
		if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT)
			ubDesiredDir = gTwoCCDirection[ubDir];
		else
			ubDesiredDir = gTwoCDirection[ubDir];

		for (ubCheckDir = 0; ubCheckDir < NUM_WORLD_DIRECTIONS; ubCheckDir++)
		{
			// check desired dir
			sNewSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubCheckDir));
			sNextSpot = NewGridNo(sNewSpot, DirectionInc(ubCheckDir));

			if (sNewSpot == pSoldier->sGridNo ||
				sNextSpot == sNewSpot ||
				Water(sNewSpot, pSoldier->pathing.bLevel) ||
				Water(sNextSpot, pSoldier->pathing.bLevel) ||
				gpWorldLevelData[sNewSpot].ubHeight != gpWorldLevelData[pSoldier->sGridNo].ubHeight ||
				gpWorldLevelData[sNextSpot].ubHeight != gpWorldLevelData[pSoldier->sGridNo].ubHeight ||
				ubCheckDir != ubDesiredDir && ubCheckDir != gOneCDirection[ubDesiredDir] && ubCheckDir != gOneCCDirection[ubDesiredDir] ||
				gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] < TRAVELCOST_BLOCKED)
			{
				continue;
			}

			sJumpSpot = NOWHERE;

			// check obstacle in flanking direction
			if (IsCuttableWireFenceAtGridNo(sNewSpot) && !IsCutWireFenceAtGridNo(sNewSpot) || gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_OBSTACLE)
				sJumpSpot = sNextSpot;
			else if ((gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_WALL || gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_DOOR)) // && (pSoldier->flags.ubTacticalFlags & TACTICAL_FLAG_EXTRA_FLANKING))
				sJumpSpot = sNewSpot;

			if (TileIsOutOfBounds(sJumpSpot) || !IsLocationSittableExcludingPeople(sJumpSpot, pSoldier->pathing.bLevel))
			{
				continue;
			}

			sPathCost = EstimatePlotPath(pSoldier, sJumpSpot, FALSE, FALSE, FALSE, RUNNING, FALSE, FALSE, 0);

			// check if blowing up obstacle improves path cost
			if (sPathCost > 0 && sPathCost < 2 * APBPConstants[AP_MAXIMUM])
			{
				continue;
			}

			DebugAI(AI_MSG_INFO, pSoldier, String("found fence/wall/obstacle in flanking direction, plant bomb"));

			RearrangePocket(pSoldier, HANDPOS, bTNTSlot, FOREVER);
			pSoldier->aiData.usActionData = 0;
			pSoldier->aiData.bAction = AI_ACTION_PLANT_BOMB;
			return TRUE;
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideJumpWindow(SOLDIERTYPE *pSoldier, INT32 sClosestOpponent)
{
	CHECKF(pSoldier);

	if (SoldierAI(pSoldier) &&
		gGameExternalOptions.fCanJumpThroughWindows &&
		!pSoldier->bBlindedCounter &&
		!TileIsOutOfBounds(sClosestOpponent) &&
		pSoldier->pathing.bLevel == 0 &&
		pSoldier->AnimEndHeight() >= ANIM_CROUCH &&
		IsActionAffordable(pSoldier, AI_ACTION_JUMP_WINDOW))
	{
		DebugAI(AI_MSG_INFO, pSoldier, String("check if we can jump through window"));

		UINT8 ubCheckDir;
		INT32 sNewSpot;
		INT32 sPathCost, sNewPathCost;
		INT32 sOriginalGridNo;
		INT8 bLevel = pSoldier->pathing.bLevel;

		for (ubCheckDir = 0; ubCheckDir < NUM_WORLD_DIRECTIONS; ubCheckDir++)
		{
			// cannot jump diagonally
			if (ubCheckDir % 2 != 0)
			{
				continue;
			}

			sNewSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubCheckDir));

			if (sNewSpot != pSoldier->sGridNo &&
				//gubWorldMovementCosts[sNewSpot][ubCheckDir][bLevel] == TRAVELCOST_WALL &&
				CheckWindow(pSoldier->sGridNo, ubCheckDir, gGameExternalOptions.fCanJumpThroughClosedWindows) &&
				!Water(sNewSpot, pSoldier->pathing.bLevel) &&
				IsLocationSittable(sNewSpot, pSoldier->pathing.bLevel) &&
				NewOKDestination(pSoldier, sNewSpot, TRUE, pSoldier->pathing.bLevel))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("found jumpable window at %d", sNewSpot));

				// check if jumping improves situation
				sPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
				sOriginalGridNo = pSoldier->sGridNo;
				pSoldier->sGridNo = sNewSpot;
				sNewPathCost = EstimatePlotPath(pSoldier, sClosestOpponent, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
				pSoldier->sGridNo = sOriginalGridNo;

				if (sNewPathCost > 0 && (sPathCost == 0 || sPathCost > sNewPathCost && sPathCost - sNewPathCost > APBPConstants[AP_MAXIMUM] || SameRoom(sClosestOpponent, sNewSpot)))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("jumping improves path cost"));

					if (gfTurnBasedAI && pSoldier->bActionPoints < pSoldier->bInitialActionPoints)
					{
						if (pSoldier->ubDirection != ubCheckDir &&
							pSoldier->InternalIsValidStance(ubCheckDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
							pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("turn to window, end turn before jumping"));
							pSoldier->aiData.usActionData = ubCheckDir;
							pSoldier->aiData.bNextAction = AI_ACTION_END_TURN;
							pSoldier->aiData.usNextActionData = 0;
							pSoldier->aiData.bAction = AI_ACTION_CHANGE_FACING;
							return TRUE;
						}
						else
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("already facing window, end turn before jumping"));
							pSoldier->aiData.usActionData = 0;
							pSoldier->aiData.bAction = AI_ACTION_END_TURN;
							return TRUE;
						}
					}

					if (pSoldier->ubDirection == ubCheckDir)
					{
						pSoldier->aiData.usActionData = 0;
						pSoldier->aiData.bAction = AI_ACTION_JUMP_WINDOW;
						return TRUE;
					}
					else if (pSoldier->InternalIsValidStance(ubCheckDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
						pSoldier->bActionPoints >= GetAPsToJumpThroughWindows(pSoldier, FALSE) + GetAPsToLook(pSoldier))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("turn before jumping"));
						pSoldier->aiData.usActionData = ubCheckDir;
						pSoldier->aiData.bNextAction = AI_ACTION_JUMP_WINDOW;
						pSoldier->aiData.usNextActionData = 0;
						pSoldier->aiData.bAction = AI_ACTION_CHANGE_FACING;
						return TRUE;
					}
				}
			}
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideMedic(SOLDIERTYPE *pSoldier)
{
	// limit medic AI to start of the turn to prevent high CPU load
	// no medic AI when engaged in fight
	if (!pSoldier->aiData.bUnderFire)
	{
		// if we are a doctor with medical gear, we might be able to help a wounded ally
		if (pSoldier->CanMedicAI() || pSoldier->CanFirstAidAI())
		{
			UINT8 ubPerson = GetClosestWoundedSoldierID(pSoldier, TACTICAL_RANGE * 2, pSoldier->bTeam);
			DebugAI(AI_MSG_INFO, pSoldier, String("closest wounded id %d", ubPerson));

			// are we ourselves the patient?
			if (ubPerson == pSoldier->ubID)
			{
				if (pSoldier->CheckInitialAP() && IsActionAffordable(pSoldier, AI_ACTION_DOCTOR_SELF))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("first aid: heal ourselves"));
					if (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_CROUCH)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("first aid: crouched, start healing ourselves"));
						pSoldier->aiData.bAction = AI_ACTION_DOCTOR_SELF;
						return TRUE;
					}
					// if not already crouched, crouch first
					else if (gAnimControl[pSoldier->usAnimState].ubHeight != ANIM_CROUCH &&
						IsValidStance(pSoldier, ANIM_CROUCH) &&
						GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
					{
						pSoldier->aiData.usActionData = ANIM_CROUCH;
						pSoldier->aiData.bNextAction = AI_ACTION_DOCTOR_SELF;
						pSoldier->aiData.usNextActionData = 0;

						DebugAI(AI_MSG_INFO, pSoldier, String("first aid: change stance and start healing ourselves"));
						pSoldier->aiData.bAction = AI_ACTION_CHANGE_STANCE;
						return TRUE;
					}
				}				
			}
			else if (ubPerson != NOBODY && MercPtrs[ubPerson] && AIRoleMedic(pSoldier))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("medic: found wounded friend at %d, distance %d", MercPtrs[ubPerson]->sGridNo, PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo)));
				if (PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) < 2 && IsActionAffordable(pSoldier, AI_ACTION_DOCTOR))
				{
					// see if we are facing this person
					UINT8 ubDesiredMercDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);

					DebugAI(AI_MSG_INFO, pSoldier, String("medic: we are next to friend, our dir %d, friend dir %d", pSoldier->ubDirection, ubDesiredMercDir));

					// if not already facing in that direction
					if (pSoldier->ubDirection != ubDesiredMercDir)
					{
						if (pSoldier->InternalIsValidStance(ubDesiredMercDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
							GetAPsToLook(pSoldier) <= pSoldier->bActionPoints)
						{
							pSoldier->aiData.usActionData = ubDesiredMercDir;
							if (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_CROUCH)
							{
								pSoldier->aiData.bNextAction = AI_ACTION_DOCTOR;
								pSoldier->aiData.usNextActionData = 0;
							}							
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: turn to wounded friend and start healing"));
							pSoldier->aiData.bAction = AI_ACTION_CHANGE_FACING;
							return TRUE;
						}
					}
					// looking in the right direction
					else
					{
						// if not already crouched, crouch first
						if (gAnimControl[pSoldier->usAnimState].ubHeight != ANIM_CROUCH)
						{
							if (IsValidStance(pSoldier, ANIM_CROUCH) &&
								GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
							{
								pSoldier->aiData.usActionData = ANIM_CROUCH;
								pSoldier->aiData.bNextAction = AI_ACTION_DOCTOR;
								pSoldier->aiData.usNextActionData = 0;
								DebugAI(AI_MSG_INFO, pSoldier, String("medic: change stance and start healing friend"));
								pSoldier->aiData.bAction = AI_ACTION_CHANGE_STANCE;
								return TRUE;
							}
						}
						else
						{
							// already crouched, start healing
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: start healing"));
							pSoldier->aiData.bAction = AI_ACTION_DOCTOR;
							return TRUE;
						}
					}
				}
				else
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: find path to wounded friend"));
					INT32 iPathCost = EstimatePlotPath(pSoldier, MercPtrs[ubPerson]->sGridNo, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: path cost %d", iPathCost));

					if (iPathCost != 0 && iPathCost <= 2 * APBPConstants[AP_MAXIMUM])
					{
						pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_FRIEND, 0);
						DebugAI(AI_MSG_INFO, pSoldier, String("medic: found move spot %d", pSoldier->aiData.usActionData));

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: move to wounded friend"));
							pSoldier->aiData.bAction = AI_ACTION_SEEK_FRIEND;
							return TRUE;
						}
					}
				}
			}
		}
		// if we are not a medic, but are wounded, seek a medic
		else if (pSoldier->iHealableInjury >= MIN_AI_HEALABLE_INJURY || pSoldier->iHealableInjury >= MIN_HEALABLE_INJURY && pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD)
		{
			UINT8 ubPerson = GetClosestMedicSoldierID(pSoldier, TACTICAL_RANGE * 2, pSoldier->bTeam);
			DebugAI(AI_MSG_INFO, pSoldier, String("found closest medic id %d", ubPerson));

			if (ubPerson != NOBODY)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("medic at %d, distance %d", MercPtrs[ubPerson]->sGridNo, PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo)));

				if (PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) > 1)
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: find path to wounded friend"));
					INT32 iPathCost = EstimatePlotPath(pSoldier, MercPtrs[ubPerson]->sGridNo, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: path cost %d", iPathCost));

					if (iPathCost != 0 && iPathCost <= 2 * APBPConstants[AP_MAXIMUM])
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("move to medic"));
						pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_FRIEND, 0);
						DebugAI(AI_MSG_INFO, pSoldier, String("found move spot %d", pSoldier->aiData.usActionData));

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: move to closest medic"));
							pSoldier->aiData.bAction = AI_ACTION_SEEK_FRIEND;
							return TRUE;
						}
					}
					else
					{
						// cannot reach medic and has no first aid kit
						// try to create makeshift first aid kit
						UINT16 usItem = GetFirstAidKit(FIRSTAIDKIT);
						if (usItem > 0)
						{
							OBJECTTYPE newobj;
							CreateItem(usItem, 20 + Random(20), &newobj);
							newobj.fFlags |= OBJECT_UNDROPPABLE;

							// try to place item in inventory
							if (AutoPlaceObject(pSoldier, &newobj, FALSE))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: created makeshift first aid kit"));
								pSoldier->aiData.bAction = AI_ACTION_NONE;
								return TRUE;
							}
						}
					}
				}
			}
			else
			{
				// cannot find medic and has no first aid kit
				// try to create makeshift first aid kit
				UINT16 usItem = GetFirstAidKit(FIRSTAIDKIT);
				if (usItem > 0)
				{
					OBJECTTYPE newobj;
					CreateItem(usItem, 20 + Random(20), &newobj);
					newobj.fFlags |= OBJECT_UNDROPPABLE;

					// try to place item in inventory
					if (AutoPlaceObject(pSoldier, &newobj, FALSE))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: created makeshift first aid kit"));
						pSoldier->aiData.bAction = AI_ACTION_NONE;
						return TRUE;
					}
				}
			}
		}
	}	

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideMedicCombat(SOLDIERTYPE *pSoldier)
{
	// sevenfm: limit healing to safe spots
	if (!pSoldier->aiData.bUnderFire)
	{
		// if we are a doctor with medical gear, we might be able to help a wounded ally
		if (pSoldier->CanMedicAI() || pSoldier->CanFirstAidAI())
		{
			UINT8 ubPerson = GetClosestWoundedSoldierID(pSoldier, TACTICAL_RANGE, pSoldier->bTeam);
			DebugAI(AI_MSG_INFO, pSoldier, String("found wounded friend id %d", ubPerson));

			// are we ourselves the patient?
			// sevenfm: allow healing only if spot is not dangerous
			if (ubPerson == pSoldier->ubID)
			{
				if (pSoldier->CheckInitialAP() && SafeMedicSpot(pSoldier, pSoldier->sGridNo) && IsActionAffordable(pSoldier, AI_ACTION_DOCTOR_SELF))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("first aid: heal ourselves"));
					if (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_CROUCH)
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("first aid: crouched, start healing ourselves"));
						pSoldier->aiData.bAction = AI_ACTION_DOCTOR_SELF;
						return TRUE;
					}					
					else if (gAnimControl[pSoldier->usAnimState].ubHeight != ANIM_CROUCH &&
						IsValidStance(pSoldier, ANIM_CROUCH) &&
						GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
					{
						// if not already crouched, crouch first
						pSoldier->aiData.usActionData = ANIM_CROUCH;
						pSoldier->aiData.bNextAction = AI_ACTION_DOCTOR_SELF;
						pSoldier->aiData.usNextActionData = 0;

						DebugAI(AI_MSG_INFO, pSoldier, String("first aid: change stance and start healing ourselves"));
						pSoldier->aiData.bAction = AI_ACTION_CHANGE_STANCE;
						return TRUE;
					}
				}
			}
			else if (ubPerson != NOBODY && MercPtrs[ubPerson] && AIRoleMedic(pSoldier))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("medic: friend at %d, distance %d", MercPtrs[ubPerson]->sGridNo, PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo)));

				if (PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) < 2)
				{
					// maybe use smoke for cover
					if (pSoldier->CheckInitialAP() &&
						Chance(SoldierDifficultyLevel(pSoldier) * 20) &&
						(!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || !SightCoverAtSpot(pSoldier, MercPtrs[ubPerson]->sGridNo, ANIM_STAND, FALSE)))
					{
						ATTACKTYPE	BestThrow;

						DebugAI(AI_MSG_INFO, pSoldier, String("medic: check if soldier can cover himself with smoke"));
						CheckTossSelfSmoke(pSoldier, &BestThrow);
						if (BestThrow.ubPossible)
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: prepare throw at spot %d level %d aimtime %d", BestThrow.sTarget, BestThrow.bTargetLevel, BestThrow.ubAimTime));

							// if necessary, swap the usItem from holster into the hand position
							if (BestThrow.bWeaponIn != HANDPOS)
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("medic: rearrange pocket to use smoke grenade"));
								RearrangePocket(pSoldier, HANDPOS, BestThrow.bWeaponIn, FOREVER);
							}

							// stand up before throwing if needed
							if (gAnimControl[pSoldier->usAnimState].ubEndHeight < BestThrow.ubStance &&
								pSoldier->InternalIsValidStance(AIDirection(pSoldier->sGridNo, BestThrow.sTarget), BestThrow.ubStance))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("medic: stand up before throwing smoke grenade"));
								pSoldier->aiData.usActionData = BestThrow.ubStance;
								pSoldier->aiData.bNextAction = AI_ACTION_TOSS_PROJECTILE;
								pSoldier->aiData.usNextActionData = BestThrow.sTarget;
								pSoldier->aiData.bNextTargetLevel = BestThrow.bTargetLevel;
								pSoldier->aiData.bAimTime = BestThrow.ubAimTime;

								pSoldier->aiData.bAction = AI_ACTION_CHANGE_STANCE;
								return TRUE;
							}
							else
							{
								pSoldier->aiData.usActionData = BestThrow.sTarget;
								pSoldier->bTargetLevel = BestThrow.bTargetLevel;
								pSoldier->aiData.bAimTime = BestThrow.ubAimTime;
							}
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: throw smoke grenade"));
							pSoldier->aiData.bAction = AI_ACTION_TOSS_PROJECTILE;
							return TRUE;
						}
					}

					// maybe drag this person to a safe place
					/*if (pSoldier->pathing.bLevel == 0 &&
						!pSoldier->aiData.bUnderFire &&
						(MercPtrs[ubPerson]->stats.bLife < OKLIFE || MercPtrs[ubPerson]->IsUnconscious()) &&
						MercPtrs[ubPerson]->stats.bLife >= 5 &&
						!MercPtrs[ubPerson]->IsDragged() &&
						pSoldier->CanDragPerson(ubPerson) &&
						!pSoldier->IsDragging(false) &&
						pSoldier->bInitialActionPoints > APBPConstants[AP_MINIMUM] &&
						pSoldier->bBreath > 50 &&
						Chance(SoldierDifficultyLevel(pSoldier) * 20) &&
						SpotDangerLevel(pSoldier, pSoldier->sGridNo) < 2 &&
						!SafeMedicSpot(pSoldier, pSoldier->sGridNo) &&
						(!SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || !SightCoverAtSpot(pSoldier, MercPtrs[ubPerson]->sGridNo, ANIM_STAND, FALSE) || MercPtrs[ubPerson]->aiData.bUnderFire))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("medic: check if can drag person, find safe medic spot"));

						// fake dragging this person to correctly calculate APs needed to reach safe spot
						pSoldier->ubDragPersonID = ubPerson;
						INT32 sSafeMedicSpot = FindSafeMedicSpot(pSoldier);
						pSoldier->ubDragPersonID = NOBODY;

						if (!TileIsOutOfBounds(sSafeMedicSpot))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: found safe medic spot %d, start dragging", sSafeMedicSpot));
							pSoldier->usAISkillUse = SKILLS_DRAG;
							pSoldier->aiData.usActionData = MercPtrs[ubPerson]->sGridNo;

							pSoldier->aiData.bAction = AI_ACTION_USE_SKILL;
							return TRUE;
						}
					}*/

					if (IsActionAffordable(pSoldier, AI_ACTION_DOCTOR))
					{
						// see if we are facing this person
						UINT8 ubDesiredMercDir = AIDirection(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo);
						DebugAI(AI_MSG_INFO, pSoldier, String("medic: we are next to friend, our dir %d, friend dir %d", pSoldier->ubDirection, ubDesiredMercDir));

						// if not already facing in that direction
						if (pSoldier->ubDirection != ubDesiredMercDir)
						{
							if (pSoldier->InternalIsValidStance(ubDesiredMercDir, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
								GetAPsToLook(pSoldier) <= pSoldier->bActionPoints)
							{
								pSoldier->aiData.usActionData = ubDesiredMercDir;
								if (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_CROUCH)
								{
									pSoldier->aiData.bNextAction = AI_ACTION_DOCTOR;
									pSoldier->aiData.usNextActionData = 0;
								}
								DebugAI(AI_MSG_INFO, pSoldier, String("medic: turn to wounded friend"));
								pSoldier->aiData.bAction = AI_ACTION_CHANGE_FACING;
								return TRUE;
							}
						}
						else	// looking in the right direction
						{
							// if not already crouched, crouch first
							if (gAnimControl[pSoldier->usAnimState].ubHeight != ANIM_CROUCH)
							{
								if (IsValidStance(pSoldier, ANIM_CROUCH) &&
									GetAPsToChangeStance(pSoldier, ANIM_CROUCH) <= pSoldier->bActionPoints)
								{
									pSoldier->aiData.usActionData = ANIM_CROUCH;
									pSoldier->aiData.bNextAction = AI_ACTION_DOCTOR;
									pSoldier->aiData.usNextActionData = 0;
									DebugAI(AI_MSG_INFO, pSoldier, String("medic: change stance and start healing friend"));
									pSoldier->aiData.bAction = AI_ACTION_CHANGE_STANCE;
									return TRUE;
								}
							}
							else
							{
								// already crouched, start healing
								DebugAI(AI_MSG_INFO, pSoldier, String("medic: start healing"));
								pSoldier->aiData.bAction = AI_ACTION_DOCTOR;
								return TRUE;
							}
						}
					}					
				}
				else if (pSoldier->aiData.bAlertStatus < STATUS_BLACK && !GuySawEnemy(pSoldier))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: find path to wounded friend"));
					INT32 iPathCost = EstimatePlotPath(pSoldier, MercPtrs[ubPerson]->sGridNo, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: path cost %d", iPathCost));

					if(iPathCost != 0 && iPathCost <= APBPConstants[AP_MAXIMUM])
					{
						pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, 0, AI_ACTION_SEEK_FRIEND, 0);
						DebugAI(AI_MSG_INFO, pSoldier, String("medic: found move spot %d", pSoldier->aiData.usActionData));

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData) &&
							SafeMedicSpot(pSoldier, pSoldier->aiData.usActionData))
							//(AnyCoverAtSpot(pSoldier, pSoldier->aiData.usActionData) || SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_PRONE, FALSE)))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("medic: move to wounded friend"));
							pSoldier->aiData.bAction = AI_ACTION_SEEK_FRIEND;
							return TRUE;
						}
					}					
				}
			}
		}
		// if we are not a medic, but are wounded, seek a medic
		else if (pSoldier->iHealableInjury >= MIN_AI_HEALABLE_INJURY || pSoldier->iHealableInjury >= MIN_HEALABLE_INJURY && pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD)
		{
			DebugAI(AI_MSG_INFO, pSoldier, String("we are wounded, seek medic"));
			UINT8 ubPerson = GetClosestMedicSoldierID(pSoldier, TACTICAL_RANGE, pSoldier->bTeam);
			DebugAI(AI_MSG_INFO, pSoldier, String("found closest medic id %d", ubPerson));

			if (ubPerson != NOBODY)
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("medic at %d, distance %d", MercPtrs[ubPerson]->sGridNo, PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo)));

				if (PythSpacesAway(pSoldier->sGridNo, MercPtrs[ubPerson]->sGridNo) > 1 && 
					pSoldier->aiData.bAlertStatus < STATUS_BLACK &&	
					!GuySawEnemy(pSoldier))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: find path to medic"));
					INT32 iPathCost = EstimatePlotPath(pSoldier, MercPtrs[ubPerson]->sGridNo, FALSE, FALSE, FALSE, RUNNING, pSoldier->bStealthMode, FALSE, 0);
					DebugAI(AI_MSG_INFO, pSoldier, String("medic: path cost %d", iPathCost));

					if (iPathCost != 0 && iPathCost <= APBPConstants[AP_MAXIMUM])
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("search move spot"));
						pSoldier->aiData.usActionData = InternalGoAsFarAsPossibleTowards(pSoldier, MercPtrs[ubPerson]->sGridNo, APBPConstants[AP_MAXIMUM] / 5, AI_ACTION_SEEK_FRIEND, 0);
						DebugAI(AI_MSG_INFO, pSoldier, String("found move spot %d", pSoldier->aiData.usActionData));

						if (!TileIsOutOfBounds(pSoldier->aiData.usActionData))
						{
							DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: move to closest medic"));
							pSoldier->aiData.bAction = AI_ACTION_SEEK_FRIEND;
							return TRUE;
						}
					}
					else if (!DangerousSpot(pSoldier))
					{
						// cannot reach medic and has no first aid kit
						// try to create makeshift first aid kit
						UINT16 usItem = GetFirstAidKit(FIRSTAIDKIT);
						if (usItem > 0)
						{
							OBJECTTYPE newobj;
							CreateItem(usItem, 20 + Random(20), &newobj);
							newobj.fFlags |= OBJECT_UNDROPPABLE;

							// try to place item in inventory
							if (AutoPlaceObject(pSoldier, &newobj, FALSE))
							{
								DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: created makeshift first aid kit"));
								//if(SafeSpot(pSoldier, pSoldier->sGridNo))
								if (!DangerousSpot(pSoldier))
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: safe spot, end turn"));
									pSoldier->aiData.bAction = AI_ACTION_END_TURN;
									return TRUE;
								}
							}
						}
					}
				}
			}
			else if (SafeSpot(pSoldier))
			{
				// cannot find medic and has no first aid kit
				// try to create makeshift first aid kit
				UINT16 usItem = GetFirstAidKit(FIRSTAIDKIT);
				if (usItem > 0)
				{
					OBJECTTYPE newobj;
					CreateItem(usItem, 20 + Random(20), &newobj);
					newobj.fFlags |= OBJECT_UNDROPPABLE;

					// try to place item in inventory
					if (AutoPlaceObject(pSoldier, &newobj, FALSE))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("wounded soldier: created makeshift first aid kit, end turn"));
						pSoldier->aiData.bAction = AI_ACTION_END_TURN;
						return TRUE;
					}
				}
			}
			else if (pSoldier->bTacticalOrder == TACTICAL_ORDER_NONE &&
				pSoldier->CheckInitialAP() &&
				//Chance(pSoldier->bBleeding) &&
				Chance(pSoldier->iHealableInjury / 100) &&
				pSoldier->aiData.bAIMorale <= MORALE_CONFIDENT &&
				!pSoldier->RushAttackActive() &&
				!AICheckSuccessfulAttack(pSoldier, TRUE))
			{
				// try to retreat to safe spot
				pSoldier->RetreatStart(RETREAT_DURATION);
				DebugAI(AI_MSG_INFO, pSoldier, String("start retreat to rest"));
			}
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideRetreatReverse(SOLDIERTYPE *pSoldier, INT32 sClosestThreat, INT8 bClosestThreatLevel)
{
	// alt movement mode for retreat
	if (pSoldier->IsMercBodyType() &&
		!pSoldier->bReverse &&
		pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] == 0 &&
		!pSoldier->RushAttackActive() &&
		(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral) &&
		(pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE || CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || pSoldier->aiData.bUnderFire || Chance(SoldierDifficultyLevel(pSoldier) * 10)) &&
		PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.usActionData) <= 2 &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!Water(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) &&
		(pSoldier->aiData.bUnderFire || CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || !SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE) || EnemyCanAttackSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) || EnemyCanAttackSpot(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel)))
	{
		UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER);
		UINT8 ubMoveDirection = AIDirection(pSoldier->sGridNo, pSoldier->aiData.usActionData);
		UINT8 ubOppositeDirection = gOppositeDirection[pSoldier->ubDirection];
		UINT8 ubAnimEndHeight = pSoldier->AnimEndHeight();
		INT32 sClosestThreatAtSpot;
		INT8 bClosestThreatAtSpotLevel;

		// find closest possibly seen opponent after moving to new spot
		sClosestThreatAtSpot = sClosestThreat;
		bClosestThreatAtSpotLevel = bClosestThreatLevel;

		UINT8 ubClosestThreatDirection = AIDirection(pSoldier->aiData.usActionData, sClosestThreatAtSpot);
		INT16 sMinAPsToAttack = MinAPsToAttack(pSoldier, sClosestThreat, TRUE, 0);
		INT16 sMinAPsToAttackRaise = MinAPsToAttack(pSoldier, sClosestThreat, TRUE, 0, TRUE);
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
				pSoldier->aiData.bAction = AI_ACTION_TAKE_COVER;
				return TRUE;
			}
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideSeekReverse(SOLDIERTYPE *pSoldier, INT32 sClosestThreat, INT8 bClosestThreatLevel)
{
	// alt movement mode when seeking opponent
	if (pSoldier->IsMercBodyType() &&
		!pSoldier->bReverse &&
		pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] == 0 &&
		!pSoldier->RushAttackActive() &&
		(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral) &&
		(pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE || CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || pSoldier->aiData.bUnderFire || Chance(SoldierDifficultyLevel(pSoldier) * 10)) &&
		PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.usActionData) <= 1 &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!Water(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) &&
		(pSoldier->aiData.bUnderFire || CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || !SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE) || EnemyCanAttackSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) || EnemyCanAttackSpot(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel)))
	{
		UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_SEEK_OPPONENT);
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
		UINT16 usMovementModeReverse = DetermineMovementMode(pSoldier, AI_ACTION_SEEK_OPPONENT, TRUE);
		INT32 iPathCostReverse = EstimatePlotPath(pSoldier, pSoldier->aiData.usActionData, FALSE, FALSE, FALSE, usMovementModeReverse, pSoldier->bStealthMode, TRUE, 0);

		if (iPathCostReverse > 0 && pSoldier->bActionPoints >= iPathCostReverse)
		{
			// possibly, first turn to threat when moving sideways
			/*if ((usMovementModeReverse == CRAWLING && ubAnimEndHeight == ANIM_PRONE || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND) &&
				(pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttackRaise + GetAPsToLook(pSoldier) || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND && pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttack + GetAPsToLook(pSoldier)) &&
				(ubMoveDirection == gTwoCDirection[ubClosestThreatDirection] || ubMoveDirection == gTwoCCDirection[ubClosestThreatDirection]) &&
				//!LOS(pSoldier, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, ANIM_STAND, SIGHT_NORMAL, NO_DISTANCE_LIMIT))
				!LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, NO_DISTANCE_LIMIT))
			{
				// turn to closest threat
				if (pSoldier->ubDirection != ubClosestThreatDirection &&
					pSoldier->InternalIsValidStance(ubClosestThreatDirection, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
					pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
				{
					pSoldier->aiData.usActionData = ubOpponentDir;

					DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest threat before moving"));
					return AI_ACTION_CHANGE_FACING;
				}
			}*/

			// when standing or crawling - move sideways
			if (!pSoldier->bReverse &&
				(usMovementModeReverse == CRAWLING && ubAnimEndHeight == ANIM_PRONE || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND) &&
				(ubMoveDirection == gTwoCDirection[pSoldier->ubDirection] || ubMoveDirection == gTwoCCDirection[pSoldier->ubDirection]) &&
				(ubClosestThreatDirection == pSoldier->ubDirection || ubClosestThreatDirection == gOneCDirection[pSoldier->ubDirection] || ubClosestThreatDirection == gOneCCDirection[pSoldier->ubDirection]) &&
				(pSoldier->bActionPoints < sMinAPsToAttack || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND && pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttack || pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttackRaise))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("moving sideways keeping the same threat direction, use reverse movement mode"));
				pSoldier->bReverse = TRUE;

				// possibly, first turn to threat and raise gun when moving sideways
				if (pSoldier->bReverse &&
					usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
					pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToRaise + GetAPsToLook(pSoldier) &&
					(ubMoveDirection == gTwoCDirection[ubClosestThreatDirection] || ubMoveDirection == gTwoCCDirection[ubClosestThreatDirection]) &&
					!LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, MAX_VISION_RANGE) &&
					LOS_Location(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, pSoldier->GetMaxDistanceVisible(sClosestThreatAtSpot, bClosestThreatAtSpotLevel, CALC_FROM_ALL_DIRS)))
				{
					// possibly raise weapon (only when standing)
					if (usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
						!WeaponReady(pSoldier) &&
						PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
						(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
						pSoldier->bActionPoints >= GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
					{
						// prepare next action to take cover
						pSoldier->aiData.bNextAction = AI_ACTION_SEEK_OPPONENT;
						pSoldier->aiData.usNextActionData = pSoldier->aiData.usActionData;

						DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon and take cover"));
						pSoldier->aiData.bAction = AI_ACTION_RAISE_GUN;
						return TRUE;
					}
				}

				pSoldier->aiData.bAction = AI_ACTION_SEEK_OPPONENT;
				return TRUE;
			}
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}

BOOLEAN DecideCoverReverse(SOLDIERTYPE *pSoldier, INT32 sClosestThreat, INT8 bClosestThreatLevel)
{
	// alt movement mode
	if (pSoldier->IsMercBodyType() &&
		!pSoldier->bReverse &&
		pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] == 0 &&
		!pSoldier->RushAttackActive() &&
		(pSoldier->bTeam == ENEMY_TEAM || pSoldier->bTeam == MILITIA_TEAM || pSoldier->bTeam == CIV_TEAM && !pSoldier->aiData.bNeutral) &&
		(pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE_MILITIA || pSoldier->ubSoldierClass == SOLDIER_CLASS_ELITE || CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || pSoldier->aiData.bUnderFire || Chance(SoldierDifficultyLevel(pSoldier) * 10)) &&
		PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.usActionData) <= 2 &&
		!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		!Water(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) &&
		(pSoldier->aiData.bUnderFire || CorpseWarning(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel) || !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, FALSE) || !SightCoverAtSpot(pSoldier, pSoldier->aiData.usActionData, ANIM_STAND, FALSE) || EnemyCanAttackSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) || EnemyCanAttackSpot(pSoldier, pSoldier->aiData.usActionData, pSoldier->pathing.bLevel)))
	{
		UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_TAKE_COVER);
		UINT8 ubMoveDirection = AIDirection(pSoldier->sGridNo, pSoldier->aiData.usActionData);
		UINT8 ubOppositeDirection = gOppositeDirection[pSoldier->ubDirection];
		UINT8 ubAnimEndHeight = pSoldier->AnimEndHeight();
		INT32 sClosestThreatAtSpot;
		INT8 bClosestThreatAtSpotLevel;

		// check if target spot is exactly on the movement line
		BOOLEAN fGoodTargetSpot = FALSE;
		if (PythSpacesAway(pSoldier->sGridNo, pSoldier->aiData.usActionData) <= 1)
		{
			fGoodTargetSpot = TRUE;
		}
		else
		{
			INT32 sDirectionSpot;
			sDirectionSpot = NewGridNo(pSoldier->sGridNo, ubMoveDirection);
			sDirectionSpot = NewGridNo(sDirectionSpot, ubMoveDirection);
			// check if after 2 steps in same direction we arrived at target spot
			if (sDirectionSpot == pSoldier->aiData.usActionData)
				fGoodTargetSpot = TRUE;
		}

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
				pSoldier->aiData.bAction = AI_ACTION_TAKE_COVER;
				return TRUE;
			}

			// possibly, first turn to threat and raise gun when moving sideways
			/*if ((usMovementModeReverse == CRAWLING && ubAnimEndHeight == ANIM_PRONE || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND) &&
				(pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttackRaise + GetAPsToLook(pSoldier) || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND && pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttack + GetAPsToLook(pSoldier)) &&
				(ubMoveDirection == gTwoCDirection[ubClosestThreatDirection] || ubMoveDirection == gTwoCCDirection[ubClosestThreatDirection]) &&
				//!LOS(pSoldier, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, ANIM_STAND, SIGHT_NORMAL, NO_DISTANCE_LIMIT))
				!LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, NO_DISTANCE_LIMIT))
			{
				// turn to closest threat
				if (pSoldier->ubDirection != ubClosestThreatDirection &&
					pSoldier->InternalIsValidStance(ubClosestThreatDirection, gAnimControl[pSoldier->usAnimState].ubEndHeight) &&
					pSoldier->bActionPoints >= GetAPsToLook(pSoldier))
				{
					pSoldier->aiData.usActionData = ubOpponentDir;

					// possibly raise weapon (only when standing)
					if (usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
						!WeaponReady(pSoldier) &&
						PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
						(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
						pSoldier->bActionPoints >= GetAPsToLook(pSoldier) + GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
					{
						DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon after turning"));
						pSoldier->aiData.bNextAction = AI_ACTION_RAISE_GUN;
						pSoldier->aiData.usNextActionData = 0;
					}

					DebugAI(AI_MSG_INFO, pSoldier, String("turn to closest threat before moving"));
					return AI_ACTION_CHANGE_FACING;
				}

				// possibly raise weapon (only when standing)
				if (usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
					!WeaponReady(pSoldier) &&
					PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
					(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
					pSoldier->bActionPoints >= GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
				{
					DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon before moving"));
					return AI_ACTION_RAISE_GUN;
				}
			}*/

			// when standing or crawling - move sideways
			if (!pSoldier->bReverse &&
				fGoodTargetSpot &&	// for side walking or crawl rolling it's important that target spot is exactly on the line of movement
				(usMovementModeReverse == CRAWLING && ubAnimEndHeight == ANIM_PRONE || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND) &&
				(ubMoveDirection == gTwoCDirection[pSoldier->ubDirection] || ubMoveDirection == gTwoCCDirection[pSoldier->ubDirection]) &&
				(ubClosestThreatDirection == pSoldier->ubDirection || ubClosestThreatDirection == gOneCDirection[pSoldier->ubDirection] || ubClosestThreatDirection == gOneCCDirection[pSoldier->ubDirection]) &&
				(pSoldier->bActionPoints < sMinAPsToAttack || usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND && pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttack || pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToAttackRaise))
			{
				DebugAI(AI_MSG_INFO, pSoldier, String("moving sideways keeping the same threat direction, use reverse movement mode"));
				pSoldier->bReverse = TRUE;

				// possibly, first turn to threat and raise gun when moving sideways
				if (pSoldier->bReverse &&
					usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
					pSoldier->bActionPoints >= iPathCostReverse + sMinAPsToRaise + GetAPsToLook(pSoldier) &&
					(ubMoveDirection == gTwoCDirection[ubClosestThreatDirection] || ubMoveDirection == gTwoCCDirection[ubClosestThreatDirection]) &&
					!LOS_Location(pSoldier->sGridNo, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, MAX_VISION_RANGE) &&
					LOS_Location(pSoldier->aiData.usActionData, pSoldier->pathing.bLevel, sClosestThreatAtSpot, bClosestThreatAtSpotLevel, pSoldier->GetMaxDistanceVisible(sClosestThreatAtSpot, bClosestThreatAtSpotLevel, CALC_FROM_ALL_DIRS)))
				{
					// possibly raise weapon (only when standing)
					if (usMovementModeReverse == WALKING && ubAnimEndHeight == ANIM_STAND &&
						!WeaponReady(pSoldier) &&
						PickSoldierReadyAnimation(pSoldier, FALSE, FALSE) != INVALID_ANIMATION &&
						(pSoldier->bBreath > OKBREATH * 2 || GetBPCostPer10APsForGunHolding(pSoldier, TRUE) < 30) &&
						pSoldier->bActionPoints >= GetAPsToReadyWeapon(pSoldier, PickSoldierReadyAnimation(pSoldier, FALSE, FALSE)))
					{
						// prepare next action to take cover
						pSoldier->aiData.bNextAction = AI_ACTION_TAKE_COVER;
						pSoldier->aiData.usNextActionData = pSoldier->aiData.usActionData;

						DebugAI(AI_MSG_INFO, pSoldier, String("raise weapon and take cover"));
						pSoldier->aiData.bAction = AI_ACTION_RAISE_GUN;
						return TRUE;
					}
				}

				pSoldier->aiData.bAction = AI_ACTION_TAKE_COVER;
				return TRUE;
			}
		}
	}

	pSoldier->aiData.bAction = AI_ACTION_NONE;
	return FALSE;
}


void PrepareMainRedAIWeights(SOLDIERTYPE *pSoldier, INT8 &bSeekPts, INT8 &bHelpPts, INT8 &bHidePts, INT8 &bWatchPts)
{
	// modify RED movement tendencies according to morale
	switch (pSoldier->aiData.bAIMorale)
	{
	case MORALE_HOPELESS:  bSeekPts = -99; bHelpPts = -99; bHidePts += +2; bWatchPts = -99; break;
	case MORALE_WORRIED:   bSeekPts += -2; bHelpPts += 0; bHidePts += +2; bWatchPts += 1; break;
	case MORALE_NORMAL:    bSeekPts += 0; bHelpPts += 0; bHidePts += 0; bWatchPts += 0; break;
	case MORALE_CONFIDENT: bSeekPts += +1; bHelpPts += 0; bHidePts += -1; bWatchPts += 0; break;
	case MORALE_FEARLESS:  bSeekPts += +1; bHelpPts += 0; bHidePts += -1; bWatchPts += 0; break;
	}

	// modify tendencies according to orders
	switch (pSoldier->aiData.bOrders)
	{
	case STATIONARY:   bSeekPts += -1; bHelpPts += -1; bHidePts += +1; bWatchPts += +1; break;
	case ONGUARD:      bSeekPts += -1; bHelpPts += 0; bHidePts += +1; bWatchPts += +1; break;
	case CLOSEPATROL:  bSeekPts += 0; bHelpPts += 0; bHidePts += 0; bWatchPts += 0; break;
	case RNDPTPATROL:  bSeekPts += 0; bHelpPts += 0; bHidePts += 0; bWatchPts += 0; break;
	case POINTPATROL:  bSeekPts += 0; bHelpPts += 0; bHidePts += 0; bWatchPts += 0; break;
	case FARPATROL:    bSeekPts += 0; bHelpPts += 0; bHidePts += 0; bWatchPts += 0; break;
	case ONCALL:       bSeekPts += 0; bHelpPts += +1; bHidePts += -1; bWatchPts += 0; break;
	case SEEKENEMY:    bSeekPts += +1; bHelpPts += 0; bHidePts += -1; bWatchPts += -1; break;
	}

	// modify tendencies according to attitude
	switch (pSoldier->aiData.bAttitude)
	{
	case DEFENSIVE:     bSeekPts += -1; bHelpPts += 0; bHidePts += +2; bWatchPts += +1; break;
	case BRAVESOLO:     bSeekPts += +1; bHelpPts += -1; bHidePts += -1; bWatchPts += -1; break;
	case BRAVEAID:      bSeekPts += +1; bHelpPts += +1; bHidePts += -1; bWatchPts += -1; break;
	case CUNNINGSOLO:   bSeekPts += 1; bHelpPts += -1; bHidePts += +1; bWatchPts += 0; break;
	case CUNNINGAID:    bSeekPts += 1; bHelpPts += +1; bHidePts += +1; bWatchPts += 0; break;
	case AGGRESSIVE:    bSeekPts += +1; bHelpPts += 0; bHidePts += -1; bWatchPts += 0; break;
	case ATTACKSLAYONLY:bSeekPts += +1; bHelpPts += 0; bHidePts += -1; bWatchPts += 0; break;
	}
}
