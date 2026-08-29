#ifndef __AI_H
#define __AI_H

#include "types.h"
#include "worlddef.h"
#include "Soldier Control.h"
#include "Isometric Utils.h"
#include "Rotting Corpses.h"

#define TESTAICONTROL

#define AI_PATHCOST_RADIUS 18
#define AI_PATHCOST_DIM (AI_PATHCOST_RADIUS * 2 + 1)

extern BOOLEAN gfDisplayCoverValues;
//extern INT16 gsCoverValue[WORLD_MAX];
extern INT16 * gsCoverValue;

extern void BeginMultiPurposeLocator( INT32 sGridNo, INT8 bLevel, BOOLEAN fSlideTo );

// AI actions

enum CreatureCalls
{
	CALL_NONE = 0,
	CALL_1_PREY,
	CALL_MULTIPLE_PREY,
	CALL_ATTACKED,
	CALL_CRIPPLED,
	NUM_CREATURE_CALLS
} ;

#define DONTFORCE		0
#define FORCE			1

// ANY NEW ACTIONS ADDED - UPDATE OVERHEAD.C ARRAY WITH ACTION'S STRING VALUE
#define FIRST_MOVEMENT_ACTION AI_ACTION_RANDOM_PATROL
#define LAST_MOVEMENT_ACTION AI_ACTION_MOVE_TO_CLIMB
typedef enum
{
	AI_ACTION_NONE = 0,						// maintain current position & facing

	// actions that involve a move to another tile
	AI_ACTION_RANDOM_PATROL,			// move towards a random destination
	AI_ACTION_SEEK_FRIEND,				// move towards friend in trouble
	AI_ACTION_SEEK_OPPONENT,			// move towards a reported opponent
	AI_ACTION_TAKE_COVER,					// run for nearest cover from threat
	AI_ACTION_GET_CLOSER,					// move closer to a strategic location

	AI_ACTION_POINT_PATROL,				// move towards next patrol point
	AI_ACTION_LEAVE_WATER_GAS,		// seek nearest spot of ungassed land
	AI_ACTION_SEEK_NOISE,					// seek most important noise heard
	AI_ACTION_ESCORTED_MOVE,			// go where told to by escortPlayer
	AI_ACTION_RUN_AWAY,						// run away from nearby opponent(s)

	AI_ACTION_KNIFE_MOVE,					// preparing to stab an opponent
	AI_ACTION_APPROACH_MERC,			// move up to a merc in order to talk with them; RT
	AI_ACTION_TRACK,							// track a scent
	AI_ACTION_EAT,								// monster eats corpse
	AI_ACTION_PICKUP_ITEM,				// grab things lying on the ground

	AI_ACTION_SCHEDULE_MOVE,			// move according to schedule
	AI_ACTION_WALK,								// walk somewhere (NPC stuff etc)
	AI_ACTION_RUN,								// run somewhere (NPC stuff etc)
	AI_ACTION_WITHDRAW,	// back off
	AI_ACTION_FLANK_LEFT,	// move to the left
	AI_ACTION_FLANK_RIGHT,	// move to the right
	AI_ACTION_MOVE_TO_CLIMB,			// move to edge of roof/building
	// miscellaneous movement actions
	AI_ACTION_CHANGE_FACING,			// turn to face a different direction

	AI_ACTION_CHANGE_STANCE,			// stand, crouch, or go prone
	// actions related to items and attacks
	AI_ACTION_YELLOW_ALERT,				// tell friends opponent(s) heard
	AI_ACTION_RED_ALERT,					// tell friends opponent(s) seen
	AI_ACTION_CREATURE_CALL,			// creature communication
	AI_ACTION_PULL_TRIGGER,				// go off to activate a panic trigger

	AI_ACTION_USE_DETONATOR,			// grab detonator and set off bomb(s)
	AI_ACTION_FIRE_GUN,						// shoot at nearby opponent
	AI_ACTION_TOSS_PROJECTILE,		// throw grenade at/near opponent(s)
	AI_ACTION_KNIFE_STAB,					// during the actual knifing attack
	AI_ACTION_THROW_KNIFE,				// throw a knife

	AI_ACTION_GIVE_AID,						// help injured/dying friend
	AI_ACTION_WAIT,								// RT: don't do anything for a certain length of time
	AI_ACTION_PENDING_ACTION,			// RT: wait for pending action (pickup, door open, etc) to finish
	AI_ACTION_DROP_ITEM,					// duh
	AI_ACTION_COWER,							// for civilians:	cower in fear and stay there!

	AI_ACTION_STOP_COWERING,			// stop cowering
	AI_ACTION_OPEN_OR_CLOSE_DOOR,	// schedule-provoked; open or close door
	AI_ACTION_UNLOCK_DOOR,				// schedule-provoked; unlock door (don't open)
	AI_ACTION_LOCK_DOOR,					// schedule-provoked; lock door (close if necessary)
	AI_ACTION_LOWER_GUN,					// lower gun prior to throwing knife

	AI_ACTION_ABSOLUTELY_NONE,		// like "none" but can't be converted to a wait by realtime
	AI_ACTION_CLIMB_ROOF,					// climb up or down roof
	AI_ACTION_END_TURN,						// end turn (after final stance change)
	AI_ACTION_END_COWER_AND_MOVE,	// sort of dummy value, special for civilians who are to go somewhere at end of battle
	AI_ACTION_TRAVERSE_DOWN,			// move down a level
	AI_ACTION_OFFER_SURRENDER,		// offer surrender to the player
	AI_ACTION_RAISE_GUN,
	AI_ACTION_STEAL_MOVE, // added by SANDRO

	AI_ACTION_RELOAD_GUN,

	AI_ACTION_JUMP_WINDOW,			// added by Flugente: jump through a window
	AI_ACTION_FREE_PRISONER,		// added by Flugente: free a prisoner
	AI_ACTION_USE_SKILL,			// added by Flugente: perform a skill, which one is stored in usAISkillUse

	AI_ACTION_DOCTOR,				// added by Flugente: AI-ONLY! bandage/surgery on fellow AI. DO NOT USE THIS FOR MERCS!!!
	AI_ACTION_DOCTOR_SELF,			// added by Flugente: AI-ONLY! bandage/surgery on self. DO NOT USE THIS FOR MERCS!!!
	AI_ACTION_SELFDETONATE,			// added by Flugente: blow up an explosive in own inventory
	AI_ACTION_SWITCH_FLASHLIGHT,	// sevenfm: use flag SOLDIER_AI_FLASHLIGHT to emulate having flashlight in hands
	AI_ACTION_FAKE_ALERT,			// sevenfm: show radio animation
	AI_ACTION_STOP_MEDIC,			// sevenfm: stop first aid animation
	AI_ACTION_DRINK_CANTEEN,		// sevenfm: drink from canteen in inventory
	AI_ACTION_USE_DRUG,				// sevenfm: use drug from inventory
	AI_ACTION_CANCEL_DRAG,			// sevenfm: stop dragging someone
	AI_ACTION_HANDLE_ITEM,			// sevenfm: use item in hand
	AI_ACTION_PLANT_BOMB,			// sevenfm: plant bomb using item in hand
	AI_ACTION_STOP_FOCUS,			// sevenfm: stop focusing
	AI_ACTION_LAST = AI_ACTION_PLANT_BOMB
} ActionType;


