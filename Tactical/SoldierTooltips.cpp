#ifdef PRECOMPILEDHEADERS
#include "Tactical All.h"
#else
#include "Types.h"
#include "Windows.h"
//#include "Soldier Control.h"
#include "Input.h"
#include "english.h"
#include "Isometric Utils.h"
#include "GameSettings.h"
#include "Overhead.h"
#include "Game Clock.h"
#include "Text.h"
#include "lighting.h"
#include "Interface.h"
#include "weapons.h"
#include "renderworld.h"
#include "Font Control.h"
#include "font.h"
#include "local.h"
#include "vsurface.h"
#include "line.h"
#include "los.h"
// added by SANDRO
#include "SkillCheck.h"
#include "soldier profile type.h"
#include "Soldier macros.h"
// sevenfm
#include "ai.h"
#include "AIInternals.h"
#include "opplist.h"
#include "Map Screen Interface.h"
#include "cheats.h"
#include "Drugs and Alcohol.h"
#endif

//forward declarations of common classes to eliminate includes
class OBJECTTYPE;
class SOLDIERTYPE;

// NCTH tooltip

extern BOOLEAN gfNCTHCantSeeTarget;
extern BOOLEAN gfNCTHSightBlocked;
extern BOOLEAN gfNCTHAssistedShot;
extern INT32 giNCTHSightRange;
extern INT32 giNCTHEffSightRange;
extern INT16 gsNCTHDistVis;
extern INT16 gsNCTHDistVisNoScope;

// NCTH tooltip

extern INT16 gsOCTHBaseCTH;
extern INT16 gsOCTHEffectiveMarksmanship;
extern INT16 gsOCTHMoraleBonus;
extern INT16 gsOCTHFatiguePenalty;
extern INT16 gsOCTHGunStatus;
extern INT16 gsOCTHGunCondition;
extern INT16 gsOCTHDirectionalPenalty;
extern INT16 gsOCTHAnimationPenalty;
extern INT16 gsOCTHTargetDodgePenalty;
extern INT16 gsOCTHTraitBonus;
extern INT16 gsOCTHBackgroundBonus;
extern INT16 gsOCTHStanceBonus;
extern INT16 gsOCTHOneHandBonus;
extern INT16 gsOCTHAutofirePenalty;
extern INT16 gsOCTHTotalAutoPenalty;
extern INT16 gsOCTHPercentMovementPenalty;
extern INT16 gsOCTHBeyondSightPenalty;
extern INT16 gsOCTHOverheatingPenalty;
extern INT16 gsOCTHMaxRangePenalty;
extern INT16 gsOCTHGassedPenalty;
extern INT16 gsOCTHGettingAidPenalty;
extern INT16 gsOCTHInjuryPenalty;
extern INT16 gsOCTHBreathPenalty;
extern INT16 gsOCTHAimTimeBonus;
extern INT16 gsOCTHAimBonus;
extern INT16 gsOCTHAimGearBonus;
extern INT16 gsOCTHMaxAimBonus;
extern INT16 gsOCTHMaxWeaponAim;
extern INT16 gsOCTHMaxShooterAim;
extern INT16 gsOCTHSpotterBonus;
extern INT16 gsOCTHLongRangePenalty;
extern INT16 gsOCTHWeaponRangePenalty;
extern INT16 gsOCTHWeaponLongRangePenalty;
extern INT16 gsOCTHSameSpotBonus;
extern INT16 gsOCTHFocusBonus;
extern INT16 gsOCTHBloodlustBonus;
extern INT16 gsOCTHVisualRangePenalty;
extern INT16 gsOCTHScopeTooClosePenalty;
extern INT16 gsOCTHBodyPartPenalty;
extern INT16 gsOCTHTargetStancePenalty;
extern INT16 gsOCTHHeightDiffPenalty;
extern INT16 gsOCTHScopeStabilityPenalty;
extern INT16 gsOCTHMinRangeForAimBonus;
extern INT16 gsOCTHEquipmentToHit;
extern INT16 gsOCTHWeaponToHit;
extern INT16 gsOCTHCoweringPenalty;
extern INT16 gSOCTHEffSightRange;
extern FLOAT gSOCTHEffRangeMod;
extern INT16 gsOCTHSightRange;
extern INT16 gsOCTHVisualDistance;
extern INT16 gsOCTHVisualBonus;
extern INT16 gsOCTHShootDistance;
extern INT16 gsOCTHWeaponRange;
extern INT16 gsOCTHWeightPenalty;
extern INT16 gsOCTHAutoStrengthPenalty;
extern INT16 gsOCTHBaseSkillPenalty;
extern INT16 gsOCTHAimSkillPenalty;

extern INT16 gsOCTHAccuracy;
extern INT16 gsOCTHAccuracyBonus;
extern INT16 gsOCTHEffRange;
extern INT16 gsOCTHMaxChance;
extern INT16 gsOCTHRawChance;
extern INT16 gsOCTHChance;

extern BOOLEAN gfOCTHCantSeeTarget;
extern BOOLEAN gfOCTHSightBlocked;
extern BOOLEAN gfOCTHAssistedShot;
extern UINT8 gubOCTHTarget;
extern FLOAT giOCTHScopeMagFactor;
extern FLOAT giOCTHScopeVisualFactor;


#define TOOLTIP_TEXT_SIZE 4096

