#ifdef PRECOMPILEDHEADERS
	#include "Tactical All.h"
#else
	#include "sgp.h"
	#include "soldier control.h"
	#include "soldier profile.h"
	#include "drugs and alcohol.h"
	#include "items.h"
	#include "morale.h"
	#include "points.h"
	#include "message.h"
	#include "GameSettings.h" // SANDRO - had to add this, dammit!
	#include "Random.h"
	#include "Text.h"
	#include "Interface.h"
	#include "Food.h"			// added by Flugente
	#include "Animation data.h"	// added by Flugente for SoldierBodyTypes
	#include "CampaignStats.h"	// added by Flugente
	// sevenfm
	#include "Interface Panels.h"
	#include "Sound Control.h"
	#include "Vehicles.h"
#endif

//forward declarations of common classes to eliminate includes
class OBJECTTYPE;
class SOLDIERTYPE;

extern void SoldierCollapse( SOLDIERTYPE *pSoldier );

/*UINT8 ubDrugTravelRate[]			= { 4,	2 };
UINT8 ubDrugWearoffRate[]			= { 2,	2 };
UINT8 ubDrugEffect[]				= { 15, 8 };
UINT8 ubDrugSideEffect[]			= { 20, 10 };
UINT8 ubDrugSideEffectRate[]		= { 2,	1 };*/

INT32	giDrunkModifier[] =
{
	100,		// Sober
	75,			// Feeling good,
	65,			// Borderline
	50,			// Drunk
	//100,		// HungOver
	80,			// HungOver
};

//#define HANGOVER_AP_REDUCE			5
#define HANGOVER_AP_REDUCE			10
#define HANGOVER_BP_REDUCE			200