enum QuoteActionType
{
	QUOTE_ACTION_ID_CHECKFORDEST = 1,
	QUOTE_ACTION_ID_TURNTOWARDSPLAYER,
	QUOTE_ACTION_ID_DRAWGUN,
	QUOTE_ACTION_ID_LOWERGUN,
	QUOTE_ACTION_ID_TRAVERSE_EAST,
	QUOTE_ACTION_ID_TRAVERSE_SOUTH,
	QUOTE_ACTION_ID_TRAVERSE_WEST,
	QUOTE_ACTION_ID_TRAVERSE_NORTH,
} ;

// aiData.ubAIFlags
#define AI_FLAG_1						0x01
#define AI_FLAG_2						0x02
#define AI_FLAG_3						0x04
#define AI_CAUTIOUS						0x08
#define AI_HANDLE_EVERY_FRAME			0x10
#define AI_ASLEEP						0x20
#define AI_LOCK_DOOR_INCLUDES_CLOSE		0x40
#define AI_CHECK_SCHEDULE				0x80

// aiData.bNewSituation
#define NOT_NEW_SITUATION 0
#define WAS_NEW_SITUATION 1
#define IS_NEW_SITUATION 2

// gbDiff[]
#define DIFF_ENEMY_EQUIP_MOD			0
#define DIFF_ENEMY_TO_HIT_MOD			1
#define DIFF_ENEMY_INTERRUPT_MOD		2
#define DIFF_RADIO_RED_ALERT			3
#define DIFF_MAX_COVER_RANGE			4
#define MAX_DIFF_PARMS									5		// how many different difficulty variables?

extern INT8 gbDiff[MAX_DIFF_PARMS][5];

void ActionDone(SOLDIERTYPE *pSoldier);
INT16 ActionInProgress(SOLDIERTYPE *pSoldier);

INT8 CalcMorale(SOLDIERTYPE *pSoldier);
INT32 CalcPercentBetter(INT32 iOldValue, INT32 iNewValue, INT32 iOldScale, INT32 iNewScale);
void CallAvailableEnemiesTo(INT32 sGridNo);
void CallAvailableKingpinMenTo( INT32 sGridNo );
void CallAvailableTeamEnemiesTo( INT32 sGridNo, INT8 bTeam );
void CallEldinTo( INT32 sGridNo );
void CancelAIAction(SOLDIERTYPE *pSoldier, UINT8 ubForce);
void CheckForChangingOrders(SOLDIERTYPE *pSoldier );

INT8 ClosestPanicTrigger( SOLDIERTYPE * pSoldier );

INT32 ClosestKnownOpponent(SOLDIERTYPE *pSoldier, INT32 *psGridNo, INT8 *pbLevel, UINT8 *pubOpponentID = NULL);
INT32 ClosestActiveOpponent(SOLDIERTYPE *pSoldier, INT32 *psGridNo = NULL, INT8 *pbLevel = NULL, UINT8 *pubOpponentID = NULL);
INT16 DistanceToClosestActiveOpponent(SOLDIERTYPE *pSoldier, INT32 sSpot);
UINT8 ClosestKnownOpponentID(SOLDIERTYPE *pSoldier, INT32 sSpot);
INT32 ClosestPC( SOLDIERTYPE *pSoldier, INT32 * psDistance );
INT32 ClosestUnDisguisedPC( SOLDIERTYPE *pSoldier, INT32 * psDistance );	// Flugente: like ClosestPC(...), but does not account for covert or not visible mercs
BOOLEAN CanAutoBandage( BOOLEAN fDoFullCheck );

void DebugAI( STR szOutput );
INT8 DecideAction(SOLDIERTYPE *pSoldier);
INT8 DecideActionBlack(SOLDIERTYPE *pSoldier);
INT8 DecideActionEscort(SOLDIERTYPE *pSoldier);
INT8 DecideActionGreen(SOLDIERTYPE *pSoldier);
INT8 DecideActionRed(SOLDIERTYPE *pSoldier);
INT8 DecideActionYellow(SOLDIERTYPE *pSoldier);

INT16 DistanceToClosestFriend( SOLDIERTYPE * pSoldier );

void EndAIDeadlock(void);
void EndAIGuysTurn( SOLDIERTYPE *pSoldier );

INT8	ExecuteAction(SOLDIERTYPE *pSoldier);

INT32 FindAdjacentSpotBeside(SOLDIERTYPE *pSoldier, INT32 sGridNo);
INT32 FindBestNearbyCover(SOLDIERTYPE *pSoldier, INT32 *piPercentBetter, INT8 bReserveAP = 0, BOOLEAN fAttack = FALSE);
INT32 FindClosestDoor( SOLDIERTYPE * pSoldier );
INT32 FindNearbyPointOnEdgeOfMap( SOLDIERTYPE * pSoldier, INT8 * pbDirection );
INT32 FindNearestEdgePoint( INT32 sGridNo );
INT32 FindNearestPassableSpot( INT32 sGridNo, UINT8 usSearchRadius = 5 );

//Kris:	Added these as I need specific searches on certain sides.
enum
{
	NORTH_EDGEPOINT_SEARCH,
	EAST_EDGEPOINT_SEARCH,
	SOUTH_EDGEPOINT_SEARCH,
	WEST_EDGEPOINT_SEARCH,
};
INT32 FindNearestEdgepointOnSpecifiedEdge( INT32 sGridNo, INT8 bEdgeCode );