// sevenfm
STR16 gStrAlertStatus[] = {L"Green", L"Yellow", L"Red", L"Black"};
STR16 gStrAttitude[] = {L"DEFENSIVE", L"BRAVESOLO", L"BRAVEAID", L"CUNNINGSOLO", L"CUNNINGAID", L"AGGRESSIVE", L"MAXATTITUDES", L"ATTACKSLAYONLY"};
STR16 gStrOrders[] = {L"STATIONARY", L"ONGUARD", L"CLOSEPATROL", L"FARPATROL", L"POINTPATROL", L"ONCALL", L"SEEKENEMY", L"RNDPTPATROL", L"SNIPER"};
STR16 gStrTeam[] = {L"OUR_TEAM", L"ENEMY_TEAM", L"CREATURE_TEAM", L"MILITIA_TEAM", L"CIV_TEAM", L"LAST_TEAM", L"PLAYER_PLAN", L"LAN_TEAM_ONE", L"LAN_TEAM_TWO", L"LAN_TEAM_THREE", L"LAN_TEAM_FOUR"};
STR16 gStrClass[] = {L"SOLDIER_CLASS_NONE", L"SOLDIER_CLASS_ADMINISTRATOR", L"SOLDIER_CLASS_ELITE", L"SOLDIER_CLASS_ARMY", L"SOLDIER_CLASS_GREEN_MILITIA", L"SOLDIER_CLASS_REG_MILITIA", L"SOLDIER_CLASS_ELITE_MILITIA", L"SOLDIER_CLASS_CREATURE", L"SOLDIER_CLASS_MINER", L"SOLDIER_CLASS_ZOMBIE"};
STR16 gStrAction[] = {
	L"AI_ACTION_NONE",

	L"AI_ACTION_RANDOM_PATROL",
	L"AI_ACTION_SEEK_FRIEND",
	L"AI_ACTION_SEEK_OPPONENT",
	L"AI_ACTION_TAKE_COVER",
	L"AI_ACTION_GET_CLOSER",

	L"AI_ACTION_POINT_PATROL",
	L"AI_ACTION_LEAVE_WATER_GAS",
	L"AI_ACTION_SEEK_NOISE",
	L"AI_ACTION_ESCORTED_MOVE",
	L"AI_ACTION_RUN_AWAY",

	L"AI_ACTION_KNIFE_MOVE",
	L"AI_ACTION_APPROACH_MERC",
	L"AI_ACTION_TRACK",
	L"AI_ACTION_EAT",
	L"AI_ACTION_PICKUP_ITEM",

	L"AI_ACTION_SCHEDULE_MOVE",
	L"AI_ACTION_WALK",
	L"AI_ACTION_RUN",
	L"AI_ACTION_WITHDRAW",
	L"AI_ACTION_FLANK_LEFT",
	L"AI_ACTION_FLANK_RIGHT",
	L"AI_ACTION_MOVE_TO_CLIMB",

	L"AI_ACTION_CHANGE_FACING",

	L"AI_ACTION_CHANGE_STANCE",

	L"AI_ACTION_YELLOW_ALERT",
	L"AI_ACTION_RED_ALERT",
	L"AI_ACTION_CREATURE_CALL",
	L"AI_ACTION_PULL_TRIGGER",

	L"AI_ACTION_USE_DETONATOR",
	L"AI_ACTION_FIRE_GUN",
	L"AI_ACTION_TOSS_PROJECTILE",
	L"AI_ACTION_KNIFE_STAB",
	L"AI_ACTION_THROW_KNIFE",

	L"AI_ACTION_GIVE_AID",
	L"AI_ACTION_WAIT",
	L"AI_ACTION_PENDING_ACTION",
	L"AI_ACTION_DROP_ITEM",
	L"AI_ACTION_COWER",

	L"AI_ACTION_STOP_COWERING",
	L"AI_ACTION_OPEN_OR_CLOSE_DOOR",
	L"AI_ACTION_UNLOCK_DOOR",
	L"AI_ACTION_LOCK_DOOR",
	L"AI_ACTION_LOWER_GUN",

	L"AI_ACTION_ABSOLUTELY_NONE",
	L"AI_ACTION_CLIMB_ROOF",
	L"AI_ACTION_END_TURN",
	L"AI_ACTION_END_COWER_AND_MOVE",
	L"AI_ACTION_TRAVERSE_DOWN",
	L"AI_ACTION_OFFER_SURRENDER",
	L"AI_ACTION_RAISE_GUN",
	L"AI_ACTION_STEAL_MOVE",

	L"AI_ACTION_RELOAD_GUN",

	L"AI_ACTION_JUMP_WINDOW",
	L"AI_ACTION_FREE_PRISONER",
	L"AI_ACTION_USE_SKILL",
	L"AI_ACTION_DOCTOR",
	L"AI_ACTION_DOCTOR_SELF",
	L"AI_ACTION_SELFDETONATE",
	L"AI_ACTION_SWITCH_FLASHLIGHT",
	L"AI_ACTION_FAKE_ALERT",
	L"AI_ACTION_STOP_MEDIC",
	L"AI_ACTION_DRINK_CANTEEN",
	L"AI_ACTION_USE_DRUG",
	L"AI_ACTION_CANCEL_DRAG",
	L"AI_ACTION_HANDLE_ITEM",
	L"AI_ACTION_PLANT_BOMB",
	L"AI_ACTION_STOP_FOCUS"
};

STR16 SeenStr( INT32 value) 
{
	switch(value)
	{
	case -4: return L"HEARD 3";
	case -3: return L"HEARD 2";
	case -2: return L"HEARD LAST";
	case -1: return L"HEARD THIS";
	case 0: return L"NOT HEARD OR SEEN";
	case 1: return L"SEEN CURR";
	case 2: return L"SEEN THIS";
	case 3: return L"SEEN LAST";
	case 4: return L"SEEN 2";
	case 5: return L"SEEN 3";
	}
	return L"unknown";
}

struct MOUSETT
{
	CHAR16 FastHelpText[ TOOLTIP_TEXT_SIZE ];
	INT32 iX;
	INT32 iY;
	INT32 iW;
};

extern struct MOUSETT mouseTT;
extern BOOL mouseTTrender,mouseTTdone;

const int	DL_Limited		= 1;
const int	DL_Basic		= 2;
const int	DL_Full			= 3;
const int	DL_Debug		= 4;

void DisplayWeaponInfo( SOLDIERTYPE*, CHAR16*, UINT8, UINT8 );
void DrawMouseTooltip(void);

#define MAX(a, b) (a > b ? a : b)

