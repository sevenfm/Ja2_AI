#ifdef PRECOMPILEDHEADERS
#include "AI All.h"
#else
#include <stdlib.h>
#include "Isometric Utils.h"
#include "ai.h"
#include "AIInternals.h"
#include "los.h"
#include "Weapons.h"
#include "Opplist.h"
#include "pathai.h"
#include "items.h"
#include "World Items.h"
#include "Points.h"
#include "message.h"
#include "Map Edgepoints.h"
#include "renderworld.h"
#include "Render Fun.h"
#include "Boxing.h"
#include "Text.h"
#ifdef _DEBUG
#include "renderworld.h"
#include "video.h"
#endif
#include "worldman.h"
#include "strategicmap.h"
#include "environment.h"
#include "lighting.h"
#include "Buildings.h"
#include "GameSettings.h"
#include "Soldier Profile.h"
// sevenfm
#include "rotting corpses.h"
#include "vehicles.h"
#include "soldier macros.h"
#include "drugs and alcohol.h"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////
// SANDRO - all "APBPConstants[AP_CROUCH]" and "APBPConstants[AP_PRONE]" here    
//           were changed to GetAPsCrouch() and GetAPsProne()					 
//		  - also all "APBPConstants[AP_PICKUP_ITEM]" were replaced by GetBasicAPsToPickupItem()
////////////////////////////////////////////////////////////////////////////////////////////////////////

INT16 * gsCoverValue = NULL;
#ifdef _DEBUG

INT16	gsBestCover;
#ifndef PATHAI_VISIBLE_DEBUG
// NB Change this to true to get visible cover debug -- CJC
BOOLEAN gfDisplayCoverValues = FALSE;
#endif
extern void RenderCoverDebug( void );
#endif

INT16	gubAIPathCosts[AI_PATHCOST_DIM][AI_PATHCOST_DIM];

extern UINT8 gubAICounter;
extern BOOLEAN gfTurnBasedAI;

INT32 CalcPercentBetter(INT32 iOldValue, INT32 iNewValue, INT32 iOldScale, INT32 iNewScale)
{
	INT32 iValueChange,iScaleSum,iPercentBetter;//,loopCnt,tempInt;

	// calculate how much better the new cover would be than the current cover
	iValueChange = iNewValue - iOldValue;


	// here, the change in cover HAS to be an improvement over current cover
	if (iValueChange <= 0)
	{
		return(NOWHERE);
	}

	iScaleSum = iOldScale + iNewScale;

	// here, the change in cover HAS to be an improvement over current cover
	if (iScaleSum <= 0)
	{
		return(NOWHERE);
	}

	iPercentBetter = (iValueChange * 100) / iScaleSum;

	return(iPercentBetter);
}

void AICenterXY( INT32 sGridNo, FLOAT * pdX, FLOAT * pdY )
{
	INT16		sXPos, sYPos;

	sXPos = sGridNo % WORLD_COLS;
	sYPos = sGridNo / WORLD_COLS;

	*pdX = (FLOAT) (sXPos * CELL_X_SIZE + CELL_X_SIZE / 2);
	*pdY = (FLOAT) (sYPos * CELL_Y_SIZE + CELL_Y_SIZE / 2);
}

INT8 CalcWorstCTGTForPosition( SOLDIERTYPE * pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft )
{
	// When considering a gridno for cover, we want to take into account cover if we
	// lie down, so we return the LOWEST chance to get through for that location.
	INT8		bCubeLevel, bThisCTGT,bWorstCTGT = 100;

	for (bCubeLevel = 1; bCubeLevel <= 3; bCubeLevel++)
	{
		switch (bCubeLevel)
		{
		case 1:
			if (iMyAPsLeft < GetAPsCrouch(MercPtrs[ubOppID],TRUE) + GetAPsProne(MercPtrs[ubOppID],TRUE))
			{
				continue;
			}
			break;
		case 2:
			if (iMyAPsLeft < GetAPsCrouch(MercPtrs[ubOppID],TRUE))
			{
				continue;
			}
			break;
		default:
			break;
		}

		bThisCTGT = SoldierToLocationChanceToGetThrough( pSoldier, sOppGridNo, bLevel, bCubeLevel, ubOppID );
		if (bThisCTGT < bWorstCTGT)
		{
			bWorstCTGT = bThisCTGT;
			// if there is perfect cover
			if (bWorstCTGT == 0)
				// then bail from the for loop, it can't possible get any better
				break;
		}
	}
	return( bWorstCTGT );
}

INT8 CalcCTGTForPosition(SOLDIERTYPE * pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft)
{
	// determine cover depending on APs left and movement mode
	/*INT8 bCTGT, bTotalCTGT;
	INT8 bMinCubeLevel;
	UINT8 ubCounter = 0;

	if (gusNPCMovementMode == CRAWLING ||
		gusNPCMovementMode == SWATTING && iMyAPsLeft >= GetAPsProne(MercPtrs[ubOppID], TRUE) + APBPConstants[AP_LOOK_CROUCHED] ||
		iMyAPsLeft >= GetAPsCrouch(MercPtrs[ubOppID], TRUE) + GetAPsProne(MercPtrs[ubOppID], TRUE) + APBPConstants[AP_LOOK_STANDING])
	{
		bMinCubeLevel = 1;
	}
	else if (gusNPCMovementMode == SWATTING ||
		iMyAPsLeft >= GetAPsCrouch(MercPtrs[ubOppID], TRUE) + APBPConstants[AP_LOOK_STANDING])
	{
		bMinCubeLevel = 2;
	}
	else
	{
		bMinCubeLevel = 3;
	}

	for (UINT8 cnt = bMinCubeLevel; cnt <= 3; cnt++)
	{
		bCTGT = AISoldierToLocationChanceToGetThrough(pSoldier, sOppGridNo, bLevel, cnt);
		bTotalCTGT += (4 - cnt) * bCTGT;
		ubCounter += (4 - cnt);

		if (bCTGT == 100)
		{
			break;
		}
	}
	
	return bTotalCTGT / ubCounter;*/

	// determine cover depending on APs left and movement mode
	INT8 bCTGT;
	INT8 bCubeLevel;

	if (gusNPCMovementMode == CRAWLING ||
		gusNPCMovementMode == SWATTING && iMyAPsLeft >= GetAPsProne(MercPtrs[ubOppID], TRUE) + APBPConstants[AP_LOOK_CROUCHED] ||
		iMyAPsLeft >= GetAPsCrouch(MercPtrs[ubOppID], TRUE) + GetAPsProne(MercPtrs[ubOppID], TRUE) + APBPConstants[AP_LOOK_STANDING])
	{
		bCubeLevel = 1;
	}
	else if (gusNPCMovementMode == SWATTING ||
		iMyAPsLeft >= GetAPsCrouch(MercPtrs[ubOppID], TRUE) + APBPConstants[AP_LOOK_STANDING])
	{
		bCubeLevel = 2;
	}
	else
	{
		bCubeLevel = 3;
	}

	bCTGT = AISoldierToLocationChanceToGetThrough(pSoldier, sOppGridNo, bLevel, bCubeLevel);

	return(bCTGT);
}

INT8 CalcAverageCTGTForPosition( SOLDIERTYPE * pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft )
{
	// When considering a gridno for cover, we want to take into account cover if we
	// lie down, so we return the LOWEST chance to get through for that location.
	INT8		bCubeLevel;
	INT32		iTotalCTGT = 0, bValidCubeLevels = 0;

	for (bCubeLevel = 1; bCubeLevel <= 3; bCubeLevel++)
	{
		switch (bCubeLevel)
		{
		case 1:
			if (iMyAPsLeft < GetAPsCrouch(MercPtrs[ubOppID],TRUE) + GetAPsProne(MercPtrs[ubOppID],TRUE))
			{
				continue;
			}
			break;
		case 2:
			if (iMyAPsLeft < GetAPsCrouch(MercPtrs[ubOppID],TRUE))
			{
				continue;
			}
			break;
		default:
			break;
		}
		iTotalCTGT += SoldierToLocationChanceToGetThrough( pSoldier, sOppGridNo, bLevel, bCubeLevel, ubOppID );
		bValidCubeLevels++;
	}
	iTotalCTGT /= bValidCubeLevels;
	return( (INT8) iTotalCTGT );
}

INT8 CalcCTGTForPositionNearby(SOLDIERTYPE *pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft)
{
	INT8	bCTGT, bBestCTGT = 0;
	UINT8	ubMovementCost;
	INT32	sTempSpot;
	UINT8	ubDirection;

	// check adjacent reachable tiles
	for (ubDirection = 0; ubDirection < NUM_WORLD_DIRECTIONS; ubDirection++)
	{
		sTempSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(ubDirection));

		if (sTempSpot != pSoldier->sGridNo)
		{
			ubMovementCost = gubWorldMovementCosts[sTempSpot][ubDirection][bLevel];

			if (ubMovementCost < TRAVELCOST_BLOCKED && 
				!InWaterOrGas(pSoldier, sTempSpot) &&
				NewOKDestination(pSoldier, sTempSpot, FALSE, bLevel) )
			{
				// pretend he's standing at sTempSpot
				pSoldier->sGridNo = sTempSpot;
				AICenterXY(sTempSpot, &(pSoldier->dXPos), &(pSoldier->dYPos));

				bCTGT = CalcCTGTForPosition(pSoldier, ubOppID, sOppGridNo, bLevel, iMyAPsLeft);

				if (bCTGT > bBestCTGT)
				{
					bBestCTGT = bCTGT;
				}

				if (bCTGT == 100)
				{
					break;
				}
			}
		}
	}

	return bBestCTGT;
}

INT8 CalcBestCTGT( SOLDIERTYPE *pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft )
{
	// NOTE: CTGT stands for "ChanceToGetThrough..."

	// using only ints for maximum execution speed here
	// CJC: Well, so much for THAT idea!
	INT32 sCentralGridNo, sAdjSpot, sNorthGridNo, sSouthGridNo, sCheckSpot;
	BOOLEAN sOKTest;

	INT8 bThisCTGT, bBestCTGT = 0;

	sCheckSpot = -1;

	sCentralGridNo = pSoldier->sGridNo;

	// precalculate these for speed
	// what was struct for?
	sOKTest = NewOKDestination( pSoldier, sCentralGridNo, IGNOREPEOPLE , bLevel );
	sNorthGridNo = NewGridNo( sCentralGridNo, DirectionInc(NORTH) );
	sSouthGridNo = NewGridNo( sCentralGridNo, DirectionInc(SOUTH) );

	// look into all 8 adjacent tiles & determine where the cover is the worst
	// Lalien: shouldn't this start at 0 than?
	for (UINT8 sDir = 0; sDir < NUM_WORLD_DIRECTIONS; sDir++)
	{
		// get the gridno of the adjacent spot lying in that direction
		sAdjSpot = NewGridNo( sCentralGridNo, DirectionInc( sDir ) );

		// if it wasn't out of bounds
		if (sAdjSpot != sCentralGridNo)
		{
			// if the adjacent spot can we walked on and isn't in water or gas
			if ( NewOKDestination( pSoldier, sAdjSpot, IGNOREPEOPLE, bLevel ) && !InWaterOrGas( pSoldier, sAdjSpot ))
			{
				switch (sDir)
				{
				case NORTH:
				case EAST:
				case SOUTH:
				case WEST:
					sCheckSpot = sAdjSpot;
					break;
				case NORTHEAST:
				case NORTHWEST:
					// spot to the NORTH is guaranteed to be in bounds since NE/NW was
					sCheckSpot = sNorthGridNo;
					break;
				case SOUTHEAST:
				case SOUTHWEST:
					// spot to the SOUTH is guaranteed to be in bounds since SE/SW was
					sCheckSpot = sSouthGridNo;
					break;
				}

				// ATE: OLD STUFF
				// if the adjacent gridno is reachable from the starting spot
				if ( NewOKDestination( pSoldier, sCheckSpot, FALSE, bLevel ) )
				{
					// the dude could move to this adjacent gridno, so put him there
					// "virtually" so we can calculate what our cover is from there

					// NOTE: GOTTA SET THESE 3 FIELDS *BACK* AFTER USING THIS FUNCTION!!!
					pSoldier->sGridNo = sAdjSpot;	 // pretend he's standing at 'sAdjSpot'
					AICenterXY( sAdjSpot, &(pSoldier->dXPos), &(pSoldier->dYPos) );
					bThisCTGT = CalcWorstCTGTForPosition( pSoldier, ubOppID, sOppGridNo, bLevel, iMyAPsLeft );
					if (bThisCTGT > bBestCTGT)
					{
						bBestCTGT = bThisCTGT;
						// if there is no cover
						if (bBestCTGT == 100)
							// then bail from the for loop, it can't possible get any better
							break;
					}
				}
			}
		}
	}

	return( bBestCTGT );
}


