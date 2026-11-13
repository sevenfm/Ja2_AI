#ifndef _ARTILLERY_SAM_
#define _ARTILLERY_SAM_

/**
 * @file
 * @author travnik
 *
 * @brief	Артиллерийский налёт через захваченную базу ПВО.
 *
 *	База ПВО может наводить артналёт, если выполняются условия:
 *	1. база ПВО находится под контролем стороны, заказывающей налёт;
 *	2. на базе ПВО не взорван компьютер (bSAMCondition >= MIN_CONDITION_FOR_SAM_SITE_TO_WORK);
 *	3. сектор-цель находится в зоне поражения ПВО (ubSAMControlledSectors).
 *
 *	Мощность удара, тип боеприпаса и стоимость вынесены в Skills_Settings.ini,
 *	секция [Radio Operator], ключи RADIO_OPERATOR_SAM_ARTILLERY_*.
 *
 *	Весь игровой код механики собран здесь; в существующих файлах остаются
 *	только компактные вызовы этих функций.
 */

#include "types.h"

class SOLDIERTYPE;
class OBJECTTYPE;

class SAMArtillery
{
public:
	// travnik: включена ли механика вообще (общая настройка артиллерии + отдельный флаг SAM-налётов)
	static BOOLEAN IsEnabled();

	// travnik: работает ли ПВО ubSamId (1..NUMBER_OF_SAMS) для стороны bTeam (контроль + целый компьютер)
	static BOOLEAN IsSAMSiteOperationalForTeam( INT8 ubSamId, UINT8 bTeam );

	// travnik: находится ли база ПВО ubSamId под контролем стороны bTeam (без проверки компьютера)
	static BOOLEAN IsSAMSiteControlledByTeam( INT8 ubSamId, UINT8 bTeam );

	// travnik: находится ли сектор (sX, sY) в зоне поражения базы ПВО ubSamId
	static BOOLEAN IsSectorInSAMRange( INT8 ubSamId, INT16 sX, INT16 sY );

	// travnik: найти индекс работающей ПВО стороны bTeam, контролирующей сектор (sX, sY); 0, если такой нет
	static INT8 FindOperationalSAMIdForTeamAtSector( UINT8 bTeam, INT16 sX, INT16 sY );

	// travnik: может ли сторона заказать артналёт через ПВО по сектору радиста (проверка вкл. настроек и боеприпаса для игрока)
	static BOOLEAN CanTeamOrderSAMArtilleryStrike( INT16 sRadioX, INT16 sRadioY, UINT8 bTeam );

	// travnik: стоимость вызова для игрока (учитывает прогресс игры)
	static UINT32 GetSAMArtilleryCost();

	// travnik: достигнут ли минимальный прогресс игры для SAM-налётов
	static BOOLEAN IsSAMArtilleryProgressReached();

	// travnik: разрешён ли настроенный тип боеприпаса и существует ли такой снаряд
	static BOOLEAN IsSAMArtilleryAmmoAvailable();

	// travnik: предмет снаряда для SAM-удара (с fallback на MORTAR_SHELL для фугаса)
	static UINT16 GetShellItem();

	// travnik: параметры залпа из настроек (число миномётов 1-4 и число волн)
	static void GetStrikeParams( INT16* psNumMortars, INT16* psNumWaves );

	// travnik: проставить снаряду флаги числа миномётов (ARTILLERY_STRIKE_COUNT_*)
	static void SetShellCountFlags( OBJECTTYPE* pShellObj, INT16 sNumMortars );

	// travnik: проверить, что боеприпас доступен и денег хватает (с сообщениями игроку); без списания
	static BOOLEAN VerifyCost( UINT8 bTeam );

	// travnik: списать деньги за налёт (вызывать только после VerifyCost и когда налёт точно состоится)
	static void ChargeCost( UINT8 bTeam );

	// travnik: стартовый gridno залпа - край карты текущего сектора со стороны базы ПВО
	static INT32 GetLaunchGridNoTowardsSAM( const SOLDIERTYPE* pSoldier, INT8 ubSamId );
};

#endif //_ARTILLERY_SAM_