void SoldierTooltip( SOLDIERTYPE* pSoldier )
{
	if(!pSoldier)
		return;

	// WANNE: No tooltips on bloodcats, bugs, tanks, robots
	// sevenfm: allow in debug mode
	if (gGameExternalOptions.ubSoldierTooltipDetailLevel < 4 &&
		(CREATURE_OR_BLOODCAT(pSoldier) || TANK(pSoldier) || AM_A_ROBOT( pSoldier)))
	{
		return;
	}

	SGPRect		aRect;
	extern void GetSoldierScreenRect(SOLDIERTYPE*,SGPRect*);
	GetSoldierScreenRect( pSoldier,	&aRect );
	INT16		a1,a2;
	BOOLEAN		fDrawTooltip = FALSE;

	// sevenfm: do not show tooltip if ALT is pressed for adding autofire bullets
	// EDIT: commented this out because with default autofire bullets> 1 it will confuse players
	//SOLDIERTYPE *pShooter;
	//GetSoldier( &pShooter, gusSelectedSoldier );
	//if(gfUICtHBar && pShooter && pShooter->bDoAutofire > 1)
	//	return;

	if ( gfKeyState[ALT] && pSoldier &&
		IsPointInScreenRectWithRelative( gusMouseXPos, gusMouseYPos, &aRect, &a1, &a2 ) )
	{
		MOUSETT		*pRegion = &mouseTT;
		//CHAR16		pStrInfo[ sizeof( pRegion->FastHelpText ) ];
		CHAR16		pStrInfo[ TOOLTIP_TEXT_SIZE ];
		
		int			iNVG = 0;
		INT32		usSoldierGridNo;
		BOOLEAN		fDisplayBigSlotItem	= FALSE;
		BOOLEAN		fMercIsUsingScope	= FALSE;
		UINT16		iCarriedRL = 0;
		INT32		iRangeToTarget = 0;
		UINT8		ubTooltipDetailLevel = gGameExternalOptions.ubSoldierTooltipDetailLevel;
		UINT32		uiMaxTooltipDistance = gGameExternalOptions.ubStraightSightRange;

		fDrawTooltip = TRUE;

		// get the gridno the cursor is at
		GetMouseMapPos( &usSoldierGridNo );

		// get the distance to enemy's tile from the selected merc
		if ( gusSelectedSoldier != NOBODY && gusUIFullTargetID != NOBODY )
		{
			//CHRISL: Changed the second parameter to use the same information as the 'F' hotkey uses.
			//iRangeToTarget = GetRangeInCellCoordsFromGridNoDiff( MercPtrs[ gusSelectedSoldier ]->sGridNo, sSoldierGridNo ) / 10;
			iRangeToTarget = GetRangeInCellCoordsFromGridNoDiff( MercPtrs[ gusSelectedSoldier ]->sGridNo, MercPtrs[ gusUIFullTargetID ]->sGridNo ) / 10;
		}
		// WANNE: If we want to show the tooltip of milita and no merc is present in the sector
		else
		{
			return;
		}

		//SCORE: If UDT range, we work it out as half actual LOS
		// SANDRO - don't use this if detail set to debug!
		if ( gGameExternalOptions.gfAllowUDTRange && gGameExternalOptions.ubSoldierTooltipDetailLevel != DL_Debug && !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS) )
		{
			uiMaxTooltipDistance = (UINT32)( MercPtrs[ gusSelectedSoldier ]->GetMaxDistanceVisible(MercPtrs[ gusUIFullTargetID ]->sGridNo, 0, CALC_FROM_WANTED_DIR) * (gGameExternalOptions.ubUDTModifier));
			uiMaxTooltipDistance /= 100;
		}
		//SCORE: Otherwise if we're using dynamics then do this
		// SANDRO - don't use this if detail set to debug!
		else if ( gGameExternalOptions.fEnableDynamicSoldierTooltips && gGameExternalOptions.ubSoldierTooltipDetailLevel != DL_Debug && !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS) )
		{
			OBJECTTYPE* pObject = &(MercPtrs[gusSelectedSoldier]->inv[HANDPOS]);
			for (attachmentList::iterator iter = (*pObject)[0]->attachments.begin(); iter != (*pObject)[0]->attachments.end(); ++iter) {
				if ( Item[iter->usItem].visionrangebonus > 0 && iter->exists())
				{
					fMercIsUsingScope = TRUE;
					break;
				}
			}

			if ( fMercIsUsingScope )
			{
				// set detail level to (at least) Full
				ubTooltipDetailLevel = MAX(DL_Full,ubTooltipDetailLevel);
			}
		}
		//SCORE: removed to enable scopes to affect range of tooltips.
		//else
		//{
		//If we're using original settings and no scope, we do this
		if (gGameExternalOptions.fEnableDynamicSoldierTooltips && fMercIsUsingScope == 0 && !gGameExternalOptions.gfAllowUDTRange)
		{
				// add 10% to max tooltip viewing distance per level of the merc
				// sevenfm: fixed incorrect integer calculation
				uiMaxTooltipDistance = (INT32)( uiMaxTooltipDistance * ( 1 + ( (FLOAT)( EffectiveExpLevel( MercPtrs[ gusSelectedSoldier ], TRUE ) ) / 10.0 ) ) ); // SANDRO - changed to effective level calc

				if ( !(Item[MercPtrs[gusSelectedSoldier]->inv[HEAD1POS].usItem].nightvisionrangebonus > 0) &&
					!(Item[MercPtrs[gusSelectedSoldier]->inv[HEAD2POS].usItem].nightvisionrangebonus > 0) &&
 					!DayTime() )
				{
					// if night reduce max tooltip viewing distance by a factor of 4 if merc is not wearing NVG
					uiMaxTooltipDistance >>= 2;
				}
		}				
		//SCORE: Dynamic detail, otherwise do what we usually do
		// SANDRO - don't use this if detail set to debug!
		if ( gGameExternalOptions.gfAllowUDTDetail && gGameExternalOptions.ubSoldierTooltipDetailLevel != DL_Debug && !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS) )
		{
			//Check range. Less than a third? Full. Less than 2 thirds? Basic. Otherwise Limited.
			if ( iRangeToTarget <= (INT32)(uiMaxTooltipDistance / 3) )
			{
				ubTooltipDetailLevel = DL_Full;
			}
			else if ( iRangeToTarget <= (INT32)((uiMaxTooltipDistance / 3)*2) )
			{
				ubTooltipDetailLevel = DL_Basic;
			}
			else if ( iRangeToTarget <= (INT32)uiMaxTooltipDistance )
			{
				ubTooltipDetailLevel = DL_Limited;
			}
			else
			{
				return;
			}

		}
		else
		{
				if ( iRangeToTarget <= (INT32)(uiMaxTooltipDistance / 2) )
				{
					// at under half the maximum view distance set tooltip detail to (at least) Basic
					ubTooltipDetailLevel = MAX(DL_Basic,ubTooltipDetailLevel);
				}
				else if ( iRangeToTarget <= (INT32)uiMaxTooltipDistance )
				{
					// at under the maximum view distance set tooltip detail to (at least) Limited
					ubTooltipDetailLevel = MAX(DL_Limited,ubTooltipDetailLevel);
				}
				else
				{
					// beyond visual range, do not display tooltip if player has not chosen full or debug details
					if ( ubTooltipDetailLevel < DL_Full )
						return;
				}
		}
		
		swprintf( pStrInfo, L"" );

		// sevenfm: AI tooltip
		if( gGameExternalOptions.fExtTooltipAI )
		{
			swprintf( pStrInfo, L"%s|[ |A|I |Info |]\n", pStrInfo );
			swprintf( pStrInfo, L"%s|Alert |Status: %s\n", pStrInfo, gStrAlertStatus[pSoldier->aiData.bAlertStatus] );
			swprintf( pStrInfo, L"%s|Orders: %s\n", pStrInfo, gStrOrders[pSoldier->aiData.bOrders] );
			swprintf( pStrInfo, L"%s|Attitude: %s\n", pStrInfo, gStrAttitude[pSoldier->aiData.bAttitude] );
			swprintf( pStrInfo, L"%s|Class: %s\n", pStrInfo, gStrClass[pSoldier->ubSoldierClass] );
			swprintf( pStrInfo, L"%s|Team: %s |Side: %d\n", pStrInfo, gStrTeam[pSoldier->bTeam], pSoldier->bSide );
			if( pSoldier->bTeam == CIV_TEAM && pSoldier->ubCivilianGroup > 0 )
			{
				swprintf(pStrInfo, L"%s|Civ |Group: %s |Hostile %d\n", pStrInfo, zCivGroupName[pSoldier->ubCivilianGroup].szCurGroup, gTacticalStatus.fCivGroupHostile[pSoldier->ubCivilianGroup]);
			}
			if( pSoldier->aiData.bNeutral )
			{
				swprintf( pStrInfo, L"%s|Neutral\n", pStrInfo );
			}
			swprintf( pStrInfo, L"%s|A|I |Morale/|Range |Change: %d/%d\n", pStrInfo, pSoldier->aiData.bAIMorale, RangeChangeDesire(pSoldier) );
			swprintf( pStrInfo, L"%s|Last |Action: %s\n", pStrInfo, gStrAction[pSoldier->aiData.bLastAction] );
			swprintf( pStrInfo, L"%s|OpponentsSeen: %d |LastTurn: %d,| Public %d\n", pStrInfo, pSoldier->aiData.bOppCnt, CountSeenEnemiesLastTurn(pSoldier), CountSeenEnemiesLastTurn(pSoldier, TRUE) );
			swprintf( pStrInfo, L"%s|Friends|Black: %d\n", pStrInfo, CountFriendsBlack(pSoldier) );
			swprintf( pStrInfo, L"%s|Soldier Level: %d |Diff: %d\n", pStrInfo, pSoldier->stats.bExpLevel, SoldierDifficultyLevel( pSoldier ) );
			INT32 usOrigin;
			swprintf( pStrInfo, L"%s|Roaming |Range: %d\n", pStrInfo, RoamingRange( pSoldier, &usOrigin ) );
			//swprintf( pStrInfo, L"%s|Saw |Enemy: %d |Knows |Position: %d\n", pStrInfo, GuySawEnemy(pSoldier), GuyKnowsEnemyPosition( pSoldier ) );
			swprintf( pStrInfo, L"%s|Aware |of |Opposition: %d\n", pStrInfo,  gTacticalStatus.Team[ pSoldier->bTeam ].bAwareOfOpposition);
			swprintf( pStrInfo, L"%s|Collapsed %d |BreathCollapsed %d\n", pStrInfo,  pSoldier->bCollapsed, pSoldier->bBreathCollapsed);
			//swprintf( pStrInfo, L"%s|Interrupt |Duel |Points %d |Counter %d\n", pStrInfo,  pSoldier->aiData.bInterruptDuelPts, pSoldier->aiData.ubInterruptCounter);			
			if(pSoldier->ubPreviousAttackerID != NOBODY && MercPtrs[pSoldier->ubPreviousAttackerID])
			{
				swprintf( pStrInfo, L"%s|Under |Fire %d AttackerID %d AttackerTarget %d\n", pStrInfo, pSoldier->aiData.bUnderFire, pSoldier->ubPreviousAttackerID, MercPtrs[pSoldier->ubPreviousAttackerID]->sLastTarget );
			}
			else
			{
				swprintf(pStrInfo, L"%s|Under |Fire %d AttackerID %d\n", pStrInfo, pSoldier->aiData.bUnderFire, pSoldier->ubPreviousAttackerID);
			}
			swprintf(pStrInfo, L"%s|Retreat %d |Rush %d/%d |Stop|Seek %d\n", pStrInfo, pSoldier->RetreatValue(), pSoldier->RushAttackValue(), RUSH_ATTACK_DURATION, pSoldier->StopSeekValue());
			if(MercUnderTheInfluence(pSoldier, DRUG_TYPE_ADRENALINE))
			{
				swprintf(pStrInfo, L"%s|Adrenaline |Effect %d |SideEffect %d\n", pStrInfo, pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE], pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE]);
			}
			if (MercUnderTheInfluence(pSoldier, DRUG_TYPE_PSYCHO))
			{
				swprintf(pStrInfo, L"%s|Psycho |Effect %d |SideEffect %d\n", pStrInfo, pSoldier->drugs.bDrugEffect[DRUG_TYPE_PSYCHO], pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_PSYCHO]);
			}
			//swprintf(pStrInfo, L"%s|Adrenaline |Used %d\n", pStrInfo, pSoldier->drugs.bTimesDrugUsedSinceSleep[DRUG_TYPE_ADRENALINE]);			

			swprintf( pStrInfo, L"%s|Visible %d |Moved %d\n", pStrInfo,  pSoldier->bVisible, pSoldier->aiData.bMoved );
			//swprintf( pStrInfo, L"%s|Last |Target |Collapsed %d\n", pStrInfo, LastTargetCollapsed(pSoldier) );
			//swprintf( pStrInfo, L"%s|Last |Target |Suppressed %d\n", pStrInfo, LastTargetSuppressed(pSoldier) );
			//swprintf( pStrInfo, L"%s|TeamSawEnemyThisTurn %d\n", pStrInfo, TeamSawEnemyPublic(pSoldier->bTeam, SEEN_THIS_TURN) );
			//swprintf( pStrInfo, L"%s|TeamSawEnemyLastTurn %d\n", pStrInfo, TeamSawEnemyPublic(pSoldier->bTeam, SEEN_LAST_TURN) );

			swprintf( pStrInfo, L"%s|Noise %d %d %d |Public |Noise %d %d %d\n", pStrInfo, pSoldier->aiData.sNoiseGridno, pSoldier->bNoiseLevel, pSoldier->aiData.ubNoiseVolume, gsPublicNoiseGridNo[pSoldier->bTeam], gbPublicNoiseLevel[pSoldier->bTeam], gubPublicNoiseVolume[pSoldier->bTeam] );

			if(pSoldier->IsFocusing())
				swprintf(pStrInfo, L"%s|Focusing |Spot %d |Level %d |Turns %d\n", pStrInfo, pSoldier->sFocusGridNo, pSoldier->bFocusLevel, pSoldier->usSkillCounter[SOLDIER_COUNTER_FOCUS]);

			if (pSoldier->IsSpotting())
				swprintf(pStrInfo, L"%s|Spotting |Spot %d |Level %d |Turns %d\n", pStrInfo, pSoldier->sFocusGridNo, pSoldier->bFocusLevel, pSoldier->usSkillCounter[SOLDIER_COUNTER_SPOTTER]);

			// show watched locations:
			INT8	bLoop;
			swprintf( pStrInfo, L"%s---Watched Locations---\n", pStrInfo);
			for ( bLoop = 0; bLoop < NUM_WATCHED_LOCS; bLoop++ )
			{		
				if (!TileIsOutOfBounds(gsWatchedLoc[ pSoldier->ubID ][ bLoop ]) )
				{
					swprintf( pStrInfo, L"%sGridNo %d Points %d\n", pStrInfo, gsWatchedLoc[ pSoldier->ubID ][ bLoop ], gubWatchedLocPoints[ pSoldier->ubID ][ bLoop ] );
				}
			}

			// sevenfm: show flank info
			if( pSoldier->IsFlanking() )
			{
				swprintf( pStrInfo, L"%s|Flank %s\n", pStrInfo, pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_LEFT ? L"left" : L"right" );
				swprintf( pStrInfo, L"%s|Flank original dir %d\n", pStrInfo, pSoldier->sTacticalData );
				swprintf( pStrInfo, L"%s|Last Flank spot %d\n", pStrInfo, pSoldier->sTacticalSpot );
			}
			else if (pSoldier->bTacticalOrder == TACTICAL_ORDER_FLANK_APPROACH)
			{
				swprintf(pStrInfo, L"%s|Flank approach\n", pStrInfo);
				swprintf(pStrInfo, L"%s|Last Flank spot %d\n", pStrInfo, pSoldier->sTacticalSpot);
			}

			//swprintf( pStrInfo, L"%s|Size:  %d\n", pStrInfo, sizeof( pRegion->FastHelpText ) );			

			//sevenfm: show opponents info
			swprintf( pStrInfo, L"%s---Public list (not neutral)---\n", pStrInfo );
			for(UINT16 oppID = 0; oppID < MAX_NUM_SOLDIERS; oppID++)
			{
				if ( gbPublicOpplist[pSoldier->bTeam][oppID] != NOT_HEARD_OR_SEEN &&
					!MercPtrs[oppID]->aiData.bNeutral )
				{
					swprintf( pStrInfo, L"%s[%d] %s %s\n", pStrInfo, oppID, MercPtrs[oppID]->GetName(), SeenStr(gbPublicOpplist[pSoldier->bTeam][oppID]) );
					//swprintf( pStrInfo, L"%s[%d] %s %d\n", pStrInfo, oppID, MercPtrs[oppID]->GetName(), gbPublicOpplist[pSoldier->bTeam][oppID] );
				}
			}
			swprintf( pStrInfo, L"%s---Soldier list (not neutral)---\n", pStrInfo );
			for(UINT16 oppID = 0; oppID < MAX_NUM_SOLDIERS; oppID++)
			{
				if ( pSoldier->aiData.bOppList[oppID] !=  NOT_HEARD_OR_SEEN &&
					!MercPtrs[oppID]->aiData.bNeutral )
				{
					swprintf( pStrInfo, L"%s[%d] %s %s\n", pStrInfo, oppID, MercPtrs[oppID]->GetName(), SeenStr(pSoldier->aiData.bOppList[oppID]) );
				}
			}
			swprintf( pStrInfo, L"%s|What I know %d\n", pStrInfo, WhatIKnowThatPublicDont(pSoldier, FALSE) );

			swprintf( pStrInfo, L"%s \n", pStrInfo );			
		}

		// sevenfm: CTH tooltip
		if( gGameExternalOptions.fExtTooltipCTH && !UsingNewCTHSystem()  )
		{
			swprintf( pStrInfo, L"%s --|O|C|T|H |Info-- \n", pStrInfo );

			if (gubOCTHTarget != NOBODY)
				swprintf(pStrInfo, L"%s|Target |I|D %d\n", pStrInfo, gubOCTHTarget);
			if (pSoldier->bTilesMoved || pSoldier->TilesMoved())
				swprintf(pStrInfo, L"%s|Target |Turn/Total |Moved %d/%d\n", pStrInfo, pSoldier->bTilesMoved, pSoldier->TilesMoved());

			if (gfOCTHCantSeeTarget)
				swprintf(pStrInfo, L"%s|Can't |See |Target\n", pStrInfo);
			if (gfOCTHSightBlocked)
				swprintf(pStrInfo, L"%s|Sight |Blocked\n", pStrInfo);
			if (gfOCTHAssistedShot)
				swprintf(pStrInfo, L"%s|Assisted |Shot\n", pStrInfo);
			
			swprintf(pStrInfo, L"%s|Effective |Marksmanship %d\n", pStrInfo, gsOCTHEffectiveMarksmanship);
			swprintf(pStrInfo, L"%s|Morale |Bonus %d\n", pStrInfo, gsOCTHMoraleBonus);
			swprintf(pStrInfo, L"%s|Fatigue |Penalty %d\n", pStrInfo, -gsOCTHFatiguePenalty);
			swprintf(pStrInfo, L"%s|Gun |Status/|Condition %d/%d\n", pStrInfo, gsOCTHGunStatus, gsOCTHGunCondition);
			swprintf( pStrInfo, L"%s|Base |Chance %d\n", pStrInfo, gsOCTHBaseCTH );
			swprintf(pStrInfo, L"%s|Target/|Weapon |Range %d/%d\n", pStrInfo, gsOCTHShootDistance, gsOCTHWeaponRange);

			swprintf(pStrInfo, L"%s|Max |Visual |Range %d\n", pStrInfo, gsOCTHVisualDistance);
			swprintf(pStrInfo, L"%s|Visual |Bonus %d%%\n", pStrInfo, gsOCTHVisualBonus);
			swprintf(pStrInfo, L"%s|Sight |Range/|Effective %d/%d\n", pStrInfo, gsOCTHSightRange, gSOCTHEffSightRange);					
			
			swprintf( pStrInfo, L"%s|Scope |Mag/|Factor %.1f/%.1f\n", pStrInfo, giOCTHScopeMagFactor, giOCTHScopeVisualFactor );
			if (gSOCTHEffRangeMod < 1.0f)
				swprintf(pStrInfo, L"%s|Effective |Sight |Mod %.1f\n", pStrInfo, gSOCTHEffRangeMod);
			if (gsOCTHMinRangeForAimBonus)
				swprintf(pStrInfo, L"%s|Min |Aim |Range %d\n", pStrInfo, gsOCTHMinRangeForAimBonus);
			if (gsOCTHScopeTooClosePenalty)
				swprintf(pStrInfo, L"%s|Scope |Too |Close %d\n", pStrInfo, -gsOCTHScopeTooClosePenalty);			
			if (gsOCTHBodyPartPenalty)
				swprintf(pStrInfo, L"%s|Body |Part |Penalty %d\n", pStrInfo, -gsOCTHBodyPartPenalty);
			if (gsOCTHTargetStancePenalty)
				swprintf(pStrInfo, L"%s|Target |Stance |Penalty %d\n", pStrInfo, -gsOCTHTargetStancePenalty);
			if (gsOCTHTargetDodgePenalty)
				swprintf(pStrInfo, L"%s|Target |Dodge |Penalty %d\n", pStrInfo, -gsOCTHTargetDodgePenalty);
			if (gsOCTHHeightDiffPenalty)
				swprintf(pStrInfo, L"%s|Height |Diff |Penalty %d\n", pStrInfo, -gsOCTHHeightDiffPenalty);
			if(gsOCTHTraitBonus)
				swprintf(pStrInfo, L"%s|Trait |Bonus %d\n", pStrInfo, gsOCTHTraitBonus);
			if(gsOCTHBackgroundBonus)
				swprintf(pStrInfo, L"%s|Background |Bonus %d\n", pStrInfo, gsOCTHBackgroundBonus);
			if (gsOCTHStanceBonus)
				swprintf(pStrInfo, L"%s|Stance |Bonus %d\n", pStrInfo, gsOCTHStanceBonus);
			if (gsOCTHOneHandBonus)
				swprintf(pStrInfo, L"%s|One |Hand |Bonus %d\n", pStrInfo, gsOCTHOneHandBonus);
			if (gsOCTHAutofirePenalty)
				swprintf(pStrInfo, L"%s|Auto |Penalty %d\n", pStrInfo, gsOCTHAutofirePenalty);
			if (gsOCTHTotalAutoPenalty)
				swprintf(pStrInfo, L"%s|Shot |Auto |Penalty %d\n", pStrInfo, gsOCTHTotalAutoPenalty);
			if (gsOCTHGassedPenalty)
				swprintf(pStrInfo, L"%s|Gassed |Penalty %d\n", pStrInfo, gsOCTHGassedPenalty);
			if (gsOCTHGettingAidPenalty)
				swprintf(pStrInfo, L"%s|Getting |Aid |Penalty %d\n", pStrInfo, gsOCTHGettingAidPenalty);
			if(gsOCTHInjuryPenalty)
				swprintf(pStrInfo, L"%s|Injury |Penalty %d\n", pStrInfo, gsOCTHInjuryPenalty);
			if (gsOCTHBreathPenalty)
				swprintf(pStrInfo, L"%s|Breath |Penalty %d\n", pStrInfo, gsOCTHBreathPenalty);
			if (gsOCTHWeightPenalty)
				swprintf(pStrInfo, L"%s|Weight |Penalty %d\n", pStrInfo, gsOCTHWeightPenalty);
			if (gsOCTHAutoStrengthPenalty)
				swprintf(pStrInfo, L"%s|Auto |Strength |Penalty %d\n", pStrInfo, gsOCTHAutoStrengthPenalty);
			if (gsOCTHBaseSkillPenalty)
				swprintf(pStrInfo, L"%s|Base/|Aim |Skill |Penalty %d/%d\n", pStrInfo, gsOCTHBaseSkillPenalty, gsOCTHAimSkillPenalty);

			swprintf(pStrInfo, L"%s|Aim |Bonus/|Aim |Gear |Bonus %d/%d\n", pStrInfo, gsOCTHAimBonus, gsOCTHAimGearBonus);
			swprintf(pStrInfo, L"%s|Max |Weapon/|Shooter |Aim %d/%d\n", pStrInfo, gsOCTHMaxWeaponAim, gsOCTHMaxShooterAim);
			swprintf(pStrInfo, L"%s|Aim |Time |Bonus/|Max %d/%d\n", pStrInfo, gsOCTHAimTimeBonus, gsOCTHMaxAimBonus);			
			if (gsOCTHSpotterBonus)
				swprintf( pStrInfo, L"%s|Spotter |Bonus %d\n", pStrInfo, gsOCTHSpotterBonus );			
			if (gsOCTHScopeStabilityPenalty)
				swprintf(pStrInfo, L"%s|Stability |Penalty %d\n", pStrInfo, -gsOCTHScopeStabilityPenalty);
			if (gsOCTHLongRangePenalty)
				swprintf(pStrInfo, L"%s|Long |Range |Penalty %d\n", pStrInfo, -gsOCTHLongRangePenalty);
			swprintf( pStrInfo, L"%s|Weapon |Range |Penalty %d\n", pStrInfo, -gsOCTHWeaponRangePenalty );
			if (gsOCTHWeaponLongRangePenalty)
				swprintf(pStrInfo, L"%s|Weapon |Too |Close %d\n", pStrInfo, -gsOCTHWeaponLongRangePenalty);
			if (gsOCTHSameSpotBonus)
				swprintf(pStrInfo, L"%s|Same |Spot |Bonus %d\n", pStrInfo, gsOCTHSameSpotBonus);
			if (gsOCTHFocusBonus)
				swprintf(pStrInfo, L"%s|Focus |Bonus %d\n", pStrInfo, gsOCTHFocusBonus);
			if (gsOCTHBloodlustBonus)
				swprintf(pStrInfo, L"%s|Bloodlust |Bonus %d\n", pStrInfo, gsOCTHBloodlustBonus);
			swprintf( pStrInfo, L"%s|Visual |Range |Penalty %d\n", pStrInfo, gsOCTHVisualRangePenalty );
			swprintf( pStrInfo, L"%s|ToHit |Weapon/|Equipment %d/%d\n", pStrInfo, gsOCTHWeaponToHit, gsOCTHEquipmentToHit );
			swprintf(pStrInfo, L"%s|Effective |Accuracy/|Bonus %d/%d\n", pStrInfo, gsOCTHAccuracy, gsOCTHAccuracyBonus);

			if (gsOCTHAnimationPenalty || gsOCTHDirectionalPenalty)
				swprintf(pStrInfo, L"%s|Movement (|Anim/|Dir) %d (%d/%d)\n", pStrInfo, max(gsOCTHAnimationPenalty, gsOCTHDirectionalPenalty), gsOCTHAnimationPenalty, gsOCTHDirectionalPenalty);
			//if (gsOCTHInfoDirectionalPenalty)
			//	swprintf(pStrInfo, L"%s|Movement (|Directional) %d\n", pStrInfo, gsOCTHInfoDirectionalPenalty);
			if (gsOCTHPercentMovementPenalty)
				swprintf(pStrInfo, L"%s|Movement (|Percent) %d\n", pStrInfo, gsOCTHPercentMovementPenalty);
			if (gsOCTHCoweringPenalty)
				swprintf( pStrInfo, L"%s|Target |Cowering %d\n", pStrInfo, -gsOCTHCoweringPenalty );
			if (gsOCTHBeyondSightPenalty)
				swprintf(pStrInfo, L"%s|Beyond |Sight |Penalty %d\n", pStrInfo, -gsOCTHBeyondSightPenalty);
			if (gsOCTHOverheatingPenalty)
				swprintf(pStrInfo, L"%s|Overheating |Penalty %d\n", pStrInfo, -gsOCTHOverheatingPenalty);
			if (gsOCTHMaxRangePenalty)
				swprintf(pStrInfo, L"%s|Max |Range |Penalty %d\n", pStrInfo, -gsOCTHMaxRangePenalty);
			
			swprintf( pStrInfo, L"%s|Effective |Range %d\n", pStrInfo, gsOCTHEffRange);
			swprintf(pStrInfo, L"%s|Raw |Chance %d\n", pStrInfo, gsOCTHRawChance);
			swprintf(pStrInfo, L"%s|Chance/|Max |Chance %d/%d\n", pStrInfo, gsOCTHChance, gsOCTHMaxChance);

			swprintf( pStrInfo, L"%s \n", pStrInfo );
		}
		else if (gGameExternalOptions.fExtTooltipNCTH && UsingNewCTHSystem())
		{
			swprintf(pStrInfo, L"%s --|N|C|T|H |Info-- \n", pStrInfo);

			if (gfNCTHCantSeeTarget)
				swprintf(pStrInfo, L"%s|Can't |See |Target\n", pStrInfo);
			if (gfNCTHSightBlocked)
				swprintf(pStrInfo, L"%s|Sight |Blocked\n", pStrInfo);
			if (gfNCTHAssistedShot)
				swprintf(pStrInfo, L"%s|Assisted |Shot\n", pStrInfo);

			swprintf(pStrInfo, L"%s|Sight |Range/|Effective %d/%d\n", pStrInfo, giNCTHSightRange, giNCTHEffSightRange);
			swprintf(pStrInfo, L"%s|Distance |Visible/|No |Scope %d/%d\n", pStrInfo, gsNCTHDistVis, gsNCTHDistVisNoScope);

			swprintf(pStrInfo, L"%s \n", pStrInfo);
		}

		// WANNE: Check if enemy soldier is in line of sight but only if player has not chosen debug details
		if ( ubTooltipDetailLevel < DL_Full)
		{
			// Get the current selected merc
			SOLDIERTYPE* pMerc = MercPtrs[ gusSelectedSoldier ];

			if ( pMerc->aiData.bOppList[pSoldier->ubID] != SEEN_CURRENTLY )
			{
				// We do not see the enemy. Return and do not display the tooltip.
				return;
			}
		}
		
		if ( ubTooltipDetailLevel == DL_Debug || CHEATER_CHEAT_LEVEL())
		{			
			// display "debug" info
			swprintf( pStrInfo, L"%s|[ |Debug |Info |]\n", pStrInfo );
			swprintf(pStrInfo, L"%s|%s\n", pStrInfo, pSoldier->GetName());
			if ( gGameExternalOptions.fEnableSoldierTooltipLocation )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_LOCATION], pStrInfo, usSoldierGridNo );
			if ( gGameExternalOptions.fEnableSoldierTooltipBrightness )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_BRIGHTNESS], pStrInfo, SHADE_MIN - LightTrueLevel( usSoldierGridNo, gsInterfaceLevel ), SHADE_MIN );
			if ( gGameExternalOptions.fEnableSoldierTooltipRangeToTarget )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_RANGE_TO_TARGET], pStrInfo, iRangeToTarget );
			if ( gGameExternalOptions.fEnableSoldierTooltipID )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_ID], pStrInfo, pSoldier->ubID );
			if (gGameExternalOptions.fEnableSoldierTooltipOrders)
			{
				swprintf(pStrInfo, L"%s|Orders: %s\n", pStrInfo, gStrOrders[pSoldier->aiData.bOrders]);
				if ((pSoldier->usSoldierFlagMask & SOLDIER_MILITIA_ORDER))
					swprintf(pStrInfo, L"%s|Militia order\n", pStrInfo);
			}
				//swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_ORDERS], pStrInfo, pSoldier->aiData.bOrders );
			if ( gGameExternalOptions.fEnableSoldierTooltipAttitude )
				swprintf( pStrInfo, L"%s|Attitude: %s\n", pStrInfo, gStrAttitude[pSoldier->aiData.bAttitude] );
				//swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_ATTITUDE], pStrInfo, pSoldier->aiData.bAttitude );
			if ( gGameExternalOptions.fEnableSoldierTooltipActionPoints )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_CURRENT_APS], pStrInfo, pSoldier->bActionPoints );
			if ( gGameExternalOptions.fEnableSoldierTooltipHealth )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_CURRENT_HEALTH], pStrInfo, pSoldier->stats.bLife );
			if ( gGameExternalOptions.fEnableSoldierTooltipEnergy )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_CURRENT_ENERGY], pStrInfo, pSoldier->bBreath );
			if ( gGameExternalOptions.fEnableSoldierTooltipMorale )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_CURRENT_MORALE], pStrInfo, pSoldier->aiData.bMorale );
			//Moa: show shock and suppression values in debug tooltip
			if ( gGameExternalOptions.fEnableSoldierTooltipShock )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_SHOCK], pStrInfo, pSoldier->aiData.bShock );
			// sevenfm: ubSuppressionPoints always show 0 because this value is cleared after each attack
			// changed this to ubLastSuppression - it stores suppression points from last attack
			if ( gGameExternalOptions.fEnableSoldierTooltipSuppressionPoints )
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_SUPPRESION], pStrInfo, pSoldier->ubSuppressionPoints );
			// sevenfm: show additional suppression info
			if ( gGameExternalOptions.fEnableSoldierTooltipSuppressionInfo )
			{				 
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_SUPPRESSION_AP], pStrInfo, pSoldier->ubAPsLostToSuppression );
				//swprintf(pStrInfo, L"%s|ubLastSuppression %d\n", pStrInfo, pSoldier->ubLastSuppression);
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_SUPPRESSION_TOLERANCE], pStrInfo, CalcSuppressionTolerance( pSoldier ) );
				//swprintf( pStrInfo, gzTooltipStrings[STR_TT_EFFECTIVE_SHOCK], pStrInfo, pSoldier->aiData.bShock );
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_AI_MORALE], pStrInfo, pSoldier->aiData.bAIMorale );
			}
			
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Added by SANDRO - show enemy skills
			if ( gGameExternalOptions.fEnableSoldierTooltipTraits )
			{				
				if ( gGameOptions.fNewTraitSystem )
				{
					if (( pSoldier->stats.ubSkillTraits[0] == pSoldier->stats.ubSkillTraits[1] ) && pSoldier->stats.ubSkillTraits[0] != 0 )
					{
						CHAR16 pStrAux[50]; 
						swprintf( pStrAux, L"(%s)", gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[0] ]);
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_1], pStrInfo, pStrAux );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_2], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[1] + NEWTRAIT_MERCSKILL_EXPERTOFFSET ] );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_3], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[2] ] );
						//swprintf( pStrInfo, L"%s\n", pStrInfo );
					}
					else if (( pSoldier->stats.ubSkillTraits[1] == pSoldier->stats.ubSkillTraits[2] ) && pSoldier->stats.ubSkillTraits[1] != 0 )
					{
						CHAR16 pStrAux[50]; 
						swprintf( pStrAux, L"(%s)", gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[1] ]);
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_1], pStrInfo, pStrAux );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_2], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[2] + NEWTRAIT_MERCSKILL_EXPERTOFFSET ] );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_3], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[0] ] );
					}
					else if (( pSoldier->stats.ubSkillTraits[0] == pSoldier->stats.ubSkillTraits[2] ) && pSoldier->stats.ubSkillTraits[0] != 0 )
					{
						CHAR16 pStrAux[50]; 
						swprintf( pStrAux, L"(%s)", gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[0] ]);
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_1], pStrInfo, pStrAux );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_2], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[2] + NEWTRAIT_MERCSKILL_EXPERTOFFSET ] );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_3], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[1] ] );
					}
					else
					{
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_1], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[0] ] );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_2], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[1] ] );
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_3], pStrInfo, gzMercSkillTextNew[ pSoldier->stats.ubSkillTraits[2] ] );
					}
				}
				else
				{
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_1], pStrInfo, gzMercSkillText[pSoldier->stats.ubSkillTraits[0]] );
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_SKILL_TRAIT_2], pStrInfo, gzMercSkillText[pSoldier->stats.ubSkillTraits[1]] );
				}
			}
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		}

		// armor info code block start
		if ( ubTooltipDetailLevel >= DL_Full )
		{
			if ( gGameExternalOptions.fEnableSoldierTooltipHelmet )
			{
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_HELMET], pStrInfo, pSoldier->inv[HELMETPOS].usItem ? ItemNames[ pSoldier->inv[HELMETPOS].usItem ] : gzTooltipStrings[STR_TT_NO_HELMET] );
			}
			if ( gGameExternalOptions.fEnableSoldierTooltipVest )
			{
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_VEST], pStrInfo, pSoldier->inv[VESTPOS].usItem ? ItemNames[ pSoldier->inv[VESTPOS].usItem ] : gzTooltipStrings[STR_TT_NO_VEST] );
			}
			if ( gGameExternalOptions.fEnableSoldierTooltipLeggings )
			{
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_LEGGINGS], pStrInfo, pSoldier->inv[LEGPOS].usItem ? ItemNames[ pSoldier->inv[LEGPOS].usItem ] : gzTooltipStrings[STR_TT_NO_LEGGING] );
			}
		}
		else
		{
			if ( !(	!gGameExternalOptions.fEnableSoldierTooltipHelmet &&
					!gGameExternalOptions.fEnableSoldierTooltipVest &&
					!gGameExternalOptions.fEnableSoldierTooltipLeggings) )
				// do not display the armor line if all the armor toggles are set to false
			{
				if ( ArmourPercent( pSoldier ) )
				{
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_ARMOR] );
					if ( ubTooltipDetailLevel == DL_Basic )
					{
						if ( gGameExternalOptions.fEnableSoldierTooltipHelmet && pSoldier->inv[HELMETPOS].usItem  )
							swprintf( pStrInfo, L"%s%s ", pStrInfo, gzTooltipStrings[STR_TT_HELMET] );
						if ( gGameExternalOptions.fEnableSoldierTooltipVest && pSoldier->inv[VESTPOS].usItem )
							swprintf( pStrInfo, L"%s%s ", pStrInfo, gzTooltipStrings[STR_TT_VEST] );
						if ( gGameExternalOptions.fEnableSoldierTooltipLeggings && pSoldier->inv[LEGPOS].usItem )
							swprintf( pStrInfo, L"%s%s", pStrInfo, gzTooltipStrings[STR_TT_LEGGINGS] );
						wcscat( pStrInfo, L"\n" );
					}
					else // ubTooltipDetailLevel == DL_Limited
					{
						swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_ARMOR_2], gzTooltipStrings[STR_TT_WORN] );
					}
				}
				else
				{
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_ARMOR_2], gzTooltipStrings[STR_TT_NO_ARMOR] );
				}
			}
		}
		// armor info code block end

		// head slots info code block start
		if ( ubTooltipDetailLevel != DL_Debug )
		{

			if( Item[pSoldier->inv[HEAD1POS].usItem].nightvisionrangebonus > 0 )
				iNVG = HEAD1POS;
			else if( Item[pSoldier->inv[HEAD2POS].usItem].nightvisionrangebonus > 0 )
				iNVG = HEAD2POS;

			if ( !(	!gGameExternalOptions.fEnableSoldierTooltipHeadItem1 &&
					!gGameExternalOptions.fEnableSoldierTooltipHeadItem2) )
				// do not display the NVG/mask lines if both head slot toggles are set to false
			{
				if ( ubTooltipDetailLevel >= DL_Full )
				{
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_NVG], pStrInfo,
						iNVG ? ItemNames[ pSoldier->inv[ iNVG ].usItem ] : gzTooltipStrings[STR_TT_NO_NVG] );
				}
				else
				{
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_NVG], pStrInfo, iNVG ? gzTooltipStrings[STR_TT_WORN] : gzTooltipStrings[STR_TT_NO_NVG] );
				}
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_GAS_MASK], pStrInfo,
					( FindGasMask(pSoldier) != NO_SLOT ) ? gzTooltipStrings[STR_TT_WORN] : gzTooltipStrings[STR_TT_NO_MASK] );
			}
		}
		else // gGameExternalOptions.ubSoldierTooltipDetailLevel == DL_Debug
		{
			if ( gGameExternalOptions.fEnableSoldierTooltipHeadItem1 )
			{
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_HEAD_POS_1], pStrInfo, ItemNames[ pSoldier->inv[HEAD1POS].usItem ] );
			}
			if ( gGameExternalOptions.fEnableSoldierTooltipHeadItem2 )
			{
				swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_HEAD_POS_2], pStrInfo, ItemNames[ pSoldier->inv[HEAD2POS].usItem ] );
			}
		}
		// head slots info code block end

		// weapon in primary hand info code block start
		if ( gGameExternalOptions.fEnableSoldierTooltipWeapon )
		{
			DisplayWeaponInfo( pSoldier, pStrInfo, HANDPOS, ubTooltipDetailLevel );
		} // gGameExternalOptions.fEnableSoldierTooltipWeapon
		// weapon in primary hand info code block end

		// weapon in off hand info code block start
		if ( gGameExternalOptions.fEnableSoldierTooltipSecondHand )
		{
			if ( pSoldier->inv[SECONDHANDPOS].usItem )
			{
				// if there's something in the slot display it
				wcscat( pStrInfo, L"\n" );
				DisplayWeaponInfo( pSoldier, pStrInfo, SECONDHANDPOS, ubTooltipDetailLevel );
			}
		}
		// weapon in off hand info code block end

		// large objects in big inventory slots info code block start
		for ( UINT8 BigSlot = BIGPOCKSTART; BigSlot < BIGPOCKFINAL; BigSlot++ )
		{
			if (pSoldier->inv[BigSlot].exists() == false || (pSoldier->inv[BigSlot].fFlags & OBJECT_MODIFIED) && !CHEATER_CHEAT_LEVEL())
				continue; // slot is empty, move on to the next slot

			switch( BigSlot )
			{
				// if display of this big slot is toggled off then move on to the next slot
				case BIGPOCK1POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot1 )
						continue;
					break;
				case BIGPOCK2POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot2 )
						continue;
					break;
				case BIGPOCK3POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot3 )
						continue;
				 break;
				case BIGPOCK4POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot4 )
						continue;
					break;
