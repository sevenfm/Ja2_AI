/**
 * @file
 * @author travnik
 *
 * @brief	Артиллерийский налёт через захваченную базу ПВО (см. ArtillerySAM.h)
 */

#ifdef PRECOMPILEDHEADERS
#include "Tactical All.h"
#else
#include "builddefines.h"
#include <stdio.h>
#include <string.h>
#include "wcheck.h"
#include "types.h"
#include "ArtillerySAM.h"
#include "Soldier Control.h"
#include "Overhead.h"
#include "strategicmap.h"
#include "Campaign Types.h"
#include "GameSettings.h"
#include "Game Clock.h"
#include "Map Screen Interface Map.h"
#include "Queen Command.h"
#include "Explosion Control.h"
#include "Items.h"
#include "Item Types.h"
#include "World Items.h"
#include "Handle Items.h"
#include "Text.h"
#include "Message.h"
#include "Sound Control.h"
#include "finances.h"
#include "LaptopSave.h"
#include "CampaignStats.h"	// travnik: INCIDENT_ARTILLERY_* flags
#include "Campaign.h"
#include "Map Information.h"	// travnik: gMapInformation for launch gridno
#include "physics.h"
#include "Weapons.h"
#include "Random.h"
#include "Debug.h"
#include "Map Screen Helicopter.h"
#endif
#include "isometric utils.h"
#include "strategic.h"

// travnik: таблица зон поражения ПВО (extern отсутствует в заголовках, определена в strategicmap.cpp)
extern UINT8 ubSAMControlledSectors[ MAP_WORLD_Y ][ MAP_WORLD_X ];

// travnik: SAM-сайты нумеруются с 1 в ubSAMControlledSectors и таблицах gpSamSectorX/Y
#define SAM_ARTILLERY_MIN_ID		1

BOOLEAN SAMArtillery::IsEnabled()
{
	// travnik: мастер-выключатель - общий флаг артиллерии и отдельный флаг для SAM-налётов
	return gSkillTraitValues.fROAllowArtillery && gSkillTraitValues.fROSAMArtillery;
}

BOOLEAN SAMArtillery::IsSAMSiteControlledByTeam( INT8 ubSamId, UINT8 bTeam )
{
	// travnik: корректность индекса
	if ( ubSamId < SAM_ARTILLERY_MIN_ID || ubSamId > NUMBER_OF_SAMS )
		return FALSE;

	INT16 sSamX = gpSamSectorX[ubSamId - 1];
	INT16 sSamY = gpSamSectorY[ubSamId - 1];
	StrategicMapElement* pSAMStrategicMap = &(StrategicMap[CALCULATE_STRATEGIC_INDEX(sSamX, sSamY)]);

	// travnik: база должна быть под контролем заказчика налёта
	if ( bTeam == ENEMY_TEAM )
		return pSAMStrategicMap->fEnemyControlled;

	if ( bTeam == OUR_TEAM || bTeam == MILITIA_TEAM )
		return !pSAMStrategicMap->fEnemyControlled;

	return FALSE;
}

BOOLEAN SAMArtillery::IsSAMSiteOperationalForTeam( INT8 ubSamId, UINT8 bTeam )
{
	// travnik: корректность индекса
	if ( ubSamId < SAM_ARTILLERY_MIN_ID || ubSamId > NUMBER_OF_SAMS )
		return FALSE;

	// travnik: база должна быть под контролем заказчика налёта
	if ( !IsSAMSiteControlledByTeam( ubSamId, bTeam ) )
		return FALSE;

	INT16 sSamX = gpSamSectorX[ubSamId - 1];
	INT16 sSamY = gpSamSectorY[ubSamId - 1];
	StrategicMapElement* pSAMStrategicMap = &(StrategicMap[CALCULATE_STRATEGIC_INDEX(sSamX, sSamY)]);

	// travnik: компьютер (центр управления) не должен быть взорван/повреждён
	if ( pSAMStrategicMap->bSAMCondition < MIN_CONDITION_FOR_SAM_SITE_TO_WORK )
		return FALSE;

	return TRUE;
}

