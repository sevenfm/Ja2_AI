/*
	Filename		:		pathai.c
	Author			:		Ray E. Bornert II
	Date			:		1992-MAR-15

	Skip list additions
	Author			:		Chris Camfield
	Date			:		1997-NOV
	*/

#ifdef PRECOMPILEDHEADERS
#include "Tactical All.h"
#else
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "debug.h"
#include "MemMan.h"
#include "Overhead Types.h"

#include "Animation Cache.h"
#include "Animation Data.h"
#include "Animation Control.h"
#include "container.h"
#include "interface.h"
#include <math.h>

#include "input.h"
#include "english.h"
#include "worlddef.h"
#include "worldman.h"
//	#include "renderworld.h"
#include "pathai.h"
#include "Points.h"
#include "ai.h"
#include "Random.h"
#include "message.h"
#include "structure wrap.h"
#include "keys.h"
#include "gamesettings.h"
#include "Buildings.h"
#include "soldier profile.h" // added by SANDRO
// sevenfm
#include "opplist.h"
#include "Render Fun.h"
#include "AIinternals.h"
#include "Meanwhile.h"
#include "Boxing.h"
#include "Smell.h"
#include "Soldier macros.h"
#include "Drugs And Alcohol.h"
#endif
#include "connect.h"

#include "LOS.h"  //ddd

//forward declarations of common classes to eliminate includes
class OBJECTTYPE;
class SOLDIERTYPE;

extern UINT16 gubAnimSurfaceIndex[TOTALBODYTYPES][NUMANIMATIONSTATES];

// sevenfm:
extern BOOLEAN InGas(SOLDIERTYPE *pSoldier, INT32 sGridNo);
extern BOOLEAN InGasSpot(SOLDIERTYPE *pSoldier, INT32 sGridNo, INT8 bLevel, BOOLEAN fCheckMask);
extern INT16	gubAIPathCosts[AI_PATHCOST_DIM][AI_PATHCOST_DIM];

//extern UINT8 gubDiagCost[20];
// skiplist has extra level of pointers every 4 elements, so a level 5is optimized for
// 4 to the power of 5 elements, or 2 to the power of 10, 1024

//#define PATHAI_VISIBLE_DEBUG

//#define PATHAI_SKIPLIST_DEBUG

#ifdef PATHAI_VISIBLE_DEBUG
#include "video.h"

//extern INT16 gsCoverValue[WORLD_MAX];
extern INT16 * gsCoverValue;
BOOLEAN gfDisplayCoverValues = TRUE;
BOOLEAN gfDrawPathPoints = TRUE;
#endif

BOOLEAN gfPlotPathToExitGrid = FALSE;
BOOLEAN gfRecalculatingExistingPathCost = FALSE;
UINT8 gubGlobalPathFlags = 0;
UINT8 gubBuildingInfoToSet;
INT32 iMaxSkipListLevel = MAX_SKIPLIST_LEVEL;
INT32 iMaxTrailTree = MAX_TRAIL_TREE;
INT32 iMaxPathQ = MAX_PATHQ;
UINT8 gfPlotPathEndDirection;

extern BOOLEAN gfGeneratingMapEdgepoints;

#define VEHICLE

//#define TRAILCELLTYPE UINT16 //Lalien: moved to pathai.h

// OLD PATHAI STUFF
/////////////////////////////////////////////////
//dnl ch50 071009
struct path_s
{
	INT32					iLocation;
	INT32					sPathNdx;
	struct path_s*			pNext[ABSMAX_SKIPLIST_LEVEL];
	TRAILCELLTYPE			usCostSoFar;
	TRAILCELLTYPE			usCostToGo;
	TRAILCELLTYPE			usTotalCost;
	INT16					ubTotalAPCost;
	INT8					bLevel;
	UINT8					ubLegDistance;
};

typedef struct path_s path_t;

struct trail_s
{
	INT32 nextLink;
	INT8	stepDir;
	INT8	fFlags;
	INT32 sGridNo;
};
typedef struct trail_s trail_t;

enum
{
	STEP_BACKWARDS = 0x01,
} TrailFlags;

#define EASYWATERCOST	TRAVELCOST_FLAT / 2
#define ISWATER(t)	(((t)==TRAVELCOST_KNEEDEEP) || ((t)==TRAVELCOST_DEEPWATER))
#define NOPASS (TRAVELCOST_BLOCKED)
//#define VEINCOST TRAVELCOST_FLAT	 //actual cost for bridges and doors and such
//#define ISVEIN(v) ((v==TRAVELCOST_VEINMID) || (v==TRAVELCOST_VEINEND))

static path_t * pathQ;
static UINT16	gusPathShown, gusAPtsToMove;
static INT32	queRequests;
static INT32	iSkipListSize;
static INT32	iClosedListSize;
static INT8		bSkipListLevel;
static INT32	iSkipListLevelLimit[ABSMAX_SKIPLIST_LEVEL] = { 0, 4, 16, 64, 256, 1024, 4192, 16384, 65536, 262144 };//dnl ch50 071009

#define ESTIMATE0	((dx>dy) ?		(dx)		:		(dy))
#define ESTIMATE1	((dx<dy) ? ((dx*14)/10+dy) : ((dy*14)/10+dx) )
#define ESTIMATE2	FLATCOST*( (dx<dy) ? ((dx*14)/10+dy) : ((dy*14)/10+dx) )
#define ESTIMATEn	((int)(FLATCOST*sqrt(dx*dx+dy*dy)))
#define ESTIMATEC ( ( (dx<dy) ? (TRAVELCOST_BUMPY * (dx * 14 + dy * 10) / 10) : ( TRAVELCOST_BUMPY * (dy * 14 + dx * 10) / 10 ) ) )
//#define ESTIMATEC (((dx<dy) ? ( (TRAVELCOST_FLAT * dx * 14) / 10 + dy) : (TRAVELCOST_FLAT * dy * 14 ) / 10 + dx) ) )
#define ESTIMATE ESTIMATEC

#define MAXCOST (16384)//(9990)
//#define MAXCOST (255)
//#define TOTALCOST( pCurrPtr ) (pCurrPtr->usCostSoFar + pCurrPtr->usCostToGo)
#define TOTALCOST( ptr ) (ptr->usTotalCost)
#define XLOC(a) (a%WORLD_COLS)
#define YLOC(a) (a/WORLD_COLS)
//#define LEGDISTANCE(a,b) ( abs( XLOC(b)-XLOC(a) ) + abs( YLOC(b)-YLOC(a) ) )
#define LEGDISTANCE( x1, y1, x2, y2 ) ( abs( x2 - x1 ) + abs( y2 - y1 ) )
//#define FARTHER(ndx,NDX) ( LEGDISTANCE( ndx->sLocation,sDestination) > LEGDISTANCE(NDX->sLocation,sDestination) )
#define FARTHER(ndx,NDX) ( ndx->ubLegDistance > NDX->ubLegDistance )

#define SETLOC( str, loc ) \
{\
	(str).iLocation = loc; \
}

static TRAILCELLTYPE * trailCost;
static UINT8 * trailCostUsed;
static UINT8 gubGlobalPathCount = 0;
static trail_t * trailTree;

static INT32 trailTreeNdx = 0;//dnl ch50 071009

#define QHEADNDX (0)
#define QPOOLNDX (iMaxPathQ-1)

static path_t * pQueueHead;
static path_t * pClosedHead;


#define pathQNotEmpty (pQueueHead->pNext[0] != NULL)
#define pathFound (pQueueHead->pNext[0]->iLocation == iDestination)
#define pathNotYetFound (!pathFound)

// Note, the closed list is maintained as a doubly-linked list;
// it's a regular queue, essentially, as we always add to the end
// and remove from the front

// pNext[0] is used for the next pointers
// and pNext[1] is used for prev pointers

// experiment 1, seemed to fail
#define ClosedListAdd( pNew ) \
{\
	pNew->pNext[0] = pClosedHead->pNext[0]; \
	pClosedHead->pNext[0] = pNew; \
	pNew->iLocation = -1; \
	iClosedListSize++; \
}

#define ClosedListGet( pNew )\
{\
if (queRequests<QPOOLNDX)\
{\
	pNew = pathQ + (queRequests); \
	queRequests++; \
	pNew->pathing.bLevel = RandomSkipListLevel(); \
}\
	else if (iClosedListSize > 0)\
{\
	pNew = pClosedHead->pNext[0]; \
	pClosedHead->pNext[0] = pNew->pNext[0]; \
	iClosedListSize--; \
	queRequests++; \
	memset(pNew->pNext, 0, sizeof(path_t *)* ABSMAX_SKIPLIST_LEVEL); \
	pNew->pathing.bLevel = RandomSkipListLevel(); \
}\
	else\
{\
	pNew = NULL; \
}\
}


#define SkipListRemoveHead()\
{\
	pDel = pQueueHead->pNext[0]; \
for (iLoop = 0; iLoop < __min(bSkipListLevel, pDel->bLevel); iLoop++)\
{\
	pQueueHead->pNext[iLoop] = pDel->pNext[iLoop]; \
}\
	iSkipListSize--; \
	ClosedListAdd(pDel); \
}

#define SkipListInsert( pNew )\
{\
	pCurr = pQueueHead; \
	uiCost = TOTALCOST(pNew); \
	memset(pUpdate, 0, MAX_SKIPLIST_LEVEL * sizeof(path_t *)); \
for (iCurrLevel = bSkipListLevel - 1; iCurrLevel >= 0; iCurrLevel--)\
{\
	pNext = pCurr->pNext[iCurrLevel]; \
while (pNext)\
{\
if (uiCost > TOTALCOST(pNext) || (uiCost == TOTALCOST(pNext) && FARTHER(pNew, pNext)))\
{\
	pCurr = pNext; \
	pNext = pCurr->pNext[iCurrLevel]; \
}\
			else\
{\
	break; \
}\
}\
	pUpdate[iCurrLevel] = pCurr; \
}\
	pCurr = pCurr->pNext[0]; \
for (iCurrLevel = 0; iCurrLevel < pNew->pathing.bLevel; iCurrLevel++)\
{\
if (!(pUpdate[iCurrLevel]))\
{\
	break; \
}\
	pNew->pNext[iCurrLevel] = pUpdate[iCurrLevel]->pNext[iCurrLevel]; \
	pUpdate[iCurrLevel]->pNext[iCurrLevel] = pNew; \
}\
	iSkipListSize++; \
if (iSkipListSize > iSkipListLevelLimit[bSkipListLevel])\
{\
	pCurr = pQueueHead; \
	pNext = pQueueHead->pNext[bSkipListLevel - 1]; \
while (pNext)\
{\
if (pNext->pathing.bLevel > bSkipListLevel)\
{\
	pCurr->pNext[bSkipListLevel] = pNext; \
	pCurr = pNext; \
}\
	pNext = pNext->pNext[bSkipListLevel - 1]; \
}\
	pCurr->pNext[bSkipListLevel] = pNext; \
	bSkipListLevel++; \
}\
}

#define REMQUEHEADNODE()							SkipListRemoveHead();

#define DELQUENODE(ndx) SkipListRemoveHead()

#define REMAININGCOST(ptr)					\
	(\
	(dy = abs(iDestY - iLocY)), \
	(dx = abs(iDestX - iLocX)), \
	ESTIMATE						\
	)
/*
#define REMAININGCOST(ptr)					\
(								\
(locY = (ptr)->iLocation/MAPWIDTH),			\
(locX = (ptr)->iLocation%MAPWIDTH),			\
(dy = abs(iDestY-locY)),					\
(dx = abs(iDestX-locX)),					\
ESTIMATE						\
)
*/

#define NEWQUENODE ClosedListGet( pNewPtr )

#define QUEINSERT(ndx) SkipListInsert( ndx )

#define GREENSTEPSTART		0
#define REDSTEPSTART			16
#define PURPLESTEPSTART		32
#define BLUESTEPSTART			48
#define ORANGESTEPSTART		64

INT16 gubNPCAPBudget = 0;
UINT16 gusNPCMovementMode;
UINT8 gubNPCDistLimit = 0;
BOOLEAN gfNPCCircularDistLimit = FALSE;
UINT8	gubNPCPathCount;

BOOLEAN gfPlotDirectPath = FALSE;
BOOLEAN gfEstimatePath = FALSE;
BOOLEAN	gfPathAroundObstacles = TRUE;

// Lesh: fix CTD in Tixa
// The game was trying to store very long path (270 cells) in 256-cell array and corrupted
// some data, not belonged to array. It was ordinary array boundary break.
UINT32* guiPlottedPath = NULL;
UINT32* guiPathingData = NULL;
static INT32 giPathDataSize;
static INT32 giPlotCnt;
static INT32 guiEndPlotGridNo;

INT32 dirDelta[8] =
{
	-WORLD_COLS,		//N
	1 - WORLD_COLS,		//NE
	1,					//E
	1 + WORLD_COLS,		//SE
	WORLD_COLS,			//S
	WORLD_COLS - 1,		//SW
	-1,					//W
	-WORLD_COLS - 1		//NW
};

#define LOOPING_CLOCKWISE 0
#define LOOPING_COUNTERCLOCKWISE 1
#define LOOPING_REVERSE 2

#ifdef COUNT_PATHS
UINT32 guiSuccessfulPathChecks = 0;
UINT32 guiTotalPathChecks = 0;
UINT32 guiFailedPathChecks = 0;
UINT32 guiUnsuccessfulPathChecks = 0;
#endif


INT8 RandomSkipListLevel(void)
{
	INT8 bLevel = 1;

	while (Random(4) == 0 && bLevel < iMaxSkipListLevel - 1)
	{
		bLevel++;
	}
	return(bLevel);
}

void ReconfigurePathAI(INT32 iNewMaxSkipListLevel, INT32 iNewMaxTrailTree, INT32 iNewMaxPathQ)
{
	// make sure the specified parameters are reasonable
	iNewMaxSkipListLevel = __max(iNewMaxSkipListLevel, ABSMAX_SKIPLIST_LEVEL);
	iNewMaxTrailTree = __max(iNewMaxTrailTree, ABSMAX_TRAIL_TREE);
	iNewMaxPathQ = __max(iNewMaxPathQ, ABSMAX_PATHQ);
	// assign them
	iMaxSkipListLevel = iNewMaxSkipListLevel;
	iMaxTrailTree = iNewMaxTrailTree;
	iMaxPathQ = iNewMaxPathQ;
	// relocate the head of the closed list to the end of the array portion being used
	pClosedHead = &(pathQ[QPOOLNDX]);
	memset(pClosedHead, 0, sizeof(path_t));
}

void RestorePathAIToDefaults(void)
{
	iMaxSkipListLevel = MAX_SKIPLIST_LEVEL;
	iMaxTrailTree = MAX_TRAIL_TREE;
	iMaxPathQ = MAX_PATHQ;
	// relocate the head of the closed list to the end of the array portion being used
	pClosedHead = &(pathQ[QPOOLNDX]);
	memset(pClosedHead, 0, sizeof(path_t));
}

