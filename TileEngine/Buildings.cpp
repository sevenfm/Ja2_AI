#ifdef PRECOMPILEDHEADERS
	#include "TileEngine All.h"
#else
	#include "Types.h"
	#include "Buildings.h"
	//#include "Isometric Utils.h"
	#include "Pathai.h"
	#include "Structure Wrap.h"
	//#include "Random.h"
	//#include "Overhead.h"
	#include "Render Fun.h"
	#include "Strategicmap.h"
	#include "Sys Globals.h"
	#include "worldman.h"
#endif

#include "AIInternals.h"

// sevenfm: 4 for small maps, 8 for big maps
#define ROOF_LOCATION_CHANCE (WORLD_COLS / 40)
//#define ROOF_LOCATION_CHANCE 8

UINT8*					gubBuildingInfo = NULL;
BUILDING				gBuildings[ MAX_BUILDINGS ];
UINT8					gubNumberOfBuildings;

#ifdef ROOF_DEBUG
	extern INT16 gsCoverValue[WORLD_MAX];
	#include "video.h"
	#include "renderworld.h"
#endif

BUILDING * CreateNewBuilding( UINT8 * pubBuilding )
{
	// sevenfm: r8404
	if (gubNumberOfBuildings >= MAX_BUILDINGS-1)
	//if (gubNumberOfBuildings >= MAX_BUILDINGS)
	{
		return( NULL );
	}
	// increment # of buildings
	gubNumberOfBuildings++;
	
	// clear entry
	gBuildings[ gubNumberOfBuildings ].ubNumClimbSpots = 0;
	*pubBuilding = gubNumberOfBuildings;

	// return pointer (have to subtract 1 since we just added 1
	return( &(gBuildings[ gubNumberOfBuildings ]) );
}