BOOLEAN ApplyDrugs( SOLDIERTYPE *pSoldier, OBJECTTYPE *pObject, BOOLEAN fUseAPs )
{
	UINT32  ubDrugType;
	INT8	bRegenPointsGained;
	UINT16	usItem;

	usItem = pObject->usItem;

	// Determine what type of drug....
	ubDrugType = Item[usItem].drugtype;

	// If not a drug, return
	if ( ubDrugType == 0 )
	{
		return( FALSE );
	}

	// how did this even happen?
	if (!pSoldier || !pObject || !(pObject->exists()) || (*pObject)[0]->data.objectStatus < 1)
		return(FALSE);

	// don't feed our machines
	if (pSoldier->ubProfile == ROBOT || IsVehicle(pSoldier))
		return(FALSE);

	BOOLEAN consumeitem = TRUE;
	// if item is also a food item, don't use it up here, it will be consumed in ApplyFood, which will be called afterwards
	UINT32 foodtype = Item[pObject->usItem].foodtype;

	// don't consume food item here, it will be consumed later in ApplyFood()
	// sevenfm: for player mercs only
	if (gGameOptions.fFoodSystem && foodtype > 0 && pSoldier->IsSoldierPC())
		consumeitem = FALSE;

	if (fUseAPs && consumeitem && !EnoughPoints(pSoldier, APBPConstants[AP_DRINK], 0, pSoldier->IsSoldierPC()))
	{
		//return( TRUE );
		return( FALSE );
	}

	// do switch for Larry!!
	if ( pSoldier->ubProfile == LARRY_NORMAL )
	{
		pSoldier = SwapLarrysProfiles( pSoldier );
	}
	else if ( pSoldier->ubProfile == LARRY_DRUNK )
	{
		gMercProfiles[ LARRY_DRUNK ].bNPCData = 0;
	}

	// sevenfm: stop inspiration when we consume any drugs
	pSoldier->StopInspire();
		
	// Flugente: we have to check for every single type of drug ( a drug applied may consist of several 'pure' drug types)	
	for (UINT8 i = DRUG_TYPE_ADRENALINE; i < DRUG_TYPE_MAX; ++i)
	{
		UINT32 drugtestflag = (1 << i);			// comparing with this flag will determine the drug

		// we do not actually test for DRUG_REGENERATION, because that is checked afterwards separately
		if ( (drugtestflag & DRUG_REGENERATION ) != 0 )
			continue;
		
		if ( (ubDrugType & drugtestflag) != 0 )
		{
			// Add effects
			pSoldier->AddDrugValues( i, Drug[i].ubDrugEffect, Drug[i].ubDrugTravelRate, Drug[i].ubDrugSideEffect );

			// sevenfm: increase effect...
			if ( pSoldier->drugs.bDrugEffectRate[ i ] > 0 )
			{
				// Seap some in....
				pSoldier->drugs.bFutureDrugEffect[ i ] -= pSoldier->drugs.bDrugEffectRate[ i ];
				pSoldier->drugs.bDrugEffect[ i ]	 += pSoldier->drugs.bDrugEffectRate[ i ];

				// Refresh morale w/ new drug value...
				RefreshSoldierMorale( pSoldier );

				// Check if we need to stop 'adding'
				if ( pSoldier->drugs.bFutureDrugEffect[ i ] <= 0 )
				{
					pSoldier->drugs.bFutureDrugEffect[ i ] = 0;
					// Change rate to -ve..
					pSoldier->drugs.bDrugEffectRate[ i ] = -Drug[ i ].ubDrugWearoffRate;
				}
			}

			// ATE: Make guy collapse from heart attack if too much stuff taken....
			if (pSoldier->drugs.bDrugSideEffectRate[i] > (Drug[i].ubDrugSideEffect * 3) ||
				(drugtestflag & DRUG_ALCOHOL) && pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE] && Chance(pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE]) ||
				(drugtestflag & DRUG_ALCOHOL) && pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE] && Chance(pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE]) ||
				(drugtestflag & DRUG_ADRENALINE) && pSoldier->drugs.bDrugEffect[DRUG_TYPE_ALCOHOL] && Chance(pSoldier->drugs.bDrugEffect[DRUG_TYPE_ALCOHOL]) ||
				(drugtestflag & DRUG_ADRENALINE) && pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ALCOHOL] && Chance(pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ALCOHOL]))
			{			
				// Keel over...
				ScreenMsg(FONT_MCOLOR_LTRED, MSG_INTERFACE, New113Message[MSG113_DRUG_HEART_ATTACK]);
				DeductPoints( pSoldier, 0, 10000 );
				pSoldier->bActionPoints = __min(0, pSoldier->bActionPoints);
				SoldierCollapse(pSoldier);

				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// SANDRO - if our stat is damaged through facility event, make it healable
				// Permanently lower certain stats...
				if ( gGameOptions.fNewTraitSystem )
				{
					// WISDOM decrease
					if ( pSoldier->stats.bWisdom > 5 )
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_WISDOM ] += 5;
						pSoldier->stats.bWisdom -= 5;
					}
					else
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_WISDOM ] += (pSoldier->stats.bWisdom - 1);
						pSoldier->stats.bWisdom = 1;
					}
					// DEXTERITY decrease
					if ( pSoldier->stats.bDexterity > 5 )
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_DEXTERITY ] += 5;
						pSoldier->stats.bDexterity -= 5;
					}
					else
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_DEXTERITY ] += (pSoldier->stats.bDexterity - 1);
						pSoldier->stats.bDexterity = 1;
					}
					// STRENGTH decrease
					if ( pSoldier->stats.bStrength > 5 )
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_STRENGTH ] += 5;
						pSoldier->stats.bStrength -= 5;
					}
					else
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_STRENGTH ] += (pSoldier->stats.bStrength - 1);
						pSoldier->stats.bStrength = 1;
					}
					// AGILITY decrease
					if ( pSoldier->stats.bAgility > 5 )
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_AGILITY ] += 5;
						pSoldier->stats.bAgility -= 5;
					}
					else
					{
						pSoldier->ubCriticalStatDamage[ DAMAGED_STAT_AGILITY ] += (pSoldier->stats.bAgility - 1);
						pSoldier->stats.bAgility = 1;
					}
				}
				else // old system
				{
					pSoldier->stats.bAgility	= __max(1, pSoldier->stats.bWisdom-5);
					pSoldier->stats.bDexterity	= __max(1, pSoldier->stats.bDexterity-5);
					pSoldier->stats.bStrength	= __max(1, pSoldier->stats.bStrength-5);
					pSoldier->stats.bAgility	= __max(1, pSoldier->stats.bAgility-5);
					
					// make those stats RED for a while...
					// SANDRO - we don't need to do this with new system, as we simply show all damaged stats in red until healed
					pSoldier->timeChanges.uiChangeWisdomTime = GetJA2Clock();
					pSoldier->usValueGoneUp &= ~( WIS_INCREASE );
					pSoldier->timeChanges.uiChangeDexterityTime = GetJA2Clock();
					pSoldier->usValueGoneUp &= ~( DEX_INCREASE );
					pSoldier->timeChanges.uiChangeStrengthTime = GetJA2Clock();
					pSoldier->usValueGoneUp &= ~( STRENGTH_INCREASE );
					pSoldier->timeChanges.uiChangeAgilityTime = GetJA2Clock();
					pSoldier->usValueGoneUp &= ~( AGIL_INCREASE );
				}
				////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if(pSoldier->ubProfile != NO_PROFILE)
				{
					// export stat changes to profile
					gMercProfiles[pSoldier->ubProfile].bWisdom = pSoldier->stats.bWisdom;
					gMercProfiles[pSoldier->ubProfile].bDexterity = pSoldier->stats.bDexterity;
					gMercProfiles[pSoldier->ubProfile].bStrength = pSoldier->stats.bStrength;
					gMercProfiles[pSoldier->ubProfile].bAgility = pSoldier->stats.bAgility;

					// SANDRO - merc records - stat damaged
					gMercProfiles[pSoldier->ubProfile].records.usTimesStatDamaged++;
				}
			}
		}
	}

	if ( (ubDrugType & DRUG_REGENERATION) != 0 )
	{
		// each use of a regen booster over 1, each day, reduces the effect
		bRegenPointsGained = REGEN_POINTS_PER_BOOSTER * (*pObject)[0]->data.objectStatus / 100;
		// are there fractional %s left over?
		/*if ( ( (*pObject)[0]->data.objectStatus % (100 / REGEN_POINTS_PER_BOOSTER ) ) != 0 )
		{
			// chance of an extra point
			if ( PreRandom( 100 / REGEN_POINTS_PER_BOOSTER ) < (UINT32) ( (*pObject)[0]->data.objectStatus % (100 / REGEN_POINTS_PER_BOOSTER ) ) )
			{
				bRegenPointsGained++;
			}
		}*/

		// sevenfm: scale bRegenBoostersUsedToday with max value of REGEN_POINTS_PER_BOOSTER
		bRegenPointsGained -= REGEN_POINTS_PER_BOOSTER * pSoldier->bRegenBoostersUsedToday / 4;
		if (bRegenPointsGained > 0)
		{
			// can't go above the points you get for a full boost
			pSoldier->bRegenerationCounter = min( pSoldier->bRegenerationCounter + bRegenPointsGained, REGEN_POINTS_PER_BOOSTER );
		}
		pSoldier->bRegenBoostersUsedToday++;

		// set flag: we are on drugs
		pSoldier->usSoldierFlagMask |= SOLDIER_DRUGGED;
	}

	// increase drug counter if not alcoholic drug
	if ( usItem != ALCOHOL && usItem != WINE && usItem != BEER )
	{
		if (pSoldier->ubProfile != NO_PROFILE && gMercProfiles[pSoldier->ubProfile].ubNumTimesDrugUseInLifetime < 255)
		{
			gMercProfiles[ pSoldier->ubProfile ].ubNumTimesDrugUseInLifetime++;
		}
	}

	// ATE: use kit points...
	if ( consumeitem )
	{
		if (Item[usItem].foodtype > 0 && Food[Item[usItem].foodtype].ubPortionSize)
		{
			UseKitPoints(pObject, Food[Item[usItem].foodtype].ubPortionSize, pSoldier);
		}
		else if ( usItem == ALCOHOL )
			UseKitPoints( pObject, 10, pSoldier );
		else if ( usItem == WINE )
			UseKitPoints( pObject, 20, pSoldier );
		else if ( usItem == BEER )
			UseKitPoints( pObject, 100, pSoldier );
		else
		{
			// consume completely
			UINT16 ptsused = (*pObject)[0]->data.objectStatus;

			if (pSoldier->IsSoldierPC())
				gCampaignStats.AddConsumption(CAMPAIGN_CONSUMED_MEDICAL, (FLOAT)(ptsused * Item[usItem].ubWeight / 100.0) );

			// remove object
			pObject->RemoveObjectsFromStack(1);
		}

		if ( fUseAPs )
			DeductPoints( pSoldier, APBPConstants[AP_DRINK], 0 );
	}
	
	// sevenfm: only for player mercs or visible enemies
	if(pSoldier->IsSoldierPC() || pSoldier->bVisible == TRUE)
	{
		if ((ubDrugType & DRUG_ALCOHOL) != 0)
			ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_DRANK_SOME], pSoldier->GetName(), ShortItemNames[usItem]);
		else
			ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_MERC_TOOK_DRUG], pSoldier->GetName());

		if (!pSoldier->IsSoldierPC() && pSoldier->bVisible == TRUE)
			ShowRadioLocator(pSoldier->ubID, SHOW_LOCATOR_NOISE);

		// play sound
		if (usItem == ALCOHOL || usItem == WINE || usItem == BEER || Item[usItem].containsliquid)
		{
			PlayMiscSound("Alcohol", MIDVOLUME, pSoldier->sGridNo);
		}
	}	

	// sevenfm: special effects
	if( ubDrugType & DRUG_ALCOHOL )
	{
		if( pSoldier->bBreath > 0 &&
			!pSoldier->bCollapsed &&
			!pSoldier->bBreathCollapsed &&
			pSoldier->stats.bLife >= OKLIFE )
		{
			// alcohol reduces actual shock
			if( usItem == ALCOHOL )
				pSoldier->aiData.bShock = 2 * pSoldier->aiData.bShock / 3;
			else if ( usItem == WINE )
				pSoldier->aiData.bShock = 4 * pSoldier->aiData.bShock / 5;
			else if ( usItem == BEER )
				pSoldier->aiData.bShock = 9 * pSoldier->aiData.bShock / 10;
			else
				pSoldier->aiData.bShock = 9 * pSoldier->aiData.bShock / 10;

			INT8 bDrunkLevel = GetDrunkLevel(pSoldier);

			// add chance to be inspired
			if (pSoldier->ubProfile != NO_PROFILE &&
				HasBackgroundFlag(pSoldier->ubProfile, BACKGROUND_RUSSIAN_VODKA) &&
				!pSoldier->IsInspired() &&
				bDrunkLevel != HUNGOVER &&
				Chance(50))
			{
				if (InTurnbasedCombat())
					pSoldier->Inspire(2, TRUE);
				else
					pSoldier->Inspire(10, TRUE);
			}
		}
	}

	// Dirty panel
	fInterfacePanelDirty = DIRTYLEVEL2;
	fCharacterInfoPanelDirty = TRUE;

	// sevenfm: apply food only for player mercs
	if (!consumeitem  && pSoldier->IsSoldierPC())
		ApplyFood( pSoldier, pObject, TRUE, TRUE );

	return TRUE;
}