// CHRISL: Added new large pockets introduced by new inventory system
				case BIGPOCK5POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot5 )
						continue;
					break;
				case BIGPOCK6POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot6 )
						continue;
					break;
				case BIGPOCK7POS:
					if ( !gGameExternalOptions.fEnableSoldierTooltipBigSlot7 )
						continue;
				    break;
			}

			if ( Item[ pSoldier->inv[ BigSlot ].usItem ].rocketlauncher )
				iCarriedRL = pSoldier->inv[ BigSlot ].usItem; // remember that enemy is carrying a rocket launcher when check for rocket ammo is made later on

			if ( (	Item[ pSoldier->inv[ BigSlot ].usItem ].usItemClass == IC_LAUNCHER ) ||
				(	Item[ pSoldier->inv[ BigSlot ].usItem ].usItemClass == IC_GUN ) )
			{
				// it's a firearm
				if ( (	Weapon[pSoldier->inv[ BigSlot ].usItem].ubWeaponClass != HANDGUNCLASS ) &&
					(	Weapon[pSoldier->inv[ BigSlot ].usItem].ubWeaponClass != SMGCLASS ) )
				{
					// it's a long gun or heavy weapon
					fDisplayBigSlotItem = TRUE;
				}
			}
			else
			{
				// check for rocket ammo
				if ( ( iCarriedRL != 0 ) &&												// soldier is carrying a RL ...
					ValidLaunchable( pSoldier->inv[ BigSlot ].usItem, iCarriedRL ) )	// this item is launchable by the RL
				{
					fDisplayBigSlotItem = TRUE;
				}
			}
			if ( fDisplayBigSlotItem )
			{
				wcscat( pStrInfo, gzTooltipStrings[STR_TT_IN_BACKPACK] );
				DisplayWeaponInfo( pSoldier, pStrInfo, BigSlot, ubTooltipDetailLevel );
				fDisplayBigSlotItem = FALSE;
			}
		}
		// large objects in big inventory slots info code block end

		pRegion->iX = gusMouseXPos;
		pRegion->iY = gusMouseYPos;

		/*if ( ubTooltipDetailLevel == DL_Debug )
			swprintf( pRegion->FastHelpText, L"%s\n|String |Length|: %d", pStrInfo, wcslen(pStrInfo) );
		else
			wcscpy( pRegion->FastHelpText, pStrInfo );*/
		wcscpy( pRegion->FastHelpText, pStrInfo );
	}

	if ( gfKeyState[ ALT ] && fDrawTooltip )
	{
		DrawMouseTooltip();
		SetRenderFlags( RENDER_FLAG_FULL );
	}
} // SoldierTooltip(SOLDIERTYPE* pSoldier)