//dnl ch50 071009
BOOLEAN InitPathAI(void)
{
	guiPathingData = (UINT32*)MemAlloc(MAX_PATH_DATA_LENGTH * sizeof(UINT32));
	guiPlottedPath = (UINT32*)MemAlloc(MAX_PATH_DATA_LENGTH * sizeof(UINT32));
	pathQ = (path_t*)MemAlloc(ABSMAX_PATHQ * sizeof(path_t));
	trailCost = (UINT16*)MemAlloc(WORLD_MAX * sizeof(TRAILCELLTYPE));
	trailCostUsed = (UINT8*)MemAlloc(WORLD_MAX);
	trailTree = (trail_t*)MemAlloc(ABSMAX_TRAIL_TREE * sizeof(trail_t));
	if (!guiPlottedPath || !guiPathingData || !pathQ || !trailCost || !trailCostUsed || !trailTree)
		return(FALSE);
	pQueueHead = &pathQ[QHEADNDX];
	pClosedHead = &pathQ[QPOOLNDX];
	memset(trailCostUsed, 0, WORLD_MAX);
	RestorePathAIToDefaults();
	return(TRUE);
}

void ShutDownPathAI(void)
{
	MemFree(guiPathingData);
	MemFree(guiPlottedPath);
	MemFree(pathQ);
	MemFree(trailCostUsed);
	MemFree(trailCost);
	MemFree(trailTree);
}

///////////////////////////////////////////////////////////////////////
//	FINDBESTPATH													/
////////////////////////////////////////////////////////////////////////
INT32 FindBestPath(SOLDIERTYPE *s, INT32 sDestination, INT8 bLevel, INT16 usMovementMode, INT8 bCopy, UINT8 fFlags)
{
	s->sPlotSrcGrid = s->sGridNo;

	//__try
	//{
	INT32 iDestination = sDestination, iOrigination;
	INT32 iCnt = -1, iStructIndex;
	INT32 iLoopStart = 0, iLoopEnd = 0;
	INT8	bLoopState = LOOPING_CLOCKWISE;
	//BOOLEAN fLoopForwards = FALSE;
	BOOLEAN	fCheckedBehind = FALSE;
	UINT8	ubMerc;
	INT32 iDestX, iDestY, iLocX, iLocY, dx, dy;
	INT32	newLoc, curLoc;
	//INT32 curY;
	INT32 curCost, newTotCost, nextCost;
	INT32	sCurPathNdx;
	INT32 prevCost;
	INT32 iWaterToWater;
	INT16 ubCurAPCost, ubAPCost;
	INT16 ubNewAPCost = 0;
#ifdef VEHICLE
	//BOOLEAN fTurnSlow = FALSE;
	//BOOLEAN fReverse = FALSE; // stuff for vehicles turning
	BOOLEAN fMultiTile, fVehicle;
	//INT32 iLastDir, iPrevToLastDir;
	//INT8 bVehicleCheckDir;
	//UINT16 adjLoc;
	STRUCTURE_FILE_REF * pStructureFileRef = NULL;
	UINT16							usAnimSurface;
	//INT32 iCnt2, iCnt3;
	BOOLEAN fVehicleIgnoreObstacles = FALSE;
#endif

	INT32			iLastDir = 0;

	path_t *	pNewPtr;
	path_t *	pCurrPtr;

	path_t *	pUpdate[ABSMAX_SKIPLIST_LEVEL];
	path_t *	pCurr;
	path_t *	pNext;
	path_t *	pDel;
	UINT32		uiCost;
	INT32		iCurrLevel, iLoop;

	BOOLEAN		fHiddenStructVisible;				// Used for hidden struct visiblity
	UINT16		usOKToAddStructID = 0;

	BOOLEAN		fCopyReachable;
	BOOLEAN		fCopyPathCosts;
	BOOLEAN		fVisitSpotsOnlyOnce;
	INT32		iOriginationX, iOriginationY, iX, iY;

	BOOLEAN		fTurnBased;
	BOOLEAN		fPathingForPlayer;
	INT32		iDoorGridNo = -1;
	BOOLEAN		fDoorIsObstacleIfClosed = 0; // if false, door is obstacle if it is open
	DOOR_STATUS *pDoorStatus;
	DOOR		*pDoor;
	STRUCTURE	*pDoorStructure;
	BOOLEAN		fDoorIsOpen = FALSE;
	BOOLEAN		fNonFenceJumper;
	BOOLEAN		fNonSwimmer;
	BOOLEAN		fPathAroundPeople;
	BOOLEAN		fConsiderPersonAtDestAsObstacle;
	BOOLEAN		fGoingThroughDoor = FALSE; // for one tile
	BOOLEAN		fContinuousTurnNeeded;
	BOOLEAN		fCloseGoodEnough;
	UINT16		usMovementModeToUseForAPs;
	INT16		sClosePathLimit = 0;

	// sevenfm
	BOOLEAN		fSneaking = FALSE;
	BOOLEAN		fWornStealth = FALSE;
	BOOLEAN		fWeAttack = FALSE;
	BOOLEAN		fAllowComplexAI = AllowComplexAI(s);
	BOOLEAN		fCamo = FALSE;
	BOOLEAN		fSmartFlanking = FALSE;
	BOOLEAN		fFlashlight = FALSE;

#ifdef PATHAI_SKIPLIST_DEBUG
	CHAR8				zTempString[1000], zTS[50];
#endif

#ifdef PATHAI_VISIBLE_DEBUG
	UINT16		usCounter = 0;
#endif

	if (s->HighWornStealth())
		fWornStealth = TRUE;
	if (WeAttack(s->bTeam))
		fWeAttack = TRUE;
	if (s->HighWoodCamo() || s->HighDesertCamo() || s->HighUrbanCamo() || fWornStealth >= 65)
		fCamo = TRUE;

	// sevenfm: sneaky behavior
	if (s->bTeam == ENEMY_TEAM &&
		s->IsMercBodyType() &&		
		s->aiData.bAlertStatus >= STATUS_RED &&		
		!s->IsBoxer() &&
		!s->RushAttackActive() &&
		!MercUnderTheInfluence(s, DRUG_TYPE_ADRENALINE) &&
		s->aiData.bOrders != STATIONARY && 	
		(s->aiData.bAttitude == CUNNINGSOLO || 
		s->aiData.bAttitude == CUNNINGAID || 
		s->aiData.bAttitude == DEFENSIVE || 
		s->bTacticalOrder == TACTICAL_ORDER_FLANK_APPROACH || 
		s->aiData.bAIMorale < MORALE_FEARLESS || 
		fWornStealth ||
		fCamo ||
		s->bStealthMode ||
		AISpecialRole(s) || 
		AIRoleMedic(s)))
		//!GuySawEnemy(s, SEEN_LAST_TURN))
	{
		fSneaking = TRUE;

		if (fAllowComplexAI &&
			gfTurnBasedAI &&
			(s->bTacticalOrder == TACTICAL_ORDER_NONE || s->bTacticalOrder == TACTICAL_ORDER_RETREAT || s->bTacticalOrder == TACTICAL_ORDER_FLANK_APPROACH) &&
			//(s->aiData.bAIMorale < MORALE_FEARLESS || AISpecialRole(s) || AIRoleMedic(s)) &&
			//(s->aiData.bOrders == SEEKENEMY || s->aiData.bOrders == FARPATROL || AISpecialRole(s) || AIRoleMedic(s)) &&
			//(s->aiData.bAttitude == CUNNINGSOLO || s->aiData.bAttitude == CUNNINGAID || AISpecialRole(s) || AIRoleMedic(s)) &&
			//!GuySawEnemy(s, SEEN_LAST_TURN) &&
			(!(AICheckSuccessfulAttack(s, TRUE) && CountFriendsBlack(s)) && (fWornStealth || fCamo) || AISpecialRole(s) || AIRoleMedic(s) || s->bStealthMode))
		{
			fSmartFlanking = TRUE;
		}
	}

	if(s->GetBestEquippedFlashLightRange() > 0)
	{
		fFlashlight = TRUE;
	}

	fVehicle = FALSE;
	iOriginationX = iOriginationY = 0;
	iOrigination = s->sGridNo;

	if (TileIsOutOfBounds(sDestination) && sDestination != GRIDSIZE)//dnl ch50 071009 GRIDSIZE is use when want to travel all over the map, one of usage is when seeking suitable place for edge points
	{
		return(0); // There is no path to nowhere, because everywhere you can go is somewhere!
	}
	else if (sDestination < GRIDSIZE && !GridNoOnVisibleWorldTile(iDestination))//dnl ch53 111009
	{
		return(0);
	}

	if (TileIsOutOfBounds(iOrigination))
	{
#ifdef JA2BETAVERSION
		ScreenMsg( FONT_MCOLOR_RED, MSG_TESTVERSION, L"ERROR! Trying to calculate path for [%d] %s from off-world gridno %d to %d", s->ubID, s->GetName(), iOrigination, sDestination );
#endif
		DebugMiscInfo(String("FindBestPath: Trying to calculate path for [%d] %s from off-world gridno %d to %d", s->ubID, s->GetNameS().c_str(), iOrigination, sDestination));
		return(0);
	}
	else if (!GridNoOnVisibleWorldTile(iOrigination))
	{
#ifdef JA2BETAVERSION
		if(!is_networked)ScreenMsg( FONT_MCOLOR_RED, MSG_TESTVERSION, L"ERROR!	Trying to calculate path from non-visible gridno %d to %d", iOrigination, sDestination );
#endif
		return(0);
	}
	else if (s->pathing.bLevel != bLevel)
	{
		// pathing to a different level... bzzzt!
		return(0);
	}

	if (gubGlobalPathFlags)
	{
		fFlags |= gubGlobalPathFlags;
	}

	fTurnBased = ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT));

	fPathingForPlayer = ((s->bTeam == gbPlayerNum) && (!gTacticalStatus.fAutoBandageMode) && !(s->flags.uiStatusFlags & SOLDIER_PCUNDERAICONTROL));
	// sevenfm: allow jumping with a backpack
	//fNonFenceJumper = !( IS_MERC_BODY_TYPE( s ) ) || (UsingNewInventorySystem() == true && FindBackpackOnSoldier( s ) != ITEM_NOT_FOUND);//Moa: added backpack check
	fNonFenceJumper = !(IS_MERC_BODY_TYPE(s));
	fNonSwimmer = !(IS_MERC_BODY_TYPE(s));
	if (fNonSwimmer)
	{
		if (Water(sDestination, bLevel))
		{
			return(0);
		}
	}
	fPathAroundPeople = ((fFlags & PATH_THROUGH_PEOPLE) == 0);

	// anv: vehicles can drive through people and structures if shift is pressed 
	// (or rather route through can be planned, actual driving through is dependent on SOLDIER_RAM_THROUGH_OBSTACLES flag set in Handle UI)
	if (_KeyDown(SHIFT) && fPathingForPlayer && (s->flags.uiStatusFlags & SOLDIER_VEHICLE))
	{
		fVehicleIgnoreObstacles = TRUE;
	}
	// AI can't press shift, let's assume only tanks ram through (we wouldn't want AI cars to get destroyed kamikazing through walls, no matter how funny that would be)
	else if (!fPathingForPlayer && TANK(s))
	{
		fVehicleIgnoreObstacles = TRUE;
	}
	if (fVehicleIgnoreObstacles &&
		((gGameExternalOptions.fAllowCarsDrivingOverPeople && !TANK(s)) ||
		(gGameExternalOptions.fAllowTanksDrivingOverPeople && TANK(s))))
	{
		fPathAroundPeople = FALSE;
	}
	fCloseGoodEnough = ((fFlags & PATH_CLOSE_GOOD_ENOUGH) != 0);
	if (fCloseGoodEnough)
	{
		sClosePathLimit = __min(PythSpacesAway(s->sGridNo, sDestination) - 1, PATH_CLOSE_RADIUS);
		if (sClosePathLimit <= 0)
		{
			return(0);
		}
	}

	fConsiderPersonAtDestAsObstacle = (BOOLEAN)(fPathingForPlayer && fPathAroundPeople && !(fFlags & PATH_IGNORE_PERSON_AT_DEST));

	if (bCopy >= COPYREACHABLE)
	{
		fCopyReachable = TRUE;
		fCopyPathCosts = (bCopy == COPYREACHABLE_AND_APS);
		fVisitSpotsOnlyOnce = (bCopy == COPYREACHABLE);
		// make sure we aren't trying to copy path costs for an area greater than the AI array...
		if (fCopyPathCosts && gubNPCDistLimit > AI_PATHCOST_RADIUS)
		{
			// oy!!!! dis no supposed to happen!
			gubNPCDistLimit = AI_PATHCOST_RADIUS;
		}
	}
	else
	{
		fCopyReachable = FALSE;
		fCopyPathCosts = FALSE;
		fVisitSpotsOnlyOnce = FALSE;
	}

	gubNPCPathCount++;

	if (gubGlobalPathCount == 255)
	{
		// reset arrays!
		memset(trailCostUsed, 0, WORLD_MAX);
		gubGlobalPathCount = 1;
	}
	else
	{
		gubGlobalPathCount++;
	}

	// only allow nowhere destination if distance limit set	
	if (TileIsOutOfBounds(sDestination))
	{
		/*
		if (gubNPCDistLimit == 0)
		{
		return( FALSE );
		}
		*/
	}
	else
	{
		// the very first thing to do is make sure the destination tile is reachable
		if (!NewOKDestination(s, sDestination, fConsiderPersonAtDestAsObstacle, bLevel))
		{
			gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;
			return(FALSE);
		}

		if (sDestination == s->sGridNo)
		{
			return(FALSE);
		}
	}

	if (gubNPCAPBudget)
	{
		ubAPCost = MinAPsToStartMovement(s, usMovementMode);
		if (ubAPCost > gubNPCAPBudget)
		{
			gubNPCAPBudget = 0;
			gubNPCDistLimit = 0;
			return(0);
		}
		else
		{
			gubNPCAPBudget = gubNPCAPBudget - ubAPCost;
		}
	}

#ifdef COUNT_PATHS
	guiTotalPathChecks++;
#endif

#ifdef VEHICLE

	fMultiTile = ((s->flags.uiStatusFlags & SOLDIER_MULTITILE) != 0);
	if (fMultiTile)
	{
		// Get animation surface...
		// Chris_C... change this to use parameter.....
		usAnimSurface = DetermineSoldierAnimationSurface(s, usMovementMode);
		// Get structure ref...
		pStructureFileRef = GetAnimationStructureRef(s->ubID, usAnimSurface, usMovementMode);

		if (pStructureFileRef)
		{
			fVehicle = ((s->flags.uiStatusFlags & SOLDIER_VEHICLE) != 0);

			fContinuousTurnNeeded = ((s->flags.uiStatusFlags & (SOLDIER_MONSTER | SOLDIER_ANIMAL | SOLDIER_VEHICLE)) != 0);

			/*
			if (fVehicle && s->bReverse)
			{
			fReverse = TRUE;
			}
			*/
			/*
			if (fVehicle || s->ubBodyType == COW || s->ubBodyType == BLOODCAT) // or a vehicle
			{
			fTurnSlow = TRUE;
			}
			*/
			if (gfEstimatePath)
			{
				usOKToAddStructID = IGNORE_PEOPLE_STRUCTURE_ID;
			}
			else if (s->pLevelNode != NULL && s->pLevelNode->pStructureData != NULL)
			{
				usOKToAddStructID = s->pLevelNode->pStructureData->usStructureID;
			}
			else
			{
				usOKToAddStructID = INVALID_STRUCTURE_ID;
			}
			if (fVehicleIgnoreObstacles)
			{
				usOKToAddStructID = VEHICLE_IGNORE_OBSTACLES_STRUCTURE_ID;
			}
		}
		else
		{
			// turn off multitile pathing
			fMultiTile = FALSE;
			fContinuousTurnNeeded = FALSE;
		}

	}
	else
	{
		fContinuousTurnNeeded = FALSE;
	}