void HandleEndTurnDrugAdjustments( SOLDIERTYPE *pSoldier )
{
	INT32 cnt, cnt2;
	INT32	iNumLoops;

	// if were not on drugs, nothing to do here
	if ( ( pSoldier->usSoldierFlagMask & SOLDIER_DRUGGED ) == 0 )
		return;

	// We test for every 'pure' drug separately
	for (cnt = DRUG_TYPE_ADRENALINE; cnt < DRUG_TYPE_MAX; ++cnt)
	{
		// omit DRUG_TYPE_REGENERATION, because that is checked afterwards separately
		if ( cnt == DRUG_TYPE_REGENERATION )
			continue;
		
		// sevenfm: for DRUG_TYPE_ALCOHOL, only increase effect, will decrease in HourlyUpdate
		if( cnt != DRUG_TYPE_ALCOHOL )
		{
			// remove any side effect if side effect rate == 0
			if (pSoldier->drugs.bDrugSideEffectRate[cnt] == 0 && pSoldier->drugs.bDrugSideEffect[cnt] > 0)
			{
				pSoldier->drugs.bDrugSideEffect[cnt] = 0;
				pSoldier->drugs.bDrugSideEffectRate[cnt] = 0;
			}

			// If side effect rate is non-zero....
			if ( pSoldier->drugs.bDrugSideEffectRate[ cnt ] > 0 )
			{
				// Subtract some...
				pSoldier->drugs.bDrugSideEffect[ cnt ] -= pSoldier->drugs.bDrugSideEffectRate[ cnt ];

				// If we're done, we're done!
				if ( pSoldier->drugs.bDrugSideEffect[ cnt ] <= 0 )
				{
					pSoldier->drugs.bDrugSideEffect[ cnt ] = 0;
					pSoldier->drugs.bDrugSideEffectRate[ cnt ] = 0;
					//fInterfacePanelDirty	= DIRTYLEVEL1;
				}
			}			
			

			// IF drug rate is negative, it's being worn off...
			if ( pSoldier->drugs.bDrugEffectRate[ cnt ] < 0 )
			{
				pSoldier->drugs.bDrugEffect[ cnt ]				-= ( -1 * pSoldier->drugs.bDrugEffectRate[ cnt ] );

				// Have we run out?
				if ( pSoldier->drugs.bDrugEffect[ cnt ] <= 0 )
				{
					pSoldier->drugs.bDrugEffect[ cnt ] = 0;

					// Dirty panel
					//fInterfacePanelDirty = DIRTYLEVEL2;

					// Start the bad news!
					pSoldier->drugs.bDrugSideEffectRate[ cnt ] = Drug[cnt].ubDrugSideEffectRate;

					// The drug rate is 0 now too
					pSoldier->drugs.bDrugEffectRate[ cnt ]		= 0;

					// morale downer only if side effect exists and this effect is allowed for a drug
					if ( Drug[cnt].ubDrugSideEffect > 0 && Drug[cnt].ubMoralBacklash > 0 )
					{
						// Once for each 'level' of crash....
						iNumLoops = ( pSoldier->drugs.bDrugSideEffect[ cnt ] / Drug[cnt].ubDrugSideEffect ) + 1;

						for ( cnt2 = 0; cnt2 < iNumLoops; ++cnt2 )
						{
							// OK, give a much BIGGER morale downer
							if ( cnt == DRUG_TYPE_ALCOHOL )
							{
								HandleMoraleEvent( pSoldier, MORALE_ALCOHOL_CRASH, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
							}
							else
							{
								HandleMoraleEvent( pSoldier, MORALE_DRUGS_CRASH, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ	);
							}
						}
					}
				}
			}
		}

		// Add increase ineffect....
		if ( pSoldier->drugs.bDrugEffectRate[ cnt ] > 0 )
		{
			// Seap some in....
			pSoldier->drugs.bFutureDrugEffect[ cnt ] -= pSoldier->drugs.bDrugEffectRate[ cnt ];
			pSoldier->drugs.bDrugEffect[ cnt ]		 += pSoldier->drugs.bDrugEffectRate[ cnt ];

			// Refresh morale w/ new drug value...
			RefreshSoldierMorale( pSoldier );

			// Check if we need to stop 'adding'
			if ( pSoldier->drugs.bFutureDrugEffect[ cnt ] <= 0 )
			{
				pSoldier->drugs.bFutureDrugEffect[ cnt ] = 0;
				// Change rate to -ve..
				pSoldier->drugs.bDrugEffectRate[ cnt ] = -Drug[cnt].ubDrugWearoffRate;
			}
		}
	}

	if ( pSoldier->bRegenerationCounter > 0)
	{
		// sevenfm: restore 1-3 points every turn, less points with each use
		INT8 bLifePerRegenPoint = LIFE_GAIN_PER_REGEN_POINT + min((INT16)Random(3), max(0, 4 - pSoldier->bRegenBoostersUsedToday));

		/*if( gGameOptions.fNewTraitSystem &&
			pSoldier->stats.bLifeMax >= OKLIFE + bLifePerRegenPoint )
		{
			// sevenfm: damage max health when using regen boosters
			INT8 bStatLoss = Random( min( bLifePerRegenPoint, pSoldier->stats.bLifeMax - pSoldier->stats.bLife ) ) / 2 ;
			if( bStatLoss > 0 )
			{
				pSoldier->stats.bLifeMax -= bStatLoss;
				pSoldier->bBleeding -= bStatLoss;	// r9189
				pSoldier->ubCriticalStatDamage[DAMAGED_STAT_HEALTH] += bStatLoss;

				if (pSoldier->ubProfile != NO_PROFILE)
				{
					gMercProfiles[ pSoldier->ubProfile ].bLifeMax = pSoldier->stats.bLifeMax;
					gMercProfiles[ pSoldier->ubProfile ].records.usTimesStatDamaged++;
				}
			}
		}*/

		//ScreenMsg(FONT_ORANGE, MSG_INTERFACE, L"%s bLife %d / bLifeMax %d bRegenerationCounter %d bLifePerRegenPoint %d bBleeding %d", pSoldier->GetName(), pSoldier->stats.bLife, pSoldier->stats.bLifeMax, pSoldier->bRegenerationCounter, bLifePerRegenPoint, pSoldier->bBleeding);

		// increase life
		pSoldier->bPoisonLife = max(pSoldier->bPoisonSum, pSoldier->bPoisonLife + pSoldier->stats.bLife - min(pSoldier->stats.bLife + bLifePerRegenPoint, pSoldier->stats.bLifeMax));
		pSoldier->stats.bLife = min(pSoldier->stats.bLife + bLifePerRegenPoint, pSoldier->stats.bLifeMax);
		
		//SANDRO - Insta-healable injury reduction
		if( pSoldier->iHealableInjury > 0 )
		{
			pSoldier->iHealableInjury = max(0, (pSoldier->iHealableInjury - (100 * bLifePerRegenPoint)));
		}

		if ( pSoldier->stats.bLife == pSoldier->stats.bLifeMax )
		{
			pSoldier->bBleeding = 0;
			pSoldier->bPoisonBleeding = 0;
			pSoldier->iHealableInjury = 0;
		}
		else if ( pSoldier->bBleeding + pSoldier->stats.bLife > pSoldier->stats.bLifeMax )
		{
			// got to reduce amount of bleeding
			pSoldier->bBleeding = (pSoldier->stats.bLifeMax - pSoldier->stats.bLife);
			pSoldier->bPoisonBleeding = min(pSoldier->bPoisonBleeding, (pSoldier->stats.bLifeMax - pSoldier->stats.bLife));
		}

		// sevenfm: auto bandage
		if(pSoldier->bBleeding)
		{
			INT8 bBandagePoints = 5 + Random(20 - 5 * min(4, pSoldier->bRegenBoostersUsedToday));
			pSoldier->bBleeding -= min(pSoldier->bBleeding, bBandagePoints);
			pSoldier->bPoisonBleeding -= min(pSoldier->bPoisonBleeding, bBandagePoints);
		}

		// decrement counter
		pSoldier->bRegenerationCounter--;
	}

	// Flugente: always do the following checks. Thereby, if the effect runs out, our stats will be back to normal
	//////////////// STRENGTH ////////////////
	pSoldier->bExtraStrength	= pSoldier->drugs.bDrugEffect[ DRUG_TYPE_STRENGTH ] - pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_STRENGTH ];

	//////////////// DEXTERITY ////////////////
	pSoldier->bExtraDexterity	= pSoldier->drugs.bDrugEffect[ DRUG_TYPE_DEXTERITY ] - pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_DEXTERITY ];

	//////////////// AGILITY ////////////////
	pSoldier->bExtraAgility		= pSoldier->drugs.bDrugEffect[ DRUG_TYPE_AGILITY ] - pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_AGILITY ];

	//////////////// WISDOM ////////////////
	pSoldier->bExtraWisdom		= pSoldier->drugs.bDrugEffect[DRUG_TYPE_WISDOM] - pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_WISDOM];

	// if our sideeffect count is 1 (which should occur a while AFTER we took the drug), we suddenly become blind for a few turns...
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_BLIND ] == 0 && pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_BLIND ] == 1 )
	{
		pSoldier->bBlindedCounter = 3;
	}

	// if our sideeffect count is 1 (which should occur a while AFTER we took the drug), we get a heart-attack and get knocked out...
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_KNOCKOUT ] == 0 && pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_KNOCKOUT ] == 1 )
	{
		// Keel over...
		DeductPoints( pSoldier, 0, 20000 );
	}

	// if we have a life damaging effect, deduct life points
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_LIFEDAMAGE ] == 0 && pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_LIFEDAMAGE ] > 0 )
	{
		pSoldier->EVENT_SoldierGotHit( 0, 10, 0, pSoldier->ubDirection, 320, NOBODY , FIRE_WEAPON_NO_SPECIAL, pSoldier->bAimShotLocation, 0, -1 );
	}

	// if we took an antidote, reduce poisoning
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_CUREPOISON ] > 0 )
	{
		if (  pSoldier->bPoisonSum > 0 )
		{
			if ( pSoldier->bPoisonBleeding > 0 )
			{
				pSoldier->bPoisonBleeding--;
			}
			else if ( pSoldier->bPoisonLife > 0 )
			{
				pSoldier->bPoisonLife--;
			}

			pSoldier->bPoisonSum--;
		}
	}

	// etorphine stuns while it lasts. It can also damage and possibly kill the target if overdosed. The dosage depends on our bodytype
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_STUNANDKILL ] )
	{
		// Keel over...
		DeductPoints( pSoldier, 0, 20000 );

		UINT8 bodyweight = 10;
		switch ( pSoldier->ubBodyType )
		{
		case REGMALE:
		case MANCIV:
			bodyweight = 10;
			break;

		case BIGMALE:
		case STOCKYMALE:
			bodyweight = 12;
			break;

		case REGFEMALE:
			bodyweight = 9;
			break;

		case ADULTFEMALEMONSTER:
		case AM_MONSTER:
			bodyweight = 30;
			break;

		case YAF_MONSTER:
		case YAM_MONSTER:
			bodyweight = 20;
			break;

		case LARVAE_MONSTER:
			bodyweight = 4;
			break;

		case INFANT_MONSTER:
			bodyweight = 10;
			break;

		case QUEENMONSTER:
			bodyweight = 100;
			break;

		case FATCIV:
		case MINICIV:
		case DRESSCIV:
			bodyweight = 8;
			break;

		case HATKIDCIV:
		case KIDCIV:
			bodyweight = 4;
			break;

		case CRIPPLECIV:
			bodyweight = 7;
			break;

		case COW:
			bodyweight = 20;
			break;

		case CROW:
			bodyweight = 1;
			break;

		case BLOODCAT:
			bodyweight = 20;
			break;

		case ROBOTNOWEAPON:
		case HUMVEE:
		case TANK_NW:
		case TANK_NE:
		case ELDORADO:
		case ICECREAMTRUCK:
		case JEEP:
			// this should not happen anyway...
			bodyweight = 100;
			break;
		}

		if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_STUNANDKILL ] > bodyweight )
		{
			if ( Chance( 10 * (pSoldier->drugs.bDrugEffect[ DRUG_TYPE_STUNANDKILL ] - bodyweight) ) )
				pSoldier->EVENT_SoldierGotHit( 0, 400, 0, pSoldier->ubDirection, 320, NOBODY , FIRE_WEAPON_NO_SPECIAL, pSoldier->bAimShotLocation, 0, -1 );
		}
	}

	// if all drug effects have ended, delete flag
	if ( !MercUnderTheInfluence(pSoldier) )
		pSoldier->usSoldierFlagMask &= ~SOLDIER_DRUGGED;

	// Dirty panel
	fInterfacePanelDirty = DIRTYLEVEL2;
}