BUILDING * GenerateBuilding( INT32 sDesiredSpot )
{
	INT32			uiLoop;
	INT32			uiLoop2;
	INT32			sTempGridNo, sNextTempGridNo, sVeryTemporaryGridNo;
	INT32			sStartGridNo, sCurrGridNo, sPrevGridNo = NOWHERE, sRightGridNo;
	UINT8			ubDirection, ubTempDirection;
	BOOLEAN			fFoundDir, fFoundWall;
	UINT32			uiChanceIn = ROOF_LOCATION_CHANCE; // chance of a location being considered
	INT32			sWallGridNo;
	INT8			bDesiredOrientation;
	INT8			bSkipSpots = 0;
	SOLDIERTYPE 	FakeSoldier;
	BUILDING *		pBuilding;
	UINT8			ubBuildingID = 0;
	INT32			iLoopCount = 0;

	pBuilding = CreateNewBuilding( &ubBuildingID );
	if (!pBuilding)
	{
		return( NULL );
	}

	FakeSoldier.sGridNo = sDesiredSpot;
	FakeSoldier.pathing.bLevel = 1;
	FakeSoldier.bTeam = 1;

	// Set reachable
	RoofReachableTest( sDesiredSpot, ubBuildingID );

	// From sGridNo, search until we find a spot that isn't part of the building
	ubDirection = NORTHWEST;
	sTempGridNo = sDesiredSpot;
	// using diagonal directions to hopefully prevent picking a
	// spot that
	while( (gpWorldLevelData[ sTempGridNo ].usFlags & MAPELEMENT_REACHABLE ) )
	{
		sNextTempGridNo = NewGridNo( sTempGridNo, DirectionInc( ubDirection ) );
		if ( sTempGridNo == sNextTempGridNo )
		{
			// hit edge of map!??!
			return( NULL );
		}
		else
		{
			sTempGridNo = sNextTempGridNo;
		}
	}

	// we've got our spot
	sStartGridNo = sTempGridNo;

	sCurrGridNo = sStartGridNo;
	sVeryTemporaryGridNo = NewGridNo( sCurrGridNo, DirectionInc( EAST ) );
	if ( gpWorldLevelData[ sVeryTemporaryGridNo ].usFlags & MAPELEMENT_REACHABLE )
	{
		// go north first
		ubDirection = NORTH;
	}
	else
	{
		// go that way (east)
		ubDirection = EAST;
	}

	gpWorldLevelData[ sStartGridNo ].usExtFlags[0] |= MAPELEMENT_EXT_ROOFCODE_VISITED;

	uiLoop2 = 0;
	while(uiLoop2 < WORLD_MAX)
	{
		// if point to (2 clockwise) is not part of building and is not visited,
		// or is starting point, turn!
		sRightGridNo = NewGridNo( sCurrGridNo, DirectionInc( gTwoCDirection[ ubDirection ] ) );
		sTempGridNo = sRightGridNo;
		if ( ( ( !(gpWorldLevelData[ sTempGridNo ].usFlags & MAPELEMENT_REACHABLE) && !(gpWorldLevelData[ sTempGridNo ].usExtFlags[0] & MAPELEMENT_EXT_ROOFCODE_VISITED) ) || (sTempGridNo == sStartGridNo) ) && (sCurrGridNo != sStartGridNo) )
		{
			iLoopCount++;

			if ( iLoopCount >= 10 )
			{
				return( NULL );
			}
			ubDirection = gTwoCDirection[ ubDirection ];
			// try in that direction
			continue;
		}
		iLoopCount = 0;

		// if spot ahead is part of building, turn
		sTempGridNo = NewGridNo( sCurrGridNo, DirectionInc( ubDirection ) );
		if ( gpWorldLevelData[ sTempGridNo ].usFlags & MAPELEMENT_REACHABLE )
		{
			// first search for a spot that is neither part of the building or visited

			// we KNOW that the spot in the original direction is blocked, so only loop 3 times
			ubTempDirection = gTwoCDirection[ ubDirection ];
			fFoundDir = FALSE;
			for ( uiLoop = 0; uiLoop < 3; uiLoop++ )
			{
				sTempGridNo = NewGridNo( sCurrGridNo, DirectionInc( ubTempDirection ) );
				if ( !(gpWorldLevelData[ sTempGridNo ].usFlags & MAPELEMENT_REACHABLE) && !(gpWorldLevelData[ sTempGridNo ].usExtFlags[0] & MAPELEMENT_EXT_ROOFCODE_VISITED) )
				{
					// this is the way to go!
					fFoundDir = TRUE;
					break;
				}
				ubTempDirection = gTwoCDirection[ ubTempDirection ];
			}
			if (!fFoundDir)
			{
				// now search for a spot that is just not part of the building
				ubTempDirection = gTwoCDirection[ ubDirection ];
				fFoundDir = FALSE;
				for ( uiLoop = 0; uiLoop < 3; uiLoop++ )
				{
					sTempGridNo = NewGridNo( sCurrGridNo, DirectionInc( ubTempDirection ) );
					if ( !(gpWorldLevelData[ sTempGridNo ].usFlags & MAPELEMENT_REACHABLE) )
					{
						// this is the way to go!
						fFoundDir = TRUE;
						break;
					}
					ubTempDirection = gTwoCDirection[ ubTempDirection ];
				}
				if (!fFoundDir)
				{
					// WTF is going on?
					return( NULL );
				}
			}
			ubDirection = ubTempDirection;
			// try in that direction
			continue;
		}

		// move ahead
		sPrevGridNo = sCurrGridNo;
		sCurrGridNo = sTempGridNo;
		sRightGridNo = NewGridNo( sCurrGridNo, DirectionInc( gTwoCDirection[ ubDirection ] ) );

#ifdef ROOF_DEBUG
		if (gsCoverValue[sCurrGridNo] == 0x7F7F)
		{
			gsCoverValue[sCurrGridNo] = 1;
		}
		else if (gsCoverValue[sCurrGridNo] >= 0)
		{
			gsCoverValue[sCurrGridNo]++;
		}

		DebugAI( String( "Roof code visits %d", sCurrGridNo ) );
#endif

		if (sCurrGridNo == sStartGridNo)
		{
			// done
			break;
		}

		if ( !(gpWorldLevelData[ sCurrGridNo ].usExtFlags[0] & MAPELEMENT_EXT_ROOFCODE_VISITED) )
		{
			gpWorldLevelData[ sCurrGridNo ].usExtFlags[0] |= MAPELEMENT_EXT_ROOFCODE_VISITED;

			// sevenfm: why mark tile as building when it's not in a building?
			//if( FindStructure( sCurrGridNo, STRUCTURE_ROOF ) != NULL )
			if( InARoom(sCurrGridNo, NULL) )
				gubBuildingInfo[ sCurrGridNo ] = ubBuildingID;

			// consider this location as possible climb gridno
			// there must be a regular wall adjacent to this for us to consider it a
			// climb gridno

			// if the direction is east or north, the wall would be in our gridno;
			// if south or west, the wall would be in the gridno two clockwise
			fFoundWall = FALSE;

			// There must not be roof here either.  There are places where a pitched roof butts up against a flat roof.
			// Don't mark such a border as a climb point.  Otherwise AI units will get stuck.
			if (FindStructure( sCurrGridNo, STRUCTURE_ROOF ) == NULL &&
				NewOKDestination( &FakeSoldier, sCurrGridNo, FALSE, 0 ) )
			{
				switch( ubDirection )
				{
				case NORTH:
					sWallGridNo = sCurrGridNo;
					bDesiredOrientation = OUTSIDE_TOP_RIGHT;
					break;
				case EAST:
					sWallGridNo = sCurrGridNo;
					bDesiredOrientation = OUTSIDE_TOP_LEFT;
					break;
				case SOUTH:
					sWallGridNo = ( sCurrGridNo + DirectionInc( gTwoCDirection[ ubDirection ] ) );
					bDesiredOrientation = OUTSIDE_TOP_RIGHT;
					break;
				case WEST:
					sWallGridNo = ( sCurrGridNo + DirectionInc( gTwoCDirection[ ubDirection ] ) );
					bDesiredOrientation = OUTSIDE_TOP_LEFT;
					break;
				default:
					// what the heck?
					return( NULL );
				}

				if (bDesiredOrientation == OUTSIDE_TOP_LEFT)
				{
					if (WallExistsOfTopLeftOrientation( sWallGridNo ))
					{
						fFoundWall = TRUE;
					}
				}
				else 
				{
					if (WallExistsOfTopRightOrientation( sWallGridNo ))
					{
						fFoundWall = TRUE;
					}
				}
			}
			if (fFoundWall)
			{
#ifdef ROOF_DEBUG
				gsCoverValue[sCurrGridNo] = 50;
#endif
				if (bSkipSpots > 0)
				{
					bSkipSpots--;
				}
				else if ( Random( uiChanceIn ) == 0 )
				{					
					pBuilding->sUpClimbSpots[ pBuilding->ubNumClimbSpots ] = sCurrGridNo;
					pBuilding->sDownClimbSpots[ pBuilding->ubNumClimbSpots ] = sRightGridNo;
					pBuilding->ubNumClimbSpots++;

					if ( pBuilding->ubNumClimbSpots == MAX_CLIMBSPOTS_PER_BUILDING)
					{
						// gotta stop!
						return( pBuilding );
					}

					// if location is added as a spot, reset uiChanceIn
					uiChanceIn = ROOF_LOCATION_CHANCE;
#ifdef ROOF_DEBUG
					gsCoverValue[sCurrGridNo] = 99;
#endif
					// skip the next spot
					bSkipSpots = 1;					
				}
				else
				{
					// didn't pick this location, so increase chance that next location
					// will be considered
					if (uiChanceIn > 2)
					{
						uiChanceIn--;
					}
				}
			}
			else
			{
				// can't select this spot				
				if ( ( !TileIsOutOfBounds(sPrevGridNo)) && (pBuilding->ubNumClimbSpots > 0) )
				{
					if ( pBuilding->sDownClimbSpots[ pBuilding->ubNumClimbSpots - 1 ] == sCurrGridNo )
					{
						// unselect previous spot
						pBuilding->ubNumClimbSpots--;
						// overwrote a selected spot so go into automatic selection for later
						uiChanceIn = 1;
#ifdef ROOF_DEBUG
						// reset marker
						gsCoverValue[sPrevGridNo] = 1;
#endif
					}
				}

				// skip the next gridno
				bSkipSpots = 1;
			}
		}
		uiLoop2++;
	}
	// If we've run out of loop before we've run out of building, then there is
	// something that has gone pear shaped
	if(uiLoop2 >= WORLD_MAX)
	{
		INT32 x = 0;
		INT32 y = 0;
		while((sDesiredSpot - ((y + 1) * WORLD_COLS)) >= 0)
		{
			y++;
		}
		x = sDesiredSpot - (y * WORLD_COLS);
		DebugMsg (TOPIC_JA2,DBG_LEVEL_2,String( "113/UC Warning! Building Walk Algorithm has covered the entire map! Building %d located at [%d,%d] must be bogus.", ubBuildingID, x, y ));
	}

	// at end could prune # of locations if there are too many

	return( pBuilding );
}