#endif

	if (!fContinuousTurnNeeded)
	{
		iLoopStart = 0;
		iLoopEnd = 0;
		bLoopState = LOOPING_CLOCKWISE;
	}

	ubCurAPCost = 0;
	queRequests = 2;

	//initialize the path data structures
	memset(pathQ, 0, iMaxPathQ * sizeof(path_t));
	memset(trailTree, 0, iMaxTrailTree * sizeof(trail_t));

#if defined( PATHAI_VISIBLE_DEBUG )
	if (gfDisplayCoverValues && gfDrawPathPoints)
	{
		memset( gsCoverValue, 0x7F, sizeof( INT16 ) * WORLD_MAX );
	}
#endif

	bSkipListLevel = 1;
	iSkipListSize = 0;
	iClosedListSize = 0;

	trailTreeNdx = 0;

	//set up common info
	if (fCopyPathCosts)
	{
		iOriginationY = (iOrigination / WORLD_COLS);
		iOriginationX = (iOrigination % WORLD_COLS);
	}

	iDestY = (iDestination / WORLD_COLS);
	iDestX = (iDestination % WORLD_COLS);

	// if origin and dest is water, then user wants to stay in water!
	// so, check and set waterToWater flag accordingly	
	if (TileIsOutOfBounds(iDestination))
	{
		iWaterToWater = 0;
	}
	else
	{
		if (ISWATER(gubWorldMovementCosts[iOrigination][0][bLevel]) && ISWATER(gubWorldMovementCosts[iDestination][0][bLevel]))
			iWaterToWater = 1;
		else
			iWaterToWater = 0;
	}

	//setup Q and first path record

	SETLOC(*pQueueHead, iOrigination);
	pQueueHead->usCostSoFar = MAXCOST;
	pQueueHead->bLevel = (INT8)(iMaxSkipListLevel - 1);

	pClosedHead->pNext[0] = pClosedHead;
	pClosedHead->pNext[1] = pClosedHead;

	//setup first path record
	iLocY = iOrigination / WORLD_COLS;
	iLocX = iOrigination % WORLD_COLS;

	SETLOC(pathQ[1], iOrigination);
	pathQ[1].sPathNdx = 0;
	pathQ[1].usCostSoFar = 0;
	if (fCopyReachable)
	{
		pathQ[1].usCostToGo = 100;
	}
	else
	{
		pathQ[1].usCostToGo = REMAININGCOST(&(pathQ[1]));
	}
	pathQ[1].usTotalCost = pathQ[1].usCostSoFar + pathQ[1].usCostToGo;
	pathQ[1].ubLegDistance = LEGDISTANCE(iLocX, iLocY, iDestX, iDestY);
	pathQ[1].bLevel = 1;
	pQueueHead->pNext[0] = &(pathQ[1]);
	iSkipListSize++;

	trailTreeNdx = 0;
	trailCost[iOrigination] = 0;
	pCurrPtr = pQueueHead->pNext[0];
	pCurrPtr->sPathNdx = trailTreeNdx;
	trailTreeNdx++;

	// sevenfm: move some functions out of loop
	INT16 sBackpackMovementPenalty = BackpackMovementPenalty(s);
	BOOLEAN fScubaFins = FALSE;
	BOOLEAN fKneePads = FALSE;

	if (s->inv[LEGPOS].exists() && HasItemFlag(s->inv[LEGPOS].usItem, SCUBA_FINS) || s->bTeam == ENEMY_TEAM && FindUsableItemWithFlag(s, SCUBA_FINS) != NO_SLOT)
		fScubaFins = TRUE;

	if (s->inv[LEGPOS].exists() && (Armour[Item[s->inv[LEGPOS].usItem].ubClassIndex].ubCoverage < 30 || HasAttachmentItemClass(&(s->inv[LEGPOS]), IC_ARMOUR)))
		fKneePads = TRUE;

	do
	{
		//remove the first and best path so far from the que
		pCurrPtr = pQueueHead->pNext[0];
		curLoc = pCurrPtr->iLocation;
		curCost = pCurrPtr->usCostSoFar;
		sCurPathNdx = pCurrPtr->sPathNdx;

		// remember the cost used to get here...
		prevCost = gubWorldMovementCosts[trailTree[sCurPathNdx].sGridNo][trailTree[sCurPathNdx].stepDir][bLevel];

#if defined( PATHAI_VISIBLE_DEBUG )
		if (gfDisplayCoverValues && gfDrawPathPoints)
		{
			if (gsCoverValue[ curLoc ] > 0)
			{
				gsCoverValue[ curLoc ] *= -1;
			}
		}
#endif

#ifdef VEHICLE
		/*
		if (fTurnSlow)
		{
		if (pCurrPtr->pNext[0] == 0)
		{
		if (fReverse)
		{
		iLastDir = gOppositeDirection[s->ubDirection];
		}
		else
		{
		iLastDir = s->ubDirection;
		}
		// start prev-to-last dir at same as current (could cause a problem)
		iPrevToLastDir = iLastDir;
		}
		else
		{
		iPrevToLastDir = trailTree[trailTree[pCurrPtr->sPathNdx].nextLink].dirDelta;
		iLastDir = trailTree[pCurrPtr->sPathNdx].dirDelta;
		}

		}
		*/
#endif

		if (gubNPCAPBudget)
		{
			ubCurAPCost = pCurrPtr->ubTotalAPCost;
		}
		if (fCopyReachable && prevCost != TRAVELCOST_FENCE)
		{
			gpWorldLevelData[curLoc].usFlags |= MAPELEMENT_REACHABLE;
			if (gubBuildingInfoToSet > 0)
			{
				gubBuildingInfo[curLoc] = gubBuildingInfoToSet;
			}
		}

		DELQUENODE(pCurrPtr);

		if (trailCostUsed[curLoc] == gubGlobalPathCount && trailCost[curLoc] < curCost)
			goto NEXTDIR;

		//DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "PATHAI %d", curLoc ) );

		// 0verhaul:	Cannot change direction over a fence!
		if (prevCost == TRAVELCOST_FENCE)
		{
			iLoopStart = trailTree[pCurrPtr->sPathNdx].stepDir;
			if (bLoopState == LOOPING_CLOCKWISE) {
				iLoopEnd = gOneCCDirection[iLoopStart];
			}
			else {
				iLoopEnd = gOneCDirection[iLoopStart];
			}
		}
		else
		{
			iLoopStart = iLoopEnd = 0;
		}

		if (fContinuousTurnNeeded)
		{
			if (trailTreeNdx < 2)
			{
				iLastDir = s->ubDirection;
			}
			else if (trailTree[pCurrPtr->sPathNdx].fFlags & STEP_BACKWARDS)
			{
				iLastDir = gOppositeDirection[trailTree[pCurrPtr->sPathNdx].stepDir];
			}
			else
			{
				iLastDir = trailTree[pCurrPtr->sPathNdx].stepDir;
			}
			iLoopStart = iLastDir;
			iLoopEnd = iLastDir;
			bLoopState = LOOPING_CLOCKWISE;
			fCheckedBehind = FALSE;
		}

		//contemplate a new path in each direction
		//for ( iCnt = iLoopStart; iCnt != iLoopEnd; iCnt = (iCnt + iLoopIncrement) % MAXDIR )
		for (iCnt = iLoopStart;;)
		{
#ifdef VEHICLE
			/*
			if (fTurnSlow)
			{
			if (iLastDir == iPrevToLastDir)
			{
			if ( iCnt != iLastDir && iCnt != gOneCDirection[ iLastDir ] && iCnt != gOneCCDirection[ iLastDir ])
			{
			goto NEXTDIR;
			}
			}
			else
			{
			if ( iCnt != iLastDir )
			{
			goto NEXTDIR;
			}
			}
			}
			*/

			if (bLoopState == LOOPING_REVERSE)
			{
				iStructIndex = gOppositeDirection[gOneCDirection[iCnt]];
			}
			else
			{
				iStructIndex = gOneCDirection[iCnt];
			}

			if (fMultiTile)
			{
				if (fContinuousTurnNeeded)
				{
					if (iCnt != iLastDir)
					{
						if (!OkayToAddStructureToWorld(curLoc, bLevel, &(pStructureFileRef->pDBStructureRef[iStructIndex]), usOKToAddStructID, FALSE, s->ubID))
						{
							// we have to abort this loop and possibly reset the loop conditions to
							// search in the other direction (if we haven't already done the other dir)
							if (bLoopState == LOOPING_CLOCKWISE)
							{
								iLoopStart = iLastDir;
								iLoopEnd = iCnt;
								bLoopState = LOOPING_COUNTERCLOCKWISE; // backwards
								// when we go to the bottom of the loop, iLoopIncrement will be added to iCnt
								// which is good since it avoids duplication of effort
								iCnt = iLoopStart;
								goto NEXTDIR;
							}
							else if (bLoopState == LOOPING_COUNTERCLOCKWISE && !fCheckedBehind)
							{
								// check rear dir
								bLoopState = LOOPING_REVERSE;

								// NB we're stuck with adding 1 to the loop counter down below so configure to accomodate...
								//iLoopStart = (iLastDir + (MAXDIR / 2) - 1) % MAXDIR;
								iLoopStart = gOppositeDirection[gOneCCDirection[iLastDir]];
								iLoopEnd = (iLoopStart + 2) % MAXDIR;
								iCnt = iLoopStart;
								fCheckedBehind = TRUE;
								goto NEXTDIR;
							}
							else
							{
								// done
								goto ENDOFLOOP;
							}

						}
					}
				}
				else if (pStructureFileRef)
				{
					// check to make sure it's okay for us to turn to the new direction in our current tile
					if (!OkayToAddStructureToWorld(curLoc, bLevel, &(pStructureFileRef->pDBStructureRef[iStructIndex]), usOKToAddStructID))
					{
						goto NEXTDIR;
					}
				}
			}

#endif

			newLoc = curLoc + dirDelta[iCnt];

			if (newLoc < 0 || newLoc >= WORLD_MAX)
			{
				goto NEXTDIR;
			}

			if (fVisitSpotsOnlyOnce && trailCostUsed[newLoc] == gubGlobalPathCount)
			{
				// on a "reachable" test, never revisit locations!
				goto NEXTDIR;
			}


			//if (gpWorldLevelData[newLoc].sHeight != ubLevel)
			//ATE: Movement onto cliffs? Check vs the soldier's gridno height
			// CJC: PREVIOUS LOCATION's height
			if (gpWorldLevelData[newLoc].ubHeight != gpWorldLevelData[curLoc].ubHeight)
			{
				goto NEXTDIR;
			}

			if (gubNPCDistLimit)
			{
				if (gfNPCCircularDistLimit)
				{
					if (PythSpacesAway(iOrigination, newLoc) > gubNPCDistLimit)
					{
						goto NEXTDIR;
					}
				}
				else
				{
					if (SpacesAway(iOrigination, newLoc) > gubNPCDistLimit)
					{
						goto NEXTDIR;
					}
				}
			}

			// AI check for mines
			if (gpWorldLevelData[newLoc].usFlags & MAPELEMENT_ENEMY_MINE_PRESENT && s->bSide != 0)
			{
				goto NEXTDIR;
			}

			// sevenfm: neutral civilians avoid all mines
			if ((gpWorldLevelData[newLoc].usFlags & (MAPELEMENT_ENEMY_MINE_PRESENT | MAPELEMENT_PLAYER_MINE_PRESENT)) &&
				s->bTeam == CIV_TEAM &&
				s->ubCivilianGroup == NON_CIV_GROUP &&
				//s->aiData.bNeutral &&
				s->ubProfile == NO_PROFILE)
			{
				goto NEXTDIR;
			}

			// sevenfm: skip deep water if not in deep water already
			if (!(s->flags.uiStatusFlags & SOLDIER_PC) &&
				s->ubProfile == NO_PROFILE &&
				s->aiData.bOrders != SEEKENEMY &&
				DeepWater(newLoc, bLevel) &&
				!DeepWater(s->sGridNo, bLevel) &&
				!FindNotDeepWaterNearby(newLoc, bLevel) &&
				!s->IsFlanking())
			{
				goto NEXTDIR;
			}
			// sevenfm: skip gas if not in gas already
			if ((!(s->flags.uiStatusFlags & SOLDIER_PC) || gTacticalStatus.fAutoBandageMode || s->flags.uiStatusFlags & SOLDIER_PCUNDERAICONTROL) &&
				InGasSpot(s, newLoc, bLevel) &&
				!InGasSpot(s, s->sGridNo, bLevel))
			{
				goto NEXTDIR;
			}

			// sevenfm: try to be more cautious - avoid light at night and fresh corpses
			/*if (s->bTeam == ENEMY_TEAM &&
				s->aiData.bAlertStatus >= STATUS_RED &&
				s->bStealthMode &&
				(InLightAtNight(newLoc, bLevel) && !InLightAtNight(s->sGridNo, bLevel) || 
				CorpseWarning(s, newLoc, bLevel, TRUE)) && !CorpseWarning(s, s->sGridNo, bLevel))
			{
				goto NEXTDIR;
			}*/

			// WANNE: Known mines (for enemy or player) do not explode - BEGIN
			if (gpWorldLevelData[newLoc].usFlags & (MAPELEMENT_ENEMY_MINE_PRESENT | MAPELEMENT_PLAYER_MINE_PRESENT))
			{
				if (s->bSide == 0)
				{
					// For our team, skip a location with a known mines unless it is the end of our
					// path; for others on our side, skip such locations completely;
					if (s->bTeam != gbPlayerNum || newLoc != iDestination)
					{
						if (gpWorldLevelData[newLoc].usFlags & MAPELEMENT_PLAYER_MINE_PRESENT)
						{
							goto NEXTDIR;
						}
					}
				}
				else
				{
					// For the enemy, always skip known mines
					if (gpWorldLevelData[newLoc].usFlags & MAPELEMENT_ENEMY_MINE_PRESENT)
					{
						goto NEXTDIR;
					}
				}
			}

			//how much is admission to the next tile
			if (gfPathAroundObstacles && !fVehicleIgnoreObstacles)
			{
				nextCost = gubWorldMovementCosts[newLoc][iCnt][bLevel];

				//ATE:	Check for differences from reality
				// Is next cost an obstacle
				if (nextCost == TRAVELCOST_HIDDENOBSTACLE)
				{
					if (fPathingForPlayer)
					{
						// Is this obstacle a hidden tile that has not been revealed yet?
						if (DoesGridNoContainHiddenStruct(newLoc, &fHiddenStructVisible))
						{
							// Are we not visible, if so use terrain costs!
							if (!fHiddenStructVisible)
							{
								// Set cost of terrain!
								nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
							}
						}
					}
				}
				else if (nextCost == TRAVELCOST_NOT_STANDING)
				{
					// for path plotting purposes, use the terrain value
					nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
				}
				else if (nextCost == TRAVELCOST_EXITGRID)
				{
					if (gfPlotPathToExitGrid)
					{
						// replace with terrain cost so that we can plot path, otherwise is obstacle
						nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
					}
				}
				else if (nextCost == TRAVELCOST_FENCE && fNonFenceJumper)
				{
					goto NEXTDIR;
				}
				else if (IS_TRAVELCOST_DOOR(nextCost))
				{
					// don't let anyone path diagonally through doors!
					if (iCnt & 1)
					{
						goto NEXTDIR;
					}

					switch (nextCost)
					{
					case TRAVELCOST_DOOR_CLOSED_HERE:
						fDoorIsObstacleIfClosed = TRUE;
						iDoorGridNo = newLoc;
						break;
					case TRAVELCOST_DOOR_CLOSED_N:
						fDoorIsObstacleIfClosed = TRUE;
						iDoorGridNo = newLoc + dirDelta[NORTH];
						break;
					case TRAVELCOST_DOOR_CLOSED_W:
						fDoorIsObstacleIfClosed = TRUE;
						iDoorGridNo = newLoc + dirDelta[WEST];
						break;
					case TRAVELCOST_DOOR_OPEN_HERE:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc;
						break;
					case TRAVELCOST_DOOR_OPEN_N:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTH];
						break;
					case TRAVELCOST_DOOR_OPEN_NE:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTHEAST];
						break;
					case TRAVELCOST_DOOR_OPEN_E:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[EAST];
						break;
					case TRAVELCOST_DOOR_OPEN_SE:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[SOUTHEAST];
						break;
					case TRAVELCOST_DOOR_OPEN_S:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[SOUTH];
						break;
					case TRAVELCOST_DOOR_OPEN_SW:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[SOUTHWEST];
						break;
					case TRAVELCOST_DOOR_OPEN_W:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[WEST];
						break;
					case TRAVELCOST_DOOR_OPEN_NW:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTHWEST];
						break;
					case TRAVELCOST_DOOR_OPEN_N_N:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTH] + dirDelta[NORTH];
						break;
					case TRAVELCOST_DOOR_OPEN_NW_N:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTHWEST] + dirDelta[NORTH];
						break;
					case TRAVELCOST_DOOR_OPEN_NE_N:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTHEAST] + dirDelta[NORTH];
						break;
					case TRAVELCOST_DOOR_OPEN_W_W:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[WEST] + dirDelta[WEST];
						break;
					case TRAVELCOST_DOOR_OPEN_SW_W:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[SOUTHWEST] + dirDelta[WEST];
						break;
					case TRAVELCOST_DOOR_OPEN_NW_W:
						fDoorIsObstacleIfClosed = FALSE;
						iDoorGridNo = newLoc + dirDelta[NORTHWEST] + dirDelta[WEST];
						break;
					default:
						break;
					}

					if (fPathingForPlayer && gpWorldLevelData[iDoorGridNo].usExtFlags[0] & MAPELEMENT_EXT_DOOR_STATUS_PRESENT)
					{
						// check door status
						pDoorStatus = GetDoorStatus(iDoorGridNo);
						if (pDoorStatus)
						{
							fDoorIsOpen = (pDoorStatus->ubFlags & DOOR_PERCEIVED_OPEN) != 0;
						}
						else
						{
							// door destroyed?
							nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
						}
					}
					else
					{
						// check door structure
						pDoorStructure = FindStructure(iDoorGridNo, STRUCTURE_ANYDOOR);
						if (pDoorStructure)
						{
							fDoorIsOpen = (pDoorStructure->fFlags & STRUCTURE_OPEN) != 0;
						}
						else
						{
							// door destroyed?
							nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
						}
					}

					// now determine movement cost... if it hasn't been changed already
					if (IS_TRAVELCOST_DOOR(nextCost))
					{
						if (fDoorIsOpen)
						{
							if (fDoorIsObstacleIfClosed)
							{
								nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
							}
							else
							{
								nextCost = TRAVELCOST_OBSTACLE;
							}
						}
						else
						{
							if (fDoorIsObstacleIfClosed)
							{
								// door is closed and this should be an obstacle, EXCEPT if we are calculating
								// a path for an enemy or NPC with keys
								if (fPathingForPlayer || (s && (s->flags.uiStatusFlags & SOLDIER_MONSTER || s->flags.uiStatusFlags & SOLDIER_ANIMAL)))
								{
									nextCost = TRAVELCOST_OBSTACLE;
								}
								else
								{
									// have to check if door is locked and NPC does not have keys!
									pDoor = FindDoorInfoAtGridNo(iDoorGridNo);
									if (pDoor)
									{
										if (!pDoor->fLocked || s->flags.bHasKeys)
										{
											// add to AP cost
											if (gubNPCAPBudget)
											{
												fGoingThroughDoor = TRUE;
											}
											nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
										}
										else
										{
											nextCost = TRAVELCOST_OBSTACLE;
										}
									}
									else
									{
										nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
									}
								}
							}
							else
							{
								nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
							}
						}
					}
				}
				else if ((nextCost == TRAVELCOST_SHORE || nextCost == TRAVELCOST_KNEEDEEP || nextCost == TRAVELCOST_DEEPWATER) && fNonSwimmer)
				{
					// creatures and animals can't go in water
					nextCost = TRAVELCOST_OBSTACLE;
				}

				// Apr. '96 - moved up be ahead of AP_Budget stuff
				if ((nextCost >= NOPASS)) // || ( nextCost == TRAVELCOST_DOOR ) )
					goto NEXTDIR;
			}
			else if (fVehicleIgnoreObstacles)
			{
				nextCost = TRAVELCOST_FLAT;
			}
			else
			{
				nextCost = TRAVELCOST_FLAT;
			}

			if (newLoc > GRIDSIZE)
			{
				// WHAT THE??? hack.
				goto NEXTDIR;
			}

			// if contemplated tile is NOT final dest and someone there, disqualify route
			// when doing a reachable test, ignore all locations with people in them
			if (fPathAroundPeople && ((newLoc != iDestination) || fCopyReachable))
			{
				// ATE: ONLY cancel if they are moving.....
				ubMerc = WhoIsThere2(newLoc, s->pathing.bLevel);

				// sevenfm: for player mercs, ignore invisible opponents
				if (ubMerc < TOTAL_SOLDIERS && 
					ubMerc != s->ubID && 
					(!s->IsSoldierPC() || MercPtrs[ubMerc]->bSide == s->bSide || MercPtrs[ubMerc]->aiData.bNeutral || MercPtrs[ubMerc]->bVisible >= 0 || LOS_Soldier(s, MercPtrs[ubMerc], LOS_NORMAL, CALC_FROM_ALL_DIRS)))
				{
					goto NEXTDIR;
				}
			}