INT32 FindNearestUngassedLand(SOLDIERTYPE *pSoldier);
INT32 FindSpotMaxDistFromOpponents(SOLDIERTYPE *pSoldier);
INT32 FindSweetCoverSpot(SOLDIERTYPE *pSoldier);

void FreeUpNPCFromAttacking(UINT8 ubID);
void FreeUpNPCFromPendingAction( SOLDIERTYPE * pSoldier );
void FreeUpNPCFromTurning(SOLDIERTYPE *pSoldier, INT8 bLook);
void FreeUpNPCFromStanceChange(SOLDIERTYPE *pSoldier );
void FreeUpNPCFromLoweringGun( SOLDIERTYPE *pSoldier );
void FreeUpNPCFromRoofClimb(SOLDIERTYPE *pSoldier );

//UINT8 GetClosestOpponent( SOLDIERTYPE *pSoldier );
UINT8 GetMostThreateningOpponent( SOLDIERTYPE *pSoldier );
INT16 DistanceToClosestEnemy(INT32 sMySpot, INT8 bMyTeam);

void HandleSoldierAI( SOLDIERTYPE *pSoldier );
void HandleInitialRedAlert( INT8 bTeam, UINT8 ubCommunicate);

void InitPanicSystem();
INT16 InWaterOrGas(SOLDIERTYPE *pSoldier, INT32 sGridno);
BOOLEAN IsActionAffordable(SOLDIERTYPE *pSoldier, INT8 bAction = AI_ACTION_NONE);
BOOLEAN InitAI( void );

void MakeClosestEnemyChosenOne();
void ManChecksOnFriends(SOLDIERTYPE *pSoldier);

void NewDest(SOLDIERTYPE *pSoldier, INT32 sGridNo);
INT32 NextPatrolPoint(SOLDIERTYPE *pSoldier);
BOOLEAN CheckPathForObstacles(SOLDIERTYPE *pSoldier);
BOOLEAN CheckNextPathForObstacles(SOLDIERTYPE *pSoldier);
BOOLEAN CheckPathForGas(SOLDIERTYPE *pSoldier);

INT8 PanicAI(SOLDIERTYPE *pSoldier, UINT8 ubCanMove);
void HaltMoveForSoldierOutOfPoints(SOLDIERTYPE *pSoldier);

INT32 RandDestWithinRange(SOLDIERTYPE *pSoldier);
INT16 RandomFriendWithin(SOLDIERTYPE *pSoldier);

INT16 RoamingRange(SOLDIERTYPE *pSoldier, INT32 *pusFromGridNo);

void SetCivilianDestination(UINT8 ubWho, INT32 sGridNo);
void SetNewSituation( SOLDIERTYPE * pSoldier );

UINT8 SoldierDifficultyLevel( SOLDIERTYPE * pSoldier );
void SoldierTriesToContinueAlongPath(SOLDIERTYPE *pSoldier);
void StartNPCAI(SOLDIERTYPE *pSoldier);
void TempHurt(SOLDIERTYPE *pVictim, SOLDIERTYPE *pAttacker);
int TryToResumeMovement(SOLDIERTYPE *pSoldier, INT32 sGridNo);

BOOLEAN ValidCreatureTurn( SOLDIERTYPE * pCreature, INT8 bNewDirection );

BOOLEAN WearGasMaskIfAvailable( SOLDIERTYPE * pSoldier );
BOOLEAN RemoveGasMask(SOLDIERTYPE * pSoldier);
INT16 WhatIKnowThatPublicDont(SOLDIERTYPE *pSoldier, BOOLEAN fInSightOnly);

INT32 FindClosestClimbPoint (SOLDIERTYPE *pSoldier, BOOLEAN fClimbUp );
INT32 FindFlankingSpot(SOLDIERTYPE *pSoldier, INT32 sPos, INT8 bAction );
BOOLEAN CanClimbFromHere (SOLDIERTYPE * pSoldier);

// HEADROCK HAM 3.6: Adding includes for A* cover calculations
INT8 CalcCTGTForPosition(SOLDIERTYPE * pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft);
INT8 CalcCTGTForPositionNearby(SOLDIERTYPE *pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft);
INT8 CalcWorstCTGTForPosition(SOLDIERTYPE * pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft);
INT8 CalcBestCTGT( SOLDIERTYPE *pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft );
INT8 CalcAverageCTGTForPosition( SOLDIERTYPE * pSoldier, UINT8 ubOppID, INT32 sOppGridNo, INT8 bLevel, INT32 iMyAPsLeft );
UINT8 NumberOfTeamMatesAdjacent( SOLDIERTYPE * pSoldier, INT32 sGridNo );

// Flugente: get the id of the closest soldier (coser than x tiles) of a specific team with a specific flag that we can currently see
UINT8 GetClosestFlaggedSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam, UINT32 aFlag, BOOLEAN fCheckSight = TRUE );
// get the id of the closest soldier (closer than x tiles) of a specific team that is wounded that we can currently see
UINT8 GetClosestWoundedSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam );
// get the id of the closest medic (closer than x tiles) of a specific team
UINT8 GetClosestMedicSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam );
// get the id of the closest officer
UINT8 GetClosestOfficerSoldierID( SOLDIERTYPE * pSoldier, INT16 aRange, UINT8 auTeam );
// get the id of the closest friend under drugs
UINT8 GetClosestSeenDruggedFriendID(SOLDIERTYPE * pSoldier);

INT8 ZombieDecideActionGreen(SOLDIERTYPE *pSoldier);
INT8 ZombieDecideActionYellow(SOLDIERTYPE *pSoldier);
INT8 ZombieDecideActionRed(SOLDIERTYPE *pSoldier);
INT8 ZombieDecideActionBlack(SOLDIERTYPE *pSoldier);

INT8 TankDecideActionGreen(SOLDIERTYPE *pSoldier);
INT8 TankDecideActionYellow(SOLDIERTYPE *pSoldier);
INT8 TankDecideActionRed(SOLDIERTYPE *pSoldier);
INT8 TankDecideActionBlack(SOLDIERTYPE *pSoldier);