BUILDING * FindBuilding( INT32 sGridNo )
{
	UINT8					ubBuildingID;
	//UINT16					usRoomNo;
	
	if ( TileIsOutOfBounds( sGridNo ) )
	{
		return( NULL );
	}

	// id 0 indicates no building
	ubBuildingID = gubBuildingInfo[ sGridNo ];

	if ( ubBuildingID == NO_BUILDING )
	{
		return( NULL );

		// need extra checks to see if is valid spot...
		// must have valid room information and be a flat-roofed
		// building
		/*if ( InARoom( sGridNo, &usRoomNo ) && (FindStructure( sGridNo, STRUCTURE_NORMAL_ROOF ) != NULL) )
		{
			return( GenerateBuilding( sGridNo ) );
		}
		else
		{
			return( NULL );
		}*/
	}
	else if ( ubBuildingID > gubNumberOfBuildings ) // huh?
	{
		return( NULL );
	}

	return( &(gBuildings[ ubBuildingID ]) );
}

extern BOOLEAN CheckRoof(INT32 sGridNo);

BOOLEAN InBuilding( INT32 sGridNo )
{
	if (FindBuilding(sGridNo) || InARoom(sGridNo, NULL) && CheckRoof(sGridNo))
	{
		return TRUE;
	}
	return FALSE;
}