#ifdef VEHICLE
			if (fMultiTile)
			{
				// vehicle test for obstacles: prevent movement to next tile if
				// a tile covered by the vehicle in that position & direction
				// has an obstacle in it

				// because of the order in which animations are stored (dir 7 first,
				// then 0 1 2 3 4 5 6), we must subtract 1 from the direction
				// ATE: Send in our existing structure ID so it's ignored!

				if (!OkayToAddStructureToWorld(newLoc, bLevel, &(pStructureFileRef->pDBStructureRef[iStructIndex]), usOKToAddStructID, FALSE, s->ubID))
				{
					goto NEXTDIR;
				}

				/*
				// vehicles aren't moving any more....
				if (fVehicle)
				{
				// transmogrify pathing costs for vehicles!
				switch(nextCost)
				{
				case TRAVELCOST_THICK		:	nextCost = TRAVELCOST_GRASS;
				break;
				case TRAVELCOST_SHORE		:
				case TRAVELCOST_KNEEDEEP:
				case TRAVELCOST_DEEPWATER:
				//						case TRAVELCOST_VEINEND	:
				//						case TRAVELCOST_VEINMID	:
				//case TRAVELCOST_DOOR		:
				case TRAVELCOST_FENCE		:	nextCost = TRAVELCOST_OBSTACLE;
				break;

				default									:	break;
				}
				}
				*/
			}
