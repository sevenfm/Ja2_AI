#ifdef PRECOMPILEDHEADERS
	#include "Strategic All.h"
#endif

#include "PrisonerRansom.h"
#include "GameSettings.h"
#include "Campaign.h"

// travnik: базова€ сумма выкупа дл€ указанного типа военнопленного
UINT32 CPrisonerRansom::GetBaseRansomMoney(UINT8 ubPrisonerType)
{
	if ( ubPrisonerType >= PRISONER_MAX )
		return 0;

	return gGameExternalOptions.usPrisonerRansomMoney[ubPrisonerType];
}

// travnik: выкуп растет линейно вместе с текущим прогрессом игры (0-100%)
FLOAT CPrisonerRansom::GetProgressMultiplier()
{
	FLOAT fProgress = (FLOAT)CurrentPlayerProgressPercentage();
	FLOAT fCoefficient = gGameExternalOptions.fPrisonerRansomProgressCoefficient;

	return 1.0f + fCoefficient * fProgress / 100.0f;
}

// travnik: пор€док обработки военнопленных совпадает с пор€дком допроса:
// администраторы, регул€рные, элита, офицеры
UINT8 CPrisonerRansom::GetPrisonerTypeByProcessedIndex(UINT32 uiProcessedIndex, const UINT8* pubInterrogatedPrisoners)
{
	UINT32 uiBoundary = 0;

	for ( UINT8 ubPrisonerType = PRISONER_ADMIN; ubPrisonerType < PRISONER_MAX; ++ubPrisonerType )
	{
		uiBoundary += pubInterrogatedPrisoners[ubPrisonerType];

		if ( uiProcessedIndex < uiBoundary )
			return ubPrisonerType;
	}

	return PRISONER_ADMIN;
}

UINT32 CPrisonerRansom::GetRansomMoney(UINT8 ubPrisonerType)
{
	UINT32 uiBaseRansom = GetBaseRansomMoney( ubPrisonerType );

	return (UINT32)( uiBaseRansom * GetProgressMultiplier() + 0.5f );
}

UINT32 CPrisonerRansom::GetRansomMoneyByProcessedIndex(UINT32 uiProcessedIndex, const UINT8* pubInterrogatedPrisoners)
{
	UINT8 ubPrisonerType = GetPrisonerTypeByProcessedIndex( uiProcessedIndex, pubInterrogatedPrisoners );

	return GetRansomMoney( ubPrisonerType );
}