BOOLEAN SAMArtillery::IsSectorInSAMRange( INT8 ubSamId, INT16 sX, INT16 sY )
{
	// travnik: корректность индекса и координат
	if ( ubSamId < SAM_ARTILLERY_MIN_ID || ubSamId > NUMBER_OF_SAMS )
		return FALSE;

	if ( sX < 1 || sX > 16 || sY < 1 || sY > 16 )
		return FALSE;

	// travnik: именно эта база ПВО должна контролировать сектор
	return ubSAMControlledSectors[sY][sX] == ubSamId;
}

INT8 SAMArtillery::FindOperationalSAMIdForTeamAtSector( UINT8 bTeam, INT16 sX, INT16 sY )
{
	for ( INT8 ubSamId = SAM_ARTILLERY_MIN_ID; ubSamId <= NUMBER_OF_SAMS; ++ubSamId )
	{
		if ( IsSAMSiteOperationalForTeam(ubSamId, bTeam) && IsSectorInSAMRange(ubSamId, sX, sY) )
			return ubSamId;
	}

	return 0;
}

BOOLEAN SAMArtillery::CanTeamOrderSAMArtilleryStrike( INT16 sRadioX, INT16 sRadioY, UINT8 bTeam )
{
	if ( !IsEnabled() )
		return FALSE;

	// travnik: налёты только на поверхности
	if ( gbWorldSectorZ > 0 )
		return FALSE;

	// travnik: игрок не может заказать налёт, если тип боеприпаса недоступен (нет снаряда/прогресс)
	if ( bTeam == OUR_TEAM || bTeam == MILITIA_TEAM )
	{
		if ( !IsSAMArtilleryAmmoAvailable() )
			return FALSE;
	}

	return FindOperationalSAMIdForTeamAtSector( bTeam, sRadioX, sRadioY ) > 0;
}

UINT32 SAMArtillery::GetSAMArtilleryCost()
{
	// travnik: базовая цена, масштабируемая прогрессом игры, как у ополченческого артналёта
	INT64 llPercent = 100 + (INT64)gSkillTraitValues.sVOSAMArtilleryCostProgress * HighestPlayerProgressPercentage() / 100;
	if ( llPercent < 0 )
		llPercent = 0;

	return (UINT32)( (INT64)gSkillTraitValues.usVOSAMArtilleryCost * llPercent / 100 );
}

BOOLEAN SAMArtillery::IsSAMArtilleryProgressReached()
{
	return HighestPlayerProgressPercentage() >= gSkillTraitValues.ubVOSAMArtilleryMinProgress;
}

BOOLEAN SAMArtillery::IsSAMArtilleryAmmoAvailable()
{
	// travnik: тип боеприпаса должен быть разрешён настройкой, прогресс достигнут и снаряд должен существовать
	if ( gSkillTraitValues.ubVOSAMArtilleryAmmoType >= ARTILLERY_AMMO_MAX )
		return FALSE;
	if ( !IsSAMArtilleryProgressReached() )
		return FALSE;

	UINT16 usItem = GetShellItem();

	return usItem != 0 && ItemIsMortarShell( usItem );
}

UINT16 SAMArtillery::GetShellItem()
{
	UINT16 usItem = GetMortarShellOfType( GetArtilleryAmmoExplosiveType( gSkillTraitValues.ubVOSAMArtilleryAmmoType ) );

	// travnik: HE может откатиться на стандартный миномётный снаряд
	if ( usItem == 0 && gSkillTraitValues.ubVOSAMArtilleryAmmoType == ARTILLERY_AMMO_HE )
		usItem = MORTAR_SHELL;

	return usItem;
}