#endif

			// NEW Apr 21 by Ian: abort if cost exceeds budget
			if (gubNPCAPBudget)
			{
				switch (nextCost)
				{
				case TRAVELCOST_NONE: ubAPCost = 0;
					break;

				case TRAVELCOST_DIRTROAD:
				case TRAVELCOST_FLAT: ubAPCost = APBPConstants[AP_MOVEMENT_FLAT];
					break;
				//case TRAVELCOST_BUMPY	:
				case TRAVELCOST_GRASS: ubAPCost = APBPConstants[AP_MOVEMENT_GRASS];
					break;
				case TRAVELCOST_THICK:	ubAPCost = APBPConstants[AP_MOVEMENT_BUSH];
					break;
				case TRAVELCOST_DEBRIS: ubAPCost = APBPConstants[AP_MOVEMENT_RUBBLE];
					break;
				case TRAVELCOST_SHORE: ubAPCost = APBPConstants[AP_MOVEMENT_SHORE]; // wading shallow water
					break;
				case TRAVELCOST_KNEEDEEP:	ubAPCost = APBPConstants[AP_MOVEMENT_LAKE]; // wading waist/chest deep - very slow
					break;

				case TRAVELCOST_DEEPWATER:ubAPCost = APBPConstants[AP_MOVEMENT_OCEAN]; // can swim, so it's faster than wading
					break;

				case TRAVELCOST_FENCE:
					// SANDRO - slightly changed
					if ((UsingNewInventorySystem() == true) && FindBackpackOnSoldier(s) != ITEM_NOT_FOUND)
						ubAPCost = GetAPsToJumpFence(s, TRUE);
					else
						ubAPCost = GetAPsToJumpFence(s, FALSE);
					break;

				case TRAVELCOST_OBSTACLE:
				default:	goto NEXTDIR;	// Cost too much to be considered!
					break;
				}

				usMovementModeToUseForAPs = usMovementMode;

				// ATE: if water, force to be walking always!
				if (nextCost == TRAVELCOST_SHORE || nextCost == TRAVELCOST_KNEEDEEP || nextCost == TRAVELCOST_DEEPWATER)
				{
					usMovementModeToUseForAPs = WALKING;
				}

				// adjust AP cost for movement mode
				// CHRISL: Adjusted system to use different move costs while wearing a backpack
				// SANDRO - slightly changed this to add backpack modifier after Athletics trait check
				switch (usMovementModeToUseForAPs)
				{
				case RUNNING:
				case ADULTMONSTER_WALKING:
					ubAPCost += APBPConstants[AP_MODIFIER_RUN];
					if (s->IsVehicle() && (gpWorldLevelData[newLoc].ubTerrainID == PAVED_ROAD || gpWorldLevelData[newLoc].ubTerrainID == DIRT_ROAD) && ubAPCost > 2)
					{
						ubAPCost -= 2 * APBPConstants[AP_MAXIMUM] / 100;
					}
					break;
				case WALKING_WEAPON_RDY:
				case WALKING_DUAL_RDY:
					ubAPCost += APBPConstants[AP_MODIFIER_WALK] + APBPConstants[AP_MODIFIER_READY];	//WALKCOST);
					break;
				case WALKING_ALTERNATIVE_RDY:
					ubAPCost += APBPConstants[AP_MODIFIER_WALK];	//WALKCOST);
					break;
				case WALKING:
					ubAPCost += APBPConstants[AP_MODIFIER_WALK];	//WALKCOST);
					if (!(s->MercInWater()) && ((gAnimControl[s->usAnimState].uiFlags & ANIM_FIREREADY) || (gAnimControl[s->usAnimState].uiFlags & ANIM_FIRE)))
					{
						ubAPCost += APBPConstants[AP_MODIFIER_READY];	//WALKCOST);
					}
					break;
				case ROBOT_WALK:
					ubAPCost += APBPConstants[AP_MODIFIER_WALK];	//WALKCOST);
					break;
				case SWATTING:
					ubAPCost += APBPConstants[AP_MODIFIER_SWAT];	//SWATCOST);
					break;
				case CRAWLING:
					ubAPCost += APBPConstants[AP_MODIFIER_CRAWL];	//CRAWLCOST);
					// sevenfm: kneepads help crawling
					if (fKneePads)
						ubAPCost -= APBPConstants[AP_MAXIMUM] / 100;
					break;
				default:
					ubAPCost += APBPConstants[AP_MODIFIER_WALK];
					break;
				}

				// Check for reverse mode
				if (s->bReverse)
					ubAPCost += APBPConstants[AP_REVERSE_MODIFIER];

				///////////////////////////////////////////////////////////////////////////////////////////////
				// SANDRO - STOMP traits - Athletics trait decreases movement cost
				if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(s, ATHLETICS_NT))
				{
					ubAPCost = max(1, (INT16)(((FLOAT)ubAPCost * (100.0f - (FLOAT)gSkillTraitValues.ubATAPsMovementReduction) / 100.0f) + 0.5f));
				}

				ubAPCost += sBackpackMovementPenalty;

				// Moa: scuba fins and swimming background
				if (fScubaFins)
				{
					if (TERRAIN_IS_HIGH_WATER(gpWorldLevelData[newLoc].ubTerrainID))
					{
						ubAPCost /= 2;
					}
					else if (s->bTeam != ENEMY_TEAM)
					{
						ubAPCost *= 2;
					}
				}

				if (TERRAIN_IS_HIGH_WATER(gpWorldLevelData[newLoc].ubTerrainID))
				{
					ubAPCost = ubAPCost * (100 + s->GetBackgroundValue(BG_SWIMMING)) / 100;
				}

				// Flugente: dragging someone
				if (s->IsDragging())
				{
					ubAPCost = (INT16)(ubAPCost * gItemSettings.fDragAPCostModifier);
				}

				// Check for stealth mode
				if (s->bStealthMode)
				{
					// STOMP traits - Stealthy trait decreases stealth AP modifier
					if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(s, STEALTHY_NT))
					{
						ubAPCost += (max(0, (INT16)(((FLOAT)APBPConstants[AP_STEALTH_MODIFIER] * (FLOAT)(100 - gSkillTraitValues.ubSTStealthModeSpeedBonus) / 100.0f) + 0.5f)));
					}
					else
					{
						ubAPCost += APBPConstants[AP_STEALTH_MODIFIER];
					}
				}				

				// don't make the mistake of adding directly to
				// ubCurAPCost, that must be preserved for remaining dirs!
				if (iCnt & 1)
				{
					ubAPCost = (INT16)(ubAPCost * 14.0f / 10.0f + 0.5f);
				}

				if (nextCost == TRAVELCOST_FENCE)
				{
					switch (usMovementModeToUseForAPs)
					{
					case RUNNING:
					case WALKING:
					case WALKING_WEAPON_RDY:
					case WALKING_DUAL_RDY:
					case WALKING_ALTERNATIVE_RDY:
						// Here pessimistically assume the path will continue after hopping the fence
						ubAPCost += GetAPsCrouch(s, TRUE);
						break;

					case SWATTING:

						// Add cost to stand once there BEFORE jumping....
						ubAPCost += GetAPsCrouch(s, TRUE);
						break;

					case CRAWLING:

						// r8635
						// Add cost to stand up before jump and go prone after jump
						ubAPCost += GetAPsCrouch(s, TRUE) + (2 * GetAPsProne(s, TRUE));
						// Can't do it here.....
						//goto NEXTDIR;
						break;
					}
				}
				else if (nextCost == TRAVELCOST_NOT_STANDING)
				{
					switch (usMovementModeToUseForAPs)
					{
					case RUNNING:
					case WALKING:
					case WALKING_WEAPON_RDY:
					case WALKING_DUAL_RDY:
					case WALKING_ALTERNATIVE_RDY:
						// charge crouch APs for ducking head!
						ubAPCost += GetAPsCrouch(s, TRUE);
						break;

					default:
						break;
					}
				}
				else if (fGoingThroughDoor)
				{
					ubAPCost += GetAPsToOpenDoor(s);
					fGoingThroughDoor = FALSE;
				}

				ubNewAPCost = ubCurAPCost + ubAPCost;

				if (ubNewAPCost > gubNPCAPBudget)
					goto NEXTDIR;
			}

			if (fCloseGoodEnough)
			{
				if (PythSpacesAway(newLoc, sDestination) <= sClosePathLimit)
				{
					// stop the path here!
					iDestination = newLoc;
					sDestination = newLoc;
					fCloseGoodEnough = FALSE;
				}
			}
			//make the destination look very attractive
			if (newLoc == iDestination)
				nextCost = 0;
			//?
			else if (gfPlotDirectPath && nextCost < NOPASS)
				nextCost = TRAVELCOST_FLAT;

			// make water cost attractive for water to water paths
			if (iWaterToWater)
			{
				if (ISWATER(prevCost))
					prevCost = EASYWATERCOST;
				if (ISWATER(nextCost))
					nextCost = EASYWATERCOST;
			}

			// NOTE: on September 24, 1997, Chris went back to a diagonal bias system
			// SANDRO - you know this is here twice if gubNPCAPBudget is set
			if (iCnt & 1 && !gubNPCAPBudget)
			{
				// moving on a diagonal
				//nextCost = gubDiagCost[nextCost];
				nextCost = (INT32)(nextCost * 14.0f / 10.0f + 0.5f);
				//nextCost++;
			}

			if (bLoopState == LOOPING_REVERSE)
			{
				// penalize moving backwards to encourage turning sooner
				nextCost += 50;
			}

			// sevenfm: penalize turns for vehicle
			/*if(s->IsVehicle())
			{
				if (iCnt == iLastDir || iCnt == gOneCDirection[iLastDir] || iCnt == gOneCCDirection[iLastDir])
					nextCost += 0;
				else if (iCnt == gOppositeDirection[iLastDir] || iCnt == gOneCDirection[gOppositeDirection[iLastDir]] || iCnt == gOneCCDirection[gOppositeDirection[iLastDir]])
					nextCost += 40;
				else
					nextCost += 20;
			}*/

			// boxers try to avoid slipping on blood and marbles
			if (s->IsBoxer() &&
				gTacticalStatus.bBoxingState == BOXING &&
				//(MarblesExistAtLocation(newLoc, bLevel, NULL) || bLevel == 0 && GetTerrainType(newLoc) == FLAT_FLOOR && TypeRangeExistsInObjectLayer(newLoc, HUMANBLOOD, CREATUREBLOOD, NULL)))
				(MarblesExistAtLocation(newLoc, bLevel, NULL) || (bLevel > 0 || GetTerrainType(newLoc) == FLAT_FLOOR) && BloodStrength(newLoc, bLevel) > 0))
			{
				nextCost += 20;
			}

			// bloodcats/zombies avoid light
			if (s->bTeam != gbPlayerNum &&
				(s->IsZombie() || s->IsBloodcat()) &&
				!AreInMeanwhile())
			{
				if (s->IsBloodcat() && InGasSpot(s, newLoc, bLevel))
				{
					nextCost += 40;
				}
				else if ((s->aiData.bAlertStatus == STATUS_RED || s->IsZombie() && s->aiData.bAlertStatus < STATUS_BLACK) &&
						!GuySawEnemy(s, SEEN_THIS_TURN) &&
						!CountFriendsBlack(s, NOWHERE) &&
						(InLightAtNight(newLoc, bLevel) || s->IsBloodcat() && CorpseWarning(s, newLoc, bLevel)))
				{
					nextCost += 20;
				}
			}

			// penalize locations with light and fresh corpses
			if (s->bTeam != gbPlayerNum &&
				!s->IsBoxer() &&
				!AreInMeanwhile() &&
				s->IsMercBodyType() &&
				!s->IsZombie())
			{
				// common penalty
				if (InGasSpot(s, newLoc, bLevel))
				{
					nextCost += 40;
				}
				// for enemy soldiers
				else if (s->aiData.bAlertStatus >= STATUS_RED)
				{
					if (InLightAtNight(newLoc, bLevel) && !fFlashlight || CorpseWarning(s, newLoc, bLevel))
					{
						nextCost += 40;
					}
					else if (s->bTeam == ENEMY_TEAM && (s->bStealthMode || fSneaking))
					{
						if (s->HighWoodCamo() && !TerrainJungle(newLoc, bLevel))
						{
							nextCost += 20;
						}
						else if (s->HighDesertCamo() && !TerrainDesert(newLoc, bLevel))
						{
							nextCost += 20;
						}
						else if (s->HighUrbanCamo() && !TerrainUrban(newLoc, bLevel))
						{
							nextCost += 20;
						}
						else if (fWornStealth && !FindShadowAtSpot(newLoc, bLevel))
						{
							nextCost += 20;
						}
						else if (fSmartFlanking && !PathCoverAtSpot(s, newLoc, bLevel))
						{
							nextCost += 20;
							//nextCost += CalcSpotDanger(s, newLoc, bLevel) / 4;
						}
						/*else if (fAllowComplexAI &&
							(usMovementMode == CRAWLING && !SightCoverAtSpot(s, newLoc, ANIM_PRONE, FALSE, FALSE) ||
							usMovementMode == SWATTING && !SightCoverAtSpot(s, newLoc, ANIM_CROUCH, FALSE, FALSE)))
							//(usMovementMode == CRAWLING && !SightCoverAtSpot(s, newLoc, ANIM_PRONE, FALSE, FALSE) && TerrainDensity(newLoc, bLevel, 1, TRUE) < 30 || 
							//usMovementMode == SWATTING && !AnyCoverAtSpot(s, newLoc, FALSE) && TerrainDensity(newLoc, bLevel, 1, FALSE) < 30))
						{
							nextCost += 20;
						}*/
						/*else if (fAllowComplexAI && usMovementMode <= SWATTING && s->aiData.bAIMorale < MORALE_FEARLESS && !AnyCoverAtSpot(s, newLoc))
						{
							nextCost += 20;
						}*/
						/*else if (fAllowComplexAI && usMovementMode <= SWATTING && CountObstaclesNearSpot(newLoc, bLevel) == 0)
						{
							nextCost += 20;
						}*/
						/*else if (fSmartFlanking && CheckDangerousDirection(s, newLoc, bLevel))
						{
							nextCost += 20;
						}*/
						/*else if (fAllowComplexAI && 
								(AICheckIsMedic(s) || AICheckIsSniper(s)) && !AnyCoverAtSpot(s, newLoc, FALSE) && !SightCoverAtSpot(s, newLoc, ANIM_STAND, FALSE, FALSE))
						{
							nextCost += 20;
						}
						else if (fAllowComplexAI &&
								(usMovementMode == CRAWLING && !SightCoverAtSpot(s, newLoc, ANIM_PRONE, FALSE, FALSE) || usMovementMode == SWATTING && !AnyCoverAtSpot(s, newLoc, FALSE)))
						{
							nextCost += 20;
						}*/					
					}
				}
			}

			newTotCost = curCost + nextCost;

			// have we found a path to the current location that
			// costs less than the best so far to the same location?
			if (trailCostUsed[newLoc] != gubGlobalPathCount || newTotCost < trailCost[newLoc])
			{

#if defined( PATHAI_VISIBLE_DEBUG )

				if (gfDisplayCoverValues && gfDrawPathPoints)
				{
					if (gsCoverValue[newLoc] == 0x7F7F)
					{
						gsCoverValue[newLoc] = usCounter++;
					}
					/*
					else if (gsCoverValue[newLoc] >= 0)
					{
					gsCoverValue[newLoc]++;
					}
					else
					{
					gsCoverValue[newLoc]--;
					}
					*/
				}
#endif

				//NEWQUENODE;
				{
					if (queRequests < QPOOLNDX)
					{
						pNewPtr = pathQ + (queRequests);
						queRequests++;
						memset( pNewPtr->pNext, 0, sizeof( path_t *) * ABSMAX_SKIPLIST_LEVEL );
						pNewPtr->bLevel = RandomSkipListLevel();
					}
					else if (iClosedListSize > 0)
					{
						pNewPtr = pClosedHead->pNext[0];
						pClosedHead->pNext[0] = pNewPtr->pNext[0];
						iClosedListSize--;
						queRequests++;
						memset( pNewPtr->pNext, 0, sizeof( path_t *) * ABSMAX_SKIPLIST_LEVEL );
						pNewPtr->bLevel = RandomSkipListLevel();
					}
					else
					{
						pNewPtr = NULL;
					}
				}

				if (pNewPtr == NULL)
				{
#ifdef COUNT_PATHS
					guiFailedPathChecks++;
#endif
					gubNPCAPBudget = 0;
					gubNPCDistLimit = 0;
					return(0);
				}

				//make new path to current location
				trailTree[trailTreeNdx].nextLink = sCurPathNdx;
				trailTree[trailTreeNdx].stepDir = (INT8)iCnt;
				if (bLoopState == LOOPING_REVERSE)
				{
					trailTree[trailTreeNdx].fFlags = STEP_BACKWARDS;
				}
				else
				{
					trailTree[trailTreeNdx].fFlags = 0;
				}
				trailTree[trailTreeNdx].sGridNo = newLoc;
				pNewPtr->sPathNdx = trailTreeNdx;
				trailTreeNdx++;

				if (trailTreeNdx >= iMaxTrailTree)
				{
#ifdef COUNT_PATHS
					guiFailedPathChecks++;
#endif
					gubNPCAPBudget = 0;
					gubNPCDistLimit = 0;
					return(0);
				}

				iLocY = newLoc / WORLD_COLS;
				iLocX = newLoc % WORLD_COLS;
				SETLOC(*pNewPtr, newLoc);
				pNewPtr->usCostSoFar = (UINT16)newTotCost;
				pNewPtr->usCostToGo = (UINT16)REMAININGCOST(pNewPtr);
				if (fCopyReachable)
				{
					pNewPtr->usCostToGo = 100;
				}
				else
				{
					pNewPtr->usCostToGo = (UINT16)REMAININGCOST(pNewPtr);
				}

				pNewPtr->usTotalCost = newTotCost + pNewPtr->usCostToGo;
				pNewPtr->ubLegDistance = (UINT8)LEGDISTANCE(iLocX, iLocY, iDestX, iDestY);

				if (gubNPCAPBudget)
				{
					//save the AP cost so far along this path
					pNewPtr->ubTotalAPCost = ubNewAPCost;
					// update the AP costs in the AI array of path costs if necessary...
					if (fCopyPathCosts)
					{
						iX = AI_PATHCOST_RADIUS + iLocX - iOriginationX;
						iY = AI_PATHCOST_RADIUS + iLocY - iOriginationY;
						gubAIPathCosts[iX][iY] = ubNewAPCost;
					}
				}

				//update the trail map to reflect the newer shorter path
				trailCost[newLoc] = (UINT16)newTotCost;
				trailCostUsed[newLoc] = gubGlobalPathCount;

				//do a sorted que insert of the new path
				// COMMENTED OUT TO DO BOUNDS CHECKER CC JAN 18 99
				//QUEINSERT(pNewPtr);
				//#define SkipListInsert( pNewPtr )
				{
					pCurr = pQueueHead;
					uiCost = TOTALCOST(pNewPtr);
					memset(pUpdate, 0, MAX_SKIPLIST_LEVEL * sizeof(path_t *));
					AssertMsg((bSkipListLevel <= MAX_SKIPLIST_LEVEL), String("bSkipListLevel=%d but must be less then %d", bSkipListLevel, MAX_SKIPLIST_LEVEL));//dnl ch50 071009
					for (iCurrLevel = bSkipListLevel - 1; iCurrLevel >= 0; iCurrLevel--)
					{
						pNext = pCurr->pNext[iCurrLevel];
						while (pNext)
						{
							if (uiCost > TOTALCOST(pNext) || (uiCost == TOTALCOST(pNext) && FARTHER(pNewPtr, pNext)))
							{
								pCurr = pNext;
								pNext = pCurr->pNext[iCurrLevel];
							}
							else
							{
								break;
							}
						}
						pUpdate[iCurrLevel] = pCurr;
					}
					pCurr = pCurr->pNext[0];
					for (iCurrLevel = 0; iCurrLevel < pNewPtr->bLevel; iCurrLevel++)
					{
						if (!(pUpdate[iCurrLevel]))
						{
							break;
						}
						pNewPtr->pNext[iCurrLevel] = pUpdate[iCurrLevel]->pNext[iCurrLevel];
						pUpdate[iCurrLevel]->pNext[iCurrLevel] = pNewPtr;
					}
					iSkipListSize++;
					if (iSkipListSize > iSkipListLevelLimit[bSkipListLevel])
					{
						pCurr = pQueueHead;
						pNext = pQueueHead->pNext[bSkipListLevel - 1];
						while (pNext)
						{
							if (pNext->bLevel > bSkipListLevel)
							{
								pCurr->pNext[bSkipListLevel] = pNext;
								pCurr = pNext;
							}
							pNext = pNext->pNext[bSkipListLevel - 1];
						}
						pCurr->pNext[bSkipListLevel] = pNext;
						bSkipListLevel++;
					}
				}

#ifdef PATHAI_SKIPLIST_DEBUG
				// print out contents of queue
				{
					INT32				iLoop;
					INT8				bTemp;

					zTempString[0] = '\0';
					pCurr = pQueueHead;
					iLoop = 0;
					while (pCurr)
					{
						sprintf(zTS, "\t%ld", pCurr->sPathNdx);
						if (pCurr == pNewPtr)
						{
							strcat(zTS, "*");
						}
						strcat(zTempString, zTS);
						pCurr = pCurr->pNext[0];
						iLoop++;
						if (iLoop > 50)
						{
							break;
						}
					}
					DebugMsg( TOPIC_JA2, DBG_LEVEL_3, zTempString );


					zTempString[0] = '\0';
					pCurr = pQueueHead;
					iLoop = 0;
					while( pCurr )
					{
						sprintf( zTS, "\t%d", pCurr->pathing.bLevel );
						strcat( zTempString, zTS );
						pCurr = pCurr->pNext[0];
						iLoop++;
						if (iLoop > 50)
						{
							break;
						}
					}
					DebugMsg( TOPIC_JA2, DBG_LEVEL_3, zTempString );

					zTempString[0] = '\0';
					bTemp = pQueueHead->pathing.bLevel;
					pCurr = pQueueHead;
					iLoop = 0;
					while( pCurr )
					{
						bTemp = pQueueHead->pathing.bLevel;
						while ( pCurr->pNext[ bTemp - 1 ] == NULL )
						{
							bTemp--;
						}
						sprintf( zTS, "\t%d", bTemp );
						strcat( zTempString, zTS );
						pCurr = pCurr->pNext[0];
						iLoop++;
						if (iLoop > 50)
						{
							break;
						}
					}
					DebugMsg( TOPIC_JA2, DBG_LEVEL_3, zTempString );
					DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "------" );
				}