void LogDecideInfo(SOLDIERTYPE *pSoldier);
void LogKnowledgeInfo(SOLDIERTYPE *pSoldier);

// sevenfm:
INT16 MaxNormalVisionDistance( void );
UINT8 FlankingDirection(SOLDIERTYPE *pSoldier);
BOOLEAN GuySawEnemy( SOLDIERTYPE * pSoldier, UINT8 ubMax = SEEN_3_TURNS_AGO );
BOOLEAN GuyHeardEnemy( SOLDIERTYPE * pSoldier, UINT8 ubMax = HEARD_2_TURNS_AGO );
BOOLEAN CanClimbFlank(SOLDIERTYPE *pSoldier);
INT32 ClosestSeenThisTurnOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel);
INT32 ClosestSeenLastTurnOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel);
INT32 ClosestPossiblySeenOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel);
INT32 ClosestUnseenOpponent(SOLDIERTYPE *pSoldier, INT32 * psGridNo, INT8 * pbLevel);
UINT8 ClosestOpponentInContactID(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fCheckSight);
BOOLEAN InSmoke( INT32 sGridNo, INT8 bLevel, BOOLEAN fCheckNearbySource );
BOOLEAN InFire(INT32 sGridNo, INT8 bLevel);
BOOLEAN InSmokeNearby(INT32 sGridNo, INT8 bLevel);
BOOLEAN NightLight( void );
BOOLEAN DuskLight(void);
UINT8 CountSeenEnemiesLastTurn( SOLDIERTYPE *pSoldier, BOOLEAN fOnlyPublicKnown = FALSE);
UINT8 CountEngagedEnemiesLastTurn(SOLDIERTYPE *pSoldier);
UINT8 CanAttackEnemy(SOLDIERTYPE *pSoldier, INT8 bStance, BOOLEAN fExactSight);
BOOLEAN GuyKnowsEnemyPosition( SOLDIERTYPE * pSoldier );
BOOLEAN EnemySeenSoldierRecently( SOLDIERTYPE *pSoldier, INT8 bMax, BOOLEAN fOnlyAlerted );
BOOLEAN EnemyHeardSoldierRecently( SOLDIERTYPE *pSoldier, INT8 bMax, BOOLEAN fOnlyAlerted );
BOOLEAN CheckDoorAtGridno( UINT32 usGridNo );
BOOLEAN CheckDoorNearGridno( UINT32 usGridNo );
BOOLEAN FindBombNearby( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance );
BOOLEAN AllowDeepWater( SOLDIERTYPE *pSoldier );
INT16 DistanceToClosestNotSeekEnemyFriend( SOLDIERTYPE *pSoldier, INT32 sGridNo );
UINT8 CountPublicKnownEnemies( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance );
UINT8 CountPublicKnownEnemies(SOLDIERTYPE *pSoldier);
UINT8 CountKnownEnemiesInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoom);
UINT16 SumDistanceToKnownEnemiesInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoom, INT32 sSpot);
UINT8 CountFriendsInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoom);
UINT8 CountKnownEnemies(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, INT8 bLevel = -1);
UINT8 CountKnownEnemiesInDirection(SOLDIERTYPE *pSoldier, UINT8 ubDirection, INT16 sDistance, BOOLEAN fAdjacent);
INT16 SeekRange(SOLDIERTYPE *pSoldier, INT32 *pusFromGridNo);
BOOLEAN TerrainCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel);
UINT8 CountNotNeutral(void);

UINT8 ClosestSeenThreatID( SOLDIERTYPE *pSoldier, UINT8 ubMax = SEEN_CURRENTLY );	// first call PrepareThreatlist to make threat list
UINT8 ClosestKnownThreatID( SOLDIERTYPE *pSoldier );								// first call PrepareThreatlist to make threat list
void PrepareThreatlist( SOLDIERTYPE *pSoldier );

UINT8 CountFriendsFlankSameSpot( SOLDIERTYPE *pSoldier, INT32 sSpot = NOWHERE );
UINT8 CountRushAttackFriends(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance);
UINT8 CountRushAttackTeam(INT8 bTeam);
UINT8 CountFriendsLastAttackHit( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance );
UINT8 CountFriendsShortRangeWeapon(SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance);
UINT8 CountNearbyFriends( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance );
UINT8 CountNearbyNeutrals( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance );
UINT8 CountNearbyFriendsOnRoof( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT16 sDistance );
UINT8 CountFriendsBetweenMeAndSpotFromSpot( SOLDIERTYPE *pSoldier, INT32 sTargetGridNo );
UINT8 CountFriendsInDirection( SOLDIERTYPE *pSoldier, UINT8 ubDirection, INT16 sDistance, BOOLEAN fCheckSight );
UINT8 CountFriendsInDirectionFromSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubDirection, INT16 sDistance);
UINT8 CountFriendsCoverDirection( SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent, INT32 sHisGridNo, UINT8 ubCheckDir );
UINT8 CountFriendsBlack( SOLDIERTYPE *pSoldier, INT32 sClosestOpponent = NOWHERE );
UINT8 CountFriendsSeenEnemy( SOLDIERTYPE *pSoldier, INT16 sDistance, UINT8 ubMax = SEEN_CURRENTLY );
UINT8 CountSeenCovertOpponents( SOLDIERTYPE *pSoldier, UINT8 ubMax = SEEN_CURRENTLY );
UINT8 CountNearbyKnownCovertOpponents( SOLDIERTYPE *pSoldier );
UINT8 CountFriendsPossiblySeeFlashlightOpponent(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel, UINT8 ubID);
UINT8 CountFriendsUsedRadio(SOLDIERTYPE *pSoldier, INT16 sDistance);
UINT8 CountFriendsSeeOpponent(SOLDIERTYPE *pSoldier, UINT8 ubOpponentID);

BOOLEAN WeAttack(INT8 bTeam);
BOOLEAN SectorAttack(INT8 bTeam);
UINT8 SectorLeader(INT8 bTeam);
BOOLEAN AllowComplexAI(SOLDIERTYPE *pSoldier);
BOOLEAN GarrisonDefense(SOLDIERTYPE *pSoldier);