INT32 CalcCoverValue(SOLDIERTYPE *pMe, INT32 sMyGridNo, INT32 iMyThreat, INT32 iMyAPsLeft,
	UINT32 uiThreatIndex, INT32 iRange, INT32 *iTotalScale)
{
	//DebugCover(pMe, String("\nCalcCoverValue [%d] vs [%d] sMyGridNo %d iMyThreat %d iMyAPsLeft %d uiThreatIndex %d iRange %d", pMe->ubID, Threat[uiThreatIndex].pOpponent->ubID, sMyGridNo, iMyThreat, iMyAPsLeft, uiThreatIndex, iRange));

	// all 32-bit integers for max. speed
	INT32	iMyPosValue, iHisPosValue, iCoverValue;
	INT32	iReductionFactor, iThisScale;
	INT32	sHisGridNo = Threat[uiThreatIndex].sGridNo;
	INT32	sMyRealGridNo = NOWHERE;
	INT32	sHisRealGridNo = NOWHERE;
	INT16	sTempX, sTempY;
	FLOAT	dMyX, dMyY, dHisX, dHisY;
	INT8	bHisBestCTGT, bHisActualCTGT, bHisCTGT, bMyCTGT, bMyCTGT1, bMyCTGT2, bMyCTGT3;
	INT32	iRangeChange, iRangeFactor, iRCD;
	SOLDIERTYPE *pHim = Threat[uiThreatIndex].pOpponent;
	INT8	bHisLevel = Threat[uiThreatIndex].bLevel;
	INT8	bMyLevel = pMe->pathing.bLevel;

	// sevenfm
	UINT8 ubMyCTH, ubHisCTH;
	//INT32 iAggressionBonus;
	INT32 sNewDistance = PythSpacesAway(sMyGridNo, sHisGridNo);
	UINT8 ubFriendlyFireChance = 0;
	UINT8 ubShootingChance;
	BOOLEAN fMyCover = FALSE;		// cover at new spot
	BOOLEAN fEnemyCover = FALSE;
	BOOLEAN fMyCurrentCover = FALSE;	// cover at current spot	
	UINT8 ubCoverBonus;
	UINT16 usMyGunRange = AIGunRange(pMe);
	UINT16 usHisGunRange = AIGunRange(pHim);

	BOOLEAN fMySight = FALSE;			// sight to prone enemy at new spot
	BOOLEAN fHisSight = FALSE;		// sight to prone enemy at new spot
	BOOLEAN fMyCurrentSight = FALSE;	// sight to prone enemy at current spot
	BOOLEAN fHisCurrentSight = FALSE;		// sight to prone enemy at new spot
	//INT16	sHisSightRange = CALC_FROM_ALL_DIRS;
	BOOLEAN fMyGun = AICheckHasGun(pHim);
	BOOLEAN fHisGun = AICheckHasGun(pMe);

	BOOLEAN fForceRaise = TRUE;
	BOOLEAN fMyAttack = FALSE;
	BOOLEAN fHisAttack = FALSE;
	UINT16	usTrueState;
	BOOLEAN fAggressiveAI = AggressiveAI(pMe);

	/*if (Threat[uiThreatIndex].bKnowledge == SEEN_CURRENTLY || Threat[uiThreatIndex].bKnowledge == SEEN_THIS_TURN)
		sHisSightRange = CALC_FROM_ALL_DIRS;
	else
		//sHisSightRange = MAX_VISION_RANGE;
		sHisSightRange = AIMaxDistanceVisible(pHim, sMyGridNo, bMyLevel);*/

	// THE FOLLOWING STUFF IS *VEERRRY SCAARRRY*, BUT SHOULD WORK.	IF YOU REALLY
	// HATE IT, THEN CHANGE ChanceToGetThrough() TO WORK FROM A GRIDNO TO GRIDNO

	sMyRealGridNo = pMe->sGridNo;		// remember where I REALLY am
	dMyX = pMe->dXPos;
	dMyY = pMe->dYPos;

	sHisRealGridNo = pHim->sGridNo;		// remember where he REALLY is
	dHisX = pHim->dXPos;
	dHisY = pHim->dYPos;

	// if this is theoretical, and I'm not actually at sMyGridNo right now
	if (pMe->sGridNo != sMyGridNo)
	{
		pMe->sGridNo = sMyGridNo;				// but pretend I'm standing at sMyGridNo
		ConvertGridNoToCenterCellXY( sMyGridNo, &sTempX, &sTempY );
		pMe->dXPos = (FLOAT) sTempX;
		pMe->dYPos = (FLOAT) sTempY;
	}

	// if this is theoretical, and he's not actually at hisGrid right now
	if (pHim->sGridNo != sHisGridNo)
	{
		pHim->sGridNo = sHisGridNo;			// but pretend he's standing at sHisGridNo
		ConvertGridNoToCenterCellXY( sHisGridNo, &sTempX, &sTempY );
		pHim->dXPos = (FLOAT) sTempX;
		pHim->dYPos = (FLOAT) sTempY;
	}

	if( DeepWater(sHisGridNo, bHisLevel) || InGas(pHim,sHisGridNo) )
	{
		bHisActualCTGT = 0;
	}
	else
	{
		// optimistically assume we'll be behind the best cover available at this spot
		//bHisActualCTGT = CalcWorstCTGTForPosition( pHim, pMe->ubID, sMyGridNo, bMyLevel, iMyAPsLeft );
		bHisActualCTGT = CalcCTGTForPosition(pHim, pMe->ubID, sMyGridNo, bMyLevel, iMyAPsLeft);
	}

	// normally, that will be the cover I'll use, unless worst case over-rides it
	bHisCTGT = bHisActualCTGT;

	// only calculate his best case CTGT if there is room for improvement!
	if (bHisActualCTGT < 100)
	{
		// calculate where my cover is worst if opponent moves just 1 tile over
		//bHisBestCTGT = CalcBestCTGT(pHim, pMe->ubID, sMyGridNo, bMyLevel, iMyAPsLeft);
		bHisBestCTGT = CalcCTGTForPositionNearby(pHim, pMe->ubID, sMyGridNo, bMyLevel, iMyAPsLeft);

		// if he can actually improve his CTGT by moving to a nearby gridno
		if (bHisBestCTGT > bHisActualCTGT)
		{
			// he may not take advantage of his best case, so take only 2/3 of best
			bHisCTGT = ((2 * bHisBestCTGT) + bHisActualCTGT) / 3;
		}
	}

	// if my intended gridno is in water or gas, I can't attack at all from there
	// sevenfm: smoke is ok
	if( DeepWater(sMyGridNo, bMyLevel) || InGas(pMe,sMyGridNo) )
	{
		bMyCTGT = 0;
	}
	else
	{
		// put him at sHisGridNo if necessary!
		if (pHim->sGridNo != sHisGridNo )
		{
			pHim->sGridNo = sHisGridNo;
			ConvertGridNoToCenterCellXY( sHisGridNo, &sTempX, &sTempY );
			pHim->dXPos = (FLOAT) sTempX;
			pHim->dYPos = (FLOAT) sTempY;
		}

		// let's not assume anything about the stance the enemy might take, so take an average
		// value... no cover give a higher value than partial cover
		//bMyCTGT = CalcAverageCTGTForPosition( pMe, pHim->ubID, sHisGridNo, bHisLevel, iMyAPsLeft );
		
		// sevenfm: calculate average CTGT with weight - 3 prone, 2 crouched, 1 standing
		// sevenfm: take into account friendly fire chance
		gUnderFire.Clear();
		gUnderFire.Enable();
		bMyCTGT1 = AISoldierToLocationChanceToGetThrough(pMe, sHisGridNo, bHisLevel, 1);		
		gUnderFire.Disable();
		ubFriendlyFireChance = gUnderFire.Chance(pMe->bTeam, pMe->bSide, TRUE);

		// sevenfm: assume if we have ground CTGT = 100, all higher will be 100
		if (bMyCTGT1 < 100)
			bMyCTGT2 = AISoldierToLocationChanceToGetThrough(pMe, sHisGridNo, bHisLevel, 2);
		else
			bMyCTGT2 = 100;

		if (bMyCTGT2 < 100)
			bMyCTGT3 = AISoldierToLocationChanceToGetThrough(pMe, sHisGridNo, bHisLevel, 3);
		else
			bMyCTGT3 = 100;

		// ground value is most important
		bMyCTGT = (3 * bMyCTGT1 + 2 * bMyCTGT2 + bMyCTGT3) / 6;

		if (ubFriendlyFireChance > MIN_CHANCE_TO_ACCIDENTALLY_HIT_SOMEONE)
			bMyCTGT = 1;
	}

	// sevenfm: estimate CTH
	BOOLEAN fMovementPenalty = FALSE;
	if (pMe->aiData.bUnderFire || pMe->usSoldierFlagMask2 & SOLDIER_ATTACKED_THIS_TURN)
		fMovementPenalty = TRUE;

	ubHisCTH = AIEstimateCTH(pHim, pMe, sHisGridNo, sMyGridNo, fMovementPenalty);
	ubMyCTH = AIEstimateCTH(pMe, pHim, sMyGridNo, sHisGridNo, FALSE);

	ubShootingChance = 100;
	if (ubFriendlyFireChance > 0)
		ubShootingChance /= 2;
	//ubShootingChance = max(0, ubShootingChance);
	//ubShootingChance = min(100, ubShootingChance);

	// find sight at new spot
	usTrueState = pMe->usAnimState;
	pMe->usAnimState = STANDING;
	if (LOS_Soldier(pMe, pHim, LOS_READY, CALC_FROM_ALL_DIRS))
	{
		fMySight = TRUE;
	}
	pMe->usAnimState = usTrueState;

	usTrueState = pHim->usAnimState;
	pHim->usAnimState = STANDING;
	//if (LOS_Soldier(pHim, pMe, LOS_READY, sHisSightRange))
	if (LOS_Soldier(pHim, pMe, LOS_UNREADY, CALC_FROM_ALL_DIRS))
	{
		fHisSight = TRUE;
	}
	pHim->usAnimState = usTrueState;

	// UNDO ANY TEMPORARY "DAMAGE" DONE ABOVE	
	if (!TileIsOutOfBounds(sMyRealGridNo))
	{
		pMe->sGridNo = sMyRealGridNo;		// put me back where I belong!
		pMe->dXPos = dMyX;					// also change the 'x'
		pMe->dYPos = dMyY;					// and the 'y'
	}

	if (!TileIsOutOfBounds(sHisRealGridNo))
	{
		pHim->sGridNo = sHisRealGridNo;		// put HIM back where HE belongs!
		pHim->dXPos = dHisX;				// also change the 'x'
		pHim->dYPos = dHisY;				// and the 'y'
	}

	// find sight at current spot
	usTrueState = pMe->usAnimState;
	pMe->usAnimState = STANDING;
	if (LOS_Soldier(pMe, pHim, LOS_READY, CALC_FROM_ALL_DIRS))
	{
		fMyCurrentSight = TRUE;
	}
	pMe->usAnimState = usTrueState;

	usTrueState = pHim->usAnimState;
	pHim->usAnimState = STANDING;
	//if (LOS_Soldier(pHim, pMe, LOS_READY, sHisSightRange))
	if (LOS_Soldier(pHim, pMe, LOS_UNREADY, CALC_FROM_ALL_DIRS))
	{
		fHisCurrentSight = TRUE;
	}
	pHim->usAnimState = usTrueState;

	// sevenfm: special calculations for zombies: zombie is very dangerous at close range
	if( pHim->IsZombie() || !fHisGun )
	{
		if( sNewDistance < TACTICAL_RANGE / 2 )
		{
			// 100 - 25
			bHisCTGT = 100 - 150 * sNewDistance / TACTICAL_RANGE;
		}
		else
		{
			// 25 - 0
			bHisCTGT = 100 * TACTICAL_RANGE * TACTICAL_RANGE / (16 * sNewDistance * sNewDistance);
		}
		ubHisCTH = 100;
	}

	// sevenfm: if soldier has no gun, use different calculation
	if( pMe->IsZombie() || !fMyGun )
	{
		if (sNewDistance < TACTICAL_RANGE / 2)
		{
			bMyCTGT = 100 - 150 * sNewDistance / TACTICAL_RANGE;
		}
		else
		{
			bMyCTGT = 100 * TACTICAL_RANGE * TACTICAL_RANGE / (16 * sNewDistance * sNewDistance);
		}
		ubMyCTH = 100;
	}

	// these values should be < 1 million each

	//iHisPosValue = bHisCTGT * Threat[uiThreatIndex].iValue * Threat[uiThreatIndex].iAPs;
	//iMyPosValue =	bMyCTGT *	iMyThreat * iMyAPsLeft;
	iHisPosValue = bHisCTGT * Threat[uiThreatIndex].iValue * Threat[uiThreatIndex].iAPs * ubHisCTH / 100;
	iMyPosValue = bMyCTGT * iMyThreat * iMyAPsLeft * ubMyCTH / 100;

	// sevenfm: reduce enemy threat if we can crouch/prone at new location
	// max bonus for crouch at 100
	if( iMyAPsLeft >= GetAPsCrouch(pMe,TRUE) )
	{
		// -5% of enemy threat, +5% my threat
		iHisPosValue -= 5 * iHisPosValue * min(100, iRange) / (100 * 100);
		iMyPosValue += 5 * iMyPosValue * min(100, iRange) / (100 * 100);
	}
	// max bonus for prone at 200
	if( iMyAPsLeft >= GetAPsProne(pMe, TRUE) + GetAPsCrouch(pMe,TRUE) )
	{
		// -10% of enemy threat, +10% my threat
		iHisPosValue -= 10 * iHisPosValue * min(200, iRange) / (200 * 100);
		iMyPosValue += 10 * iMyPosValue * min(200, iRange) / (200 * 100);
	}

	fMyCover = AnyCoverFromSpot(sMyGridNo, bMyLevel, sHisGridNo, bHisLevel);
	fMyCurrentCover = AnyCoverFromSpot(sMyRealGridNo, bMyLevel, sHisGridNo, bHisLevel);	
	fEnemyCover = AnyCoverFromSpot(sHisGridNo, bHisLevel, sMyGridNo, bMyLevel);

	// add penalty to enemy position, bonus to my position if soldier has cover at spot
	// max 25% at DAY_VISION_RANGE/2, 0 at zero range
	if( !pHim->IsZombie() && !pMe->IsZombie() )
	{
		ubCoverBonus = 25;
		if (sNewDistance < (INT32)(DAY_VISION_RANGE / 2))
		{
			ubCoverBonus = ubCoverBonus * 2 * sNewDistance / DAY_VISION_RANGE;
		}
		if (fMyCover)
		{			
			iHisPosValue -= iHisPosValue * ubCoverBonus / 100;
			iMyPosValue += iMyPosValue * ubCoverBonus / 100;
		}
		if (fEnemyCover)
		{
			iHisPosValue += iHisPosValue * ubCoverBonus / 100;
			iMyPosValue -= iMyPosValue * ubCoverBonus / 100;
		}
	}

	// if we are checking original gridno and weapon is raised, don't add raise cost
	if (WeaponReady(pMe) && sMyGridNo == pMe->sGridNo)
	{
		fForceRaise = FALSE;
	}

	INT16 sMinAttackAP = AIGunMinAPsToShoot(pMe, fForceRaise) + APBPConstants[AP_CHANGE_FACING];
	INT16 sHisMinAttackAP = AIGunMinAPsToShoot(pHim, FALSE) + APBPConstants[AP_CHANGE_FACING];

	if (!pMe->IsZombie() && fMyGun && iMyAPsLeft >= sMinAttackAP)
	{
		fMyAttack = TRUE;
	}
	if (!pHim->IsZombie() && fHisGun && Threat[uiThreatIndex].iAPs >= sHisMinAttackAP)
	{
		fHisAttack = TRUE;
	}

	// sevenfm: take into account sight	
	if (fHisSight)
	{
		iHisPosValue += iHisPosValue / 4;
	}
		
	if(fMySight)
	{
		iMyPosValue += iMyPosValue / 4;
	}

	// take into account if enemy has to turn to attack me
	if (!pHim->IsZombie() &&
		pHim->IsMercBodyType() &&
		fHisGun &&
		//pHim->AnimEndHeight() == ANIM_PRONE &&
		AIDirection(sHisGridNo, sMyGridNo) != pHim->ubDirection &&
		(Threat[uiThreatIndex].bKnowledge == SEEN_CURRENTLY || Threat[uiThreatIndex].bKnowledge == SEEN_THIS_TURN || Threat[uiThreatIndex].bKnowledge == SEEN_LAST_TURN || Threat[uiThreatIndex].bKnowledge == HEARD_THIS_TURN || Threat[uiThreatIndex].bKnowledge == HEARD_LAST_TURN))
	{
		iHisPosValue -= iHisPosValue / 8;
	}

	// try to account for who outnumbers who
	/*UINT8 ubMySide = CountSeenEnemiesLastTurn(pHim);
	//UINT8 ubMySide = CountEngagedEnemiesLastTurn(pHim);
	if( ubMySide > 0 )
	{
		iHisPosValue = iHisPosValue / ubMySide;
	}

	UINT8 ubHisSide = CountSeenEnemiesLastTurn(pMe);
	//UINT8 ubHisSide = CountEngagedEnemiesLastTurn(pMe);
	if( ubHisSide > 0 )
	{
		iMyPosValue = iMyPosValue / ubHisSide;
	}*/

	// instead of dividing by number of seen enemies, use weight function based on distance and estimated CTH
	// find chance that I will attack this enemy
	UINT8 ubMyChance = AIEstimateAttackWeight(pMe, sMyGridNo, pHim, sHisGridNo);
	iMyPosValue = iMyPosValue * ubMyChance / 100;

	// find chance that this enemy will attack me
	UINT8 ubHisChance = AIEstimateAttackWeight(pHim, sHisGridNo, pMe, sMyGridNo);
	iHisPosValue = iHisPosValue * ubHisChance / 100;

	// if my positional value is worth something at all here
	if (iMyPosValue > 0)
	{
		// if I CAN'T crouch when I get there, that makes it significantly less
		// appealing a spot (how much depends on range), so that's a penalty to me
		if (iMyAPsLeft < GetAPsCrouch(pMe,TRUE))
			// subtract another 1 % penalty for NOT being able to crouch per tile
			// the farther away we are, the bigger a difference crouching will make!
			iMyPosValue -= ((iMyPosValue * (AIM_PENALTY_TARGET_CROUCHED + (iRange / CELL_X_SIZE))) / 100);
	}

	// modify my value according to friendly fire chance
	iMyPosValue = iMyPosValue * ubShootingChance / 100;

	// high morale prefers decreasing the range (positive factor), while very
	// low morale (HOPELESS) prefers increasing it

	// new code
	BOOLEAN fAdvance = FALSE;
	BOOLEAN fKeepDistance = TRUE;
	BOOLEAN fAttackedThisTurn = pMe->FiredThisTurn();
	BOOLEAN fRetreat = (pMe->RetreatActive() || pMe->HighShockLevel() || pMe->HighWoundShock());
	BOOLEAN fEnemyHelpless = (pHim->stats.bLife < OKLIFE || pHim->IsCowering() || pHim->IsUnconscious());
	BOOLEAN fHighMorale = (pMe->RushAttackActive() || MercUnderTheInfluence(pMe, DRUG_TYPE_ADRENALINE));

	// don't advance if neutral, zombies only seek cover if cannot advance to enemy
	if (pMe->aiData.bNeutral || pMe->IsZombie() || pMe->bBlindedCounter > 0)
	{
		fAdvance = FALSE;
		fKeepDistance = FALSE;
	}
	else if (pMe->IsMonster() || pMe->IsBloodcat())
	{
		fAdvance = TRUE;
		fKeepDistance = TRUE;
	}
	/*else if (fAggressiveAI && (!pHim->IsZombie() && !pHim->IsBloodcat() && fHisGun
		|| fEnemyHelpless))
	{
		fAdvance = TRUE;
		fKeepDistance = TRUE;
	}*/
	else
	{
		// possibly allow advance
		if ((!pHim->IsZombie() && !pHim->IsBloodcat() && fHisGun || fEnemyHelpless) &&					// don't advance to zombie/bloodcat/hth unless opponent is helpless			
			(usMyGunRange < sNewDistance || fEnemyHelpless || fMyCover || fHighMorale) &&				// allow advance if out of weapon range or opponent is helpless or moving into cover
			(sNewDistance > TACTICAL_RANGE / 2 || fMyAttack || fEnemyHelpless || sNewDistance > TACTICAL_RANGE / 4 && fMyCover && !fHisSight || sNewDistance > TACTICAL_RANGE / 4 && !fMyGun) &&	// don't move too close to enemy
			!Water(sMyGridNo, bMyLevel) &&																// don't advance into water
			!(bMyLevel > 0 && RoofEdge(sMyGridNo)) &&													// don't advance to roof edge
			//!InSmoke(sMyGridNo, bMyLevel, FALSE) &&
			(pMe->bTacticalOrder == TACTICAL_ORDER_NONE || pMe->bTacticalOrder == TACTICAL_ORDER_RUSH_ATTACK) &&	// don't advance when flanking/sniping/retreating
			!AICheckSpotTooFar(pMe, sHisGridNo))														// don't advance if enemy is considered too far to seek
			//(pMe->aiData.bOrders != STATIONARY || fMyCover || bMyLevel > 0 && !RoofEdge(sMyGridNo) && (fMyCover || !fHisSight)) &&
			//(!InARoom(sMyRealGridNo, NULL) || InARoom(sMyGridNo, NULL) && fMyCover || pMe->pathing.bLevel > 0 && !RoofEdge(sMyGridNo) && (fMyCover || !fHisSight)))
		{
			fAdvance = TRUE;
		}

		// possibly allow retreat
		if ((pMe->aiData.bAIMorale < MORALE_CONFIDENT || fRetreat) &&
			!fHighMorale &&
			(pMe->CheckInitialAP() || fAttackedThisTurn) &&
			!Water(sMyGridNo, bMyLevel) &&
			fMyGun &&
			(usMyGunRange >= sNewDistance || fRetreat) &&
			// allow if he has no ranged attack, allow moving into cover after shooting or under heavy fire
			(fMySight && (pHim->IsZombie() || pHim->IsBloodcat() || !fHisGun) || (fMyCover || !fHisSight) && (fAttackedThisTurn || fRetreat)))
		{
			fKeepDistance = FALSE;
		}
	}

	iRCD = RangeChangeDesire( pMe );

	UINT8 ubSoldierDifficulty = SoldierDifficultyLevel(pMe);
	UINT16 usShortRange = TACTICAL_RANGE * 3 / 4;
	// sevenfm: boost aggression for soldiers with short range weapons
	if (usMyGunRange < usShortRange || fAggressiveAI)
	{
		ubSoldierDifficulty = min(ubSoldierDifficulty, max(1, 4 * usMyGunRange / usShortRange));
	}

	if (iRCD)
	{
		iRangeChange = Threat[uiThreatIndex].iOrigRange - iRange;

		if (iRangeChange)
		{
			//iRangeFactor = (iRangeChange * (morale - 1)) / 4;
			iRangeFactor = (iRangeChange * iRCD) / 2;

			// sevenfm: reduce range factor if soldier has no cover from enemy
			if (fAggressiveAI ||
				!fMyCover &&
				(fMyCurrentCover || InARoom(sMyRealGridNo, NULL)) &&
				(pMe->aiData.bOrders != SEEKENEMY || pMe->aiData.bAIMorale < MORALE_FEARLESS || AISpecialRole(pMe)) &&
				!pMe->RushAttackAdvance() &&
				!MercUnderTheInfluence(pMe, DRUG_TYPE_ADRENALINE) &&
				usMyGunRange >= sNewDistance)
			{
				iRangeFactor = iRangeFactor * (100 - bHisCTGT * Threat[uiThreatIndex].iAPs / (2 * APBPConstants[AP_MAXIMUM])) / 100;
			}

			// aggression booster for stupider enemies
			if (fAdvance && iRangeFactor > 0)
			{
				iMyPosValue += 100 * iRangeFactor * (4 - ubSoldierDifficulty) / 4;
			}

			// if factor is positive increase positional value, else decrease it
			// change both values, since one or the other could be 0
			if (iRangeFactor > 0 && fAdvance)
			{
				iMyPosValue = (iMyPosValue * (100 + iRangeFactor)) / 100;
				iHisPosValue = (100 * iHisPosValue) / (100 + iRangeFactor);
			}
			else if (iRangeFactor < 0 && fKeepDistance)
			{
				iMyPosValue = (100 * iMyPosValue) / (100 - iRangeFactor);
				iHisPosValue = (iHisPosValue * (100 - iRangeFactor)) / 100;
			}
		}
	}

	// sevenfm: add bonus if we can attack here this turn, penalty if cannot attack		
	if (iMyPosValue + iHisPosValue > 0 && fMyAttack)
	{
		iHisPosValue -= iHisPosValue * iMyPosValue/ (iMyPosValue + iHisPosValue);
		/*else if (fHisAttack && pMe->aiData.bOrders != SEEKENEMY)
		{
			iMyPosValue -= iMyPosValue * iHisPosValue / (iMyPosValue + iHisPosValue);
		}*/
	}

	// the farther apart we are, the less important the cover differences are
	// the less certain his position, the less important cover differences are
	iReductionFactor = ((MAX_THREAT_RANGE - iRange) * Threat[uiThreatIndex].iCertainty) / MAX_THREAT_RANGE;

	// divide by a 100 to make the numbers more manageable and avoid 32-bit limit
	iThisScale = max( iMyPosValue, iHisPosValue) / 100;
	iThisScale = (iThisScale * iReductionFactor) / 100;
	*iTotalScale += iThisScale;
	// this helps to decide the percent improvement later

	// POSITIVE COVER VALUE INDICATES THE COVER BENEFITS ME, NEGATIVE RESULT
	// MEANS IT BENEFITS THE OTHER GUY.
	// divide by a 100 to make the numbers more manageable and avoid 32-bit limit
	iCoverValue = (iMyPosValue - iHisPosValue) / 100;
	iCoverValue = (iCoverValue * iReductionFactor) / 100;

	//DebugCover(pMe, String("iMyPosValue %d iHisPosValue %d", iMyPosValue, iHisPosValue));
	//DebugCover(pMe, String("iReductionFactor %d iThisScale %d iTotalScale %d iCoverValue %d", iReductionFactor, iThisScale, *iTotalScale, iCoverValue));

	return( iCoverValue );
}