#endif

			}

		NEXTDIR:
			if (bLoopState == LOOPING_CLOCKWISE) // backwards
			{
				iCnt = gOneCCDirection[ iCnt ];
			}
			else
			{
				iCnt = gOneCDirection[ iCnt ];
			}
			if ( iCnt == iLoopEnd )
			{
			ENDOFLOOP:
				break;
			}
			else if ( fContinuousTurnNeeded && iCnt == gOppositeDirection[ iLoopStart ] )
			{
				fCheckedBehind = TRUE;
			}
		}
	}
	while (pathQNotEmpty && pathNotYetFound);


#if defined( PATHAI_VISIBLE_DEBUG )
	if (gfDisplayCoverValues && gfDrawPathPoints)
	{
		SetRenderFlags(RENDER_FLAG_FULL);
		if (guiCurrentScreen == GAME_SCREEN)
		{
			RenderWorld();
			RenderCoverDebug();
			InvalidateScreen();
			EndFrameBufferRender();
			RefreshScreen(NULL);
		}
	}
#endif

	// work finished. Did we find a path?
	if (pathQNotEmpty && pathFound)
	{
		//dnl ch53 121009
		INT32 _nextLink;
		INT32 z, _z;

		_z = 0;
		z = pQueueHead->pNext[0]->sPathNdx;

		while(z)
		{
			_nextLink = trailTree[z].nextLink;
			trailTree[z].nextLink = _z;
			_z = z;
			z = _nextLink;
		}

		// if this function was called because a solider is about to embark on an actual route
		// (as opposed to "test" path finding (used by cursor, etc), then grab all pertinent
		// data and copy into soldier's database
		if (bCopy == COPYROUTE)
		{
			z = _z;

			for (iCnt = 0; z && (iCnt < MAX_PATH_LIST_SIZE); iCnt++)
			{
				s->pathing.usPathingData[iCnt] = (INT16)trailTree[z].stepDir;

				z = trailTree[z].nextLink;
			}

			s->pathing.usPathIndex = 0;
			s->pathing.usPathDataSize = (UINT16)iCnt;
		}
		else if (bCopy == NO_COPYROUTE)
		{
			z = _z;

			for (iCnt = 0; z != 0; iCnt++)
			{
				guiPathingData[iCnt] = trailTree[z].stepDir;

				z = trailTree[z].nextLink;
			}

			giPathDataSize = iCnt;

		}

#if defined( PATHAI_VISIBLE_DEBUG )
		if (gfDisplayCoverValues && gfDrawPathPoints)
		{
			SetRenderFlags(RENDER_FLAG_FULL);
			RenderWorld();
			RenderCoverDebug();
			InvalidateScreen();
			EndFrameBufferRender();
			RefreshScreen(NULL);
		}
#endif


		// return path length : serves as a "successful" flag and a path length counter
#ifdef COUNT_PATHS
		guiSuccessfulPathChecks++;
#endif
		gubNPCAPBudget = 0;
		gubNPCDistLimit = 0;

		//TEMP:	This is returning zero when I am generating edgepoints, so I am force returning 1 until
		//		this is fixed?
		if (gfGeneratingMapEdgepoints)
		{
			return TRUE;
		}

		//Assert(!iCnt);
		return(iCnt);
	}

#ifdef COUNT_PATHS
	if (fCopyReachable)
	{
		// actually this is a success
		guiSuccessfulPathChecks++;
	}
	else
	{
		guiUnsuccessfulPathChecks++;
	}
#endif

	// failed miserably, report...
	gubNPCAPBudget = 0;
	gubNPCDistLimit = 0;
	return(0);
}

void GlobalReachableTest(INT32 sStartGridNo)
{
	SOLDIERTYPE s;
	INT32 iCurrentGridNo = 0;

	// WDS - Clean up inventory handling
	//memset( &s, 0, SIZEOF_SOLDIERTYPE );
	s.initialize();
	s.sGridNo = sStartGridNo;
	s.pathing.bLevel = 0;
	s.bTeam = 1;

	//reset the flag for gridno's
	for( iCurrentGridNo =0; iCurrentGridNo < WORLD_MAX; iCurrentGridNo++ )
	{
		gpWorldLevelData[ iCurrentGridNo ].usFlags &= ~( MAPELEMENT_REACHABLE );
	}

	ReconfigurePathAI(ABSMAX_SKIPLIST_LEVEL, ABSMAX_TRAIL_TREE, ABSMAX_PATHQ);
	gubNPCDistLimit = 0;//dnl without this each save will not calculate properly map edgepoints
	FindBestPath(&s, GRIDSIZE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);//dnl ch50 071009
	RestorePathAIToDefaults();
}

void LocalReachableTest(INT32 sStartGridNo, INT8 bRadius)
{
	SOLDIERTYPE s;
	INT32 iCurrentGridNo = 0;
	INT32	iX, iY;

	// WDS - Clean up inventory handling
	//memset( &s, 0, SIZEOF_SOLDIERTYPE );
	s.initialize();
	s.sGridNo = sStartGridNo;

	//if we are moving on the ground level
	if (gsInterfaceLevel == I_ROOF_LEVEL)
	{
		s.pathing.bLevel = 1;
	}
	else
	{
		s.pathing.bLevel = 0;
	}

	s.bTeam = OUR_TEAM;

	//reset the flag for gridno's
	for (iY = -bRadius; iY <= bRadius; iY++)
	{
		for (iX = -bRadius; iX <= bRadius; iX++)
		{
			iCurrentGridNo = sStartGridNo + iX + iY * MAXCOL;
			if (iCurrentGridNo >= 0 && iCurrentGridNo <= WORLD_MAX)
			{
				gpWorldLevelData[iCurrentGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
			}
		}
	}

	// set the dist limit
	gubNPCDistLimit = bRadius;
	// make the function call
	FindBestPath(&s, GRIDSIZE, s.pathing.bLevel, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);//dnl ch50 071009
	// reset dist limit
	gubNPCDistLimit = 0;
}

void GlobalItemsReachableTest(INT32 sStartGridNo1, INT32 sStartGridNo2)
{
	SOLDIERTYPE s;
	INT32 iCurrentGridNo = 0;

	// WDS - Clean up inventory handling
	//memset( &s, 0, SIZEOF_SOLDIERTYPE );
	s.initialize();
	s.sGridNo = sStartGridNo1;
	s.pathing.bLevel = 0;
	s.bTeam = 1;

	//reset the flag for gridno's
	for (iCurrentGridNo = 0; iCurrentGridNo < WORLD_MAX; iCurrentGridNo++)
	{
		gpWorldLevelData[iCurrentGridNo].usFlags &= ~(MAPELEMENT_REACHABLE);
	}

	ReconfigurePathAI(ABSMAX_SKIPLIST_LEVEL, ABSMAX_TRAIL_TREE, ABSMAX_PATHQ);
	FindBestPath(&s, GRIDSIZE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);//dnl ch50 071009

	if (!TileIsOutOfBounds(sStartGridNo2))
	{
		s.sGridNo = sStartGridNo2;
		FindBestPath(&s, GRIDSIZE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);//dnl ch50 071009
	}
	RestorePathAIToDefaults();
}

void RoofReachableTest(INT32 sStartGridNo, UINT8 ubBuildingID)
{
	SOLDIERTYPE s;
	INT32 sGridNo;

	// WDS - Clean up inventory handling
	//memset( &s, 0, SIZEOF_SOLDIERTYPE );
	s.initialize();
	s.sGridNo = sStartGridNo;
	s.pathing.bLevel = 1;
	s.bTeam = 1;

	// clearing flags

	//for( sGridNo = 0 ; sGridNo < NOWHERE ; ++sGridNo )
	//for( sGridNo = 0 ; sGridNo < MAX_MAP_POS; ++sGridNo )
	for (sGridNo = 0; sGridNo < WORLD_MAX; ++sGridNo)
	{
		gpWorldLevelData[sGridNo].usFlags &= (~MAPELEMENT_REACHABLE);
	}

	gubBuildingInfoToSet = ubBuildingID;

	ReconfigurePathAI(ABSMAX_SKIPLIST_LEVEL, ABSMAX_TRAIL_TREE, ABSMAX_PATHQ);
	FindBestPath(&s, GRIDSIZE, 1, WALKING, FINDCLIMBPOINTS, 0);//dnl ch50 071009
	RestorePathAIToDefaults();

	// set start position to reachable since path code sets it unreachable
	gpWorldLevelData[sStartGridNo].usFlags |= MAPELEMENT_REACHABLE;

	// reset building variable
	gubBuildingInfoToSet = 0;
}

void ErasePath(char bEraseOldOne)
{
	INT32 iCnt;

	// NOTE: This routine must be called BEFORE anything happens that changes
	//		a merc's gridno, else the....

	//EraseAPCursor();

	if (gfUIHandleShowMoveGrid)
	{
		gfUIHandleShowMoveGrid = FALSE;

		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS4);
		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS9);
		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS2);
		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS13);
		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS15);
		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS19);
		RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS20);
	}

	if (!gusPathShown)
	{
		//OldPath = FALSE;
		return;
	}

	gusPathShown = FALSE;

	for (iCnt = 0; iCnt < giPlotCnt; iCnt++)
	{
		//Grid[PlottedPath[cnt]].fstep = 0;

		RemoveAllObjectsOfTypeRange(guiPlottedPath[iCnt], FOOTPRINTS, FOOTPRINTS);

		RemoveAllOnRoofsOfTypeRange(guiPlottedPath[iCnt], FOOTPRINTS, FOOTPRINTS);

		//RemoveAllObjectsOfTypeRange( guiPlottedPath[iCnt], FIRSTPOINTERS, FIRSTPOINTERS );
	}

	giPlotCnt = 0;
	memset(guiPlottedPath, 0, 256 * sizeof(UINT32));
}