void SAMArtillery::GetStrikeParams( INT16* psNumMortars, INT16* psNumWaves )
{
	if ( psNumMortars )
		*psNumMortars = (INT16)max( 1, min( 4, (INT32)gSkillTraitValues.ubVOSAMArtilleryMortars ) );

	if ( psNumWaves )
		*psNumWaves = (INT16)max( 1, (INT32)gSkillTraitValues.usVOSAMArtilleryWaves );
}

void SAMArtillery::SetShellCountFlags( OBJECTTYPE* pShellObj, INT16 sNumMortars )
{
	if ( !pShellObj )
		return;

	// travnik: число миномётов кодируется флагами ARTILLERY_STRIKE_COUNT_* (1, 2, 4)
	switch ( max(1, min(4, sNumMortars)) )
	{
	case 1:
		(*pShellObj)[0]->data.ubWireNetworkFlag = ARTILLERY_STRIKE_COUNT_1;
		break;

	case 2:
		(*pShellObj)[0]->data.ubWireNetworkFlag = ARTILLERY_STRIKE_COUNT_2;
		break;

	case 3:
		(*pShellObj)[0]->data.ubWireNetworkFlag = (ARTILLERY_STRIKE_COUNT_1 | ARTILLERY_STRIKE_COUNT_2);
		break;

	case 4:
	default:
		(*pShellObj)[0]->data.ubWireNetworkFlag = ARTILLERY_STRIKE_COUNT_4;
		break;
	}
}

BOOLEAN SAMArtillery::VerifyCost( UINT8 bTeam )
{
	// travnik: деньги платит только игрок (налёты ополчения, вызванные радистом игрока, тоже платные)
	if ( bTeam != OUR_TEAM && bTeam != MILITIA_TEAM )
		return TRUE;

	// travnik: тип боеприпаса/прогресс/наличие снаряда
	if ( !IsSAMArtilleryAmmoAvailable() )
	{
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[ MSG113_ARTILLERY_AMMO_UNAVAILABLE ] );
		return FALSE;
	}

	UINT32 uiCost = GetSAMArtilleryCost();

	if ( uiCost > 0 && LaptopSaveInfo.iCurrentBalance < (INT32)uiCost )
	{
		ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, New113Message[ MSG113_ARTILLERY_NOT_ENOUGH_MONEY ] );
		return FALSE;
	}

	return TRUE;
}

void SAMArtillery::ChargeCost( UINT8 bTeam )
{
	// travnik: деньги платит только игрок
	if ( bTeam != OUR_TEAM && bTeam != MILITIA_TEAM )
		return;

	UINT32 uiCost = GetSAMArtilleryCost();

	// travnik: списание непосредственно перед исполнением
	if ( uiCost > 0 )
		AddTransactionToPlayersBook( FACILITY_OPERATIONS, 0, GetWorldTotalMin(), -(INT32)uiCost );
}

INT32 SAMArtillery::GetLaunchGridNoTowardsSAM( const SOLDIERTYPE* pSoldier, INT8 ubSamId )
{
	if ( !pSoldier || ubSamId < SAM_ARTILLERY_MIN_ID || ubSamId > NUMBER_OF_SAMS )
		return NOWHERE;

	// travnik: снаряды прилетают от края карты со стороны базы ПВО
	INT16 sSamX = gpSamSectorX[ubSamId - 1];
	INT16 sSamY = gpSamSectorY[ubSamId - 1];

	INT32 sStartingGridNo = gMapInformation.sNorthGridNo;

	if ( sSamY < pSoldier->sSectorY )
		sStartingGridNo = gMapInformation.sNorthGridNo;
	else if ( sSamY > pSoldier->sSectorY )
		sStartingGridNo = gMapInformation.sSouthGridNo;
	else if ( sSamX < pSoldier->sSectorX )
		sStartingGridNo = gMapInformation.sWestGridNo;
	else if ( sSamX > pSoldier->sSectorX )
		sStartingGridNo = gMapInformation.sEastGridNo;

	if ( sStartingGridNo == -1 )
		sStartingGridNo = gMapInformation.sCenterGridNo;

	return sStartingGridNo;
}