UINT8 CountTeamCombat( INT8 bTeam );
UINT8 CountTeamSeeSoldier( INT8 bTeam, SOLDIERTYPE *pSoldier );
UINT8 CountTeamNearSpot( INT8 bTeam, INT32 sSpot, UINT8 ubDistance );
UINT8 CountTeamUnconsciousNearSpot(INT8 bTeam, INT32 sSpot, UINT8 ubDistance);
UINT8 CountTeamUnderAttack(INT8 bTeam, INT32 sGridNo, INT16 sDistance);

BOOLEAN TeamKnowsSoldier( INT8 bTeam, UINT8 ubID );
BOOLEAN TeamSeesGridNo( INT8 bTeam, INT32 sGridNo, INT8 bLevel, INT8 bStance );
BOOLEAN TeamAtGridno( INT8 bTeam, INT32 sGridNo, INT8 bLevel);
BOOLEAN TeamSawEnemy( INT8 bTeam, UINT8 ubMax = SEEN_3_TURNS_AGO );
BOOLEAN TeamSawEnemyPublic( INT8 bTeam, UINT8 ubMax = SEEN_3_TURNS_AGO );
BOOLEAN TeamHeardEnemyPublic(INT8 bTeam, UINT8 ubMax = HEARD_3_TURNS_AGO);
UINT8 CorpseWarning( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT8 bLevel);
UINT16 CorpseMoralePenalty(SOLDIERTYPE *pSoldier);
INT32	CountCorpses(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, BOOLEAN fCheckSight);
INT32	CountCorpsesInRoom(SOLDIERTYPE *pSoldier, UINT16 usRoomNo, INT8 bLevel);
INT32	CountCorpsesInDirection(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubDirection, INT16 sDistance, BOOLEAN fCheckSight, BOOLEAN fFresh);
BOOLEAN CorpseEnemyTeam(ROTTING_CORPSE *pCorpse);
BOOLEAN CorpseMilitiaTeam(ROTTING_CORPSE *pCorpse);
BOOLEAN UseSightCoverAdvance( SOLDIERTYPE *pSoldier );
UINT8	RedSmokeDanger( INT32 sGridNo, INT8 bLevel );
BOOLEAN	RedSmokeOnMap(void);
BOOLEAN CheckArtilleryStrike( void );
BOOLEAN CheckRoof( INT32 sGridNo );
BOOLEAN CheckWindow(INT32 sSpot, UINT8 ubDirection, BOOLEAN fAllowClosed);
BOOLEAN CheckClosedWindow(INT32 sSpot, UINT8 ubDirection);
UINT8 AIEstimateAttackWeight(SOLDIERTYPE *pAttacker, INT32 sAttackerSpot, SOLDIERTYPE *pMe, INT32 sMySpot);
BOOLEAN FindClosestVisibleSmoke( SOLDIERTYPE *pSoldier, INT32 &sSpot, INT8 &bLevel, BOOLEAN fOnlyGas, INT32 sMaxDistance = -1);
BOOLEAN FindClosestGas(SOLDIERTYPE *pSoldier, INT32 &sSpot, INT8 &bLevel, INT32 sMaxDistance = -1);
UINT16 CountVisibleSmokeEffects(SOLDIERTYPE *pSoldier, INT16 sDistance, INT8 bType);
void	SetTeamHalfAP( INT8 bTeam );
UINT8	TerrainDensity( INT32 sSpot, INT8 bLevel, UINT8 ubDistance, BOOLEAN fGrass );
UINT8	TerrainJungleDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance);
UINT8	TerrainDesertDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance);
UINT8	TerrainUrbanDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance);
UINT8	TerrainSnowDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance);
UINT8	TerrainDarknessDensity(INT32 sSpot, INT8 bLevel, UINT8 ubDistance);
BOOLEAN	TerrainJungle(INT32 sSpot, INT8 bLevel);
BOOLEAN	TerrainDesert(INT32 sSpot, INT8 bLevel);
BOOLEAN	TerrainUrban(INT32 sSpot, INT8 bLevel);
BOOLEAN	TerrainSnow(INT32 sSpot, INT8 bLevel);
BOOLEAN	TerrainDark(INT32 sSpot, INT8 bLevel);

INT32	RandomizeLocation(INT32 sSpot, INT8 bLevel, UINT8 ubTimes, SOLDIERTYPE *pSightSoldier);
INT32	RandomizeOpponentLocation(INT32 sSpot, SOLDIERTYPE *pOpponent, INT16 sMaxDistance);
UINT8	CountNearbyFences(INT32 sSpot, INT8 bLevel);
BOOLEAN FindFenceAroundSpot(INT32 sSpot);
BOOLEAN	FindNearbyExplosiveStructure(INT32 sSpot, INT8 bLevel);
BOOLEAN CheckDangerousDirection(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel);

//BOOLEAN SoldierIsStandingAtPublicKnownSpot( SOLDIERTYPE *pSoldier );
BOOLEAN PossiblyUpdateKnowledge( SOLDIERTYPE *pSoldier );
INT32	FindClosestUnseenSpot( SOLDIERTYPE *pSoldier, INT32 sCheckSpot, INT8 bCheckLevel, INT32 iSearchRange, INT32 iIgnoreRange, INT16 sAPLimit, BOOLEAN fTeamSight);
void	AddNoiseAtRandomSightCoverSpot( SOLDIERTYPE *pSoldier, INT32 sCheckSpot, INT8 bCheckLevel, BOOLEAN fTeamSight, BOOLEAN fTeamNoise );

#define MAX_SPOT_LIST_SIZE 2048
#define MAX_SUSPICION 100

enum{
	TACTICAL_ORDER_NONE = 0,
	TACTICAL_ORDER_SNIPER,
	TACTICAL_ORDER_FLANK_LEFT,
	TACTICAL_ORDER_FLANK_RIGHT,
	TACTICAL_ORDER_FLANK_APPROACH,
	TACTICAL_ORDER_RUSH_ATTACK,
	TACTICAL_ORDER_RETREAT
};

#define RUSH_ATTACK_DURATION 5
#define RETREAT_DURATION 3
#define FLANKING_DURATION 25
#define END_FLANKING_DURATION 5

enum{
	GROUP_ORDER_NONE = 0,
	GROUP_ORDER_RUSH,
	GROUP_ORDER_ATTACK,
	GROUP_ORDER_DEFENCE,
	GROUP_ORDER_AMBUSH,
	GROUP_ORDER_RETREAT,
	GROUP_ORDER_FLEE
};