void HandleAPEffectDueToDrugs( SOLDIERTYPE *pSoldier, INT16 *pubPoints )
{
	INT8	bDrunkLevel;
	INT16	sPoints = (*pubPoints);
	INT16	sBonus = 0;

	// Are we in a side effect or good effect?
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ADRENALINE ] )
	{
		// Adjust!
		sBonus += pSoldier->drugs.bDrugEffect[DRUG_TYPE_ADRENALINE];
	}
	else if ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ADRENALINE ] )
	{
		// Adjust!
		sBonus -= pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ADRENALINE];		
	}
	
	bDrunkLevel = GetDrunkLevel( pSoldier );

	if ( bDrunkLevel == HUNGOVER )
	{
		// Reduce....
		if (pSoldier->HasBackgroundFlag(BACKGROUND_RUSSIAN_VODKA))
			sBonus -= HANGOVER_AP_REDUCE * 2;
		else
			sBonus -= HANGOVER_AP_REDUCE;
	}

	// sevenfm: adjust AP bonus/penalty to current system
	sPoints += DynamicAdjustAPConstants(sBonus);
	sPoints = max(sPoints, APBPConstants[AP_MINIMUM]);

	(*pubPoints) = sPoints;
}

void HandleBPEffectDueToDrugs( SOLDIERTYPE *pSoldier, INT16 *psPointReduction )
{
	INT8 bDrunkLevel;

	// Are we in a side effect or good effect?
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ADRENALINE ] )
	{
		// Adjust!
		(*psPointReduction) -= ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ADRENALINE ] * APBPConstants[BP_RATIO_RED_PTS_TO_NORMAL] );
	}
	else if ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ADRENALINE ] )
	{
		// Adjust!
		(*psPointReduction) += ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ADRENALINE ] * APBPConstants[BP_RATIO_RED_PTS_TO_NORMAL] );
	}

	bDrunkLevel = GetDrunkLevel( pSoldier );

	if ( bDrunkLevel == HUNGOVER )
	{
		// Reduce....
		if (pSoldier->HasBackgroundFlag(BACKGROUND_RUSSIAN_VODKA))
			(*psPointReduction) += HANGOVER_BP_REDUCE * 2;
		else
			(*psPointReduction) += HANGOVER_BP_REDUCE;
	}
}