void DisplayWeaponInfo( SOLDIERTYPE* pSoldier, CHAR16* pStrInfo, UINT8 ubSlot, UINT8 ubTooltipDetailLevel )
{
	INT32		iNumAttachments		= 0;
	BOOLEAN		fDisplayAttachment	= FALSE;

	if ( ubTooltipDetailLevel >= DL_Full )
	{
		// display exact weapon model
		swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo,
			WeaponInHand( pSoldier ) ? ItemNames[ pSoldier->inv[ubSlot].usItem ] : gzTooltipStrings[STR_TT_NO_WEAPON] );
	}
	else
	{
		if ( ubTooltipDetailLevel == DL_Limited )
		{
			// display general weapon class
			switch( Weapon[pSoldier->inv[ubSlot].usItem].ubWeaponClass )
			{
				case HANDGUNCLASS:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_HANDGUN] );
					break;
				case SMGCLASS:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_SMG] );
					break;
				case RIFLECLASS:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_RIFLE] );
					break;
				case MGCLASS:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_MG] );
					break;
				case SHOTGUNCLASS:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_SHOTGUN] );
					break;
				case KNIFECLASS:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_KNIFE] );
					break;
				default:
					swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo, gzTooltipStrings[STR_TT_HEAVY_WEAPON] );
					break;
			}
		}
		else
		{
			// display general weapon type
			swprintf( pStrInfo, gzTooltipStrings[STR_TT_CAT_WEAPON], pStrInfo,
				WeaponInHand( pSoldier) ? WeaponType[Weapon[pSoldier->inv[ubSlot].usItem].ubWeaponType] : gzTooltipStrings[STR_TT_NO_WEAPON] );
		}
	}

	if ( gGameExternalOptions.ubSoldierTooltipDetailLevel >= DL_Basic )
	{
		// display weapon attachments
		for (attachmentList::iterator iter = pSoldier->inv[ubSlot][0]->attachments.begin(); iter != pSoldier->inv[ubSlot][0]->attachments.end(); ++iter) {
			if(iter->exists()){
				fDisplayAttachment = FALSE; //Madd: changed this, it was incorrectly showing attachments when it shouldn't be

				if ( ubTooltipDetailLevel == DL_Basic || ubTooltipDetailLevel == DL_Full ) // Madd: also hidden attachments should be hidden at the full level as well... unless the mercs have x-ray vision to see that rod&spring inside the gun!! :p
				{
					// display only externally-visible weapon attachments
					if ( Item[iter->usItem].hiddenattachment )
						fDisplayAttachment = FALSE;
					else
						fDisplayAttachment = TRUE;
				}

				if ( fDisplayAttachment )
				{
					iNumAttachments++;
					if ( iNumAttachments == 1 )
						wcscat( pStrInfo, L"\n[" );
					else
						wcscat( pStrInfo, L", " );
					wcscat( pStrInfo, ItemNames[ iter->usItem ] );
				}
			}
		} // for
		if ( iNumAttachments > 0 )
			wcscat( pStrInfo, pMessageStrings[ MSG_END_ATTACHMENT_LIST ] ); // append ' attached]' to end of string
	} // gGameExternalOptions.ubSoldierTooltipDetailLevel >= DL_Basic
}