INT32 PlotPath(SOLDIERTYPE *pSold, INT32 sDestGridNo, INT8 bCopyRoute, INT8 bPlot, INT8 bStayOn, UINT16 usMovementMode, INT8 bStealth, INT8 bReverse, INT16 sAPBudget)
{
	INT16 sTileCost, sPoints = 0, sAnimCost = 0;
	INT16 sPointsWalk = 0, sPointsCrawl = 0, sPointsRun = 0, sPointsSwat = 0;
	INT16 sExtraCostStand, sExtraCostSwat, sExtraCostCrawl;
	INT16 sMovementAPsCost = 0; // added by SANDRO
	INT32 iLastGrid, sTempGrid;
	INT32 iCnt;
	INT32 sOldGrid = 0;
	INT16 sFootOrderIndex;
	INT16 sSwitchValue;
	INT16 sFootOrder[5] = { GREENSTEPSTART, PURPLESTEPSTART, BLUESTEPSTART,	ORANGESTEPSTART, REDSTEPSTART };
	UINT16	usTileIndex;
	UINT16	usTileNum;
	LEVELNODE	*pNode;
	UINT16 usMovementModeToUseForAPs;
	BOOLEAN	bIgnoreNextCost = FALSE;
	INT32    sTestGridNo;

	if (bPlot && gusPathShown)
	{
		ErasePath(FALSE);
	}

	gusAPtsToMove = 0;
	sTempGrid = pSold->sGridNo;

	sFootOrderIndex = 0;

	// check for active reverse mode ( ALT key pressed )
	if (gUIUseReverse)
		bReverse = gUIUseReverse;

	//gubNPCMovementMode = (UINT8) usMovementMode;
	// distance limit to reduce the cost of plotting a path to a location we can't reach

	// For now, use known hight adjustment
	//if ( gfRecalculatingExistingPathCost || FindBestPath( pSold, sDestGridNo, pSold->pathing.bLevel, usMovementMode, bCopyRoute, 0 ) )
	if (gfRecalculatingExistingPathCost || FindBestPath(pSold, sDestGridNo, pSold->pathing.bLevel, usMovementMode, bCopyRoute, gfEstimatePath ? PATH_IGNORE_PERSON_AT_DEST : 0))
	{
		// if soldier would be STARTING to run then he pays a penalty since it takes time to run full speed
		if (pSold->usAnimState != RUNNING)
		{
			// for estimation purposes, always pay penalty
			sPointsRun = GetAPsStartRun(pSold); // changed by SANDRO
		}

		//shadooow: a little hack to calculate correct AP cost because game doesn't allow to run in reverse and forces walk mode
		if (usMovementMode && (pSold->bReverse || bReverse))
		{
			usMovementMode = WALKING;
		}

		// Add to points, those needed to start from different stance!
		sPoints = sPoints + MinAPsToStartMovement(pSold, usMovementMode);

		// We should reduce points for starting to run if first tile is a fence...
		sTestGridNo = NewGridNo(pSold->sGridNo, DirectionInc((UINT8)guiPathingData[0]));

		// WANNE: Quickfix for wrong pathing data (direction). This fixes crash that could rarly occur
		if ((UINT8)guiPathingData[0] > 7)
		{
			guiPathingData[0] = 0;
		}

		if (gubWorldMovementCosts[sTestGridNo][guiPathingData[0]][pSold->pathing.bLevel] == TRAVELCOST_FENCE)
		{
			if (usMovementMode == RUNNING && pSold->usAnimState != RUNNING)
			{
				sPoints -= GetAPsStartRun(pSold); // changed by SANDRO
			}
		}

		// r8634
		//sPoints = sPoints + sAnimCost;
		//gusAPtsToMove = gusAPtsToMove + sAnimCost;

		if (bStayOn)
		{
			iLastGrid = giPathDataSize + 1;
		}
		else
		{
			iLastGrid = giPathDataSize;
		}

		// sevenfm: move functions out of loop
		BOOLEAN fKneePads = FALSE;
		BOOLEAN fScubaFins = FALSE;
		INT16 sBackpackMovementPenalty = BackpackMovementPenalty(pSold);

		if (pSold->inv[LEGPOS].exists() && (Armour[Item[pSold->inv[LEGPOS].usItem].ubClassIndex].ubCoverage < 30 || HasAttachmentItemClass(&(pSold->inv[LEGPOS]), IC_ARMOUR)))
			fKneePads = TRUE;

		if (pSold->inv[LEGPOS].exists() && HasItemFlag(pSold->inv[LEGPOS].usItem, SCUBA_FINS) || pSold->bTeam == ENEMY_TEAM && FindUsableItemWithFlag(pSold, SCUBA_FINS) != NO_SLOT)
			fScubaFins = TRUE;

		for (iCnt = 0; iCnt < iLastGrid; iCnt++)
		{
			sExtraCostStand = 0;
			sExtraCostSwat = 0;
			sExtraCostCrawl = 0;
			// what is the next gridno in the path?
			sOldGrid = sTempGrid;

			sTempGrid = NewGridNo(sTempGrid, DirectionInc((UINT8)guiPathingData[iCnt]));

			gfPlotPathEndDirection = GetDirection(sOldGrid, sTempGrid);

			// Get switch value...
			sSwitchValue = gubWorldMovementCosts[sTempGrid][(INT8)guiPathingData[iCnt]][pSold->pathing.bLevel];

			usMovementModeToUseForAPs = usMovementMode;

			// WANNE.WATER: If our soldier is not on the ground level and the tile is a "water" tile, then simply set the tile to "FLAT_GROUND"
			// This should fix "problems" for special modified maps
			UINT8 ubTerrainID = gpWorldLevelData[sTempGrid].ubTerrainID;

			if (TERRAIN_IS_WATER(ubTerrainID) && pSold->pathing.bLevel > 0)
				ubTerrainID = FLAT_GROUND;

			// ATE - MAKE MOVEMENT ALWAYS WALK IF IN WATER
			if (TERRAIN_IS_WATER(ubTerrainID))
			{
				usMovementModeToUseForAPs = WALKING;
			}

			// get the tile cost for that tile based on WALKING
			sTileCost = TerrainActionPoints(pSold, sTempGrid, (INT8)guiPathingData[iCnt], pSold->pathing.bLevel);

			if (bIgnoreNextCost)
			{
				bIgnoreNextCost = FALSE;
			}
			else
			{
				// ATE: If we have a 'special cost, like jump fence...
				if (sSwitchValue == TRAVELCOST_FENCE)
				{
					sPoints = sPoints + sTileCost;

					bIgnoreNextCost = TRUE;

					// If we are changing stance ( either before or after getting there....
					// We need to reflect that...
					switch (usMovementModeToUseForAPs)
					{
					case RUNNING:
					case WALKING:
					case WALKING_WEAPON_RDY:
					case WALKING_DUAL_RDY:
					case WALKING_ALTERNATIVE_RDY:

						// r8635
						// silversurfer: It doesn't matter if we continue moving after the jump. If we were standing before we will stand up again.
						/*							// Add here cost to go from crouch to stand AFTER fence hop....
						// Since it's AFTER.. make sure we will be moving after jump...
						if ( ( iCnt + 2 ) < iLastGrid )
						{
						sExtraCostStand += GetAPsCrouch(pSold, TRUE);

						// ATE: if running, charge extra point to start again
						if ( usMovementModeToUseForAPs == RUNNING )
						{
						sExtraCostStand += GetAPsStartRun(pSold);
						}

						sPoints = sPoints + sExtraCostStand;
						}*/
						// Add cost to stand up after jump
						sExtraCostStand += GetAPsCrouch(pSold, TRUE);
						if ((iCnt + 2) < iLastGrid && usMovementModeToUseForAPs == RUNNING)
						{
							sExtraCostStand += GetAPsStartRun(pSold);
						}
						sPoints = sPoints + sExtraCostStand;
						break;

					case SWATTING:

						// Add cost to stand up once BEFORE....
						sExtraCostSwat = GetAPsCrouch(pSold, TRUE);
						sPoints = sPoints + sExtraCostSwat;
						break;

					case CRAWLING:

						// r8635
						// Add cost to stand up before and go prone again after jumping
						sExtraCostCrawl += GetAPsCrouch(pSold, TRUE) + (2 * GetAPsProne(pSold, TRUE));
						sPoints = sPoints + sExtraCostCrawl;

						// Can't do it here.....
						break;

					}
				}
				else if (sTileCost > 0)
				{
					// else, movement is adjusted based on mode...					

					// so, then we must modify it for other movement styles and accumulate
					// CHRISL: Force display path to calculate AP cost differently if we're wearing a backpack
					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// SANDRO - This part have been modified "a bit" (see also "TerrainActionPoints" in "points.cpp")
					// Check movement modifiers
					switch (usMovementModeToUseForAPs)
					{
					case RUNNING:
						sMovementAPsCost = sTileCost + APBPConstants[AP_MODIFIER_RUN];
						if (pSold->IsVehicle() && (ubTerrainID == PAVED_ROAD || ubTerrainID == DIRT_ROAD) && sMovementAPsCost > 2)
						{
							sMovementAPsCost -= 2 * APBPConstants[AP_MAXIMUM] / 100;
						}
						break;
					case WALKING:
						sMovementAPsCost = sTileCost + APBPConstants[AP_MODIFIER_WALK];
						if (!(pSold->MercInWater()) && ((gAnimControl[pSold->usAnimState].uiFlags & ANIM_FIREREADY) || (gAnimControl[pSold->usAnimState].uiFlags & ANIM_FIRE)))
							sMovementAPsCost += APBPConstants[AP_MODIFIER_READY];
						break;
					case WALKING_ALTERNATIVE_RDY:
						sMovementAPsCost = sTileCost + APBPConstants[AP_MODIFIER_WALK];
						break;
					case WALKING_WEAPON_RDY:
					case WALKING_DUAL_RDY:
						sMovementAPsCost = sTileCost + APBPConstants[AP_MODIFIER_WALK] + APBPConstants[AP_MODIFIER_READY];
						break;
					case SWATTING:
						sMovementAPsCost = sTileCost + APBPConstants[AP_MODIFIER_SWAT];
						break;
					case CRAWLING:
						sMovementAPsCost = sTileCost + APBPConstants[AP_MODIFIER_CRAWL];
						// sevenfm: kneepads help crawling
						if (fKneePads)
							sMovementAPsCost -= APBPConstants[AP_MAXIMUM] / 100;
						break;
					default:
						sMovementAPsCost = sTileCost;
						break;
					}

					// Check for reverse mode
					if (pSold->bReverse || bReverse)
						sMovementAPsCost += APBPConstants[AP_REVERSE_MODIFIER];										
					
					// STOMP traits - Athletics trait decreases movement cost
					if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSold, ATHLETICS_NT))
					{
						sMovementAPsCost = max(1, (INT16)(((FLOAT)sMovementAPsCost * (100.0f - (FLOAT)gSkillTraitValues.ubATAPsMovementReduction) / 100.0f) + 0.5));
					}

					// Check for backpack
					sMovementAPsCost += sBackpackMovementPenalty;

					// Moa: scuba fins and swimming background
					if (fScubaFins)
					{
						if (TERRAIN_IS_HIGH_WATER(ubTerrainID))
							sMovementAPsCost /= 2;
						else if (pSold->bTeam != ENEMY_TEAM)
							sMovementAPsCost *= 2;
					}

					if (TERRAIN_IS_HIGH_WATER(ubTerrainID))
					{
						sMovementAPsCost = sMovementAPsCost * (100 + pSold->GetBackgroundValue(BG_SWIMMING)) / 100;
					}

					// Check for stealth mode
					if (pSold->bStealthMode || bStealth)
					{
						// STOMP traits - Stealthy trait decreases stealth AP modifier
						if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSold, STEALTHY_NT))
						{
							sMovementAPsCost += (max(0, (INT16)(((FLOAT)APBPConstants[AP_STEALTH_MODIFIER] * (FLOAT)(100 - gSkillTraitValues.ubSTStealthModeSpeedBonus) / 100.0f) + 0.5f)));
						}
						else
						{
							sMovementAPsCost += APBPConstants[AP_STEALTH_MODIFIER];
						}
					}

					// Flugente: dragging someone
					if (pSold->IsDragging())
					{
						sMovementAPsCost = (INT16)(sMovementAPsCost * gItemSettings.fDragAPCostModifier);
					}

					// moving diagonally
					if (guiPathingData[iCnt] & 1)
					{
						sMovementAPsCost = (INT16)(sMovementAPsCost * 14.0f / 10.0f + 0.5f);
					}

					// Check if doors if not player's merc (they have to open them manually)
					if (sSwitchValue == TRAVELCOST_DOOR && pSold->bTeam != gbPlayerNum)
					{
						sMovementAPsCost += GetAPsToOpenDoor(pSold) + GetAPsToOpenDoor(pSold); // Include open and close costs!
					}

					if (sSwitchValue == TRAVELCOST_NOT_STANDING)
					{
						switch (usMovementModeToUseForAPs)
						{
						case RUNNING:
						case WALKING:
						case WALKING_WEAPON_RDY:
						case WALKING_DUAL_RDY:
						case WALKING_ALTERNATIVE_RDY:
							// charge crouch APs for ducking head!
							sExtraCostStand += GetAPsCrouch(pSold, TRUE);
							break;

						default:
							break;
						}
					}

					sPoints += sMovementAPsCost + sExtraCostStand;
					///////////////////////////////////////////////////////////////////////////////////////////////////////////
				}
			}

			// THIS NEXT SECTION ONLY NEEDS TO HAPPEN FOR CURSOR UI FEEDBACK, NOT ACTUAL COSTING

			if (bPlot && ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT))) // OR USER OPTION ON... ***)
			{
				// sevenfm: use common code
				//sTileCost = TerrainActionPoints(pSold, sTempGrid, (INT8)guiPathingData[iCnt], pSold->pathing.bLevel);

				/*sPointsWalk = ActionPointCost(pSold, sTempGrid, (INT8)guiPathingData[iCnt], WALKING);
				sPointsCrawl = ActionPointCost(pSold, sTempGrid, (INT8)guiPathingData[iCnt], CRAWLING);
				sPointsSwat = ActionPointCost(pSold, sTempGrid, (INT8)guiPathingData[iCnt], SWATTING);
				sPointsRun = ActionPointCost(pSold, sTempGrid, (INT8)guiPathingData[iCnt], RUNNING);*/

				// ATE; TODO: Put stuff in here to allow for fact of costs other than movement ( jump fence, open door )

				// CHRISL: Adjusted system to use different move costs while wearing a backpack
				////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// SANDRO - This part was modified "a bit"
				sMovementAPsCost = sTileCost;
				// Check for reverse mode
				if (pSold->bReverse || bReverse)
				{
					sMovementAPsCost += APBPConstants[AP_REVERSE_MODIFIER];
				}
				// STOMP traits - Athletics trait decreases movement cost
				if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSold, ATHLETICS_NT))
				{
					sPointsWalk += max(1, (INT16)(((FLOAT)(sMovementAPsCost + APBPConstants[AP_MODIFIER_WALK]) * (100.0f - (FLOAT)gSkillTraitValues.ubATAPsMovementReduction) / 100.0f) + 0.5f));
					sPointsCrawl += max(1, (INT16)(((FLOAT)(sMovementAPsCost + APBPConstants[AP_MODIFIER_CRAWL]) * (100.0f - (FLOAT)gSkillTraitValues.ubATAPsMovementReduction) / 100.0f) + 0.5f));
					sPointsSwat += max(1, (INT16)(((FLOAT)(sMovementAPsCost + APBPConstants[AP_MODIFIER_SWAT]) * (100.0f - (FLOAT)gSkillTraitValues.ubATAPsMovementReduction) / 100.0f) + 0.5f));
					sPointsRun += max(1, (INT16)(((FLOAT)(sMovementAPsCost + APBPConstants[AP_MODIFIER_RUN]) * (100.0f - (FLOAT)gSkillTraitValues.ubATAPsMovementReduction) / 1000.f) + 0.5f));
				}
				// Specify movement modes
				else
				{
					sPointsWalk += sMovementAPsCost + APBPConstants[AP_MODIFIER_WALK];
					sPointsCrawl += sMovementAPsCost + APBPConstants[AP_MODIFIER_CRAWL];
					sPointsSwat += sMovementAPsCost + APBPConstants[AP_MODIFIER_SWAT];
					sPointsRun += sMovementAPsCost + APBPConstants[AP_MODIFIER_RUN];					
				}

				if (pSold->IsVehicle() && (ubTerrainID == PAVED_ROAD || ubTerrainID == DIRT_ROAD) && sPointsRun > 2)
				{
					sPointsRun -= 2 * APBPConstants[AP_MAXIMUM] / 100;
				}

				// sevenfm: kneepads help crawling
				if (fKneePads)
					sPointsCrawl -= APBPConstants[AP_MAXIMUM] / 100;

				// Moa: scuba fins and swimming background
				if (fScubaFins)
				{
					if (TERRAIN_IS_HIGH_WATER(ubTerrainID))
					{
						sPointsWalk /= 2;
						sPointsCrawl /= 2;
						sPointsSwat /= 2;
						sPointsRun /= 2;
					}
					else if (pSold->bTeam != ENEMY_TEAM)
					{
						sPointsWalk *= 2;
						sPointsCrawl *= 2;
						sPointsSwat *= 2;
						sPointsRun *= 2;
					}
				}

				if (TERRAIN_IS_HIGH_WATER(ubTerrainID))
				{
					sPointsWalk = (sPointsWalk * (100 + pSold->GetBackgroundValue(BG_SWIMMING))) / 100;
					sPointsCrawl = (sPointsCrawl * (100 + pSold->GetBackgroundValue(BG_SWIMMING))) / 100;
					sPointsSwat = (sPointsSwat * (100 + pSold->GetBackgroundValue(BG_SWIMMING))) / 100;
					sPointsRun = (sPointsRun * (100 + pSold->GetBackgroundValue(BG_SWIMMING))) / 100;
				}
				// walking with weapon raised?
				if (!(pSold->MercInWater()) && ((gAnimControl[pSold->usAnimState].uiFlags & ANIM_FIREREADY) || (gAnimControl[pSold->usAnimState].uiFlags & ANIM_FIRE)))
				{
					sPointsWalk += APBPConstants[AP_MODIFIER_READY];
				}
				// Check for stealth mode
				if (pSold->bStealthMode || bStealth)
				{
					// STOMP traits - Stealthy trait decreases stealth AP modifier
					if (gGameOptions.fNewTraitSystem && HAS_SKILL_TRAIT(pSold, STEALTHY_NT))
					{
						sPointsWalk += (max(0, (INT16)(((FLOAT)APBPConstants[AP_STEALTH_MODIFIER] * (FLOAT)(100 - gSkillTraitValues.ubSTStealthModeSpeedBonus) / 100.0f) + 0.5f)));
						sPointsCrawl += (max(0, (INT16)(((FLOAT)APBPConstants[AP_STEALTH_MODIFIER] * (FLOAT)(100 - gSkillTraitValues.ubSTStealthModeSpeedBonus) / 100.0f) + 0.5f)));
						sPointsSwat += (max(0, (INT16)(((FLOAT)APBPConstants[AP_STEALTH_MODIFIER] * (FLOAT)(100 - gSkillTraitValues.ubSTStealthModeSpeedBonus) / 100.0f) + 0.5f)));
						sPointsRun += (max(0, (INT16)(((FLOAT)APBPConstants[AP_STEALTH_MODIFIER] * (FLOAT)(100 - gSkillTraitValues.ubSTStealthModeSpeedBonus) / 100.0f) + 0.5f)));
					}
					else
					{
						sPointsWalk += APBPConstants[AP_STEALTH_MODIFIER];
						sPointsCrawl += APBPConstants[AP_STEALTH_MODIFIER];
						sPointsSwat += APBPConstants[AP_STEALTH_MODIFIER];
						sPointsRun += APBPConstants[AP_STEALTH_MODIFIER];
					}
				}

				// Flugente: dragging someone
				if (pSold->IsDragging())
				{
					sPointsWalk = (INT16)(sPointsWalk * gItemSettings.fDragAPCostModifier);
					sPointsCrawl = (INT16)(sPointsCrawl * gItemSettings.fDragAPCostModifier);
					sPointsSwat = (INT16)(sPointsSwat * gItemSettings.fDragAPCostModifier);
					sPointsRun = (INT16)(sPointsRun * gItemSettings.fDragAPCostModifier);
				}

				// Check for backpack
				sPointsWalk += sBackpackMovementPenalty;
				sPointsCrawl += sBackpackMovementPenalty;
				sPointsSwat += sBackpackMovementPenalty;
				sPointsRun += sBackpackMovementPenalty;

				// moving diagonally
				//if (guiPathingData[iCnt] & 1)
				//{
					//sPointsWalk = sPointsWalk * 14 / 10;
					//sPointsCrawl = sPointsCrawl * 14 / 10;
					//sPointsSwat = sPointsSwat * 14 / 10;
					//sPointsRun = sPointsRun * 14 / 10;
				//}

				if (sExtraCostStand)
				{
					sPointsWalk += sExtraCostStand;
					sPointsCrawl += sExtraCostStand;
					sPointsSwat += sExtraCostStand;
					sPointsRun += sExtraCostStand;
				}
				////////////////////////////////////////////////////////////////////////////////////////////////////////////
			}

			if (iCnt == 0 && bPlot)
			{
				gusAPtsToMove = sPoints;
				giPlotCnt = 0;
			}

			//if ( gTacticalStatus.uiFlags & TURNBASED && (gTacticalStatus.uiFlags & INCOMBAT) ) // OR USER OPTION "show paths" ON... ***
			{
				if (bPlot && ((iCnt < (iLastGrid - 1)) || (iCnt < iLastGrid && bStayOn)))
				{
					guiPlottedPath[giPlotCnt++] = sTempGrid;

					// we need a footstep graphic ENTERING the next tile

					// get the direction
					usTileNum = (UINT16)guiPathingData[iCnt] + 2;
					if (usTileNum > 8)
					{
						usTileNum = 1;
					}

					// Are we a vehicle?
					//if ( pSold->flags.uiStatusFlags & SOLDIER_VEHICLE )
					//{
					//	// did we exceed WALK cost?
					//	if ( sPointsSwat > sAPBudget)
					//	{
					//		sFootOrderIndex = 4;
					//	}
					//	else
					//	{
					//		sFootOrderIndex = 3;
					//	}
					//}
					//else
					{
						// did we exceed CRAWL cost?
						if (sFootOrderIndex == 0 && sPointsCrawl > sAPBudget)
						{
							sFootOrderIndex++;
						}

						// did we exceed WALK cost?
						if (sFootOrderIndex == 1 && sPointsSwat > sAPBudget)
						{
							sFootOrderIndex++;
						}

						// did we exceed SWAT cost?
						if (sFootOrderIndex == 2 && sPointsWalk > sAPBudget)
						{
							sFootOrderIndex++;
						}

						// did we exceed RUN cost?
						if (sFootOrderIndex == 3 && sPointsRun > sAPBudget)
						{
							sFootOrderIndex++;
						}
					}

					GetTileIndexFromTypeSubIndex(FOOTPRINTS, (UINT16)usTileNum, &usTileIndex);

					// Adjust based on what mode we are in...
					if ((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT))
					{
						// find out which color we're using
						usTileIndex = usTileIndex + sFootOrder[4];
					}
					else // turn based
					{
						// find out which color we're using
						usTileIndex = usTileIndex + sFootOrder[sFootOrderIndex];
					}

					/*
					if ( sPoints <= sAPBudget)
					{
					// find out which color we're using
					usTileIndex += sFootOrder[sFootOrderIndex];
					}
					else
					{
					// use red footprints ( offset by 16 )
					usTileIndex += REDSTEPSTART;
					}
					*/

					if (pSold->pathing.bLevel == 0)
					{
						pNode = AddObjectToTail(sTempGrid, usTileIndex);
						pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
						pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
					}
					else
					{
						pNode = AddOnRoofToTail(sTempGrid, usTileIndex);
						pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
						pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
					}

					// we need a footstep graphic LEAVING this tile

					// get the direction using the NEXT tile (thus iCnt+1 as index)
					usTileNum = (UINT16)guiPathingData[iCnt + 1] + 2;
					if (usTileNum > 8)
					{
						usTileNum = 1;
					}

					// this is a LEAVING footstep which is always the second set of 8
					usTileNum += 8;

					GetTileIndexFromTypeSubIndex(FOOTPRINTS, (UINT16)usTileNum, &usTileIndex);

					// Adjust based on what mode we are in...
					if ((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT))
					{
						// find out which color we're using
						usTileIndex = usTileIndex + sFootOrder[4];
					}
					else // turnbased
					{
						// find out which color we're using
						usTileIndex = usTileIndex + sFootOrder[sFootOrderIndex];
					}

					if (pSold->pathing.bLevel == 0)
					{
						pNode = AddObjectToTail(sTempGrid, usTileIndex);
						pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
						pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
					}
					else
					{
						pNode = AddOnRoofToTail(sTempGrid, usTileIndex);
						pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
						pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
					}
				}

			}		// end of if turn based or real-time user option "show paths" on...
		}

		if (bPlot)
		{
			gusPathShown = TRUE;
		}

	} 	// end of found a path

	// reset distance limit
	gubNPCDistLimit = 0;

	return(sPoints);
}