void HandleDamageResistanceEffectDueToDrugs( SOLDIERTYPE *pSoldier, INT32 *psPointReduction )
{
	// Are we in a side effect or good effect?
	if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_RESISTANCE ] )
	{
		// Adjust!
		(*psPointReduction) += pSoldier->drugs.bDrugEffect[ DRUG_TYPE_RESISTANCE ];
	}
	else if ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_RESISTANCE ] )
	{
		// Adjust!
		(*psPointReduction) -= pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_RESISTANCE ];
	}
}

INT8 GetDrunkLevel( SOLDIERTYPE *pSoldier )
{
	INT8 bNumDrinks;

	// If we have a -ve effect ...
	if( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] == 0 &&
		pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] == 0 )
	{
		return( SOBER );
	}

	if( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] == 0 &&
		pSoldier->drugs.bFutureDrugEffect[ DRUG_TYPE_ALCOHOL ] == 0 &&
		pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] != 0 )
	{
		return( HUNGOVER );
	}

	// Calculate how many drinks we have had....
	bNumDrinks = ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] / Drug[DRUG_TYPE_ALCOHOL].ubDrugEffect );

	if ( bNumDrinks <= 3 )
	{
		return( FEELING_GOOD );
	}
	else if ( bNumDrinks <= 4 )
	{
		return( BORDERLINE );
	}
	else
	{
		return( DRUNK );
	}
}