UINT8 NumberOfTeamMatesAdjacent( SOLDIERTYPE * pSoldier, INT32 sGridNo )
{
	UINT8	ubLoop, ubCount, ubWhoIsThere;
	INT32	sTempGridNo;

	ubCount = 0;

	for( ubLoop = 0; ubLoop < NUM_WORLD_DIRECTIONS; ubLoop++ )
	{
		sTempGridNo = NewGridNo( sGridNo, DirectionInc( ubLoop ) );

		if ( sTempGridNo != sGridNo )
		{
			ubWhoIsThere = WhoIsThere2( sTempGridNo, pSoldier->pathing.bLevel );

			if ( ubWhoIsThere != NOBODY && ubWhoIsThere != pSoldier->ubID && MercPtrs[ ubWhoIsThere ]->bTeam == pSoldier->bTeam )
			{
				ubCount++;
			}
		}
	}

	return( ubCount );
}

extern UINT32 guiAILastCoverCounter;

INT32 FindBestNearbyCover(SOLDIERTYPE *pSoldier, INT32 *piPercentBetter, INT8 bReserveAP, BOOLEAN fAttack)
{
	DebugMsg(TOPIC_JA2AI,DBG_LEVEL_3,String("FindBestNearbyCover"));
	DebugAI(AI_MSG_INFO, pSoldier, String("FindBestNearbyCover bReserveAP %d fAttack %d", bReserveAP, fAttack));

	UINT32	uiLoop;
	INT32	iCurrentCoverValue, iCoverValue, iBestCoverValue;
	INT32	iCurrentScale = -1, iCoverScale = -1, iBestCoverScale = -1;
	INT32	iDistFromOrigin, iDistCoverFromOrigin;
	INT32	sGridNo, sBestCover = NOWHERE;
	INT32	iPathCost;	
	INT32	iMyThreatValue;
	INT32	iThreatRange;
	INT32	iMaxMoveTilesLeft, iSearchRange, iRoamRange;
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sOrigin;
	
	UINT16	usMovementMode;

	UINT8	ubBackgroundLightLevel;
	UINT8	ubBackgroundLightPercent = 0;
	UINT8	ubLightPercentDifference;
	BOOLEAN	fNight;

	// sevenfm
	UINT8	ubNearbyFriends;
	BOOLEAN fProneCover;
	BOOLEAN fSightCover;
	BOOLEAN fCanSeeEnemy;
	BOOLEAN fCanSeeThisEnemy;
	BOOLEAN fAnyCover;
	BOOLEAN fCanAttack;
	UINT8	ubDiff = SoldierDifficultyLevel( pSoldier );
	INT32	iRCD = RangeChangeDesire(pSoldier);

	BOOLEAN fOriginalProneCover;
	BOOLEAN fOriginalSightCover;
	BOOLEAN fOriginalAnyCover;
	UINT8	ubMinAPsForAttack = 0;
	INT32	sOpponentGridNo;
	INT8	bOpponentLevel;
	INT32	sClosestOpponent = ClosestKnownOpponent( pSoldier, &sOpponentGridNo, &bOpponentLevel );
	UINT8	ubFriendsBlack = CountFriendsBlack(pSoldier);
	INT16	sMyNewAP = AICalcActionPoints(pSoldier);

	BOOLEAN fDangerousSpot = DangerousSpot(pSoldier);
	BOOLEAN fSafeSpot = SafeSpot(pSoldier);
	BOOLEAN fUnderAttack = AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, FALSE);
	BOOLEAN fUnderSuccessfulAttack = AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, TRUE);
	BOOLEAN fWeOutnumber = AICheckWeOutnumber(pSoldier, sClosestOpponent);
	BOOLEAN fRushAttack = pSoldier->RushAttackAdvance();
	BOOLEAN fRetreat = pSoldier->RetreatActive();
	BOOLEAN fFlankingFriends = (CountFriendsFlankSameSpot(pSoldier, sClosestOpponent) > 0);
	BOOLEAN fFriendsBlack = (CountFriendsBlack(pSoldier, sClosestOpponent) > 0);
	BOOLEAN fSuccessfulAttack = AICheckSuccessfulAttack(pSoldier, TRUE);
	BOOLEAN fAggressiveAI = AggressiveAI(pSoldier);

	// sevenfm: no cover for tanks
	if( TANK(pSoldier) )
	{
		return NOWHERE;
	}

	// calculate APs needed to attack closest enemy	
	if( !TileIsOutOfBounds(sClosestOpponent) )
	{
		ubMinAPsForAttack = AIGunMinAPsToShoot(pSoldier) + APBPConstants[AP_CHANGE_FACING];
	}

	guiAILastCoverCounter = GetJA2Clock();
	//DebugCover(pSoldier, String("\n[%d] FindBestNearbyCover", pSoldier->ubID));
	//DebugCover(pSoldier, String("AP=%d GridNo=%d Level=%d", pSoldier->bActionPoints, pSoldier->sGridNo, pSoldier->pathing.bLevel));
	//DebugCover(pSoldier, String("difficulty=%d closest opponent=%d friends black=%d", ubDiff, sClosestOpponent, ubFriendsBlack));
	//DebugCover(pSoldier, String("reserve %d AP, fAttack %d", bReserveAP, fAttack));
	//DebugCover(pSoldier, String("ubMinAPsForAttack %d", ubMinAPsForAttack));

	// There's no cover when boxing!
	if (gTacticalStatus.bBoxingState == BOXING)
	{
		//DebugCover(pSoldier, String("no cover when boxing!"));
		return (NOWHERE);
	}

	if ( gbWorldSectorZ > 0 )
	{
		fNight = FALSE;
	}
	else
	{
		ubBackgroundLightLevel = GetTimeOfDayAmbientLightLevel();

		if ( ubBackgroundLightLevel < NORMAL_LIGHTLEVEL_DAY + 2 )
		{
			fNight = FALSE;
		}
		else
		{
			fNight = TRUE;
			ubBackgroundLightPercent = gGameExternalOptions.ubBrightnessVisionMod[ ubBackgroundLightLevel ];
			//ubBackgroundLightPercent = gbLightSighting[ 0 ][ ubBackgroundLightLevel ];
		}
	}

	//DebugCover(pSoldier, String("Night=%d", fNight));

	iBestCoverValue = -1;

	// BUILD A LIST OF THREATENING GRID #s FROM PERSONAL & PUBLIC opplists

	// sevenfm: always use max search range
	iSearchRange = AI_PATHCOST_RADIUS / 2;

	if (!gfTurnBasedAI || gfHiddenInterrupt || !AllowComplexAI(pSoldier))
	{
		// don't search so far in realtime
		iSearchRange /= 2;
	}

	usMovementMode = DetermineMovementMode( pSoldier, AI_ACTION_TAKE_COVER );
	//DebugAI(AI_MSG_INFO, pSoldier, String("FindBestNearbyCover: movement mode %d", usMovementMode));
	
	if (pSoldier->aiData.bAlertStatus >= STATUS_RED)			// if already in battle
	{
		// must be able to reach the cover, so it can't possibly be more than
		// action points left (rounded down) tiles away, since minimum
		iMaxMoveTilesLeft = max( 0, (pSoldier->bActionPoints - bReserveAP - MinAPsToStartMovement( pSoldier, usMovementMode )) / (APBPConstants[AP_MAXIMUM] / 25) );

		// if we can't go as far as the usual full search range
		if (iMaxMoveTilesLeft < iSearchRange)
		{
			// then limit the search range to only as far as we CAN go
			iSearchRange = iMaxMoveTilesLeft;
		}
	}

	//DebugAI(AI_MSG_INFO, pSoldier, String("iSearchRange %d iMaxMoveTilesLeft %d usMovementMode %d ubMinAPsForAttack %d", iSearchRange, iMaxMoveTilesLeft, usMovementMode, ubMinAPsForAttack));

	//DebugCover(pSoldier, String("search range=%d movement mode=%d", iSearchRange, usMovementMode));

	if (iSearchRange <= 0)
	{
		//DebugCover(pSoldier, String("zero search range!"));
		return(NOWHERE);
	}

	// calculate OUR OWN general threat value (not from any specific location)
	iMyThreatValue = CalcManThreatValue(pSoldier,NOWHERE,FALSE,pSoldier);
	//DebugCover(pSoldier, String("my threat value=%d", iMyThreatValue));

	// prepare threat list from known enemies
	PrepareThreatlist(pSoldier);
	//DebugCover(pSoldier, String("found total threats %d", guiThreatCnt));

	// if no known opponents were found to threaten us, can't worry about cover
	if (!guiThreatCnt)
	{
		//NameMessage(pSoldier,"has no threats - WON'T take cover");
		//DebugCover(pSoldier, String("no threats"));
		return(sBestCover);
	}

	// log threats
	for (uiLoop = 0; uiLoop < guiThreatCnt; uiLoop++)
	{
		//DebugCover(pSoldier, String("[%d] Spot %d Level %d AP %d Certainty %d Value %d Personal %d Public %d", Threat[uiLoop].pOpponent->ubID, Threat[uiLoop].sGridNo, Threat[uiLoop].bLevel, Threat[uiLoop].iAPs, Threat[uiLoop].iCertainty, Threat[uiLoop].iValue, Threat[uiLoop].bPersonalKnowledge, Threat[uiLoop].bPublicKnowledge));
	}

	// calculate our current cover value in the place we are now, since the
	// cover we are searching for must be better than what we have now!
	iCurrentCoverValue = 0;
	iCurrentScale = 0;

	// for every opponent that threatens, consider this spot's cover vs. him
	for (uiLoop = 0; uiLoop < guiThreatCnt; uiLoop++)
	{
		// if this threat is CURRENTLY within 20 tiles
		if (Threat[uiLoop].iOrigRange <= MAX_THREAT_RANGE)
		{
			// add this opponent's cover value to our current total cover value
			iCurrentCoverValue += CalcCoverValue(pSoldier,pSoldier->sGridNo,iMyThreatValue,pSoldier->bActionPoints,uiLoop,Threat[uiLoop].iOrigRange,&iCurrentScale);
		}
	}

	// check sight cover
	fProneCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE);
	fSightCover = SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_STAND, TRUE);
	fAnyCover = AnyCoverAtSpot(pSoldier, pSoldier->sGridNo);

	// sevenfm: remember prone cover
	fOriginalProneCover = fProneCover;
	fOriginalSightCover = fSightCover;
	fOriginalAnyCover = fAnyCover;

	// reduce cover for each person adjacent to this gridno who is on our team,
	// by 10% (so locations next to several people will be very much frowned upon
	// sevenfm: only apply penalty if no cover here
	if (!fAnyCover)
	{
		iCurrentCoverValue -= abs(iCurrentCoverValue) * NumberOfTeamMatesAdjacent(pSoldier, pSoldier->sGridNo) / 10;
	}	

	// sevenfm: when defending, prefer locations with sight cover
	if (fProneCover && !InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE) && !fRushAttack && !fAggressiveAI)
	{
		iCurrentCoverValue += abs(iCurrentCoverValue) / max(2, 2 * iRCD);
	}

	// penalty for smoke locations
	if( InSmoke(pSoldier->sGridNo, pSoldier->pathing.bLevel, FALSE) )
	{
		iCurrentCoverValue -= abs(iCurrentCoverValue) / 4;
	}

	// sevenfm: check for nearby friends, add bonus/penalty	
	// only apply penalty if no cover here
	if (!fAnyCover)
	{
		ubNearbyFriends = min(5, CountNearbyFriends(pSoldier, pSoldier->sGridNo, DAY_VISION_RANGE / 8));
		iCurrentCoverValue -= ubNearbyFriends * abs(iCurrentCoverValue) / (10 - ubDiff);
	}	

	// sevenfm: penalize locations with fresh corpses
	//if(GetNearestRottingCorpseAIWarning( pSoldier->sGridNo ) > 0)
	if( CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) )
	{
		iCurrentCoverValue -= abs(iCurrentCoverValue) / (8-ubDiff);
	}

	// bonus to location if no one attacked it
	if (pSoldier->aiData.bUnderFire &&
		fAnyCover &&
		!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
		//!AICheckUnderAttack(pSoldier, pSoldier->sGridNo, FALSE) &&
		!AICheckSpotUnderAttack(pSoldier, pSoldier->sGridNo, FALSE))
	{
		iCurrentCoverValue += abs(iCurrentCoverValue) / 8;
		/*if (AICheckUnderAttack(pSoldier, pSoldier->sGridNo, TRUE))
			iCurrentCoverValue += abs(iCurrentCoverValue) / 8;
		else
			iCurrentCoverValue += abs(iCurrentCoverValue) / 16;*/
	}

	// penalize rooms for mortar operator
	if( !AICheckUnderground() && 
		pSoldier->pathing.bLevel == 0 &&
		AIRoleMortarOperator(pSoldier) && 
		InARoom(pSoldier->sGridNo, NULL) )
	{
		iCurrentCoverValue -= abs(iCurrentCoverValue) / 2;
	}

	// sevenfm: penalize locations near red smoke
	iCurrentCoverValue -= abs(iCurrentCoverValue) * RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel) / 100;

	// sevenfm: for DEFENSIVE, bonus 25% in buildings
	if( pSoldier->aiData.bAttitude == DEFENSIVE &&
		InARoom(pSoldier->sGridNo, NULL) &&
		fProneCover )
	{
		iCurrentCoverValue += abs(iCurrentCoverValue) / 4;
	}

	// bonus from terrain camo
	if (fProneCover && TerrainCoverAtSpot(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) && !fRushAttack)
	{
		iCurrentCoverValue += abs(iCurrentCoverValue) / 8;
	}

	// sevenfm: penalize water
	if( Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) )
	{
		iCurrentCoverValue -= abs(iCurrentCoverValue) / 2;
	}

	// penalty near explosive structures
	if (FindNearbyExplosiveStructure(pSoldier->sGridNo, pSoldier->pathing.bLevel))
	{
		iCurrentCoverValue -= 4 * abs(iCurrentCoverValue) / (8 - ubDiff);
	}

	// penalty near roof edge
	if(pSoldier->pathing.bLevel > 0 && RoofEdge(pSoldier->sGridNo))
	{
		iCurrentCoverValue -= abs(iCurrentCoverValue) / 2;
	}

	//DebugCover(pSoldier, String("current cover=%d prone cover=%d", iCurrentCoverValue, fOriginalProneCover));

	// determine maximum horizontal limits
	sMaxLeft  = min(iSearchRange,(pSoldier->sGridNo % MAXCOL));
	sMaxRight = min(iSearchRange,MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp   = min(iSearchRange,(pSoldier->sGridNo / MAXROW));
	sMaxDown = min(iSearchRange,MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

	iRoamRange = RoamingRange(pSoldier,&sOrigin);

	//DebugAI(AI_MSG_INFO, pSoldier, String("iRoamRange %d sMaxLeft %d sMaxRight %d sMaxUp %d sMaxDown %d", iRoamRange, sMaxLeft, sMaxRight, sMaxUp, sMaxDown));
	//DebugCover(pSoldier, String("roaming range=%d original gridno=%d", iRoamRange, sOrigin));

	// if status isn't black (life & death combat), and roaming range is limited	
	if ((pSoldier->aiData.bAlertStatus != STATUS_BLACK) && (iRoamRange < MAX_ROAMING_RANGE) &&
		(!TileIsOutOfBounds(sOrigin)))
	{
		// must try to stay within or return to the point of origin
		iDistFromOrigin = SpacesAway(sOrigin,pSoldier->sGridNo);
	}
	else
	{
		// don't care how far from origin we go
		iDistFromOrigin = -1;
	}

	// the initial cover value to beat is our current cover value
	iBestCoverValue = iCurrentCoverValue;

	INT16 usAPBudget;

	if (pSoldier->aiData.bAlertStatus >= STATUS_RED)			// if already in battle
	{
		// to speed this up, tell PathAI to cancel any paths beyond our AP reach!
		usAPBudget = max(0, pSoldier->bActionPoints - bReserveAP);
	}
	else
	{
		// even if not under pressure, limit to 1 turn's travelling distance
		// hope this isn't too expensive...
		usAPBudget = max(0, pSoldier->CalcActionPoints( ) - bReserveAP);
	}

	if( usAPBudget < MinAPsToStartMovement(pSoldier, usMovementMode) )
	{
		//DebugCover(pSoldier, String("not enough AP to start movement"));
		return NOWHERE;
	}

	//DebugCover(pSoldier, String("AP budget %d", usAPBudget));
	gubNPCAPBudget = usAPBudget;

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8) iSearchRange;
	gusNPCMovementMode = usMovementMode;

	//DebugAI(AI_MSG_INFO, pSoldier, String("usAPBudget %d bReserveAP %d iCurrentCoverValue %d", usAPBudget, bReserveAP, iCurrentCoverValue));

	//DebugCover(pSoldier, String("BestCoverValue=%d Scale=%d AP budget=%d distance limit %d", iBestCoverValue, iCurrentScale, gubNPCAPBudget, gubNPCDistLimit));

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}
			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE_AND_APS, 0 );

	//DebugCover(pSoldier, String("FindBestPath finished"));

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

	// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			//DebugCover(pSoldier, String("\n[%d]", sGridNo));			

			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				//DebugCover(pSoldier, String("out of limits"));
				continue;
			}

			// if we are limited to staying/returning near our place of origin
			if (iDistFromOrigin != -1)
			{
				iDistCoverFromOrigin = SpacesAway(sOrigin,sGridNo);

				// if this is outside roaming range, and doesn't get us closer to it
				if ((iDistCoverFromOrigin > iRoamRange) &&
					(iDistFromOrigin <= iDistCoverFromOrigin))
				{
					//DebugCover(pSoldier, String("out of roaming range and don't getting closer"));
					continue;	// then we can't go there
				}
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				//DebugCover(pSoldier, String("not reachable"));
				continue;
			}						

			// ignore blacklisted spot
			if ( sGridNo == pSoldier->pathing.sBlackList )
			{
				//DebugCover(pSoldier, String("black list"));
				continue;
			}

			iPathCost = gubAIPathCosts[AI_PATHCOST_RADIUS + sXOffset][AI_PATHCOST_RADIUS + sYOffset];
			//DebugCover(pSoldier, String("path cost=%d", iPathCost));
			
			if( iPathCost > pSoldier->bActionPoints )
			{
				//DebugCover(pSoldier, String("path cost > AP"));
				continue;
			}

			//DebugAI(AI_MSG_INFO, pSoldier, String("check spot %d iPathCost %d AP %d", sGridNo, iPathCost, pSoldier->bActionPoints));

			if (InGas(pSoldier, sGridNo))
			{
				//DebugCover(pSoldier, String("in gas"));
				continue;
			}

			// sevenfm: avoid deep water if not in deep water already
			if (DeepWater(sGridNo, pSoldier->pathing.bLevel) &&
				!DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel))
			{
				//DebugCover(pSoldier, String("deep water"));
				continue;
			}

			// sevenfm: avoid water if not in water already
			if (Water(sGridNo, pSoldier->pathing.bLevel) &&
				!Water(pSoldier->sGridNo, pSoldier->pathing.bLevel) )
			{
				//DebugCover(pSoldier, String("water"));
				continue;
			}

			// sevenfm: avoid tiles near bombs
			if (FindBombNearby(pSoldier, sGridNo, BOMB_DETECTION_RANGE))
			{
				//DebugCover(pSoldier, String("found bomb"));
				continue;
			}

			// avoid taking cover near explosive structures
			if (FindNearbyExplosiveStructure(sGridNo, pSoldier->pathing.bLevel) && !FindNearbyExplosiveStructure(pSoldier->sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// sevenfm: avoid staying at north edge
			if (NorthSpot(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// sevenfm: calculate AP cost
			/*iPathCost = EstimatePlotPath( pSoldier, sGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
			//DebugCover(pSoldier, String("estimate path cost=%d", iPathCost));
			
			// check that location is reachable and within AP limit
			if( iPathCost == 0 )
			{
				//DebugCover(pSoldier, String("cannot reach, skip location"));
				continue;
			}
			if( iPathCost > usAPBudget )
			{
				//DebugCover(pSoldier, String("path cost exceeds AP limit %d, skip location", usAPBudget));
				continue;	
			}*/

			fProneCover = SightCoverAtSpot(pSoldier, sGridNo, ANIM_PRONE, FALSE);
			//DebugCover(pSoldier, String("prone cover %d", fProneCover));
			fSightCover = SightCoverAtSpot(pSoldier, sGridNo, ANIM_STAND, TRUE);
			//DebugCover(pSoldier, String("sight cover %d", fSightCover));
			fAnyCover = AnyCoverAtSpot(pSoldier, sGridNo);
			//DebugCover(pSoldier, String("any cover %d", fAnyCover));

			// check retreat
			if (pSoldier->RetreatActive())
			{
				if( fOriginalAnyCover && !fAnyCover )
				{
					//DebugCover(pSoldier, String("retreat: no any cover"));
					continue;
				}
				if( fOriginalProneCover && !fProneCover )
				{
					//DebugCover(pSoldier, String("retreat: no prone cover"));
					continue;
				}
			}

			// sevenfm: don't move out of cover not at the start of the turn
			if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
				!fAnyCover &&
				fOriginalAnyCover &&
				(pSoldier->bTilesMoved > 0 || pSoldier->bInitialActionPoints - pSoldier->bActionPoints > APBPConstants[MAX_AP_CARRIED]) &&
				!fRushAttack &&
				!fAggressiveAI)
			{
				//DebugCover(pSoldier, String("moving out of cover not at the start of the turn, skip"));
				continue;
			}

			// sevenfm: in RED/BLACK state, skip location if no prone cover and not enough APs for attack
			if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&
				AICheckHasGun(pSoldier) &&
				!fRushAttack &&
				!fAggressiveAI &&
				!fProneCover &&
				!fAnyCover &&
				fOriginalProneCover &&
				pSoldier->bActionPoints >= ubMinAPsForAttack &&
				pSoldier->bActionPoints - iPathCost < ubMinAPsForAttack)
			{
				//DebugCover(pSoldier, String("in RED/BLACK state, no prone cover (has original prone cover)"));
				continue;
			}

			// skip location without unlimited sight cover if enemy is beyond weapon range
			/*if( pSoldier->aiData.bAlertStatus == STATUS_RED &&
				!pSoldier->RushAttackAdvance() &&
				fOriginalSightCover &&
				!fSightCover &&
				pSoldier->aiData.bOrders != SEEKENEMY &&
				pSoldier->aiData.bUnderFire &&
				!TileIsOutOfBounds(sClosestOpponent) &&
				PythSpacesAway(sGridNo, sClosestOpponent) > AIGunRange(pSoldier) + DAY_VISION_RANGE / 2 &&
				!CountFriendsBlack(pSoldier, sClosestOpponent) )
			{
				//DebugCover(pSoldier, String("in RED state, no unlimited sight cover (has original sight cover) and enemy is beyond weapon range"));
				continue;
			}*/

			// sevenfm: in RED state, skip locations with fresh corpses
			if (pSoldier->aiData.bAlertStatus == STATUS_RED &&
				!fRushAttack &&
				!fAggressiveAI &&
				!InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE) &&
				CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel) &&
				!CorpseWarning(pSoldier, pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
				(fOriginalProneCover || fOriginalAnyCover || InARoom(pSoldier->sGridNo, NULL) && pSoldier->pathing.bLevel == 0) &&
				!pSoldier->aiData.bUnderFire &&
				!TileIsOutOfBounds(sClosestOpponent) &&
				!CountFriendsBlack(pSoldier, sClosestOpponent) &&
				!AICheckSuccessfulAttack(pSoldier, TRUE))
			{
				//DebugCover(pSoldier, String("in RED state, skip locations with fresh corpses"));
				continue;
			}

			// sevenfm: in RED state, skip light at night
			if (pSoldier->aiData.bAlertStatus == STATUS_RED &&
				!fRushAttack &&
				!InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE) &&
				InLightAtNight(sGridNo, pSoldier->pathing.bLevel) &&
				!InLightAtNight(pSoldier->sGridNo, pSoldier->pathing.bLevel) &&
				(fOriginalProneCover || fOriginalAnyCover || InARoom(pSoldier->sGridNo, NULL) && pSoldier->pathing.bLevel == 0) &&
				!pSoldier->aiData.bUnderFire &&
				!TileIsOutOfBounds(sClosestOpponent) &&
				!CountFriendsBlack(pSoldier, sClosestOpponent))
			{
				//DebugCover(pSoldier, String("in RED state, skip locations with fresh corpses"));
				continue;
			}

			// zombies only try to hide
			if ( pSoldier->IsZombie() && !fSightCover )
			{
				//DebugCover(pSoldier, String("zombie: no sight cover at new spot, skip"));
				continue;
			}

			// avoid moving into red smoke
			if (RedSmokeDanger(sGridNo, pSoldier->pathing.bLevel) && !RedSmokeDanger(pSoldier->sGridNo, pSoldier->pathing.bLevel))
			{
				//DebugCover(pSoldier, String("moving into red smoke, skip"));
				continue;
			}

			// OK, this place shows potential.	How useful is it as cover?
			// EVALUATE EACH GRID #, remembering the BEST PROTECTED ONE
			iCoverValue = 0;
			iCoverScale = 0;

			// sevenfm: check sight cover
			fCanSeeEnemy = FALSE;
			fCanAttack = FALSE;

			//StartCachingLOS();
			// for every opponent that threatens, consider this spot's cover vs. him
			for (uiLoop = 0; uiLoop < guiThreatCnt; uiLoop++)
			{
				// calculate the range we would be at from this opponent
				iThreatRange = GetRangeInCellCoordsFromGridNoDiff( sGridNo, Threat[uiLoop].sGridNo );

				// if this threat would be within 20 tiles, count it
				if (iThreatRange <= MAX_THREAT_RANGE)
				{
					iCoverValue += CalcCoverValue( pSoldier, sGridNo, iMyThreatValue, pSoldier->bActionPoints - iPathCost, uiLoop, iThreatRange, &iCoverScale);
				}

				fCanSeeThisEnemy = FALSE;
				// check if soldier will be able to see any recently seen/heard opponent at new spot
				if( (Threat[uiLoop].bPersonalKnowledge == SEEN_CURRENTLY ||	Threat[uiLoop].bPersonalKnowledge == SEEN_THIS_TURN || Threat[uiLoop].bPersonalKnowledge == HEARD_THIS_TURN || Threat[uiLoop].bPublicKnowledge == SEEN_CURRENTLY ||
					(Threat[uiLoop].bPersonalKnowledge == SEEN_LAST_TURN || Threat[uiLoop].bPersonalKnowledge == HEARD_LAST_TURN) && Threat[uiLoop].sGridNo == Threat[uiLoop].pOpponent->sGridNo && Threat[uiLoop].bLevel == Threat[uiLoop].pOpponent->pathing.bLevel || Threat[uiLoop].pOpponent->ubID == pSoldier->ubPreviousAttackerID) &&
					LOS_SoldierPos(pSoldier, sGridNo, pSoldier->pathing.bLevel, ANIM_STAND, Threat[uiLoop].pOpponent, Threat[uiLoop].sGridNo, Threat[uiLoop].bLevel, Threat[uiLoop].pOpponent->AnimEndHeight(), LOS_NORMAL, CALC_FROM_ALL_DIRS))
				{
					fCanSeeThisEnemy = TRUE;
					fCanSeeEnemy = TRUE;
				}

				if( AICheckHasGun(pSoldier) &&
					PythSpacesAway(Threat[uiLoop].sGridNo, sGridNo) <= AIGunRange(pSoldier) * 3 / 2 &&
					fCanSeeThisEnemy )
				{
					fCanAttack = TRUE;
				}
			}
			//StopCachingLOS();

			//DebugCover(pSoldier, String("cover against opponents %d scale %d", iCoverValue, iCoverScale));			

			// if we plan to attack, skip locations where we cannot see enemy
			if( fAttack &&
				!fCanAttack)
			{
				//DebugCover(pSoldier, String("searching cover for attack, cannot see any enemy at new spot, skip"));
				continue;
			}

			// reduce cover for each person adjacent to this gridno who is on our team,
			// by 10% (so locations next to several people will be very much frowned upon			
			// sevenfm: only apply penalty if no cover here
			if (!fAnyCover)
			{
				iCoverValue -= abs(iCoverValue) * NumberOfTeamMatesAdjacent(pSoldier, sGridNo) / 10;
				//DebugCover(pSoldier, String("adjacent teammates: cover %d", iCoverValue));
			}

			// sevenfm: when defending, prefer locations with sight cover
			if (fProneCover && !InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE) && !fRushAttack && !fAggressiveAI)
			{
				iCoverValue += abs(iCoverValue) / max(2, 2 * iRCD);
				//DebugCover(pSoldier, String("prone sight cover: cover %d", iCoverValue));
			}

			// penalty for smoke locations
			if( InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE) )
			{
				iCoverValue -= abs(iCoverValue) / 4;
			}

			// sevenfm: check for nearby friends in 5 radius, add bonus/penalty 10%
			// only apply penalty if no cover here
			if (!fAnyCover)
			{
				ubNearbyFriends = min(5, CountNearbyFriends(pSoldier, sGridNo, DAY_VISION_RANGE / 8));
				iCoverValue -= ubNearbyFriends * abs(iCoverValue) / (10 - ubDiff);
			}			

			// sevenfm: penalize locations with fresh corpses
			if( CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel) )
			{
				iCoverValue -= abs(iCoverValue) / (8-ubDiff);
				//DebugCover(pSoldier, String("corpse warning: cover %d", iCoverValue));
			}

			// bonus to location if no one attacked it
			if (pSoldier->aiData.bUnderFire &&
				fAnyCover &&
				!CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel) &&
				//!AICheckUnderAttack(pSoldier, sGridNo, FALSE) &&
				!AICheckSpotUnderAttack(pSoldier, sGridNo, FALSE))
			{
				iCoverValue += abs(iCoverValue) / 8;
				/*if (AICheckUnderAttack(pSoldier, sGridNo, TRUE))
					iCoverValue += abs(iCoverValue) / 8;
				else
					iCoverValue += abs(iCoverValue) / 16;*/
				//DebugCover(pSoldier, String("spot not under attack: cover %d", iCoverValue));
			}

			// penalize rooms for mortar operator
			if( !AICheckUnderground() && 
				pSoldier->pathing.bLevel == 0 &&
				AIRoleMortarOperator(pSoldier) && 
				InARoom(sGridNo, NULL) )
			{
				iCoverValue -= abs(iCoverValue) / 2;
			}

			// sevenfm: penalize locations near red smoke			
			iCoverValue -= abs(iCoverValue) * RedSmokeDanger(sGridNo, pSoldier->pathing.bLevel) / 100;
			//DebugCover(pSoldier, String("red smoke: cover %d", iCoverValue));

			// sevenfm: for DEFENSIVE, bonus 25% in buildings
			if( pSoldier->aiData.bAttitude == DEFENSIVE &&
				InARoom(sGridNo, NULL) &&
				fProneCover )
			{
				iCoverValue += abs(iCoverValue) / 4;
				//DebugCover(pSoldier, String("room DEFENSIVE: cover %d", iCoverValue));
			}

			// bonus from terrain camo
			if (fProneCover && TerrainCoverAtSpot(pSoldier, sGridNo, pSoldier->pathing.bLevel) && !fRushAttack && !fAggressiveAI)
			{
				iCoverValue += abs(iCoverValue) / 8;
			}

			// sevenfm: penalize water
			if( Water(sGridNo, pSoldier->pathing.bLevel) )
			{
				iCoverValue -= abs(iCoverValue) / 2;
				//DebugCover(pSoldier, String("water: cover %d", iCoverValue));
			}

			// penalty near explosive structures
			if (FindNearbyExplosiveStructure(sGridNo, pSoldier->pathing.bLevel))
			{
				iCoverValue -= 4 * abs(iCoverValue) / (8 - ubDiff);
			}

			// penalty near roof edge
			if (pSoldier->pathing.bLevel > 0 && RoofEdge(sGridNo))
			{
				iCoverValue -= abs(iCoverValue) / 2;
			}

			if ( fNight && !( InARoom( sGridNo, NULL ) ) ) // ignore in buildings in case placed there
			{
				// reduce cover at nighttime based on how bright the light is at that location
				// using the difference in sighting distance between the background and the
				// light for this tile
				//ubLightPercentDifference = (gbLightSighting[ 0 ][ LightTrueLevel( sGridNo, pSoldier->pathing.bLevel ) ] - ubBackgroundLightPercent );
				ubLightPercentDifference = (gGameExternalOptions.ubBrightnessVisionMod[ LightTrueLevel( sGridNo, pSoldier->pathing.bLevel ) ] - ubBackgroundLightPercent );

				if ( iCoverValue >= 0 )
				{
					iCoverValue -= (iCoverValue / 100) * ubLightPercentDifference;
				}
				else
				{
					iCoverValue += (iCoverValue / 100) * ubLightPercentDifference;
				}
				//DebugCover(pSoldier, String("light at night: cover %d", iCoverValue));
			}

			//DebugCover(pSoldier, String("cover value=%d", iCoverValue));

			// if this is better than the best place found so far

			if (iCoverValue > iBestCoverValue)
			{
				// remember it instead
				sBestCover = sGridNo;
				iBestCoverValue = iCoverValue;
				iBestCoverScale = iCoverScale;

				//DebugCover(pSoldier, String("BestCoverValue=%d Scale=%d", iBestCoverScale, iBestCoverScale));
			}
		}
	}

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