UINT32	CountSuspicionValue( SOLDIERTYPE *pSoldier, SOLDIERTYPE *pVictim );
void RaiseSuspicionVictim(SOLDIERTYPE *pVictim, INT16 sAP);
BOOLEAN SeenThisTurnEnemyCanSeeMe( SOLDIERTYPE *pSoldier );
BOOLEAN SeenEnemyCanSeeMe( SOLDIERTYPE *pSoldier );
BOOLEAN EnemyAlerted( SOLDIERTYPE *pSoldier );
BOOLEAN TeamEnemyAlerted(INT8 bTeam);

BOOLEAN AICheckUnderAttack( SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fCheckSuccessfull );
BOOLEAN AICheckSpotUnderAttack(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fCheckSuccessfull, BOOLEAN fAdjacent = FALSE);
BOOLEAN AICheckIsBandaged( SOLDIERTYPE *pSoldier );
BOOLEAN AICheckHasWeaponOfType( SOLDIERTYPE *pSoldier, UINT8 ubWeaponType );
BOOLEAN AICheckHasFlamethrowerInHands(SOLDIERTYPE *pSoldier);
BOOLEAN AICheckHasGun( SOLDIERTYPE *pSoldier );
BOOLEAN AICheckShortWeaponRange( SOLDIERTYPE *pSoldier );
BOOLEAN AICheckTown( void );
UINT8 AISectorType(void);
BOOLEAN AICheckUnderground( void );
BOOLEAN AICheckSpotTooFar( SOLDIERTYPE *pSoldier, INT32 sSpot );
BOOLEAN AICheckRoamingRange(SOLDIERTYPE *pSoldier, INT32 sSpot);
BOOLEAN AICheckDefense( SOLDIERTYPE *pSoldier );
BOOLEAN AICheckInterrupt( void );

BOOLEAN AICheckSuccessfulAttack( SOLDIERTYPE *pSoldier, BOOLEAN fGroup );
BOOLEAN AICheckWeOutnumberPublic( SOLDIERTYPE *pSoldier, INT32 sSpot );
BOOLEAN AICheckWeOutnumberLocal( SOLDIERTYPE *pSoldier, INT32 sSpot );
BOOLEAN AICheckWeOutnumber( SOLDIERTYPE *pSoldier, INT32 sSpot );
BOOLEAN AICheckWeOutnumberSector(SOLDIERTYPE *pSoldier);
BOOLEAN AICheckCanCommunicate( SOLDIERTYPE *pSoldier1, SOLDIERTYPE *pSoldier2 );