INT32 EffectStatForBeingDrunk( SOLDIERTYPE *pSoldier, INT32 iStat )
{
	if (pSoldier->HasBackgroundFlag(BACKGROUND_RUSSIAN_VODKA) && GetDrunkLevel(pSoldier) < HUNGOVER)
		return iStat * (100 + 5 * GetDrunkLevel(pSoldier)) / 100;
	else
		return iStat * giDrunkModifier[GetDrunkLevel(pSoldier)] / 100;
}

BOOLEAN MercUnderTheInfluence( SOLDIERTYPE *pSoldier )
{
	for (UINT8 cnt = DRUG_TYPE_ADRENALINE; cnt < DRUG_TYPE_MAX; ++cnt)
	{
		if ( MercUnderTheInfluence(pSoldier, cnt) )
			return TRUE;
	}

	// also check regeneration effect
	if ( pSoldier->bRegenerationCounter > 0)
		return( TRUE );

	return( FALSE );
}

BOOLEAN MercUnderTheInfluence( SOLDIERTYPE *pSoldier, UINT8 aDrugType )
{
	// in case of wrong input, stay safe
	if ( aDrugType >= DRUG_TYPE_MAX )
		return( FALSE );

	// Are we in a side effect or good effect?
	if ( pSoldier->drugs.bDrugEffect[ aDrugType ] )
		return( TRUE );
		
	if ( pSoldier->drugs.bDrugSideEffect[ aDrugType ] )
		return( TRUE );

	// sevenfm: check drug type
	if ( aDrugType == DRUG_TYPE_REGENERATION && pSoldier->bRegenerationCounter > 0)
		return( TRUE );

	return( FALSE );
}