#if defined( _DEBUG ) && !defined( PATHAI_VISIBLE_DEBUG )
	if (gfDisplayCoverValues)
	{
		// do a locate?
		LocateSoldier( pSoldier->ubID, SETLOCATORFAST );
		gsBestCover = sBestCover;
		SetRenderFlags( RENDER_FLAG_FULL );
		RenderWorld();
		RenderCoverDebug( );
		InvalidateScreen( );
		EndFrameBufferRender();
		RefreshScreen( NULL );
		/*
		iLoop = GetJA2Clock();
		do
		{

		} while( ( GetJA2Clock( ) - iLoop ) < 2000 );
		*/
	}
#endif

	INT32 iMinPercentbetter = MIN_PERCENT_BETTER;
	/*if (pSoldier->SkipCoverCheck())
	{
		iMinPercentbetter *= 2;
	}*/
	if (pSoldier->aiData.bAlertStatus < STATUS_BLACK)
	{
		iMinPercentbetter += iMinPercentbetter * pSoldier->usSkillCounter[SOLDIER_COUNTER_COVER];
	}	

	// if a better cover location was found	
	if (!TileIsOutOfBounds(sBestCover))
	{
#if defined( _DEBUG ) && !defined( PATHAI_VISIBLE_DEBUG )
		gsBestCover = sBestCover;
#endif
		// cover values already take the AP cost of getting there into account in
		// a BIG way, so no need to worry about that here, even small improvements
		// are actually very significant once we get our APs back (if we live!)
		*piPercentBetter = CalcPercentBetter(iCurrentCoverValue, iBestCoverValue, iCurrentScale, iBestCoverScale);

		// if best cover value found was at least 5% better than our current cover
		if (*piPercentBetter >= iMinPercentbetter || 
			iMinPercentbetter >= MIN_PERCENT_BETTER / 2 &&
			(AnyCoverAtSpot(pSoldier, sBestCover, FALSE) && !AnyCoverAtSpot(pSoldier, pSoldier->sGridNo, FALSE) ||
			SightCoverAtSpot(pSoldier, sBestCover, ANIM_PRONE, FALSE) && !SightCoverAtSpot(pSoldier, pSoldier->sGridNo, ANIM_PRONE, FALSE)))
		{
			//DebugCover(pSoldier, String("Found BestCoverValue=%d Scale=%d PercentBetter=%d\n", iBestCoverScale, iBestCoverScale, *piPercentBetter));
			return(sBestCover);       // return the gridno of that cover
		}
	}

	//DebugCover(pSoldier, String("Not found"));
	return(NOWHERE);		// return that no suitable cover was found
}