void GenerateBuildings( void )
{
	INT32	uiLoop;

	// init building structures and variables
	memset( gubBuildingInfo, 0, WORLD_MAX * sizeof( UINT8 ) );
	memset( &gBuildings, 0, MAX_BUILDINGS * sizeof( BUILDING ) );
	gubNumberOfBuildings = 0;

	if ( (gbWorldSectorZ > 0) || gfEditMode)
	{
		return;
	}

#ifdef ROOF_DEBUG
	memset( gsCoverValue, 0x7F, sizeof( INT16 ) * WORLD_MAX );
#endif

	// reset ALL reachable flags
	// do once before we start building generation for
	// whole map
	for ( uiLoop = 0; uiLoop < WORLD_MAX; uiLoop++ )
	{
		gpWorldLevelData[ uiLoop ].usFlags &= ~(MAPELEMENT_REACHABLE);
		gpWorldLevelData[ uiLoop ].usExtFlags[0] &= ~(MAPELEMENT_EXT_ROOFCODE_VISITED);
	}

	// search through world
	// for each location in a room try to find building info
	for ( uiLoop = 0; uiLoop < WORLD_MAX; uiLoop++ )
	{
		if ( (gusWorldRoomInfo[ uiLoop ] != NO_ROOM) && (gubBuildingInfo[ uiLoop ] == NO_BUILDING) && (FindStructure( uiLoop, STRUCTURE_NORMAL_ROOF ) != NULL) )
		{
			GenerateBuilding( uiLoop );
		}
	}
}

INT32 FindClosestBuildingClimbPoint( SOLDIERTYPE *pSoldier, INT32 sStartGridNo, INT32 sDesiredGridNo, BOOLEAN fClimbUp )
{
	BUILDING *		pBuilding;
	INT32			sDistance, sClosestDistance = 1000, sClosestSpot= NOWHERE;

	pBuilding = FindBuilding( sDesiredGridNo );
	if (!pBuilding)
	{
		return( NOWHERE );
	}

	// sevenfm: should not call this function with NULL pSoldier
	if (!pSoldier)
	{
		return NOWHERE;
	}

	// WANNE: This code is from "vanilla" climbing
	UINT8			ubNumClimbSpots;
	INT32 *			psClimbSpots;
	UINT8			ubLoop;

	ubNumClimbSpots = pBuilding->ubNumClimbSpots;

	if (fClimbUp)
	{
		psClimbSpots = pBuilding->sUpClimbSpots;
	}
	else
	{
		psClimbSpots = pBuilding->sDownClimbSpots;
	}

	for ( ubLoop = 0; ubLoop < ubNumClimbSpots; ubLoop++ )
	{
		if( (WhoIsThere2( pBuilding->sUpClimbSpots[ ubLoop ], 0 ) == NOBODY ||
			WhoIsThere2( pBuilding->sUpClimbSpots[ ubLoop ], 0 ) == pSoldier->ubID ) &&
			(WhoIsThere2( pBuilding->sDownClimbSpots[ ubLoop ], 1 ) == NOBODY ||
			WhoIsThere2( pBuilding->sDownClimbSpots[ ubLoop ], 1 ) == pSoldier->ubID) &&
			!InGasSpot( pSoldier, psClimbSpots[ ubLoop], pSoldier->pathing.bLevel ) &&
			!Water( psClimbSpots[ ubLoop], pSoldier->pathing.bLevel) &&
			!InGasSpot(pSoldier, pBuilding->sUpClimbSpots[ubLoop], 0) &&
			!InGasSpot(pSoldier, pBuilding->sDownClimbSpots[ubLoop], 1))
		{
			sDistance = PythSpacesAway( sStartGridNo, psClimbSpots[ ubLoop ] );
			if (sDistance < sClosestDistance )
			{
				sClosestDistance = sDistance;
				sClosestSpot = psClimbSpots[ ubLoop ];
			}
		}
	}

	return( sClosestSpot );
}

BOOLEAN SameBuilding( INT32 sGridNo1, INT32 sGridNo2 )
{
	// sevenfm: add check for room (tile outside next to a building can have same building info as tile inside building)
	//if ( gubBuildingInfo[ sGridNo1 ] == NO_BUILDING || !InARoom(sGridNo1, NULL) )
	if ( gubBuildingInfo[ sGridNo1 ] == NO_BUILDING )
	{
		return( FALSE );
	}
	// sevenfm: add check for room
	//if ( gubBuildingInfo[ sGridNo2 ] == NO_BUILDING || !InARoom(sGridNo2, NULL) )
	if ( gubBuildingInfo[ sGridNo2 ] == NO_BUILDING )
	{
		return( FALSE );
	}
	return( (BOOLEAN) (gubBuildingInfo[ sGridNo1] == gubBuildingInfo[ sGridNo2 ]) );
}
