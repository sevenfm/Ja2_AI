/*
	Filename		:		pathai.h
	Author			:		Ray E. Bornert II
	Date			:		1992-MAR-15

	Copyright (C) 1993 HixoxiH Software
*/

#ifndef _PATHAI_H
#define _PATHAI_H
#include "isometric utils.h"

BOOLEAN InitPathAI( void );
void ShutDownPathAI( void );
INT32 PlotPath( SOLDIERTYPE *pSold, INT32 sDestGridNo, INT8 bCopyRoute, INT8 bPlot, INT8 bStayOn, UINT16 usMovementMode, INT8 bStealth, INT8 bReverse , INT16 sAPBudget);
INT32 UIPlotPath( SOLDIERTYPE *pSold, INT32 sDestGridNo, INT8 bCopyRoute, INT8 bPlot, INT8 bStayOn, UINT16 usMovementMode, INT8 bStealth, INT8 bReverse , INT16 sAPBudget);
INT32 EstimatePlotPath( SOLDIERTYPE *pSold, INT32 sDestGridNo, INT8 bCopyRoute, INT8 bPlot, INT8 bStayOn, UINT16 usMovementMode, INT8 bStealth, INT8 bReverse , INT16 sAPBudget);

void ErasePath(char bEraseOldOne);
INT32 FindBestPath(SOLDIERTYPE *s , INT32 sDestination, INT8 bLevel, INT16 usMovementMode, INT8 bCopy, UINT8 fFlags );
void GlobalReachableTest( INT32 sStartGridNo );
void GlobalItemsReachableTest( INT32 sStartGridNo1, INT32 sStartGridNo2 );
void RoofReachableTest( INT32 sStartGridNo, UINT8 ubBuildingID );
void LocalReachableTest( INT32 sStartGridNo, INT8 bRadius );

UINT8 DoorTravelCost( SOLDIERTYPE * pSoldier, INT32 iGridNo, UINT8 ubMovementCost, BOOLEAN fReturnPerceivedValue, INT32 * piDoorGridNo );
UINT8 InternalDoorTravelCost( SOLDIERTYPE * pSoldier, INT32 iGridNo, UINT8 ubMovementCost, BOOLEAN fReturnPerceivedValue, INT32 * piDoorGridNo, BOOLEAN fReturnDoorCost );

INT16 RecalculatePathCost( SOLDIERTYPE *pSoldier, UINT16 usMovementMode );

//dnl ch50 071009
// ABSOLUTE maximums
#define ABSMAX_SKIPLIST_LEVEL 10				// was 5
#define ABSMAX_TRAIL_TREE (WORLD_MAX)			// was 16384
#define ABSMAX_PATHQ (ABSMAX_TRAIL_TREE/16)		// was 512
// STANDARD maximums... configurable!
#define MAX_SKIPLIST_LEVEL ABSMAX_SKIPLIST_LEVEL// was 5
#define MAX_TRAIL_TREE ABSMAX_TRAIL_TREE		// was 16384
#define MAX_PATHQ ABSMAX_PATHQ					// was 512
// Lesh: setup max length of path
#define MAX_PATH_DATA_LENGTH MAX_PATHQ			// was 512

#define TRAILCELLTYPE UINT16 //Lalien: moved form pathai.cpp
// Exporting these global variables
extern UINT32* guiPathingData;
extern INT16 gubNPCAPBudget;
extern UINT16 gusNPCMovementMode;
extern UINT8 gubNPCDistLimit;
extern UINT8 gubNPCPathCount;
extern BOOLEAN gfPlotPathToExitGrid;
extern BOOLEAN gfNPCCircularDistLimit;
extern BOOLEAN gfEstimatePath;
extern BOOLEAN	gfPathAroundObstacles;
extern UINT8 gubGlobalPathFlags;
extern UINT8 gfPlotPathEndDirection;

// Ian's terrain values for traveling speed/pathing purposes
// Fixed by CJC March 4, 1998.	Please do not change these unless familiar
// with how this will affect the path code!