INT32 FindSpotMaxDistFromOpponents(SOLDIERTYPE *pSoldier)
{
	INT32	sGridNo;
	INT32	sBestSpot = NOWHERE;
	UINT32	uiLoop;
	INT32	iThreatRange;
	INT32	iClosestThreatRange = 4000;
	INT32	iSpotClosestThreatRange;
	INT32	sThreatLoc, sThreatGridNo[MAXMERCS];
	UINT32	uiThreatCnt = 0;
	INT32	iSearchRange;
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	//INT8	bEscapeDirection, bBestEscapeDirection = -1;
	SOLDIERTYPE *pOpponent;
	INT32	sOrigin;
	INT32	iRoamRange;

	// BUILD A LIST OF THREATENING GRID #s FROM PERSONAL & PUBLIC opplistS

	// look through all opponents for those we know of
	for (uiLoop = 0; uiLoop < guiNumMercSlots; uiLoop++)
	{
		pOpponent = MercSlots[ uiLoop ];

		// if this merc is inactive, at base, on assignment, dead, unconscious
		if (!pOpponent || (pOpponent->stats.bLife < OKLIFE))
		{
			continue;			// next merc
		}

		if( !ValidOpponent(pSoldier, pOpponent) )
		{
			continue;
		}

		if( Knowledge(pSoldier, pOpponent->ubID) == NOT_HEARD_OR_SEEN )
		{
			continue;			// check next opponent
		}

		// if the opponent is no threat at all for some reason
		if (CalcManThreatValue(pOpponent,pSoldier->sGridNo,FALSE,pSoldier) == -999)
		{
			continue;			// check next opponent
		}

		sThreatLoc = KnownLocation(pSoldier, pOpponent->ubID);

		if( TileIsOutOfBounds(sThreatLoc) )
		{
			continue;			// check next opponent
		}

		// calculate how far away this threat is (in adjusted pixels)
		iThreatRange = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo, sThreatLoc );

		if (iThreatRange < iClosestThreatRange)
		{
			iClosestThreatRange = iThreatRange;
		}

		// remember this threat's gridno
		sThreatGridNo[uiThreatCnt] = sThreatLoc;
		uiThreatCnt++;
	}

	// if no known opponents were found to threaten us, can't worry about them
	if (!uiThreatCnt)
	{
		return( sBestSpot );
	}

	// get roaming range here; for civilians, running away is limited by roam range
	if ( pSoldier->bTeam == CIV_TEAM )
	{
		iRoamRange = RoamingRange( pSoldier, &sOrigin );
		if ( iRoamRange == 0 )
		{
			return( sBestSpot );
		}
	}
	else
	{
		// dummy values
		iRoamRange = 100;
		sOrigin = pSoldier->sGridNo;
	}

	// DETERMINE CO-ORDINATE LIMITS OF SQUARE AREA TO BE CHECKED
	// THIS IS A LOT QUICKER THAN COVER, SO DO A LARGER AREA, NOT AFFECTED BY
	// DIFFICULTY SETTINGS...

	// sevenfm:
	iSearchRange = AI_PATHCOST_RADIUS / 2;
	gubNPCAPBudget = pSoldier->bActionPoints;

	if (!gfTurnBasedAI || gfHiddenInterrupt)
	{
		// don't search so far in realtime
		iSearchRange /= 2;
		gubNPCAPBudget /= 2;
	}

	// assume we have to stand up!
	// use the min macro here to make sure we don't wrap the UINT8 to 255...
	// Lesh: for some reason this code still allows to wrap UINT8 at low values of gubNPCAPBudget
	//gubNPCAPBudget = 	gubNPCAPBudget = min( gubNPCAPBudget, gubNPCAPBudget - GetAPsToChangeStance( pSoldier, ANIM_STAND ) );
	// Lesh: will be using this form
	if ( gubNPCAPBudget > GetAPsToChangeStance( pSoldier, ANIM_STAND ) )
		gubNPCAPBudget = gubNPCAPBudget - (UINT8) GetAPsToChangeStance( pSoldier, ANIM_STAND );

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8) iSearchRange;

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, DetermineMovementMode( pSoldier, AI_ACTION_RUN_AWAY ), COPYREACHABLE, 0 );//dnl ch50 121009

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
	//dnl ch58 170813 also don't use last two locations to avoid looping same decisions per turn
	if(!TileIsOutOfBounds(pSoldier->sLastTwoLocations[0]))
		gpWorldLevelData[pSoldier->sLastTwoLocations[0]].usFlags &= ~(MAPELEMENT_REACHABLE);
	else if(!TileIsOutOfBounds(pSoldier->sLastTwoLocations[1]))
		gpWorldLevelData[pSoldier->sLastTwoLocations[1]].usFlags &= ~(MAPELEMENT_REACHABLE);

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if ( sGridNo == pSoldier->pathing.sBlackList )
			{
				continue;
			}

			if ( pSoldier->bTeam == CIV_TEAM )
			{
				iRoamRange = RoamingRange( pSoldier, &sOrigin );
				if ( PythSpacesAway( sOrigin, sGridNo ) > iRoamRange )
				{
					continue;
				}
			}

			// check that spot is allowed
			if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, NOWATER, 0))
			{
				continue;		// skip on to the next potential grid
			}

			if (SpotDangerLevel(pSoldier, sGridNo) > SpotDangerLevel(pSoldier, pSoldier->sGridNo))
			{
				continue;
			}

			// OK, this place shows potential.	How useful is it as cover?

			iSpotClosestThreatRange = 4000;

			/*if ( pSoldier->bTeam == ENEMY_TEAM && GridNoOnEdgeOfMap( sGridNo, &bEscapeDirection ) && EscapeDirectionIsValid( &bEscapeDirection ) )
			{
				// We can escape!	This is better than anything else except a closer spot which we can
				// cross over from.

				// Subtract the straight-line distance from our location to this one as an estimate of
				// path cost and for looks...

				// The edge spot closest to us which is on the edge will have the highest value, so
				// it will be picked over locations further away.
				// Only reachable gridnos will be picked so this should hopefully look okay
				iSpotClosestThreatRange -= CELL_X_SIZE * PythSpacesAway(pSoldier->sGridNo, sGridNo);
			}
			else
			{
				bEscapeDirection = -1;
				// for every opponent that threatens, consider this spot's cover vs. him
				for (uiLoop = 0; uiLoop < uiThreatCnt; uiLoop++)
				{
					//iThreatRange = AdjPixelsAway(CenterX(sGridNo),CenterY(sGridNo), CenterX(sThreatGridNo[iLoop]),CenterY(sThreatGridNo[iLoop]));
					iThreatRange = GetRangeInCellCoordsFromGridNoDiff( sGridNo, sThreatGridNo[uiLoop] );
					if (iThreatRange < iSpotClosestThreatRange)
					{
						iSpotClosestThreatRange = iThreatRange;
					}
				}
			}*/

			for (uiLoop = 0; uiLoop < uiThreatCnt; uiLoop++)
			{
				iThreatRange = GetRangeInCellCoordsFromGridNoDiff(sGridNo, sThreatGridNo[uiLoop]);
				if (iThreatRange < iSpotClosestThreatRange)
				{
					iSpotClosestThreatRange = iThreatRange;
				}
			}

			// if this is better than the best place found so far
			// (i.e. the closest guy would be farther away than previously)
			if (iSpotClosestThreatRange > iClosestThreatRange)
			{
				// remember it instead
				iClosestThreatRange = iSpotClosestThreatRange;

				sBestSpot = sGridNo;
				//bBestEscapeDirection = bEscapeDirection;
			}
		}

	}

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

	/*if (bBestEscapeDirection != -1)
	{
		// Woohoo!	We can escape!	Fake some stuff with the quote-related actions
		pSoldier->ubQuoteActionID = GetTraversalQuoteActionID( bBestEscapeDirection );
	}*/

	return( sBestSpot );
}

INT32 FindNearestUngassedLand(SOLDIERTYPE *pSoldier)
{
	INT32 sGridNo, sClosestLand = NOWHERE, sPathCost, sShortestPath = 1000;
	INT16 sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32 iSearchRange, iIgnoreRange = 0;
	INT16 sDistance, sOriginalDistance;
	INT32 iMaxSearchRange = 35;
	UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_LEAVE_WATER_GAS);
	BOOLEAN fFoundReachable = FALSE;
	BOOLEAN fAvoidEnemy = FALSE;
	UINT8 ubSpotDangerLevel = 0;
	UINT8 ubCurrentSpotDangerLevel = SpotDangerLevel(pSoldier, pSoldier->sGridNo);

	sOriginalDistance = DistanceToClosestActiveOpponent(pSoldier, pSoldier->sGridNo);

	if (pSoldier->aiData.bAlertStatus >= STATUS_RED &&	// only when alert is raised
		!pSoldier->aiData.bNeutral &&					// neutral folks don't care about enemies
		pSoldier->aiData.bOrders != SEEKENEMY &&		// SEEKENEMY should move to closest opponent
		sOriginalDistance >= 0 &&						// check that there is active known opponent
		!InGas(pSoldier, pSoldier->sGridNo) &&			// when in gas/fire, always choose closest spot
		!FindBombNearby(pSoldier, pSoldier->sGridNo, BOMB_DETECTION_RANGE))
	{
		fAvoidEnemy = TRUE;
	}

	if (!AllowComplexAI(pSoldier))
	{
		iMaxSearchRange = 25;
	}

	if (!gfTurnBasedAI)
	{
		iMaxSearchRange = 15;
	}

	// start with a small search area, and expand it if we're unsuccessful
	// this should almost never need to search farther than 5 or 10 squares...
	// sevenfm: stop searching if there is no reachable spots withing 5 tiles
	for (iSearchRange = 5; iSearchRange <= iMaxSearchRange && (fFoundReachable || iSearchRange <= 5); iIgnoreRange = iSearchRange, iSearchRange += 10)
	{
		//NumMessage("Trying iSearchRange = ", iSearchRange);

		// determine maximum horizontal limits
		sMaxLeft  = min(iSearchRange,(pSoldier->sGridNo % MAXCOL));
		//NumMessage("sMaxLeft = ",sMaxLeft);
		sMaxRight = min(iSearchRange,MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
		//NumMessage("sMaxRight = ",sMaxRight);

		// determine maximum vertical limits
		sMaxUp   = min(iSearchRange,(pSoldier->sGridNo / MAXROW));
		//NumMessage("sMaxUp = ",sMaxUp);
		sMaxDown = min(iSearchRange,MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
		//NumMessage("sMaxDown = ",sMaxDown);

		// Call FindBestPath to set flags in all locations that we can
		// walk into within range.	We have to set some things up first...		

		// reset the "reachable" flags in the region we're looking at
		for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
		{
			for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
			{
				sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
				if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
				{
					continue;
				}

				gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
			}
		}

		//gubNPCAPBudget = pSoldier->bActionPoints;
		gubNPCAPBudget = 0;
		gubNPCDistLimit = (UINT8)iSearchRange;		
		FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE, 0 );	//dnl ch50 071009
		gubNPCAPBudget = 0;
		gubNPCDistLimit = 0;

		// Turn off the "reachable" flag for his current location
		// so we don't consider it
		gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

		// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
		for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
		{
			for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
			{
				// sevenfm: optimization - skip spots checked in previous loop
				if (sXOffset <= iIgnoreRange && sXOffset >= -iIgnoreRange && sYOffset <= iIgnoreRange && sYOffset >= -iIgnoreRange)
				{
					continue;
				}

				// calculate the next potential gridno
				sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
				//NumMessage("Testing gridno #",gridno);
				if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
				{
					continue;
				}

				if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
				{
					continue;
				}

				// found reachable spot
				fFoundReachable = TRUE;

				// ignore blacklisted spot
				if ( sGridNo == pSoldier->pathing.sBlackList )
				{
					continue;
				}

				if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, WATEROK, 0))
				{
					continue;		// skip on to the next potential grid
				}

				ubSpotDangerLevel = SpotDangerLevel(pSoldier, sGridNo);
				if (ubSpotDangerLevel > 0 && ubSpotDangerLevel >= ubCurrentSpotDangerLevel)
				{
					continue;
				}

				sPathCost = PlotPath(pSoldier, sGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, pSoldier->bReverse, 0);

				// check if spot is reachable
				if (sPathCost == 0)
				{
					continue;
				}
				
				if (fAvoidEnemy)
				{
					sDistance = DistanceToClosestActiveOpponent(pSoldier, pSoldier->sGridNo);

					// penalty if moving closer to enemy
					if (sDistance >= 0 && sOriginalDistance >= 0 && sDistance < sOriginalDistance)
					{
						//sPathCost += APBPConstants[AP_MAXIMUM];
						sPathCost += (sOriginalDistance - sDistance) * (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK]);
					}
				}

				// if this path is shorter than the one to the closest land found so far
				if (sPathCost < sShortestPath)
				{
					// remember it instead
					sShortestPath = sPathCost;
					//NumMessage("New shortest route = ",shortestPath);

					sClosestLand = sGridNo;
					//NumMessage("New closest land at grid = ",closestLand);
				}
			}
		}

		// if we found a piece of land in this search area		
		if (!TileIsOutOfBounds(sClosestLand))	// quit now, no need to look any farther
			break;
	}

	//NumMessage("closestLand = ",closestLand);
	return(sClosestLand);
}

INT32 FindNearbyDarkerSpot( SOLDIERTYPE *pSoldier )
{
	INT32 sGridNo, sClosestSpot = NOWHERE, sPathCost;
	INT32	iSpotValue, iBestSpotValue = 1000;
	INT16 sMaxLeft,sMaxRight,sMaxUp,sMaxDown,sXOffset,sYOffset;
	INT32 iSearchRange, iIgnoreRange = 0;
	INT32 iMaxSearchRange = 35;
	INT8 bLightLevel, bCurrLightLevel, bLightDiff;
	INT32 iRoamRange;
	INT32 sOrigin;
	UINT16 usMovementMode = DetermineMovementMode(pSoldier, AI_ACTION_LEAVE_WATER_GAS);
	BOOLEAN fFoundReachable = FALSE;
	BOOLEAN fAvoidEnemy = FALSE;
	INT32	sDistance, sOriginalDistance;

	bCurrLightLevel = LightTrueLevel( pSoldier->sGridNo, pSoldier->pathing.bLevel );
	sOriginalDistance = DistanceToClosestActiveOpponent(pSoldier, pSoldier->sGridNo);

	if (!pSoldier->aiData.bNeutral &&				// neutral folks don't care about enemies
		pSoldier->aiData.bOrders != SEEKENEMY &&	// SEEKENEMY should move to closest opponent
		sOriginalDistance >= 0 )					// check that there is active known opponent
	{
		fAvoidEnemy = TRUE;
	}

	iRoamRange = RoamingRange( pSoldier, &sOrigin );

	if (!AllowComplexAI(pSoldier))
	{
		iMaxSearchRange = 25;
	}

	if (!gfTurnBasedAI)
	{
		iMaxSearchRange = 15;
	}

	// start with a small search area, and expand it if we're unsuccessful
	// this should almost never need to search farther than 5 or 10 squares...
	for (iSearchRange = 5; iSearchRange <= iMaxSearchRange && (fFoundReachable || iSearchRange <= 5); iIgnoreRange = iSearchRange, iSearchRange += 10)
	{
		// determine maximum horizontal limits
		sMaxLeft  = min(iSearchRange,(pSoldier->sGridNo % MAXCOL));
		//NumMessage("sMaxLeft = ",sMaxLeft);
		sMaxRight = min(iSearchRange,MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
		//NumMessage("sMaxRight = ",sMaxRight);

		// determine maximum vertical limits
		sMaxUp   = min(iSearchRange,(pSoldier->sGridNo / MAXROW));
		//NumMessage("sMaxUp = ",sMaxUp);
		sMaxDown = min(iSearchRange,MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
		//NumMessage("sMaxDown = ",sMaxDown);

		// Call FindBestPath to set flags in all locations that we can
		// walk into within range.	We have to set some things up first...

		// reset the "reachable" flags in the region we're looking at
		for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
		{
			for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
			{
				sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
				if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
				{
					continue;
				}

				gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
			}
		}

		//gubNPCAPBudget = pSoldier->bActionPoints;
		gubNPCAPBudget = 0;
		gubNPCDistLimit = (UINT8)iSearchRange;
		FindBestPath(pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE, 0);	//dnl ch50 071009
		gubNPCAPBudget = 0;
		gubNPCDistLimit = 0;

		// Turn off the "reachable" flag for his current location
		// so we don't consider it
		gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

		// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
		for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
		{
			for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
			{
				// sevenfm: optimization - skip spots checked in previous loop
				if (sXOffset <= iIgnoreRange && sXOffset >= -iIgnoreRange && sYOffset <= iIgnoreRange && sYOffset >= -iIgnoreRange)
				{
					continue;
				}

				// calculate the next potential gridno
				sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
				//NumMessage("Testing gridno #",gridno);
				if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
				{
					continue;
				}

				if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
				{
					continue;
				}

				fFoundReachable = TRUE;

				// ignore blacklisted spot
				if ( sGridNo == pSoldier->pathing.sBlackList )
				{
					continue;
				}

				// require this character to stay within their roam range
				if (PythSpacesAway(sOrigin, sGridNo) > iRoamRange)
				{
					continue;
				}				

				if (InLightAtNight(sGridNo, pSoldier->pathing.bLevel))
				{
					continue;
				}

				// screen out anything brighter than our current best spot
				bLightLevel = LightTrueLevel( sGridNo, pSoldier->pathing.bLevel );

				//bLightDiff = gbLightSighting[0][ bCurrLightLevel ] - gbLightSighting[0][ bLightLevel ];
				bLightDiff = gGameExternalOptions.ubBrightnessVisionMod[ bCurrLightLevel ] - gGameExternalOptions.ubBrightnessVisionMod[ bLightLevel ];
				// if the spot is darker than our current location, then bLightDiff > 0
				// plus ignore differences of just 1 light level
				if ( bLightDiff <= 1 )
				{
					continue;
				}

				if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, WATEROK, 0))
				{
					continue;		// skip on to the next potential grid
				}

				if (SpotDangerLevel(pSoldier, sGridNo) > 0 && SpotDangerLevel(pSoldier, sGridNo) >= SpotDangerLevel(pSoldier, pSoldier->sGridNo))
				{
					continue;
				}

				sPathCost = PlotPath(pSoldier, sGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, pSoldier->bReverse, 0);

				// check if spot is reachable
				if(sPathCost == 0)
				{
					continue;		// skip on to the next potential grid
				}

				if (fAvoidEnemy)
				{
					sDistance = DistanceToClosestActiveOpponent(pSoldier, pSoldier->sGridNo);

					// penalty if moving closer to enemy
					if (sDistance >= 0 && sOriginalDistance >= 0 && sDistance < sOriginalDistance)
					{
						//sPathCost += APBPConstants[AP_MAXIMUM];
						sPathCost += (sOriginalDistance - sDistance) * (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_WALK]);
					}
				}

				// decrease the "cost" of the spot by the amount of light/darkness
				iSpotValue = sPathCost * 2 - bLightDiff;

				if ( iSpotValue < iBestSpotValue )
				{
					// remember it instead
					iBestSpotValue = iSpotValue;
					//NumMessage("New shortest route = ",shortestPath);

					sClosestSpot = sGridNo;
					//NumMessage("New closest land at grid = ",closestLand);
				}
			}
		}

		// if we found a piece of land in this search area		
		if (!TileIsOutOfBounds(sClosestSpot))	// quit now, no need to look any farther
		{
			break;
		}
	}

	return(sClosestSpot);
}

#define MINIMUM_REQUIRED_STATUS 70