BOOLEAN MercDruggedButNotDrunk( SOLDIERTYPE *pSoldier )
{
	for (UINT8 cnt = DRUG_TYPE_ADRENALINE; cnt < DRUG_TYPE_MAX; ++cnt)
	{
		if ( cnt == DRUG_TYPE_ALCOHOL )
			continue;

		if ( MercUnderTheInfluence(pSoldier, cnt) )
			return TRUE;
	}

	return( FALSE );
}

void HourlyDrugUpdate( )
{
	//INT32	cnt;
	INT32	cnt2;
	INT32	iNumLoops;
	SOLDIERTYPE *pSoldier;

	for ( UINT16 ubID = gTacticalStatus.Team[OUR_TEAM].bFirstID; ubID <= gTacticalStatus.Team[OUR_TEAM].bLastID; ++ubID )
	{
		pSoldier = MercPtrs[ubID];

		// every hour, we lower our alcohol counter

		// If side effect rate is non-zero....
		if ( pSoldier->drugs.bDrugSideEffectRate[ DRUG_TYPE_ALCOHOL ] > 0 )
		{
			// Subtract some...			
			// sevenfm: double rate when sleeping
			if ( pSoldier->flags.fMercAsleep )
				pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] -= 2 * pSoldier->drugs.bDrugSideEffectRate[ DRUG_TYPE_ALCOHOL ];
			// sevenfm: slower rate for vodka background
			else if (pSoldier->HasBackgroundFlag(BACKGROUND_RUSSIAN_VODKA))
				//pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ALCOHOL] -= pSoldier->drugs.bDrugSideEffectRate[DRUG_TYPE_ALCOHOL];
				pSoldier->drugs.bDrugSideEffect[DRUG_TYPE_ALCOHOL] -= max(1, pSoldier->drugs.bDrugSideEffectRate[DRUG_TYPE_ALCOHOL] / 2);
			else
				pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] -= pSoldier->drugs.bDrugSideEffectRate[ DRUG_TYPE_ALCOHOL ];

			// If we're done, we're done!
			if ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] <= 0 )
			{
				pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] = 0;
				pSoldier->drugs.bDrugSideEffectRate[ DRUG_TYPE_ALCOHOL ] = 0;
				//fInterfacePanelDirty	= DIRTYLEVEL1;
			}
		}

		// IF drug rate is negative, it's being worn off...
		if ( pSoldier->drugs.bDrugEffectRate[ DRUG_TYPE_ALCOHOL ] < 0 )
		{
			// sevenfm: double rate when sleeping
			if (pSoldier->flags.fMercAsleep)
				pSoldier->drugs.bDrugEffect[DRUG_TYPE_ALCOHOL] -= (-2 * pSoldier->drugs.bDrugEffectRate[DRUG_TYPE_ALCOHOL]);
			// sevenfm: increase rate for vodka background
			else if (pSoldier->HasBackgroundFlag(BACKGROUND_RUSSIAN_VODKA))
				pSoldier->drugs.bDrugEffect[DRUG_TYPE_ALCOHOL] -= (-4 * pSoldier->drugs.bDrugEffectRate[DRUG_TYPE_ALCOHOL]);
			else
				pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] -= ( -1 * pSoldier->drugs.bDrugEffectRate[ DRUG_TYPE_ALCOHOL ] );

			// Have we run out?
			if ( pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] <= 0 )
			{
				pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] = 0;

				// Start the bad news!
				pSoldier->drugs.bDrugSideEffectRate[ DRUG_TYPE_ALCOHOL ] = Drug[ DRUG_TYPE_ALCOHOL ].ubDrugSideEffectRate;

				// The drug rate is 0 now too
				pSoldier->drugs.bDrugEffectRate[ DRUG_TYPE_ALCOHOL ] = 0;

				// morale downer only if side effect exists and this effect is allowed for a drug
				if ( Drug[ DRUG_TYPE_ALCOHOL ].ubDrugSideEffect > 0 && Drug[ DRUG_TYPE_ALCOHOL ].ubMoralBacklash > 0 )
				{
					// Once for each 'level' of crash....
					iNumLoops = ( pSoldier->drugs.bDrugSideEffect[ DRUG_TYPE_ALCOHOL ] / Drug[ DRUG_TYPE_ALCOHOL ].ubDrugSideEffect ) + 1;

					for ( cnt2 = 0; cnt2 < iNumLoops; ++cnt2 )
					{
						// OK, give a much BIGGER morale downer
						HandleMoraleEvent( pSoldier, MORALE_ALCOHOL_CRASH, pSoldier->sSectorX, pSoldier->sSectorY, pSoldier->bSectorZ );
					}
				}
			}
		}

		// Add increase in effect....
		/*if ( pSoldier->drugs.bDrugEffectRate[ DRUG_TYPE_ALCOHOL ] > 0 )
		{
			// Seap some in....
			pSoldier->drugs.bFutureDrugEffect[ DRUG_TYPE_ALCOHOL ] -= pSoldier->drugs.bDrugEffectRate[ cnt ];
			pSoldier->drugs.bDrugEffect[ DRUG_TYPE_ALCOHOL ] += pSoldier->drugs.bDrugEffectRate[ cnt ];

			// Refresh morale w/ new drug value...
			RefreshSoldierMorale( pSoldier );

			// Check if we need to stop 'adding'
			if ( pSoldier->drugs.bFutureDrugEffect[ DRUG_TYPE_ALCOHOL ] <= 0 )
			{
				pSoldier->drugs.bFutureDrugEffect[ DRUG_TYPE_ALCOHOL ] = 0;
				// Change rate to -ve..
				pSoldier->drugs.bDrugEffectRate[ DRUG_TYPE_ALCOHOL ] = -Drug[cnt].ubDrugWearoffRate;
			}
		}*/
	}

	// if all drug effects have ended, delete flag
	if ( !MercUnderTheInfluence(pSoldier) )
		pSoldier->usSoldierFlagMask &= ~SOLDIER_DRUGGED;
}