MOUSETT mouseTT;
BOOL mouseTTrender, mouseTTdone;

void DrawMouseTooltip()
{
	UINT8 *pDestBuf;
	UINT32 uiDestPitchBYTES;
	static INT32 iX, iY, iW, iH;

	extern INT16 GetWidthOfString(const STR16);
	extern INT16 GetNumberOfLinesInHeight(const STR16);
	extern void DisplayHelpTokenizedString(const STR16,INT16,INT16);
	extern BOOLEAN initTooltipBuffer();
	extern PTR LockTooltipBuffer(UINT32*);
	extern void UnlockTooltipBuffer();
	extern void DisplayTooltipString( const STR16 pStringA, INT16 sX, INT16 sY );
	extern void j_log(PTR,...);

	iX = mouseTT.iX;iY = mouseTT.iY;
	iW = (INT32)GetWidthOfString( mouseTT.FastHelpText ) + 10;
	iH = (INT32)( GetNumberOfLinesInHeight( mouseTT.FastHelpText ) * (GetFontHeight(FONT10ARIAL)+1) + 8 );

	if(1)//draw at cursor
	{
		iY -= (iH / 2);
		if (gusMouseXPos > (SCREEN_WIDTH / 2))
			iX = gusMouseXPos - iW - 24;
		else
			iX = gusMouseXPos + 24;
		//if (gusMouseYPos > (SCREEN_HEIGHT / 2))
		//	iY -= 32;

		if (iY <= 0) iY += 32;
	}
	else
	{	//draw in panel
		//502,485 658,596	160*110 580,540
		iX = 580 - (iW / 2);
		iY = 540 - (iH/2);
		if (iY + iH > SCREEN_HEIGHT)	iY = SCREEN_HEIGHT - iH - 3 ;
	}

	pDestBuf = LockVideoSurface( FRAME_BUFFER, &uiDestPitchBYTES );
	SetClippingRegionAndImageWidth( uiDestPitchBYTES, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );
	RectangleDraw( TRUE, iX + 1, iY + 1, iX + iW - 1, iY + iH - 1, Get16BPPColor( FROMRGB( 65, 57, 15 ) ), pDestBuf );
	RectangleDraw( TRUE, iX, iY, iX + iW - 2, iY + iH - 2, Get16BPPColor( FROMRGB( 227, 198, 88 ) ), pDestBuf );
	UnLockVideoSurface( FRAME_BUFFER );
	ShadowVideoSurfaceRect( FRAME_BUFFER, iX + 2, iY + 2, iX + iW - 3, iY + iH - 3 );
	ShadowVideoSurfaceRect( FRAME_BUFFER, iX + 2, iY + 2, iX + iW - 3, iY + iH - 3 );

	SetFont( FONT10ARIAL );
	SetFontShadow( FONT_NEARBLACK );
	DisplayHelpTokenizedString( mouseTT.FastHelpText ,( INT16 )( iX + 5 ), ( INT16 )( iY + 5 ) );
	InvalidateRegion(	iX, iY, (iX + iW) , (iY + iH) );

	//InvalidateScreen();
}