INT8 SearchForItems( SOLDIERTYPE * pSoldier, INT8 bReason, UINT16 usItem )
{
	DebugAI( AI_MSG_INFO, pSoldier, String("---SearchForItems [%d] bReason %d usItem %d", pSoldier->ubID, bReason, usItem) );

	INT32		iSearchRange;
	INT16		sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32		sGridNo;
	INT32		sBestSpot = NOWHERE;
	INT32		iTempValue, iValue, iBestValue = 0;
	ITEM_POOL	*pItemPool;
	OBJECTTYPE	*pObj;
	INVTYPE		*pItem;
	INT32		iItemIndex, iBestItemIndex;
	BOOLEAN		fDumbEnoughtoPickup = FALSE;

	// sevenfm
	UINT8		ubMinGunStatus = MINIMUM_REQUIRED_STATUS;
	BOOLEAN		fHasGun = FALSE;
	UINT8		ubDeadliness = 0;
	INT8		bGunSlot = FindAIUsableObjClass(pSoldier, IC_GUN);

	if (bGunSlot != NO_SLOT)
	{
		fHasGun = TRUE;
		ubDeadliness = Weapon[pSoldier->inv[bGunSlot].usItem].ubDeadliness;
	}

	// if we don't have a gun, try to pick up weapon even in bad status
	// sevenfm: search for weapon in case gun is not in hand
	if (!fHasGun)
	{
		ubMinGunStatus = 35;
	}

	iTempValue = -1;
	iItemIndex = iBestItemIndex = -1;

	// No fair picking up weapons while boxing!
	if (gTacticalStatus.bBoxingState == BOXING)
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---no picking up items for boxers!") );
		return AI_ACTION_NONE;
	}

	if (pSoldier->bActionPoints < GetBasicAPsToPickupItem( pSoldier ))
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---not enough AP!") );
		return( AI_ACTION_NONE );
	}

	if ( !IS_MERC_BODY_TYPE( pSoldier ) )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---not merc bodytype!") );
		return( AI_ACTION_NONE );
	}

	// sevenfm: use maximum search range
	if (gfTurnBasedAI)
		iSearchRange = TACTICAL_RANGE / 2;
	else
		iSearchRange = TACTICAL_RANGE / 4;

	if (!AllowComplexAI(pSoldier))
		iSearchRange = iSearchRange / 2;

	// sevenfm: limit search range if blinded
	if (pSoldier->bBlindedCounter)
		iSearchRange = 1;

	DebugAI( AI_MSG_INFO, pSoldier, String("---use search range %d", iSearchRange) );

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
	//NumMessage("sMaxDown = ",sMaxDown);

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8) iSearchRange;

	// set an AP limit too, to our APs less the cost of picking up an item
	// and less the cost of dropping an item since we might need to do that
	gubNPCAPBudget = pSoldier->bActionPoints - GetBasicAPsToPickupItem( pSoldier );

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, DetermineMovementMode( pSoldier, AI_ACTION_PICKUP_ITEM ), COPYREACHABLE, 0 );//dnl ch50 071009

	// Flugente: if the soldier is 'dumb enough', he may pick up certain items... which can be used to lure the AI into traps
	if ( pSoldier->stats.bWisdom < 70 )
	{
		DebugAI( AI_MSG_INFO, pSoldier, String("---dumb enough to pick up items") );
		fDumbEnoughtoPickup = TRUE;
	}

	// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (sGridNo != pSoldier->sGridNo && !LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, NOWATER, 0))
			{
				continue;		// skip on to the next potential grid
			}

			if (sGridNo != pSoldier->sGridNo && SpotDangerLevel(pSoldier, sGridNo) > SpotDangerLevel(pSoldier, pSoldier->sGridNo))
			{
				continue;
			}

			if ( (gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_ITEMPOOL_PRESENT)
				&& (gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE) )
			{
				// ignore blacklisted spot
				if ( sGridNo == pSoldier->pathing.sBlackList )
				{
					continue;
				}

				DebugAI( AI_MSG_INFO, pSoldier, String("check spot %d, found items", sGridNo) );

				iValue = 0;
				GetItemPool( sGridNo, &pItemPool, pSoldier->pathing.bLevel );
				switch( bReason )
				{
				case SEARCH_AMMO:
					// we are looking for ammo to match the gun in usItem
					DebugAI( AI_MSG_INFO, pSoldier, String("---SEARCH_AMMO") );
					while( pItemPool )
					{
						pObj = &(gWorldItems[ pItemPool->iItemIndex ].object);
						pItem = &(Item[pObj->usItem]);
						DebugAI( AI_MSG_INFO, pSoldier, String("---check item %d at %d status %d", pObj->usItem, sGridNo, (*pObj)[0]->data.objectStatus) );

						// sevenfm: don't search for guns if we should search for ammo
						/*if ( pItem->usItemClass == IC_GUN && (*pObj)[0]->data.objectStatus >= MINIMUM_REQUIRED_STATUS )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---gun has good status") );

							// maybe this gun has ammo (adjust for whether it is better than ours!)
							if ( (*pObj)[0]->data.gun.bGunAmmoStatus < 0 || (*pObj)[0]->data.gun.ubGunShotsLeft == 0 || (Item[pObj->usItem].fingerprintid && (*pObj)[0]->data.ubImprintID != NOBODY && (*pObj)[0]->data.ubImprintID != pSoldier->ubID) )
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("---gun has no ammo or bad fingerprint") );
								iTempValue = 0;
							}
							else
							{
								iTempValue = (*pObj)[0]->data.gun.ubGunShotsLeft * Weapon[pObj->usItem].ubDeadliness / Weapon[usItem].ubDeadliness;
							}
						}
						else */
						if ( ValidAmmoType( usItem, pObj->usItem ) && !(pObj->fFlags & OBJECT_AI_UNUSABLE) )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---good ammo") );
							iTempValue = TotalPoints( pObj );
						}
						else
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---not gun, not ammo - skip") );
							iTempValue = 0;
						}
						DebugAI( AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue) );
						if (iTempValue > iValue )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---select this ammo") );
							iValue = iTempValue;
							iItemIndex = pItemPool->iItemIndex;
						}
						pItemPool = pItemPool->pNext;
					}
					break;
				case SEARCH_WEAPONS:
					DebugAI( AI_MSG_INFO, pSoldier, String("---SEARCH_WEAPONS") );
					while( pItemPool )
					{
						pObj = &(gWorldItems[ pItemPool->iItemIndex ].object);
						pItem = &(Item[pObj->usItem]);
						DebugAI( AI_MSG_INFO, pSoldier, String("---check item %d at %d status %d", pObj->usItem, sGridNo, (*pObj)[0]->data.objectStatus) );

						// sevenfm: pick up gun even with bad status if we don't have any gun
						//if (pItem->usItemClass & IC_WEAPON && (*pObj)[0]->data.objectStatus >= MINIMUM_REQUIRED_STATUS )
						if (pItem->usItemClass & IC_WEAPON && (*pObj)[0]->data.objectStatus >= ubMinGunStatus && !(pObj->fFlags & OBJECT_AI_UNUSABLE) )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---weapon has good status") );

							if( (pItem->usItemClass & IC_GUN) && 
								((*pObj)[0]->data.gun.bGunAmmoStatus < 0 || (*pObj)[0]->data.gun.ubGunShotsLeft == 0 || (( Item[pObj->usItem].fingerprintid ) && (*pObj)[0]->data.ubImprintID != NOBODY && (*pObj)[0]->data.ubImprintID != pSoldier->ubID) ) )
							{
								// jammed or out of ammo, skip it!
								DebugAI( AI_MSG_INFO, pSoldier, String("---jammed or out of ammo, skip it!") );
								iTempValue = 0;
							}
							// sevenfm: take into account trait skills
							else if (fHasGun && gGameOptions.fNewTraitSystem)
							{
								if (NUM_SKILL_TRAITS(pSoldier, SNIPER_NT) > 1 && Weapon[pObj->usItem].ubWeaponType != GUN_SN_RIFLE ||
									HAS_SKILL_TRAIT(pSoldier, SNIPER_NT) && Weapon[pObj->usItem].ubWeaponType != GUN_SN_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_AS_RIFLE ||
									HAS_SKILL_TRAIT(pSoldier, AUTO_WEAPONS_NT) && Weapon[pObj->usItem].ubWeaponType != GUN_LMG && Weapon[pObj->usItem].ubWeaponType != GUN_AS_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_SMG ||
									HAS_SKILL_TRAIT(pSoldier, RANGER_NT) && Weapon[pObj->usItem].ubWeaponType != GUN_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_SHOTGUN)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("---weapon not matching soldier's skill trait, skip it!"));
									iTempValue = 0;
								}
							}
							//else if ( Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_WEAPON )
							// sevenfm: compare deadliness only if weapon in hand is a gun (not a launcher, knife etc...)
							// for example, we should pick up a gun if we hold a launcher in our hands
							else if (fHasGun)
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("---compare with gun in hand") );
								if (Weapon[pObj->usItem].ubDeadliness > ubDeadliness)
								{
									iTempValue = 100 + (Weapon[pObj->usItem].ubDeadliness - ubDeadliness);
								}
								else
								{
									iTempValue = 0;
								}
							}
							else
							{
								iTempValue = 200 + Weapon[pObj->usItem].ubDeadliness;
							}
						}
						else
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---not weapon or bad status") );
							iTempValue = 0;
						}
						DebugAI( AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue) );
						if (iTempValue > iValue )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---select this weapon") );
							iValue = iTempValue;
							iItemIndex = pItemPool->iItemIndex;
						}
						pItemPool = pItemPool->pNext;
					}
					break;
				default:
					DebugAI( AI_MSG_INFO, pSoldier, String("---search items") );
					while( pItemPool )
					{
						pObj = &(gWorldItems[ pItemPool->iItemIndex ].object);
						pItem = &(Item[pObj->usItem]);
						DebugAI( AI_MSG_INFO, pSoldier, String("---check item %d at %d status %d", pObj->usItem, sGridNo, (*pObj)[0]->data.objectStatus) );
						if ( pItem->usItemClass & IC_WEAPON && (*pObj)[0]->data.objectStatus >= MINIMUM_REQUIRED_STATUS && !(pObj->fFlags & OBJECT_AI_UNUSABLE))
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---gun has good status") );
							if( (pItem->usItemClass & IC_GUN) && 
								((*pObj)[0]->data.gun.bGunAmmoStatus < 0 || (*pObj)[0]->data.gun.ubGunShotsLeft == 0 || (( Item[pObj->usItem].fingerprintid ) && (*pObj)[0]->data.ubImprintID != NOBODY && (*pObj)[0]->data.ubImprintID != pSoldier->ubID) ) )
							{
								// jammed or out of ammo, skip it!
								DebugAI( AI_MSG_INFO, pSoldier, String("---jammed or out of ammo, skip it!") );
								iTempValue = 0;
							}
							// sevenfm: take into account trait skills
							else if (fHasGun && gGameOptions.fNewTraitSystem)
							{
								if (NUM_SKILL_TRAITS(pSoldier, SNIPER_NT) > 1 && Weapon[pObj->usItem].ubWeaponType != GUN_SN_RIFLE ||
									HAS_SKILL_TRAIT(pSoldier, SNIPER_NT) && Weapon[pObj->usItem].ubWeaponType != GUN_SN_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_AS_RIFLE ||
									HAS_SKILL_TRAIT(pSoldier, AUTO_WEAPONS_NT) && Weapon[pObj->usItem].ubWeaponType != GUN_LMG && Weapon[pObj->usItem].ubWeaponType != GUN_AS_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_SMG ||
									HAS_SKILL_TRAIT(pSoldier, RANGER_NT) && Weapon[pObj->usItem].ubWeaponType != GUN_RIFLE && Weapon[pObj->usItem].ubWeaponType != GUN_SHOTGUN)
								{
									DebugAI(AI_MSG_INFO, pSoldier, String("---weapon not matching soldier's skill trait, skip it!"));
									iTempValue = 0;
								}
							}
							else if (fHasGun)
							{
								DebugAI( AI_MSG_INFO, pSoldier, String("---compare with weapon in hand") );
								if (Weapon[pObj->usItem].ubDeadliness > ubDeadliness)
								{
									iTempValue = 100 + (Weapon[pObj->usItem].ubDeadliness - Weapon[pSoldier->inv[HANDPOS].usItem].ubDeadliness);
								}
								else
								{
									iTempValue = 0;
								}
							}
							else
							{
								iTempValue = 200 + Weapon[pObj->usItem].ubDeadliness;
							}
							DebugAI( AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue) );
						}
						else if	(pItem->usItemClass == IC_ARMOUR && (*pObj)[0]->data.objectStatus >= MINIMUM_REQUIRED_STATUS && !(pObj->fFlags & OBJECT_AI_UNUSABLE))
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---armour has good status") );

							switch( Armour[pItem->ubClassIndex].ubArmourClass )
							{
							case ARMOURCLASS_HELMET:
								DebugAI(AI_MSG_INFO, pSoldier, String("---ARMOURCLASS_HELMET"));
								if (pSoldier->inv[HELMETPOS].exists() == false)
								{
									iTempValue = 200 + EffectiveArmour( pObj );
								}
								else if ( EffectiveArmour( &(pSoldier->inv[HELMETPOS]) ) < EffectiveArmour( pObj ) )
								{
									iTempValue = 100 + (EffectiveArmour( pObj ) - EffectiveArmour( &(pSoldier->inv[HELMETPOS])));
								}
								else
								{
									iTempValue = 0;
								}
								DebugAI(AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue));
								break;
							case ARMOURCLASS_VEST:
								DebugAI(AI_MSG_INFO, pSoldier, String("---ARMOURCLASS_VEST"));
								if (pSoldier->inv[VESTPOS].exists() == false)
								{
									iTempValue = 200 + EffectiveArmour( pObj );
								}
								else if (EffectiveArmour(&(pSoldier->inv[VESTPOS])) < EffectiveArmour(pObj))
								{
									iTempValue = 100 + (EffectiveArmour(pObj) - EffectiveArmour(&(pSoldier->inv[VESTPOS])));
								}
								else
								{
									iTempValue = 0;
								}
								DebugAI(AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue));
								break;
							case ARMOURCLASS_LEGGINGS:
								DebugAI(AI_MSG_INFO, pSoldier, String("---ARMOURCLASS_LEGGINGS"));
								if (pSoldier->inv[LEGPOS].exists() == false)
								{
									iTempValue = 200 + EffectiveArmour( pObj );
								}
								else if (EffectiveArmour(&(pSoldier->inv[LEGPOS])) < EffectiveArmour(pObj))
								{
									iTempValue = 100 + (EffectiveArmour(pObj) - EffectiveArmour(&(pSoldier->inv[LEGPOS])));
								}
								else
								{
									iTempValue = 0;
								}
								DebugAI(AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue));
								break;
							default:
								// WANNE: Fix a vanilla bug: When an enemy soldier is looking for items and finds a non-helmet/vest/leggings piece of armour it was incorrectly considered for pickup.
								// Fixed by Tron (Stracciatella): Revision: 5719
								// break;
								// continue; <- silversurfer: bad idea, this causes the game to hang
								// to make sure that the item isn't considered set iTempValue to zero and get out
								{
									iTempValue = 0;
									break;
								}
							}
						}
						// Flugente: if the soldier is 'dumb enough', he may pick up 'interesting items'. This can be used to lure him into traps (a certain scene in FMJ comes to mind)
						else if ( fDumbEnoughtoPickup && pItem->usItemClass == IC_MISC && HasItemFlag(pObj->usItem, ATTENTION_ITEM) && !(pObj->fFlags & OBJECT_AI_UNUSABLE))
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---dumb soldier picks up item!") );
							// oooh... shiny!
							iTempValue = 1000;
						}
						else
						{
							iTempValue = 0;
						}

						DebugAI( AI_MSG_INFO, pSoldier, String("---iTempValue %d", iTempValue) );

						if (iTempValue > iValue )
						{
							DebugAI( AI_MSG_INFO, pSoldier, String("---select item") );
							iValue = iTempValue;
							iItemIndex = pItemPool->iItemIndex;
						}
						pItemPool = pItemPool->pNext;
					}
					break;
				}
				iValue = (3 * iValue) / (3 + PythSpacesAway( sGridNo, pSoldier->sGridNo ));
				DebugAI( AI_MSG_INFO, pSoldier, String("---iBestValue %d", iBestValue) );
				DebugAI( AI_MSG_INFO, pSoldier, String("---value modified by distance %d", iValue) );
				if (iValue > iBestValue )
				{
					DebugAI( AI_MSG_INFO, pSoldier, String("---select item at %d", sGridNo) );
					sBestSpot = sGridNo;
					iBestValue = iValue;
					iBestItemIndex = iItemIndex;
				}
			}
		}
	}

	if (!TileIsOutOfBounds(sBestSpot))
	{
		DebugAI( AI_MSG_INFO, pSoldier, String( "%d decides to pick up %S", pSoldier->ubID, ItemNames[ gWorldItems[ iBestItemIndex ].object.usItem ] ) );
		if (Item[gWorldItems[ iBestItemIndex ].object.usItem].usItemClass == IC_GUN)
		{
			//CHRISL: This is the line from ADB's code but I removed it, for now, to match what 0verhaul has been working on
			//if (pSoldier->inv[HANDPOS].exists() == true && PlaceInAnyPocket(pSoldier, &pSoldier->inv[HANDPOS], false) == false)
			if (FindBetterSpotForItem( pSoldier, HANDPOS ) == FALSE)
			{
				if (pSoldier->bActionPoints < GetBasicAPsToPickupItem( pSoldier ) + GetBasicAPsToPickupItem( pSoldier ))
				{
					return( AI_ACTION_NONE );
				}

				if (pSoldier->inv[HANDPOS].fFlags & OBJECT_UNDROPPABLE)
				{
					// destroy this item!
					DebugAI( AI_MSG_INFO, pSoldier, String( "%d decides he must drop %S first so destroys it", pSoldier->ubID, ItemNames[ pSoldier->inv[HANDPOS].usItem ] ) );
					DeleteObj( &(pSoldier->inv[HANDPOS]) );
					DeductPoints( pSoldier, GetBasicAPsToPickupItem( pSoldier ), 0, AFTERACTION_INTERRUPT );
				}
				else
				{
					// we want to drop this item!
					DebugAI( AI_MSG_INFO, pSoldier, String( "%d decides he must drop %S first", pSoldier->ubID, ItemNames[ pSoldier->inv[HANDPOS].usItem ] ) );

					pSoldier->aiData.bNextAction = AI_ACTION_PICKUP_ITEM;
					pSoldier->aiData.usNextActionData = sBestSpot;
					pSoldier->iNextActionSpecialData = iBestItemIndex;

					// sevenfm: if we decide to drop item, we should mark it as unusable first
					pSoldier->inv[HANDPOS].fFlags |= OBJECT_AI_UNUSABLE;
					pSoldier->aiData.usActionData = HANDPOS;
					
					return( AI_ACTION_DROP_ITEM );
				}
			}
		}
		pSoldier->aiData.uiPendingActionData1 = iBestItemIndex;
		pSoldier->aiData.usActionData = sBestSpot;
		return( AI_ACTION_PICKUP_ITEM );
	}

	return( AI_ACTION_NONE );
}

INT32 FindClosestDoor( SOLDIERTYPE * pSoldier )
{
	INT32		sClosestDoor = NOWHERE;
	INT32		iSearchRange;
	INT16		sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32 sGridNo;
	INT32		iDist, iClosestDist = 10;

	iSearchRange = 5;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
	//NumMessage("sMaxDown = ",sMaxDown);
	// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if (FindStructure( sGridNo, STRUCTURE_ANYDOOR ) != NULL)
			{
				iDist = PythSpacesAway( pSoldier->sGridNo, sGridNo );
				if (iDist < iClosestDist)
				{
					iClosestDist = iDist;
					sClosestDoor = sGridNo;
				}
			}
		}
	}

	return( sClosestDoor );
}

INT32 FindNearestEdgepointOnSpecifiedEdge( INT32 sGridNo, INT8 bEdgeCode )
{
	INT32			iLoop;
	INT32			*psEdgepointArray;
	INT32			iEdgepointArraySize;
	INT32			sClosestSpot = NOWHERE, sClosestDist = 0x7FFF, sTempDist;

	switch( bEdgeCode )
	{
	case NORTH_EDGEPOINT_SEARCH:
		psEdgepointArray = gps1stNorthEdgepointArray;
		iEdgepointArraySize = gus1stNorthEdgepointArraySize;
		break;
	case EAST_EDGEPOINT_SEARCH:
		psEdgepointArray = gps1stEastEdgepointArray;
		iEdgepointArraySize = gus1stEastEdgepointArraySize;
		break;
	case SOUTH_EDGEPOINT_SEARCH:
		psEdgepointArray = gps1stSouthEdgepointArray;
		iEdgepointArraySize = gus1stSouthEdgepointArraySize;
		break;
	case WEST_EDGEPOINT_SEARCH:
		psEdgepointArray = gps1stWestEdgepointArray;
		iEdgepointArraySize = gus1stWestEdgepointArraySize;
		break;
	default:
		// WTF???
		return( NOWHERE );
	}

	// Do a 2D search to find the closest map edgepoint and
	// try to create a path there

	for ( iLoop = 0; iLoop < iEdgepointArraySize; iLoop++ )
	{
		sTempDist = PythSpacesAway( sGridNo, psEdgepointArray[ iLoop ] );
		if ( sTempDist < sClosestDist )
		{
			sClosestDist = sTempDist;
			sClosestSpot = psEdgepointArray[ iLoop ];
		}
	}

	return( sClosestSpot );
}