BOOLEAN AIRoleSniper(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleMarksman(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleMachinegunner(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleRadioOperator(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleMedic(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleDoctor(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleRPGOperator(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleMortarOperator(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleGLOperator(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleOfficer(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleCommander(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleSuppress(SOLDIERTYPE *pSoldier);
BOOLEAN AIRoleCQB(SOLDIERTYPE *pSoldier);

BOOLEAN AICheckHasKey(SOLDIERTYPE *pSoldier);
BOOLEAN AICheckHasTNT(SOLDIERTYPE *pSoldier);

BOOLEAN AISpecialRole(SOLDIERTYPE *pSoldier);

BOOLEAN AggressiveAI(SOLDIERTYPE *pSoldier);

BOOLEAN AIUseFlashlightCondition(SOLDIERTYPE *pSoldier, UINT8 ubDirection);
UINT8 AIChooseFlashlightDirection(SOLDIERTYPE *pSoldier);

BOOLEAN AllowDeepWaterFlanking(SOLDIERTYPE *pSoldier);
BOOLEAN CheckFocusSpot(SOLDIERTYPE *pSoldier, INT32 sTargetSpot, INT8 bTargetLevel);
BOOLEAN FindRandomFocusSpot(SOLDIERTYPE *pSoldier, INT32 &sFocusSpot, INT8 &bFocusLevel, BOOLEAN fCover);
BOOLEAN FindCombatFocusSpot(SOLDIERTYPE *pSoldier, INT32 &sFocusSpot, INT8 &bFocusLevel);

BOOLEAN CanSeeEnemyAtSpot( SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubEnemyStance);
BOOLEAN AnyCoverFromSpot(INT32 sSpot, INT8 bLevel, INT32 sThreatLoc, INT8 bThreatLevel, BOOLEAN fAllowGrass = TRUE, BOOLEAN fAllowSmoke = FALSE);
BOOLEAN AnyCoverFromDirection(INT32 sSpot, INT8 bLevel, UINT8 ubDirection, BOOLEAN fAllowGrass = TRUE, BOOLEAN fAllowSmoke = FALSE);
BOOLEAN AnyCoverFromSpot2(INT32 sSpot, INT8 bLevel, INT32 sThreatLoc, INT8 bThreatLevel, BOOLEAN fAllowGrass = FALSE, BOOLEAN fAllowSmoke = FALSE);
BOOLEAN SightCoverNearSpot( SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubStance, BOOLEAN fUnlimited = FALSE );
BOOLEAN AnyCoverNearSpot(SOLDIERTYPE *pSoldier, INT32 sSpot);
BOOLEAN EnemyCanAttackSpot( SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel );
UINT8 CalcSpotDanger(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel);

INT32 CalcSituationValue(SOLDIERTYPE *pSoldier, INT32 sSmokeSpot, INT8 bSmokeLevel, INT16 sDistance);
BOOLEAN CheckBetween(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel, INT16 sDistance, BOOLEAN fBestChance);
BOOLEAN CheckSpotBetween(INT32 sSpot1, INT32 sSpot2, INT32 sCheckSpot);

BOOLEAN DangerousSpot(SOLDIERTYPE *pSoldier);
BOOLEAN SafeSpot(SOLDIERTYPE *pSoldier, INT32 sSpot = NOWHERE);
BOOLEAN SafeMedicSpot(SOLDIERTYPE *pSoldier, INT32 sSpot);
BOOLEAN AbortFinalSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bAction, INT32 sClosestDisturbance, INT8 bDisturbanceLevel, INT32 &sDangerousSpot);
BOOLEAN AbortPath(SOLDIERTYPE *pSoldier, INT8 bAction, INT32 sClosestDisturbance, INT8 bDisturbanceLevel, INT32 &sDangerousSpot, INT32 &sLastSafeSpot);	// needs prepared path before calling this function
UINT8 CountPathCorpses(SOLDIERTYPE *pSoldier);
UINT8 SpotDangerLevel(SOLDIERTYPE *pSoldier, INT32 sGridNo);

BOOLEAN CanRushAttack(SOLDIERTYPE *pSoldier);
UINT8 CountCanRushAttackFriends(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, INT16 sMaxPathAP, BOOLEAN fCheckStopSeek, BOOLEAN fCheckBlack);
UINT8 CountCanRushAttackTeam(INT8 bTeam);
void StartRushAttackFriends(SOLDIERTYPE *pSoldier, INT32 sSpot, INT16 sDistance, INT16 sMaxPathAP);
void StartRushAttackTeam(INT8 bTeam, INT32 sSpot);

BOOLEAN SightCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, UINT8 ubStance, BOOLEAN fUnlimited, BOOLEAN fAdjacent = TRUE);
BOOLEAN AnyCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, BOOLEAN fAdjacent = TRUE);
BOOLEAN PathCoverAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel);
BOOLEAN FindObstacleNearSpot(INT32 sSpot, INT8 bLevel);
UINT8 CountObstaclesNearSpot(INT32 sSpot, INT8 bLevel);
BOOLEAN FindShadowAtSpot(INT32 sSpot, INT8 bLevel);
BOOLEAN NorthSpot(INT32 sSpot, INT8 bLevel);

INT32 AICalcThreatTolerance(SOLDIERTYPE *pSoldier);
INT32 AICalcTotalEnemyThreatAtSpot(SOLDIERTYPE *pSoldier, INT32 sSpot);
UINT8 AIEstimateCTH(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pTarget, INT32 sMySpot, INT32 sHisSpot, BOOLEAN fMovementPenalty);
UINT8 AIEstimateSuppression(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent, INT16 sAP);
INT16 AICalcActionPoints(SOLDIERTYPE *pSoldier);

enum{
	ADVANCE_SPOT_SIGHT_COVER,
	ADVANCE_SPOT_PRONE_COVER,
	ADVANCE_SPOT_ANY_COVER
};

enum{
	SPOT_FLAG_NONE = 0,
	SPOT_FLAG_OBSTACLE = 1,
	SPOT_FLAG_SMOKE = 2
};

INT32 FindAdvanceSpot(SOLDIERTYPE *pSoldier, INT32 sTargetSpot, INT8 bAction, UINT8 ubType, BOOLEAN fUnlimited, UINT8 ubSpotFlag = SPOT_FLAG_NONE);
INT32 FindRetreatSpot(SOLDIERTYPE *pSoldier);
INT32 FindSafeMedicSpot(SOLDIERTYPE *pSoldier);
BOOLEAN CheckSuppressionDirection( SOLDIERTYPE *pSoldier, INT32 sTargetGridNo, INT8 bTargetLevel );
UINT8 AISuppressionEffectiveness(void);

BOOLEAN AIGunScoped(SOLDIERTYPE *pSoldier);
BOOLEAN AIGunInHandScoped(SOLDIERTYPE *pSoldier);
BOOLEAN AIGunExtraAmmo(SOLDIERTYPE *pSoldier);
UINT16 AIGunRange(SOLDIERTYPE *pSoldier);				// gun range in tiles
UINT16 AIGunMagSize(SOLDIERTYPE *pSoldier);
UINT16 AIGunClass(SOLDIERTYPE *pSoldier);
UINT16 AIGunType(SOLDIERTYPE *pSoldier);
INT16 AIGunMinAPsToShoot(SOLDIERTYPE *pSoldier, BOOLEAN fRaiseCost = TRUE);
FLOAT AIGunScopeMagFactor(SOLDIERTYPE *pSoldier);
UINT16 AIGunAmmo(SOLDIERTYPE *pSoldier);
BOOLEAN AIGunAutofireCapable(SOLDIERTYPE *pSoldier);
UINT8 AIGunDeadliness(SOLDIERTYPE *pSoldier);
INT16 AIGunStatus(SOLDIERTYPE *pSoldier);

BOOLEAN AICheckNVG( SOLDIERTYPE *pSoldier );

BOOLEAN SoldierAI( SOLDIERTYPE *pSoldier );
BOOLEAN VeteranAI( SOLDIERTYPE *pSoldier );
BOOLEAN EliteAI( SOLDIERTYPE *pSoldier );

UINT8 AIDirection(INT32 sSpot1, INT32 sSpot2);

void PlayLNL( SOLDIERTYPE* pSoldier );

enum {AI_MSG_START, AI_MSG_DECIDE, AI_MSG_INFO, AI_MSG_TOPIC};
void DebugAI( INT8 bMsgType, SOLDIERTYPE *pSoldier, STR szOutput, INT8 bAction = -1 );
void DebugCover( SOLDIERTYPE *pSoldier, STR szOutput );
void DebugShot( SOLDIERTYPE *pSoldier, STR szOutput );
void DebugMiscInfo( STR szOutput );
void DebugCTHInfo(STR szOutput);
void DebugBRInfo(STR szOutput);
void DebugQuestInfo(STR szOutput);
void DebugStartingGear(STR szOutput);

INT16 VisionRange( void );
INT16 DayVisionRange( void );
INT16 NightVisionRange( void );
INT16 AIDistanceVisible(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel);
INT16 AIMaxDistanceVisible(SOLDIERTYPE *pSoldier, INT32 sSpot, INT8 bLevel);
INT8 RainIntensity();

extern void SwapGoggles(SOLDIERTYPE *pTeamSoldier);

INT8 AIEstimateInterruptLevel( SOLDIERTYPE *pSoldier );
INT8 FindMaxEnemyInterruptLevel( SOLDIERTYPE *pSoldier, INT32 sGridNo, INT8 blevel, UINT8 ubDistance );

UINT8 TeamPercentKilled(INT8 bTeam);
BOOLEAN TeamHighPercentKilled(INT8 bTeam);
UINT8 ArmyPercentKilled( void );
UINT8 ArmyPercentKilledTolerance( void );
BOOLEAN LocateEnemyJammer(SOLDIERTYPE* pSoldier, INT32 &sSpot, INT8 &bLevel);
BOOLEAN FindEnemyJammer(SOLDIERTYPE* pSoldier);
BOOLEAN TeamHasActiveRadioOperator(INT8 bTeam);
BOOLEAN TeamHasScanningRadioOperator(INT8 bTeam);
BOOLEAN TeamHasListeningRadioOperator(INT8 bTeam);
UINT16 MaxTeamListenValue(INT8 bTeam);
BOOLEAN TeamHasJammingRadioOperator(INT8 bTeam);
BOOLEAN TeamCanOrderArtilleryStrike(INT8 bTeam);
UINT8 SectorCurfew( BOOLEAN fNight );
BOOLEAN ConsiderProne(SOLDIERTYPE * pSoldier);
BOOLEAN ConsiderCrouch(SOLDIERTYPE * pSoldier);

UINT8 SectorBuildingPercent(void);
INT32 RoomSize(INT32 sSpot);
INT32 BuildingSize(INT32 sSpot);

BOOLEAN SameRoom(INT32 sSpot1, INT32 sSpot2);
UINT16	RoomNo(INT32 sSpot);

void PrepareMainRedAIWeights(SOLDIERTYPE *pSoldier, INT8 &bSeekPts, INT8 &bHelpPts, INT8 &bHidePts, INT8 &bWatchPts);

BOOLEAN DecideCuttingFenceFlank(SOLDIERTYPE *pSoldier);
BOOLEAN DecideBlowUpObstacleFlank(SOLDIERTYPE *pSoldier);
BOOLEAN DecideMedic(SOLDIERTYPE *pSoldier);
BOOLEAN DecideMedicCombat(SOLDIERTYPE *pSoldier);
BOOLEAN DecideBlowUpObstacle(SOLDIERTYPE *pSoldier, INT32 sClosestOpponent);
BOOLEAN DecideJumpWindow(SOLDIERTYPE *pSoldier, INT32 sClosestOpponent);

BOOLEAN DecideRetreatReverse(SOLDIERTYPE *pSoldier, INT32 sClosestThreat, INT8 bClosestThreatLevel);
BOOLEAN DecideSeekReverse(SOLDIERTYPE *pSoldier, INT32 sClosestThreat, INT8 bClosestThreatLevel);
BOOLEAN DecideCoverReverse(SOLDIERTYPE *pSoldier, INT32 sClosestThreat, INT8 bClosestThreatLevel);

// *************************************************************
// Knowledge functions
// *************************************************************

INT8 Knowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );
INT32 KnownLocation( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );
INT8 KnownLevel( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );

BOOLEAN UsePersonalKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );

INT8 PersonalKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );
INT32 KnownPersonalLocation( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );
INT8 KnownPersonalLevel( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );

INT8 PublicKnowledge( UINT8 bTeam, UINT8 ubOpponentID );
INT32 KnownPublicLocation( UINT8 bTeam, UINT8 ubOpponentID );
INT8 KnownPublicLevel( UINT8 bTeam, UINT8 ubOpponentID );

void UpdateKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID, INT32 sSpot, INT8 bLevel );
void UpdatePersonalKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID, INT32 sSpot, INT8 bLevel );
void UpdatePublicKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID, INT32 sSpot, INT8 bLevel );

UINT8 PersonalNoiseVolume( SOLDIERTYPE *pSoldier );
INT8 PersonalNoiseLevel( SOLDIERTYPE *pSoldier );
INT32 PersonalNoiseLocation( SOLDIERTYPE *pSoldier );

UINT8 PublicNoiseVolume( UINT8 bTeam );
INT8 PublicNoiseLevel( UINT8 bTeam );
INT32 PublicNoiseLocation( UINT8 bTeam );

void SetPersonalNoise( SOLDIERTYPE* pSoldier, UINT8 ubVolume, INT32 sLocation, INT8 bLevel );
void SetPublicNoise( UINT8 bTeam, UINT8 ubVolume, INT32 sLocation, INT8 bLevel );

BOOLEAN PersonalSeenKnowledge( SOLDIERTYPE *pSoldier, UINT8 ubOpponentID );

// *************************************************************

enum
{
	COVER_TYPE_PRONE,
	COVER_TYPE_CROUCHED,
	COVER_TYPE_STANDING,
	COVER_TYPE_ANY
};

enum 
{
	SEEK_NORMAL,
	SEEK_JUMP_WINDOW,
	SEEK_PLANT_BOMB
};

class CoverCacheInfo
{
public:
	INT32 sSpot;
	INT8 bLevel;
	UINT8 ubType;
	BOOLEAN fUnlimited;
	BOOLEAN fAdjacent;
};

// *************************************************************

BOOLEAN ValidOpponent( SOLDIERTYPE* pSoldier, SOLDIERTYPE* pOpponent );
BOOLEAN ValidTeamOpponent( INT8 bTeam, SOLDIERTYPE* pOpponent );

//void StartCachingLOS(void);
//void StopCachingLOS(void);

#define MAX_TILES_MOVE_TURN (APBPConstants[AP_MAXIMUM] / (APBPConstants[AP_MOVEMENT_FLAT] + APBPConstants[AP_MODIFIER_RUN]))

#define MAX_FLANKS_RED 25

// vision range defines
#define MAX_VISION_RANGE ((INT16)(gGameExternalOptions.ubStraightSightRange * 4 * STRAIGHT_RATIO))
#define DAY_VISION_RANGE ((INT16)DayVisionRange())
#define NIGHT_VISION_RANGE ((INT16)NightVisionRange())
#define VISION_RANGE ((INT16)VisionRange())

// sevenfm: distance for tactical AI checks, roughly equal to normal day vision range
#define TACTICAL_RANGE ((INT16)(gGameExternalOptions.ubStraightSightRange * STRAIGHT_RATIO * 2))
#define BOMB_DETECTION_RANGE (TACTICAL_RANGE / 4)

// sevenfm: limit min/max flank distance depending on sight range and time of day
#define MIN_FLANK_DIST (TACTICAL_RANGE / 2)
#define MAX_FLANK_DIST (VISION_RANGE + MAX_TILES_MOVE_TURN)

// max value of bAwareOfOpposition
#define MAX_AWARE_OF_OPPOSITION 100
#define MIN_AI_HEALABLE_INJURY 500

#endif