#define TRAVELCOST_NONE						0
#define TRAVELCOST_FLAT						10
#define TRAVELCOST_BUMPY					12
#define TRAVELCOST_GRASS					12
#define TRAVELCOST_THICK					16
#define TRAVELCOST_DEBRIS					20
#define TRAVELCOST_SHORE					30
#define TRAVELCOST_KNEEDEEP				36
#define TRAVELCOST_DEEPWATER			50
#define TRAVELCOST_FENCE					40

// these values are used to indicate "this is an obstacle
// if there is a door (perceived) open/closed in this tile
#define TRAVELCOST_DOOR_CLOSED_HERE	220
#define TRAVELCOST_DOOR_CLOSED_N		221
#define TRAVELCOST_DOOR_CLOSED_W		222
#define TRAVELCOST_DOOR_OPEN_HERE		223
#define TRAVELCOST_DOOR_OPEN_N			224
#define TRAVELCOST_DOOR_OPEN_NE			225
#define TRAVELCOST_DOOR_OPEN_E			226
#define TRAVELCOST_DOOR_OPEN_SE			227
#define TRAVELCOST_DOOR_OPEN_S			228
#define TRAVELCOST_DOOR_OPEN_SW			229
#define TRAVELCOST_DOOR_OPEN_W			230
#define TRAVELCOST_DOOR_OPEN_NW			231
#define TRAVELCOST_DOOR_OPEN_N_N		232
#define TRAVELCOST_DOOR_OPEN_NW_N		233
#define TRAVELCOST_DOOR_OPEN_NE_N		234
#define TRAVELCOST_DOOR_OPEN_W_W		235
#define TRAVELCOST_DOOR_OPEN_SW_W		236
#define TRAVELCOST_DOOR_OPEN_NW_W		237
#define TRAVELCOST_NOT_STANDING		248
#define TRAVELCOST_OFF_MAP				249
#define TRAVELCOST_CAVEWALL				250
#define TRAVELCOST_HIDDENOBSTACLE 251
#define TRAVELCOST_DOOR						252
#define TRAVELCOST_OBSTACLE				253
#define TRAVELCOST_WALL						254
#define TRAVELCOST_EXITGRID				255

///ddd { window
//#define TRAVELCOST_JUMPABLEWINDOW		46
//#define TRAVELCOST_JUMPABLEWINDOW_N		47
//#define TRAVELCOST_JUMPABLEWINDOW_W		48
///ddd window }

#define TRAVELCOST_TRAINTRACKS		30
#define TRAVELCOST_DIRTROAD				9
#define TRAVELCOST_PAVEDROAD			9
#define TRAVELCOST_FLATFLOOR			10

#define TRAVELCOST_BLOCKED (TRAVELCOST_OFF_MAP)
#define IS_TRAVELCOST_DOOR( x ) (x >= TRAVELCOST_DOOR_CLOSED_HERE && x <= TRAVELCOST_DOOR_OPEN_NW_W)
#define IS_TRAVELCOST_CLOSED_DOOR( x ) (x >= TRAVELCOST_DOOR_CLOSED_HERE && x <= TRAVELCOST_DOOR_CLOSED_W)

// ------------------------------------------
// PLOT PATH defines
#define NOT_STEALTH							0
#define	STEALTH									1

#define NO_PLOT									0
#define PLOT										1

#define TEMPORARY								0
#define PERMANENT								1

#define FORWARD									0
#define REVERSE									1

#define NO_COPYROUTE						0
#define COPYROUTE								1
#define COPYREACHABLE			2
#define COPYREACHABLE_AND_APS		3
#define FINDCLIMBPOINTS				4

#define PATH_THROUGH_PEOPLE					0x01
#define PATH_IGNORE_PERSON_AT_DEST	0x02
#define PATH_CLOSE_GOOD_ENOUGH			0x04


#define PATH_CLOSE_RADIUS 5
#define PATH_CLOSE_RADIUS_SQ 25

// ------------------------------------------

#endif