INT32 FindNearestEdgePoint( INT32 sGridNo )
{
	INT16			sGridX, sGridY;
	INT16			sScreenX, sScreenY, sMaxScreenX, sMaxScreenY;
	INT16			sDist[5], sMinDist;
	INT32			iLoop;
	INT8			bMinIndex;
	INT32 *		psEdgepointArray;
	INT32			iEdgepointArraySize;
	INT32			sClosestSpot = NOWHERE, sClosestDist = 0x7FFF, sTempDist;

	ConvertGridNoToXY( sGridNo, &sGridX, &sGridY );
	GetWorldXYAbsoluteScreenXY( sGridX, sGridY, &sScreenX, &sScreenY );

	sMaxScreenX = gsBRX - gsTLX;
	sMaxScreenY = gsBRY - gsTLY;

	sDist[0] = 0x7FFF;
	sDist[1] = sScreenX;									// west
	sDist[2] = sMaxScreenX - sScreenX;		// east
	sDist[3] = sScreenY;									// north
	sDist[4] = sMaxScreenY - sScreenY;		// south

	sMinDist = sDist[0];
	bMinIndex = 0;
	for( iLoop = 1; iLoop < 5; iLoop++)
	{
		if ( sDist[ iLoop ] < sMinDist )
		{
			sMinDist = sDist[ iLoop ];
			bMinIndex = (INT8) iLoop;
		}
	}

	switch( bMinIndex )
	{
	case 1:
		psEdgepointArray = gps1stWestEdgepointArray;
		iEdgepointArraySize = gus1stWestEdgepointArraySize;
		break;
	case 2:
		psEdgepointArray = gps1stEastEdgepointArray;
		iEdgepointArraySize = gus1stEastEdgepointArraySize;
		break;
	case 3:
		psEdgepointArray = gps1stNorthEdgepointArray;
		iEdgepointArraySize = gus1stNorthEdgepointArraySize;
		break;
	case 4:
		psEdgepointArray = gps1stSouthEdgepointArray;
		iEdgepointArraySize = gus1stSouthEdgepointArraySize;
		break;
	default:
		// WTF???
		return( NOWHERE );
	}

	// Do a 2D search to find the closest map edgepoint and
	// try to create a path there

	for ( iLoop = 0; iLoop < iEdgepointArraySize; iLoop++ )
	{
		sTempDist = PythSpacesAway( sGridNo, psEdgepointArray[ iLoop ] );
		if ( sTempDist < sClosestDist )
		{
			sClosestDist = sTempDist;
			sClosestSpot = psEdgepointArray[ iLoop ];
		}
	}

	return( sClosestSpot );
}

#define EDGE_OF_MAP_SEARCH 5

INT32 FindNearbyPointOnEdgeOfMap( SOLDIERTYPE * pSoldier, INT8 * pbDirection )
{
	INT32		iSearchRange;
	INT16		sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

	INT32 sGridNo, sClosestSpot = NOWHERE;
	INT8	bDirection, bClosestDirection;
	INT32 iPathCost, iClosestPathCost = 1000;

	bClosestDirection = -1;

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// set the distance limit of the square region
	gubNPCDistLimit = EDGE_OF_MAP_SEARCH;

	iSearchRange = EDGE_OF_MAP_SEARCH;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, WALKING, COPYREACHABLE, 0 );//dnl ch50 071009

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

	// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if (GridNoOnEdgeOfMap( sGridNo, &bDirection ) )
			{
				iPathCost = PythSpacesAway( pSoldier->sGridNo, sGridNo );

				if (iPathCost < iClosestPathCost)
				{
					// this place is closer
					sClosestSpot = sGridNo;
					iClosestPathCost = iPathCost;
					bClosestDirection = bDirection;
				}
			}
		}
	}

	*pbDirection = bClosestDirection;
	return( sClosestSpot );
}

INT32 FindRouteBackOntoMap( SOLDIERTYPE * pSoldier, INT32 sDestGridNo )
{
	// the first thing to do is restore the soldier's gridno from the X and Y
	// values

	// well, let's TRY just taking a path to the place we're supposed to go...
	if ( FindBestPath( pSoldier, sDestGridNo, pSoldier->pathing.bLevel, WALKING, COPYROUTE, 0 ) )
	{
		pSoldier->pathing.bPathStored = TRUE;
		return( sDestGridNo );
	}
	else
	{
		return( NOWHERE );
	}

}

INT32 FindClosestBoxingRingSpot( SOLDIERTYPE * pSoldier, BOOLEAN fInRing )
{
	INT32		iSearchRange;
	INT16		sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

	INT32 sGridNo, sClosestSpot = NOWHERE;
	INT32 iDistance, iClosestDistance = 9999;
	//DBrot: More Rooms
	//UINT8	ubRoom;
	UINT16 usRoom;
	// sevenfm
	SOLDIERTYPE *pDarren;
	//UINT8 ubLoop;
	//BOOLEAN fBoxerSpot;
	pDarren = FindSoldierByProfileID(DARREN, FALSE);

	// set the distance limit of the square region
	iSearchRange = 7;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	/*if ( (pSoldier->bTeam == gbPlayerNum) && (fInRing == FALSE) )
	{
		// have player not go to the left of the ring
		sMaxLeft = 0;
	}*/

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if (!TileIsOutOfBounds(sGridNo) && 
				InARoom(sGridNo, &usRoom))
			{
				// sevenfm: avoid taking place of AI boxers
				/*fBoxerSpot = FALSE;
				for (ubLoop = 0; ubLoop < NUM_BOXERS; ++ubLoop)
				{
					if (sGridNo == gsBoxerGridNo[ubLoop])
					{
						fBoxerSpot = TRUE;
						break;
					}
				}*/
				if ( (fInRing && usRoom == BOXING_RING) || (!fInRing && usRoom != BOXING_RING ) && 
					LegalNPCDestination(pSoldier,sGridNo,IGNORE_PATH,NOWATER,0) )
				{
					// sevenfm: for player merc, find spot closest to Darren
					if (pDarren && pSoldier->bTeam == gbPlayerNum)
						iDistance = PythSpacesAway(pDarren->sGridNo, sGridNo) + PythSpacesAway(pSoldier->sGridNo, sGridNo);
					else
						iDistance = abs(sXOffset) + abs(sYOffset);
					
					if ( iDistance < iClosestDistance && WhoIsThere2( sGridNo, 0 ) == NOBODY )
					{
						sClosestSpot = sGridNo;
						iClosestDistance = iDistance;
					}
				}
			}

		}
	}

	return( sClosestSpot );
}

INT32 FindNearestOpenableNonDoor( INT32 sStartGridNo )
{
	INT32		iSearchRange;
	INT16		sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

	INT32 sGridNo, sClosestSpot = NOWHERE;
	INT32		iDistance, iClosestDistance = 9999;
	STRUCTURE * pStructure;

	// set the distance limit of the square region
	iSearchRange = 7;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (sStartGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((sStartGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (sStartGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((sStartGridNo / MAXROW) + 1));

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = sStartGridNo + sXOffset + (MAXCOL * sYOffset);
			pStructure = FindStructure( sGridNo, STRUCTURE_OPENABLE );
			if (pStructure)
			{
				// skip any doors
				while ( pStructure && ( pStructure->fFlags & STRUCTURE_ANYDOOR ) )
				{
					pStructure = FindNextStructure( pStructure, STRUCTURE_OPENABLE );
				}
				// if we still have a pointer, then we have found a valid non-door openable structure
				if ( pStructure )
				{
					iDistance = CardinalSpacesAway( sGridNo, sStartGridNo );
					if ( iDistance < iClosestDistance )
					{
						sClosestSpot = sGridNo;
						iClosestDistance = iDistance;
					}
				}
			}
		}
	}

	return( sClosestSpot );

}

INT32 FindFlankingSpot(SOLDIERTYPE *pSoldier, INT32 sPos, INT8 bAction )
{
	INT32 sGridNo;
	INT32 sBestSpot = NOWHERE;
	INT32 iSearchRange = TACTICAL_RANGE / 4;
	INT16 sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT16 usMovementMode;

	// INT16 sDistanceVisible = DistanceVisible( pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, pSoldier->sGridNo, pSoldier->pathing.bLevel );
	INT16 sDistanceVisible = VISION_RANGE;

	DebugMsg ( TOPIC_JA2AI , DBG_LEVEL_3 , String("FindFlankingSpot: orig loc = %d, loc to flank = %d", pSoldier->sGridNo , sPos));

	// hit the edge of the map
	/*if ( FindNearestEdgePoint ( pSoldier->sGridNo ) == pSoldier->sGridNo	)
		return NOWHERE;*/

	// sevenfm: no reason to limit AP, we can reach that tile in the next turn
	//gubNPCAPBudget= pSoldier->CalcActionPoints();
	//gubNPCAPBudget= min(pSoldier->bInitialActionPoints, iSearchRange * 3 * APBPConstants[AP_MAXIMUM] / 25);
	gubNPCAPBudget = min(pSoldier->bInitialActionPoints, APBPConstants[AP_MAXIMUM]);

	// stay away from the edges

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
	//NumMessage("sMaxDown = ",sMaxDown);

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8) iSearchRange;

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	usMovementMode = DetermineMovementMode( pSoldier, bAction );
	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE, 0 );

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

	// get direction of position to flank from soldier's position
	INT16 sDir = GetDirectionToSpot ( sPos, pSoldier) ;
	INT16 sDesiredDir;
	INT16 sTempDir;
	INT16 sTempDist, sBestDist=0;

	// determine desired direction
	switch ( bAction )
	{
	case AI_ACTION_FLANK_LEFT:
		sDesiredDir = gTwoCCDirection[ sDir ];
		break;
	case AI_ACTION_FLANK_RIGHT:
		sDesiredDir = gTwoCDirection[ sDir ];
		break;
	case AI_ACTION_WITHDRAW:
		sDesiredDir = gOppositeDirection[ sDir ];
		break;
	default:
		sDesiredDir = sDir;
	}

	DebugMsg ( TOPIC_JA2AI , DBG_LEVEL_3 , String("FindFlankingSpot: direction to loc = %d, dir to flank = %d", sDir , sDesiredDir ));

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			sTempDir = GetDirectionToSpot ( sGridNo, pSoldier );
			sTempDist = GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo , sGridNo );

			//NumMessage("Testing gridno #",gridno);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if ( sGridNo == pSoldier->pathing.sBlackList )
			{
				continue;
			}

			// sevenfm: skip tiles too close to north edge
			//if ( PythSpacesAway( FindNearestEdgePoint ( sGridNo ), sGridNo ) <= 2 )
			if (NorthSpot(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// sevenfm: don't go into deep water for flanking
			if (!AllowDeepWaterFlanking(pSoldier) &&
				DeepWater(sGridNo, pSoldier->pathing.bLevel) &&
				!DeepWater(pSoldier->sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// sevenfm: penalize locations near fresh corpses
			// avoid locations with fresh corpses
			if( CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel) )
			{
				continue;
			}			

			if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, WATEROK, 0))
			{
				continue;		// skip on to the next potential grid
			}			

			// sevenfm: skip buildings if not in building already, because soldiers often run into buildings and stop flanking
			if( InARoom( sGridNo, NULL ) && 
				!InARoom(pSoldier->sGridNo, NULL))
				//!(pSoldier->flags.ubTacticalFlags & TACTICAL_FLAG_EXTRA_FLANKING))
			{
				continue;
			}

			if (SpotDangerLevel(pSoldier, sGridNo) > SpotDangerLevel(pSoldier, pSoldier->sGridNo))
			{
				continue;
			}

			// sevenfm: penalize locations too far from noise gridno
			if (PythSpacesAway(sGridNo, sPos) > MAX_FLANK_DIST && pSoldier->aiData.bAttitude != CUNNINGSOLO)
			{
				continue;
			}

			if (PythSpacesAway(sGridNo, sPos) > sDistanceVisible + MAX_TILES_MOVE_TURN / 2)
			{
				sTempDist = sTempDist / 2;
			}

			// avoid moving too close to enemy vision range
			if (PythSpacesAway(sGridNo, sPos) < sDistanceVisible + MAX_TILES_MOVE_TURN / 4)
			{
				sTempDist = sTempDist / 2;
			}

			// sevenfm: penalize locations with no sight cover from noise gridno (supposed that we are sneaking)
			if (InLightAtNight(sPos, pSoldier->pathing.bLevel) ||
				PythSpacesAway(sGridNo, sPos) <= (INT16)(MAX_VISION_RANGE) &&
				LOS_Location( sGridNo, pSoldier->pathing.bLevel, sPos, pSoldier->pathing.bLevel, CALC_FROM_ALL_DIRS) )
			{
				//continue;
				sTempDist = sTempDist / 2;
			}

			// allow extra directions for flanking
			if ( bAction == AI_ACTION_FLANK_LEFT )
			{
				// sevenfm: allow two extra directions
				if ( sTempDir != sDesiredDir && sTempDir != gOneCDirection[sDesiredDir] && sTempDir != gOneCCDirection[sDesiredDir])
					continue;
				// prefer desired dir x1.5
				if( sTempDir == sDesiredDir )
					sTempDist = 3*sTempDist/2;
			}
			else if ( bAction == AI_ACTION_FLANK_RIGHT )
			{
				// sevenfm: allow two extra directions
				if ( sTempDir != sDesiredDir && sTempDir != gOneCDirection[sDesiredDir] && sTempDir != gOneCCDirection[sDesiredDir])
					continue;
				// prefer desired dir x1.5
				if( sTempDir == sDesiredDir )
					sTempDist = 3*sTempDist/2;
			}
			else
			{
				if ( sTempDir != sDesiredDir )
					continue;
			}

			// if this is better than the best place found so far
			if ( sTempDist > sBestDist )
			{
				// remember it instead
				sBestDist = sTempDist;
				sBestSpot = sGridNo;
			}
		}
	}

	DebugMsg ( TOPIC_JA2AI , DBG_LEVEL_3 , String("FindFlankingSpot: return grid no %d", sBestSpot ));

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

	return( sBestSpot );
}

INT32 FindClosestClimbPoint (SOLDIERTYPE *pSoldier, BOOLEAN fClimbUp )
{
	INT32 sBestSpot = NOWHERE;

	// sevenfm: safety check
	if(!pSoldier)
	{
		return NOWHERE;
	}

	//DebugMsg( TOPIC_JA2AI , DBG_LEVEL_3 , "FindClosestClimbPoint");

	if (fClimbUp)
	{
		// For the climb up case, search the nearby limits for a climb up point and take the closest.
		INT32 sGridNo;
		INT32 iSearchRange = TACTICAL_RANGE;
		INT16 sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;

		INT8 ubClimbDir;
		INT32 sClimbSpot;

		if (!AllowComplexAI(pSoldier))
		{
			iSearchRange /= 2;
		}

		// determine maximum horizontal limits
		sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
		sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));

		// determine maximum vertical limits
		sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
		sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

		for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
		{
			for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
			{
				// calculate the next potential gridno
				sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

				if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
				{
					continue;
				}

				if ( sGridNo == pSoldier->pathing.sBlackList )
				{
					continue;
				}			

				// exclude locations with tear/mustard gas (at this point, smoke is cool!)
				if ( InGas( pSoldier, sGridNo ) )
				{
					continue;
				}

				// exclude water tiles
				if ( Water( sGridNo, pSoldier->pathing.bLevel ) )
				{
					continue;
				}

				// check that there's noone at sGridNo at level 0 (this soldier is allowed)
				if( WhoIsThere2( sGridNo, 0 ) != NOBODY &&
					WhoIsThere2( sGridNo, 0 ) != pSoldier->ubID )
				{
					continue;
				}

				// sevenfm: check path to climb point
				if( FindHeigherLevel( pSoldier, sGridNo, pSoldier->ubDirection, &ubClimbDir ) &&
					LegalNPCDestination(pSoldier, sGridNo, ENSURE_PATH, WATEROK, 0 ) )
				{
					// ubClimbDir is new direction
					// check that there's noone there
					sClimbSpot = NewGridNo( sGridNo, DirectionInc( ubClimbDir));
					if (sClimbSpot != sGridNo &&
						WhoIsThere2( sClimbSpot, 1 ) == NOBODY &&
						!InGasSpot(pSoldier, sClimbSpot, 1))
					{
						// Good climb point.  Is it closer than the previous point?
						if( TileIsOutOfBounds(sBestSpot) ||
							GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo , sGridNo ) < GetRangeInCellCoordsFromGridNoDiff( pSoldier->sGridNo , sBestSpot ))
						{
							// If not, we have a new winnar!
							sBestSpot = sGridNo;
						}
					}
				}
			}
		}

		// if could not find climb spot nearby, check climb points
		/*if(sBestSpot == NOWHERE)
		{
			// need to correctly set sDesiredGridNo for building
			sBestSpot = FindClosestBuildingClimbPoint(pSoldier, pSoldier->sGridNo, pSoldier->sGridNo, fClimbUp);
			// sevenfm: check path to climb point
			if (!LegalNPCDestination(pSoldier, sBestSpot, ENSURE_PATH, WATEROK, 0))
			{
				sBestSpot = NOWHERE;
			}
		}*/
	}
	else
	{
		// Climbing down is easier.  Just find the nearest climb point ;)
		sBestSpot = FindClosestBuildingClimbPoint( pSoldier, pSoldier->sGridNo, pSoldier->sGridNo, fClimbUp);
		// sevenfm: check path to climb point
		if (!LegalNPCDestination(pSoldier, sBestSpot, ENSURE_PATH, WATEROK, 0))
		{
			sBestSpot = NOWHERE;
		}
	}

	//	DebugMsg( TOPIC_JA2AI , DBG_LEVEL_3 , String("FindClosestClimbPoint Returning %d", sBestSpot ));
	return( sBestSpot );
}


BOOLEAN CanClimbFromHere (SOLDIERTYPE * pSoldier)
{
	// sevenfm: new calculation using FindHeigherLevel/FindLowerLevel
	return FindDirectionForClimbing( pSoldier, pSoldier->sGridNo ) != DIRECTION_IRRELEVANT;
}

extern BUILDING gBuildings[ MAX_BUILDINGS ];
extern UINT8 gubNumberOfBuildings;