INT32 UIPlotPath(SOLDIERTYPE *pSold, INT32 sDestGridNo, INT8 bCopyRoute, INT8 bPlot, INT8 bStayOn, UINT16 usMovementMode, INT8 bStealth, INT8 bReverse, INT16 sAPBudget)
{
	// This function is specifically for UI calls to the pathing routine, to
	// check whether the shift key is pressed, etc.
	INT32	 sRet;

	if (_KeyDown(SHIFT))
	{
		gfPlotDirectPath = TRUE;
	}

	// If we are on the same level as the interface level, continue, else return
	if (pSold->pathing.bLevel != gsInterfaceLevel)
	{
		return(0);
	}

	if (gGameSettings.fOptions[TOPTION_ALWAYS_SHOW_MOVEMENT_PATH])
	{
		bPlot = TRUE;
	}

	// sevenfm: ignore person at destination
	/*UINT8 ubPerson = WhoIsThere2(sDestGridNo, pSold->pathing.bLevel);
	if (!gfUIFullTargetFound &&
		(ubPerson == NOBODY ||
		!MercPtrs[ubPerson] ||
		MercPtrs[ubPerson]->bVisible < 0 ||
		!LOS(pSold, MercPtrs[ubPerson], SIGHT_NORMAL, CALC_FROM_ALL_DIRS)))
	{
		gfEstimatePath = TRUE;
	}*/
	sRet = PlotPath(pSold, sDestGridNo, bCopyRoute, bPlot, bStayOn, usMovementMode, bStealth, bReverse, sAPBudget);
	//gfEstimatePath = FALSE;
	gfPlotDirectPath = FALSE;
	return(sRet);
}

INT16 RecalculatePathCost(SOLDIERTYPE *pSoldier, UINT16 usMovementMode)
{
	// AI function for a soldier already with a path; this will return the cost of that path using the given movement mode
	INT16	sRet;

	if (!pSoldier->pathing.bPathStored || pSoldier->pathing.usPathDataSize == 0)
	{
		return(0);
	}

	gfRecalculatingExistingPathCost = TRUE;
	sRet = PlotPath(pSoldier, pSoldier->pathing.sFinalDestination, NO_COPYROUTE, FALSE, FALSE, usMovementMode, FALSE, FALSE, 0);
	gfRecalculatingExistingPathCost = FALSE;
	return(sRet);
}

INT32 EstimatePlotPath(SOLDIERTYPE *pSold, INT32 sDestGridNo, INT8 bCopyRoute, INT8 bPlot, INT8 bStayOn, UINT16 usMovementMode, INT8 bStealth, INT8 bReverse, INT16 sAPBudget)
{
	// This function is specifically for AI calls to estimate path cost to a location
	// It sets stuff up to ignore all people
	INT32			sRet;

	gfEstimatePath = TRUE;

	sRet = PlotPath(pSold, sDestGridNo, bCopyRoute, bPlot, bStayOn, usMovementMode, bStealth, bReverse, sAPBudget);

	gfEstimatePath = FALSE;

	return(sRet);
}


UINT8 InternalDoorTravelCost(SOLDIERTYPE * pSoldier, INT32 iGridNo, UINT8 ubMovementCost, BOOLEAN fReturnPerceivedValue, INT32 * piDoorGridNo, BOOLEAN fReturnDoorCost)
{
	// This function will return either TRAVELCOST_DOOR (in place of closed door cost),
	// TRAVELCOST_OBSTACLE, or the base ground terrain
	// travel cost, depending on whether or not the door is open or closed etc.
	BOOLEAN				fDoorIsObstacleIfClosed = FALSE;
	INT32					iDoorGridNo = -1;
	DOOR_STATUS *	pDoorStatus;
	DOOR *				pDoor;
	STRUCTURE *		pDoorStructure;
	BOOLEAN				fDoorIsOpen;
	UINT8					ubReplacementCost;

	if (IS_TRAVELCOST_DOOR(ubMovementCost))
	{
		ubReplacementCost = TRAVELCOST_OBSTACLE;

		switch (ubMovementCost)
		{
		case TRAVELCOST_DOOR_CLOSED_HERE:
			fDoorIsObstacleIfClosed = TRUE;
			iDoorGridNo = iGridNo;
			ubReplacementCost = TRAVELCOST_DOOR;
			break;
		case TRAVELCOST_DOOR_CLOSED_N:
			fDoorIsObstacleIfClosed = TRUE;
			iDoorGridNo = iGridNo + dirDelta[NORTH];
			ubReplacementCost = TRAVELCOST_DOOR;
			break;
		case TRAVELCOST_DOOR_CLOSED_W:
			fDoorIsObstacleIfClosed = TRUE;
			iDoorGridNo = iGridNo + dirDelta[WEST];
			ubReplacementCost = TRAVELCOST_DOOR;
			break;
		case TRAVELCOST_DOOR_OPEN_HERE:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo;
			break;
		case TRAVELCOST_DOOR_OPEN_N:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTH];
			break;
		case TRAVELCOST_DOOR_OPEN_NE:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTHEAST];
			break;
		case TRAVELCOST_DOOR_OPEN_E:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[EAST];
			break;
		case TRAVELCOST_DOOR_OPEN_SE:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[SOUTHEAST];
			break;
		case TRAVELCOST_DOOR_OPEN_S:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[SOUTH];
			break;
		case TRAVELCOST_DOOR_OPEN_SW:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[SOUTHWEST];
			break;
		case TRAVELCOST_DOOR_OPEN_W:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[WEST];
			break;
		case TRAVELCOST_DOOR_OPEN_NW:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTHWEST];
			break;
		case TRAVELCOST_DOOR_OPEN_N_N:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTH] + dirDelta[NORTH];
			break;
		case TRAVELCOST_DOOR_OPEN_NW_N:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTHWEST] + dirDelta[NORTH];
			break;
		case TRAVELCOST_DOOR_OPEN_NE_N:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTHEAST] + dirDelta[NORTH];
			break;
		case TRAVELCOST_DOOR_OPEN_W_W:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[WEST] + dirDelta[WEST];
			break;
		case TRAVELCOST_DOOR_OPEN_SW_W:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[SOUTHWEST] + dirDelta[WEST];
			break;
		case TRAVELCOST_DOOR_OPEN_NW_W:
			fDoorIsObstacleIfClosed = FALSE;
			iDoorGridNo = iGridNo + dirDelta[NORTHWEST] + dirDelta[WEST];
			break;
		default:
			break;
		}

		if (pSoldier && (pSoldier->flags.uiStatusFlags & SOLDIER_MONSTER || pSoldier->flags.uiStatusFlags & SOLDIER_ANIMAL))
		{
			// can't open doors!
			ubReplacementCost = TRAVELCOST_OBSTACLE;
		}

		if (piDoorGridNo)
		{
			// return gridno of door through pointer
			*piDoorGridNo = iDoorGridNo;
		}

		if (fReturnPerceivedValue && gpWorldLevelData[iDoorGridNo].usExtFlags[0] & MAPELEMENT_EXT_DOOR_STATUS_PRESENT)
		{
			// check door status
			pDoorStatus = GetDoorStatus(iDoorGridNo);
			if (pDoorStatus)
			{
				fDoorIsOpen = (pDoorStatus->ubFlags & DOOR_PERCEIVED_OPEN) != 0;
			}
			else
			{
				// abort!
				return(ubMovementCost);
			}
		}
		else
		{
			// check door structure
			pDoorStructure = FindStructure(iDoorGridNo, STRUCTURE_ANYDOOR);
			if (pDoorStructure)
			{
				fDoorIsOpen = (pDoorStructure->fFlags & STRUCTURE_OPEN) != 0;
			}
			else
			{
				// abort!
				return(ubMovementCost);
			}
		}
		// now determine movement cost
		if (fDoorIsOpen)
		{
			if (fDoorIsObstacleIfClosed)
			{
				ubMovementCost = gTileTypeMovementCost[gpWorldLevelData[iGridNo].ubTerrainID];
			}
			else
			{
				ubMovementCost = ubReplacementCost;
			}
		}
		else
		{
			if (fDoorIsObstacleIfClosed)
			{
				// door is closed and this should be an obstacle, EXCEPT if we are calculating
				// a path for an enemy or NPC with keys

				// creatures and animals can't open doors!
				if (fReturnPerceivedValue || (pSoldier && (pSoldier->flags.uiStatusFlags & SOLDIER_MONSTER || pSoldier->flags.uiStatusFlags & SOLDIER_ANIMAL)))
				{
					ubMovementCost = ubReplacementCost;
				}
				else
				{
					// have to check if door is locked and NPC does not have keys!
					pDoor = FindDoorInfoAtGridNo(iDoorGridNo);
					if (pDoor)
					{
						if ((!pDoor->fLocked || (pSoldier && pSoldier->flags.bHasKeys)) && !fReturnDoorCost)
						{
							ubMovementCost = gTileTypeMovementCost[gpWorldLevelData[iGridNo].ubTerrainID];
						}
						else
						{
							ubMovementCost = ubReplacementCost;
						}
					}
					else
					{
						ubMovementCost = ubReplacementCost;
					}
				}
			}
			else
			{
				ubMovementCost = gTileTypeMovementCost[gpWorldLevelData[iGridNo].ubTerrainID];
			}
		}

	}
	return(ubMovementCost);

}

UINT8 DoorTravelCost(SOLDIERTYPE * pSoldier, INT32 iGridNo, UINT8 ubMovementCost, BOOLEAN fReturnPerceivedValue, INT32 * piDoorGridNo)
{
	return(InternalDoorTravelCost(pSoldier, iGridNo, ubMovementCost, fReturnPerceivedValue, piDoorGridNo, FALSE));
}