// sevenfm: new calculation using FindHeigherLevel/FindLowerLevel
INT8 FindDirectionForClimbing( SOLDIERTYPE *pSoldier, INT32 sGridNo )
{
	INT8 ubClimbDir;
	INT32 sClimbSpot;

	if(!pSoldier)
	{
		return DIRECTION_IRRELEVANT;
	}

	if (pSoldier->pathing.bLevel == 0)
	{
		// check climb up
		if( FindHeigherLevel( pSoldier, sGridNo, pSoldier->ubDirection, &ubClimbDir ) )
		{
			// ubClimbDir is new direction
			// check that there's noone there
			sClimbSpot = NewGridNo( sGridNo, DirectionInc( ubClimbDir));
			if (sClimbSpot != sGridNo &&
				WhoIsThere2( sClimbSpot, 1 ) == NOBODY &&
				!Water(sClimbSpot, 1) &&
				!InGasSpot(pSoldier, sClimbSpot, 1))
			{
				return ubClimbDir;
			}
		}
	}
	else
	{
		// check climb down
		if (FindLowerLevel(pSoldier, pSoldier->sGridNo, pSoldier->ubDirection, &ubClimbDir))
		{
			// ubClimbDir is new direction
			sClimbSpot = NewGridNo(sGridNo, DirectionInc(ubClimbDir));
			if (sClimbSpot != sGridNo &&
				WhoIsThere2(sClimbSpot, 0) == NOBODY &&
				!Water(sClimbSpot, 0) &&
				!InGasSpot(pSoldier, sClimbSpot, 0))
			{
				return ubClimbDir;
			}
		}
	}

	return DIRECTION_IRRELEVANT;
}

INT32 FindNearestPassableSpot( INT32 sGridNo, UINT8 usSearchRadius )
{
	INT32	sNearestSpot = NOWHERE;
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	sTestGridNo;
	INT32	iDist, iClosestDist = 10;

	// determine maximum horizontal limits
	sMaxLeft  = min( usSearchRadius, (sGridNo % MAXCOL));
	sMaxRight = min( usSearchRadius, MAXCOL - ((sGridNo % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp   = min( usSearchRadius, (sGridNo / MAXROW));
	sMaxDown = min( usSearchRadius, MAXROW - ((sGridNo / MAXROW) + 1));

	// SET UP DOUBLE-LOOP TO STEP THROUGH POTENTIAL GRID #s
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sTestGridNo = sGridNo + sXOffset + (MAXCOL * sYOffset);
			// is this an empty tile (no structure on it) or is the structure passable?
			if (gpWorldLevelData[sTestGridNo].pStructureHead == NULL || FindStructure( sTestGridNo, STRUCTURE_PASSABLE ) != NULL)
			{
				iDist = PythSpacesAway( sGridNo, sTestGridNo );
				if (iDist < iClosestDist)
				{
					iClosestDist = iDist;
					sNearestSpot = sTestGridNo;
				}
			}
		}
	}

	return( sNearestSpot );
}

INT32 FindAdvanceSpot(SOLDIERTYPE *pSoldier, INT32 sTargetSpot, INT8 bAction, UINT8 ubType, BOOLEAN fUnlimited, UINT8 ubSpotFlag)
{
	INT32	sGridNo, sRealGridNo;
	INT32	sBestSpot = NOWHERE;
	INT32	iSearchRange = min(AI_PATHCOST_RADIUS, MAX_TILES_MOVE_TURN);
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT32	iPathCost, iBestPathCost = 0;
	INT16	usMovementMode;
	INT32	iRoamRange, iDistFromOrigin, sOrigin;
	//BOOLEAN	fClimbingNecessary;
	//INT32	sClimbGridNo;
	BOOLEAN fHasMortar;

	if( !pSoldier )
	{
		return NOWHERE;
	}

	if (!AllowComplexAI(pSoldier))
	{
		iSearchRange /= 2;
	}

	// check target location
	if( !NewOKDestination(pSoldier, sTargetSpot, FALSE, pSoldier->pathing.bLevel) )
	{
		//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"bad destination %d", sTargetSpot);
		return NOWHERE;
	}

	fHasMortar = AIRoleMortarOperator(pSoldier);

	usMovementMode = DetermineMovementMode(pSoldier, bAction);

	UINT8 ubReserveAP = 0;
	
	if( ubType != ADVANCE_SPOT_SIGHT_COVER ) 
	{
		if (usMovementMode == RUNNING || usMovementMode == WALKING)
		{
			ubReserveAP = APBPConstants[AP_CHANGE_FACING] + GetAPsCrouch(pSoldier, TRUE) + GetAPsProne(pSoldier, TRUE);
		}
		else
		{
			ubReserveAP = APBPConstants[AP_LOOK_CROUCHED] + GetAPsProne(pSoldier, TRUE);
		}
	}
	
	if( pSoldier->bActionPoints <= ubReserveAP )
	{
		return NOWHERE;
	}	

	// check that location is reachable
	iBestPathCost = EstimatePlotPath(pSoldier, sTargetSpot, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
	if (iBestPathCost == 0)
	{
		return NOWHERE;
	}

	iRoamRange = RoamingRange(pSoldier,&sOrigin);

	// set AP limit
	gubNPCAPBudget = pSoldier->bActionPoints - ubReserveAP;
	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8) iSearchRange;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
	//NumMessage("sMaxDown = ",sMaxDown);

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}
	
	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE, 0 );	

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if ( sGridNo == pSoldier->pathing.sBlackList )
			{
				continue;
			}

			// check roaming range
			if( iRoamRange < MAX_ROAMING_RANGE && !TileIsOutOfBounds(sOrigin) )
			{
				iDistFromOrigin = SpacesAway(sOrigin, sGridNo);
				if( iDistFromOrigin > iRoamRange )
				{
					continue;
				}
			}			

			// check if we will be closer to target spot
			if (PythSpacesAway(sGridNo, sTargetSpot) > PythSpacesAway(pSoldier->sGridNo, sTargetSpot))
			{
				continue;
			}

			// exclude locations with tear/mustard gas (at this point, smoke is cool!)
			if (InGas(pSoldier, sGridNo))
			{
				continue;
			}

			// skip lighted spots
			if (InLightAtNight(sGridNo, pSoldier->pathing.bLevel) && !SightCoverAtSpot(pSoldier, sGridNo, ANIM_STAND, FALSE))
				//!InLightAtNight( pSoldier->sGridNo, pSoldier->pathing.bLevel) )
				continue;

			// skip deep water
			if (DeepWater(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// skip water
			if (Water(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// skip locations too close to target spot
			if (PythSpacesAway(sGridNo, sTargetSpot) < (INT16)(TACTICAL_RANGE / 4))
			{
				continue;
			}

			// avoid smoke
			/*if (!(ubSpotFlag & SPOT_FLAG_SMOKE) && 
				InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE))
			{
				continue;
			}*/

			// check spot flags
			if ((ubSpotFlag & SPOT_FLAG_OBSTACLE) &&
				!FindObstacleNearSpot(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			if ((ubSpotFlag & SPOT_FLAG_SMOKE) &&
				!InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE))
			{
				continue;
			}

			// skip if this spot has no cover nearby
			if ((ubType == ADVANCE_SPOT_ANY_COVER || fUnlimited) &&
				!FindObstacleNearSpot(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// avoid locations near fresh corpses
			//if( GetNearestRottingCorpseAIWarning( sGridNo ) > 0 )
			if (CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			// avoid overcrowding
			if (NumberOfTeamMatesAdjacent(pSoldier, sGridNo) > 1)
			{
				continue;
			}

			if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, NOWATER, 0))
			{
				continue;		// skip on to the next potential grid
			}

			// sevenfm: avoid rooms for mortar operators
			if (!AICheckUnderground() &&
				pSoldier->pathing.bLevel == 0 &&
				fHasMortar &&
				InARoom(sGridNo, NULL))
			{
				continue;
			}

			if (SpotDangerLevel(pSoldier, sGridNo) > SpotDangerLevel(pSoldier, pSoldier->sGridNo))
			{
				continue;
			}

			if (ubType == ADVANCE_SPOT_SIGHT_COVER)
			{
				// skip locations with no sight cover
				if (!SightCoverAtSpot(pSoldier, sGridNo, ANIM_STAND, fUnlimited))
				{
					continue;
				}
			}
			else if (ubType == ADVANCE_SPOT_PRONE_COVER)
			{
				// check prone sight cover at spot
				if (!SightCoverAtSpot(pSoldier, sGridNo, ANIM_PRONE, fUnlimited))
				{
					continue;
				}
				// check that we'll have enough APs to go prone at target spot
				/*iPathCost = EstimatePlotPath( pSoldier, sGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
				if( pSoldier->bActionPoints - iPathCost < GetAPsCrouch(pSoldier, TRUE) + GetAPsProne(pSoldier, TRUE) + APBPConstants[AP_CHANGE_FACING] )
				{
					continue;
				}*/
			}
			else if( ubType == ADVANCE_SPOT_ANY_COVER )
			{
				// check any cover at spot
				if( !AnyCoverAtSpot(pSoldier, sGridNo) )
				{
					continue;
				}
				// check that we'll have enough APs to go prone at target spot
				/*iPathCost = EstimatePlotPath( pSoldier, sGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
				if( pSoldier->bActionPoints - iPathCost < GetAPsCrouch(pSoldier, TRUE) + GetAPsProne(pSoldier, TRUE) + APBPConstants[AP_CHANGE_FACING] )
				{
					continue;
				}*/
			}			

			sRealGridNo = pSoldier->sGridNo;
			pSoldier->sGridNo = sGridNo;
			iPathCost = EstimatePlotPath( pSoldier, sTargetSpot, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
			//iPathCost = PlotPath(pSoldier, sTargetSpot, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, FALSE, 0);
			//iPathCost = EstimatePathCostToLocation( pSoldier, sTargetSpot, pSoldier->pathing.bLevel, TRUE, &fClimbingNecessary, &sClimbGridNo );
			pSoldier->sGridNo = sRealGridNo;
			
			// skip location if no path to target spot
			if( iPathCost == 0 )
			{
				//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"cannot find path to destination %d at %d", sTargetSpot, sGridNo);
				continue;
			}

			//if( sBestSpot == NOWHERE || iPathCost < iBestPathCost )
			if( iPathCost < iBestPathCost )
			{
				sBestSpot = sGridNo;
				iBestPathCost = iPathCost;
			}
		}
	}

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

	return( sBestSpot );
}

// find spot with cover, max dist from opponents
INT32 FindRetreatSpot(SOLDIERTYPE *pSoldier)
{
	INT32	sGridNo;
	INT32	sBestSpot = NOWHERE;
	INT32	iSearchRange = MAX_TILES_MOVE_TURN;
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT16	usMovementMode;
	INT32	iRoamRange, iDistFromOrigin, sOrigin;
	//BOOLEAN	fClimbingNecessary;
	//INT32	sClimbGridNo;
	UINT8	ubReserveAP;
	INT8	bAction = AI_ACTION_TAKE_COVER;
	INT32	sClosestOpponent;
	INT16	sDistance, sClosestDistance;

	if( !pSoldier )
	{
		return NOWHERE;
	}

	if (!AllowComplexAI(pSoldier))
	{
		iSearchRange /= 2;
	}

	sClosestOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL);
	if( TileIsOutOfBounds(sClosestOpponent) )
	{
		// no one to hide from
		return NOWHERE;
	}

	sClosestDistance = PythSpacesAway(pSoldier->sGridNo, sClosestOpponent);
	
	//ubReserveAP = GetAPsCrouch(pSoldier, TRUE) + GetAPsProne(pSoldier, TRUE) + APBPConstants[AP_CHANGE_FACING];
	ubReserveAP = GetAPsCrouch(pSoldier, TRUE) + APBPConstants[AP_CHANGE_FACING];
	
	if( pSoldier->bActionPoints <= ubReserveAP )
	{
		return NOWHERE;
	}

	iRoamRange = RoamingRange(pSoldier,&sOrigin);

	// set AP limit
	gubNPCAPBudget = pSoldier->bActionPoints;
	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8) iSearchRange;

	// determine maximum horizontal limits
	sMaxLeft  = min( iSearchRange, (pSoldier->sGridNo % MAXCOL));
	//NumMessage("sMaxLeft = ",sMaxLeft);
	sMaxRight = min( iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
	//NumMessage("sMaxRight = ",sMaxRight);

	// determine maximum vertical limits
	sMaxUp   = min( iSearchRange, (pSoldier->sGridNo / MAXROW));
	//NumMessage("sMaxUp = ",sMaxUp);
	sMaxDown = min( iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));
	//NumMessage("sMaxDown = ",sMaxDown);

	// Call FindBestPath to set flags in all locations that we can
	// walk into within range.	We have to set some things up first...

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}

	usMovementMode = DetermineMovementMode( pSoldier, bAction );
	FindBestPath( pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE, 0 );

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

			if ( !(sGridNo >=0 && sGridNo < WORLD_MAX) )
			{
				continue;
			}

			if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}

			if ( sGridNo == pSoldier->pathing.sBlackList )
			{
				continue;
			}

			// check roaming range
			if( iRoamRange < MAX_ROAMING_RANGE && !TileIsOutOfBounds(sOrigin) )
			{
				iDistFromOrigin = SpacesAway(sOrigin, sGridNo);
				if( iDistFromOrigin > iRoamRange )
				{
					continue;
				}
			}			

			// exclude locations with tear/mustard gas (at this point, smoke is cool!)
			if ( InGas( pSoldier, sGridNo ) )
			{
				continue;
			}

			// skip lighted spots
			if ( InLightAtNight( sGridNo, pSoldier->pathing.bLevel) &&
				!InLightAtNight( pSoldier->sGridNo, pSoldier->pathing.bLevel) )
				continue;

			// skip deep water
			if( DeepWater( sGridNo, pSoldier->pathing.bLevel ) )
			{
				continue;
			}

			// skip water
			if( Water( sGridNo, pSoldier->pathing.bLevel ) )
			{
				continue;
			}

			// avoid locations near fresh corpses
			if( CorpseWarning(pSoldier, sGridNo, pSoldier->pathing.bLevel) )
			{
				continue;
			}

			if (NumberOfTeamMatesAdjacent(pSoldier, sGridNo) > 1)
			{
				continue;
			}

			// avoid smoke
			/*if( InSmoke(sGridNo, pSoldier->pathing.bLevel, FALSE) )
			{
				continue;
			}*/

			if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, NOWATER, 0))
			{
				continue;		// skip on to the next potential grid
			}

			if (SpotDangerLevel(pSoldier, sGridNo) > SpotDangerLevel(pSoldier, pSoldier->sGridNo))
			{
				continue;
			}

			// skip if too close to enemy
			//sClosestOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL);
			sDistance = PythSpacesAway(sGridNo, sClosestOpponent);
			if (sDistance < DAY_VISION_RANGE / 2)
			{
				continue;
			}

			// skip if this spot has no cover nearby
			if (!FindObstacleNearSpot(sGridNo, pSoldier->pathing.bLevel))
			{
				continue;
			}

			if (!AnyCoverAtSpot(pSoldier, sGridNo))
			{
				continue;
			}

			// skip if spot under attack
			if (AICheckUnderAttack(pSoldier, sGridNo, FALSE))
			{
				continue;
			}

			// check prone sight cover at spot
			/*if( !SightCoverAtSpot(pSoldier, sGridNo, ANIM_PRONE) )
			{
				continue;
			}*/

			if( sDistance > sClosestDistance )
			{
				sBestSpot = sGridNo;
				sClosestDistance = sDistance;
			}
		}
	}

	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;

	return( sBestSpot );
}

// find safe spot with cover around, without fresh corpse nearby
INT32 FindSafeMedicSpot(SOLDIERTYPE *pSoldier)
{
	INT32	sGridNo;
	INT32	sBestSpot = NOWHERE;
	INT32	iSearchRange = TACTICAL_RANGE / 4;
	INT16	sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
	INT16	usMovementMode;
	INT32	iRoamRange, iDistFromOrigin, sOrigin;
	INT8	bAction = AI_ACTION_TAKE_COVER;
	INT32	sClosestOpponent;
	//INT8	bTrueReverse;
	INT32	sPathCost;
	INT32	sBestPathCost = 0;
	INT16	sAPBudget;

	if (!pSoldier)
	{
		return NOWHERE;
	}

	sClosestOpponent = ClosestKnownOpponent(pSoldier, NULL, NULL);
	if (TileIsOutOfBounds(sClosestOpponent))
	{
		// no one to hide from
		return NOWHERE;
	}

	iRoamRange = RoamingRange(pSoldier, &sOrigin);
	usMovementMode = DetermineMovementMode(pSoldier, bAction);

	// determine maximum horizontal limits
	sMaxLeft = min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
	sMaxRight = min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));

	// determine maximum vertical limits
	sMaxUp = min(iSearchRange, (pSoldier->sGridNo / MAXROW));
	sMaxDown = min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

	sAPBudget = APBPConstants[AP_MAXIMUM];
	if (pSoldier->IsDragging())
		sAPBudget = (INT16)(sAPBudget * gItemSettings.fDragAPCostModifier);

	// set AP limit
	/*gubNPCAPBudget = APBPConstants[AP_MAXIMUM];
	// set the distance limit of the square region
	gubNPCDistLimit = (UINT8)iSearchRange;	

	// reset the "reachable" flags in the region we're looking at
	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);
			if (!(sGridNo >= 0 && sGridNo < WORLD_MAX))
			{
				continue;
			}

			gpWorldLevelData[sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
		}
	}*/
	
	/*bTrueReverse = pSoldier->bReverse;
	pSoldier->bReverse = TRUE;	
	FindBestPath(pSoldier, GRIDSIZE, pSoldier->pathing.bLevel, usMovementMode, COPYREACHABLE, 0);
	pSoldier->bReverse = bTrueReverse;*/

	// Turn off the "reachable" flag for his current location
	// so we don't consider it
	//gpWorldLevelData[pSoldier->sGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);

	for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++)
	{
		for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++)
		{
			// calculate the next potential gridno
			sGridNo = pSoldier->sGridNo + sXOffset + (MAXCOL * sYOffset);

			if (TileIsOutOfBounds(sGridNo))
			{
				continue;
			}

			/*if (!(gpWorldLevelData[sGridNo].usFlags & MAPELEMENT_REACHABLE))
			{
				continue;
			}*/

			if (sGridNo == pSoldier->pathing.sBlackList)
			{
				continue;
			}

			// check roaming range
			if (iRoamRange < MAX_ROAMING_RANGE && !TileIsOutOfBounds(sOrigin))
			{
				iDistFromOrigin = SpacesAway(sOrigin, sGridNo);
				if (iDistFromOrigin > iRoamRange)
				{
					continue;
				}
			}

			if (!LegalNPCDestination(pSoldier, sGridNo, IGNORE_PATH, NOWATER, 0))
			{
				continue;		// skip on to the next potential grid
			}			

			// skip if too close to enemy
			if (PythSpacesAway(sGridNo, sClosestOpponent) < (INT16)(TACTICAL_RANGE / 2))
			{
				continue;
			}

			// skip if there are known enemies nearby
			if (CountPublicKnownEnemies(pSoldier, sGridNo, TACTICAL_RANGE / 2) > 0)
			{
				continue;
			}

			if (SpotDangerLevel(pSoldier, sGridNo) > 0)
			{
				continue;
			}

			// skip if this spot has no sight cover from enemy
			if(!SafeMedicSpot(pSoldier, sGridNo))
			{
				continue;
			}

			sPathCost = PlotPath(pSoldier, sGridNo, FALSE, FALSE, FALSE, usMovementMode, pSoldier->bStealthMode, TRUE, sAPBudget);

			// check if spot is reachable
			if (sPathCost == 0 || sPathCost > sAPBudget)
			{
				continue;
			}

			// remember closest reachable spot
			if (sBestSpot == NOWHERE || sPathCost < sBestPathCost)
			{
				sBestPathCost = sPathCost;
				sBestSpot = sGridNo;
			}
		}
	}

	//gubNPCAPBudget = 0;
	//gubNPCDistLimit = 0;

	return sBestSpot;
}